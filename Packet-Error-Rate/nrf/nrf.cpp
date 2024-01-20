#include "nrf.h"
#include <cstdio>

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
}

void NRF24::run_packet_error_rate_test(NRF24Config config) {
    int successful_transitions = 0;

    for (int i = 0; i < TOTAL_MESSAGES_TO_TEST; i++)
    {
        char* data = new char[TRANSFER_SIZE];
        std::memcpy(data, TRANSFER_MESSAGE, sizeof(TRANSFER_SIZE));

        auto send_bytes = m_nrf_comm.write(NRF24L01P_PIPE_P0, data, sizeof(TRANSFER_SIZE));   

        if(did_receive_acknowledgement()) {
            successful_transitions++;
        }
    }
    
    // print_stats(config, successful_transitions);
    print_csv_stats(config, successful_transitions);
}

bool NRF24::did_receive_acknowledgement() {
    Timer timer;
    timer.start();

    while(timer.read() < MAX_ACKNOWLEDGMENT_TIMEOUT_MS){
        char* received_data = new char[TRANSFER_SIZE];

        m_nrf_comm.read(NRF24L01P_PIPE_P0, received_data, TRANSFER_SIZE);

        if(received_data == TRANSFER_MESSAGE) {
            return true;
        }
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
    // print_nrf_info();
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

// void NRF24::print_stats(NRF24Config config, int successful_transitions){
//     printf("==============================================\r\n");
//     printf("Packet Error Rate Result of testcase %s\r\n", config.test_name);
//     printf( "The packet Error Rate have     : %d successfully transitions of %d total transitions \r\n",successful_transitions, TOTAL_MESSAGES_TO_TEST);
//     printf("==============================================\r\n");
// }