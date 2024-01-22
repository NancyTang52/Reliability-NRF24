#include "nrf.h"
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
    m_nrf_comm.enableAutoAcknowledge(NRF24L01P_PIPE_P0);
    m_nrf_comm.enable();

    print_nrf_info();
}

void NRF24::set_receiver() {
    m_nrf_comm.setReceiveMode();
}

void NRF24::run_packet_error_rate_test(NRF24Config config) {
    int successful_transitions = 0;
    
    for (int i = 0; i < TOTAL_MESSAGES_TO_TEST; i++)
    {
        send_message();

        if(did_receive_acknowledgement(config)) {
            successful_transitions++;
        }

        thread_sleep_for(100);
    }
    
    print_csv_stats(config, successful_transitions);
}

void NRF24::send_message() {
    char* data = new char[TRANSFER_SIZE];
    std::memcpy(data, MESSAGE, TRANSFER_SIZE);

    auto send_bytes = m_nrf_comm.write(NRF24L01P_PIPE_P0, data, TRANSFER_SIZE);   
    delete[] data;
}

void NRF24::ensure_connection(NRF24Config& config) {
    int counter = 0;
    while(counter < 10) {
        send_message();

        ThisThread::sleep_for(100);

        if(did_receive_acknowledgement(config)) {
            counter++;
        } else {
            printf("did NOT receive message \r\n");
        }
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

    m_nrf_comm.write(NRF24L01P_PIPE_P0, data, sizeof(data));
}

bool NRF24::did_receive_acknowledgement(NRF24Config& config) {
    char transfer_message[] = { 'C', 'o', 'd', '\0' };
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

        if(strcmp(received_data, transfer_message) ==  0) {
            return true;
        }

        delete[] received_data;
    }

    return false;
}

void NRF24::write_new_config(NRF24Config config) {
    printf("==============================================\r\n");
    printf("Running testcase %s\r\n", config.test_name.c_str());
    printf("==============================================\r\n");
    
    // write config settings to nrf
    m_nrf_comm.setRfOutputPower(config.output_power);
    m_nrf_comm.setRfFrequency(config.frequency_channel);
    m_nrf_comm.setAirDataRate(config.data_rate);
    m_nrf_comm.enableAutoRetransmit(config.auto_retransmission_delay, config.auto_retransmission_count);

    // print the info to the user, so they know what settings are tested
    print_nrf_info();
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