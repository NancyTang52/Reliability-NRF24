#include "nrf.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

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
    return generate_test_case(m_test_case);
}

void NRF24::run_throughput_test() {
    m_total_duration = 0;
    auto config = get_current_config();
    write_new_config(config);

    uint32_t message_send = 0;

    for (uint32_t i = 1; i <= TOTAL_MESSAGES_TO_TEST; i++)
    {
        printf("Sending: %d\r\n", i);
        // Convert 32 bit number to char pointer
        char* number_to_send = reinterpret_cast<char*>(&i);

        do
        {
            message_send++;
            send_message(number_to_send);
        } while(!did_receive_acknowledgement(config, number_to_send));
        
        // NOTE: don't delete the number_to_send pointer because it will delete the value of i.
        // delete number_to_send;
    }

    print_csv_stats(config, m_total_duration, message_send);
}

void NRF24::send_message(char* data, bool should_measure) {
    Timer duration_timer;

    duration_timer.start();
    auto send_bytes = m_nrf_comm.write(NRF24L01P_PIPE_P0, data, TRANSFER_SIZE);   
    duration_timer.stop();
    if(should_measure) {
        m_total_duration += duration_timer.read_high_resolution_us();
    }
}

bool NRF24::did_receive_acknowledgement(NRF24Config& config, char* message) {
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

void NRF24::ensure_connection(NRF24Config& config) {
    uint32_t number = 1000;
    // Convert 32 bit number to char pointer
    char* data = reinterpret_cast<char*>(&number);
    int counter = 0;
    while(counter < 10) {
        send_message(data, false);

        ThisThread::sleep_for(100);

        if(did_receive_acknowledgement(config, data)) {
            counter++;
        } else {
            printf("did NOT receive message \r\n");
        }
    }
    printf("Connection ensured \r\n");
}

void NRF24::acknowledge_package(){
    if(!m_nrf_comm.readable()) {
        return;
    }

    char* data = new char[TRANSFER_SIZE];

    m_nrf_comm.read(NRF24L01P_PIPE_P0, data, TRANSFER_SIZE);

    uint32_t number;
    memcpy(&number, data, TRANSFER_SIZE);
    printf("received: %d\r\n", number);

    m_nrf_comm.write(NRF24L01P_PIPE_P0, data, sizeof(data));

    delete[] data;
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

void NRF24::print_csv_stats(NRF24Config config, us_timestamp_t duration, uint32_t message_send) {
   /**
    * The order of the csv is
    * test name, output power, frequency channel, data rate, art delay, art count, duration of transmition, total message send
   */
   printf("%s;%d;%d;%d;%d;%d;%lld;%d\r\n", 
        config.test_name.c_str(), 
        m_nrf_comm.getRfOutputPower(),
        m_nrf_comm.getRfFrequency(),
        m_nrf_comm.getAirDataRate(),
        config.auto_retransmission_delay, 
        config.auto_retransmission_count, 
        duration,
        message_send);
}

void NRF24::print_nrf_info() {
    // Display the (default) setup of the nRF24L01+ chip
    printf( "nRF24L01+ Frequency    : %d MHz\r\n",  m_nrf_comm.getRfFrequency() );
    printf( "nRF24L01+ Output power : %d dBm\r\n",  m_nrf_comm.getRfOutputPower() );
    printf( "nRF24L01+ Data Rate    : %d kbps\r\n", m_nrf_comm.getAirDataRate() );
    printf( "nRF24L01+ TX Address   : 0x%010llX\r\n", m_nrf_comm.getTxAddress() );
    printf( "nRF24L01+ RX Address   : 0x%010llX\r\n", m_nrf_comm.getRxAddress() );
}