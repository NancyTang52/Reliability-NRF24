#include "nrf.h"
#include <cstdio>
#include <cstring>
#include <string>

#define MOSI_PIN    D11
#define MISO_PIN    D12
#define SCK_PIN     D13
#define CSN_PIN     D8
#define CE_PIN      D9
#define IRQ_PIN     D7
#define MAX_BYTES_NRF_CAN_HANDLE    32

NRF24::NRF24()
    : m_nrf_comm(MOSI_PIN, MISO_PIN, SCK_PIN, CSN_PIN, CE_PIN, IRQ_PIN)
{
    MBED_ASSERT(TRANSFER_SIZE <= MAX_BYTES_NRF_CAN_HANDLE);
    // NOTE: this is to fix the power up, getting stuck
    // Don't know why, but is a fix :)
    ////////////////////////////////////////////
    SPI spi(MOSI_PIN, MISO_PIN, SCK_PIN);
    spi.format(8, 3);
    ////////////////////////////////////////////

    m_nrf_comm.powerUp();

    m_nrf_comm.setTransferSize(TRANSFER_SIZE);

    m_nrf_comm.setReceiveMode();
    // m_nrf_comm.enableAutoAcknowledge(NRF24L01P_PIPE_P0);
    m_nrf_comm.enable();

    auto config = get_current_config();
    write_new_config(config);

    print_nrf_info();
}

NRF24Config NRF24::generate_test_case(int16_t test_case_number) {
    int8_t output_powers[] = OUTPUT_POWERS;
    uint16_t frequency_channels[] = FREQUENCY_CHANNELS;
    uint16_t data_rates[] = DATA_RATES;
    uint16_t delays[] = DELAYS;
    uint8_t counts[] = COUNTS;

    int16_t tc_counter = -1;

    for(auto& output_power : output_powers) 
    {
        for(auto& frequency_channel : frequency_channels) 
        {
            for(auto& data_rate : data_rates) 
            {
                for(auto& delay : delays) 
                {
                    for(auto& count : counts) 
                    {
                        tc_counter++;
                        if(tc_counter != test_case_number) {
                            continue;
                        }

                        string name = "TC_" + to_string(tc_counter);
                        return NRF24Config {
                            /* test_name: */ name,
                            /* output_power: */ output_power,
                            /* frequency_channel: */ frequency_channel,
                            /* data_rate: */ data_rate,
                            /* auto_retransmission_delay: */ delay,
                            /* auto_retransmission_count: */ count,
                        };
                    }
                }
            }
        }
    }    

    error("testcase %d not found", test_case_number);
}

void NRF24::set_receiver() {
    m_nrf_comm.setReceiveMode();
}

NRF24Config NRF24::get_current_config() {
    return generate_test_case(m_test_case_index);
}

void NRF24::run_packet_error_rate_test() {
    auto config = get_current_config();
    write_new_config(config);
    int successful_transitions = 0;

    char* data = new char[TRANSFER_SIZE];
    std::memcpy(data, MESSAGE, TRANSFER_SIZE);
    
    for (int i = 0; i < TOTAL_MESSAGES_TO_TEST; i++)
    {
        send_test_message();

        if(did_receive_acknowledgement(config, data)) {
            successful_transitions++;
        }

        thread_sleep_for(50);
    }
    
    delete[] data;
    print_csv_stats(config, successful_transitions);
}

void NRF24::next_config_index() {
    m_test_case_index++;
}

void NRF24::send_test_message() {
    char* data = new char[TRANSFER_SIZE];
    std::memcpy(data, MESSAGE, TRANSFER_SIZE);

    auto send_bytes = m_nrf_comm.write(NRF24L01P_PIPE_P0, data, TRANSFER_SIZE);   
    delete[] data;
}

void NRF24::ensure_connection(NRF24Config& config) {
    char* data = new char[TRANSFER_SIZE];
    std::memcpy(data, MESSAGE, TRANSFER_SIZE);
    int counter = 0;
    while(counter < 10) {
        send_test_message();

        ThisThread::sleep_for(100);

        if(did_receive_acknowledgement(config, data)) {
            counter++;
        } else {
            printf("did NOT receive message \r\n");
        }
    }
    printf("Connection ensured \r\n");
    delete[] data;
}

void NRF24::send_config_update() {
    NRF24Config old_config = generate_test_case(m_test_case_index);
    NRF24Config new_config = generate_test_case(m_test_case_index+1);

    char* data = new char[TRANSFER_SIZE];
    std::memcpy(data, NEXT_CONFIG_MESSAGE, TRANSFER_SIZE);
    while(true) {
        write_new_config(old_config);

        m_nrf_comm.write(NRF24L01P_PIPE_P0, data, TRANSFER_SIZE);   
        ThisThread::sleep_for(50);

        write_new_config(new_config);

        if(did_receive_acknowledgement(new_config, data)) {
            printf("Updated config on both devices! \r\n");
            delete[] data;
            next_config_index();
            return;
        }

        // send message, incase the other nrf is switched
        m_nrf_comm.write(NRF24L01P_PIPE_P0, data, TRANSFER_SIZE);   

        ThisThread::sleep_for(50);
    }
}

void NRF24::acknowledge_package(){
    if(!m_nrf_comm.readable()) {
        return;
    }
    
    char data[TRANSFER_SIZE];

    m_nrf_comm.read(NRF24L01P_PIPE_P0, data, sizeof(data));

    thread_sleep_for(100);
    printf("received: %s\r\n", data);

    if(strcmp(data, "") == 0) {
        printf("empty!\r\n");
        return;
    }

    if(strcmp(data, MESSAGE) == 0) 
    {
        m_did_receive_other_message = true;
    }
    else if(strcmp(data, NEXT_CONFIG_MESSAGE) == 0) 
    {
        printf("config update message %d \r\n", m_did_receive_other_message);
        if(m_did_receive_other_message) {
            next_config_index();
            write_new_config(generate_test_case(m_test_case_index));
            m_did_receive_other_message = false;
        }
    }

    m_nrf_comm.write(NRF24L01P_PIPE_P0, data, sizeof(data));
}

bool NRF24::did_receive_acknowledgement(NRF24Config& config, char* message) {
    //auto timeout_delay = static_cast<float>(config.auto_retransmission_delay * config.auto_retransmission_count) / 1000.0;
    auto time_passed_in_sec = static_cast<float>(MAX_ACKNOWLEDGMENT_TIMEOUT_MS) / 1000.0;

    Timer timer;
    timer.start();

    while(timer.read() < time_passed_in_sec){
        if(!m_nrf_comm.readable()) {
            continue;
        }

        char* received_data = new char[TRANSFER_SIZE];

        m_nrf_comm.read(NRF24L01P_PIPE_P0, received_data, TRANSFER_SIZE);

        if(strcmp(received_data, message) ==  0) {
            delete[] received_data;
            return true;
        }

        delete[] received_data;
    }

    return false;
}

void NRF24::write_new_config(NRF24Config config) {
    m_nrf_comm.disableAutoRetransmit();
    ThisThread::sleep_for(10);

    // write config settings to nrf
    m_nrf_comm.setRfOutputPower(config.output_power);
    m_nrf_comm.setRfFrequency(config.frequency_channel);
    m_nrf_comm.setAirDataRate(config.data_rate);
    ThisThread::sleep_for(10);
    m_nrf_comm.enableAutoRetransmit(config.auto_retransmission_delay, config.auto_retransmission_count);
}

void NRF24::print_csv_stats(NRF24Config config, int successful_transitions) {
   /**
    * The order of the csv is
    * test name, output power, frequency channel, data rate, art delay, art count, successfully transitions, total transitions
   */
   printf("%s;%d;%d;%d;%d;%d;%d;%d\r\n", 
        config.test_name.c_str(), 
        m_nrf_comm.getRfOutputPower(),
        m_nrf_comm.getRfFrequency(),
        m_nrf_comm.getAirDataRate(),
        config.auto_retransmission_delay, 
        config.auto_retransmission_count, 
        successful_transitions, 
        TOTAL_MESSAGES_TO_TEST);
}

void NRF24::print_nrf_info() {
    // Display the (default) setup of the nRF24L01+ chip
    printf( "nRF24L01+ Frequency    : %d MHz\r\n",  m_nrf_comm.getRfFrequency() );
    printf( "nRF24L01+ Output power : %d dBm\r\n",  m_nrf_comm.getRfOutputPower() );
    printf( "nRF24L01+ Data Rate    : %d kbps\r\n", m_nrf_comm.getAirDataRate() );
    printf( "nRF24L01+ TX Address   : 0x%010llX\r\n", m_nrf_comm.getTxAddress() );
    printf( "nRF24L01+ RX Address   : 0x%010llX\r\n", m_nrf_comm.getRxAddress() );
}

// void NRF24::print_stats(NRF24Config config, int successful_transitions){
//     printf("==============================================\r\n");
//     printf("Packet Error Rate Result of testcase %s\r\n", config.test_name);
//     printf( "The packet Error Rate have     : %d successfully transitions of %d total transitions \r\n",successful_transitions, TOTAL_MESSAGES_TO_TEST);
//     printf("==============================================\r\n");
// }