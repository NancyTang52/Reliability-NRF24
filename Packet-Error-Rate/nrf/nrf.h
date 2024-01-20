#ifndef NRF24_H
#define NRF24_H

#include "nRF24L01P.h"
#include <string>

struct NRF24Config {
    string test_name;
    int output_power;
    int frequency_channel;
    int data_rate;
    int auto_retransmission_delay;
    int auto_retransmission_count;
};

class NRF24 {

public:
    // The nRF24L01+ supports transfers from 1 to 32 bytes, but Sparkfun's
    // "Nordic Serial Interface Board" (http://www.sparkfun.com/products/9019)
    static constexpr int TRANSFER_SIZE = 4;
    // NOTE: the transfer size and message size need to be the same!
    static constexpr char* TRANSFER_MESSAGE = "ABCD";
    static constexpr int MAX_ACKNOWLEDGMENT_TIMEOUT_MS = 250;

    NRF24();

    void run_packet_error_rate_test(NRF24Config config);
    
private:

    bool did_receive_acknowledgement();

    void write_new_config(NRF24Config config);

    // void print_nrf_info();

    // void print_stats(NRF24Config config, int successful_transitions);

    void print_csv_stats(NRF24Config config, int successful_transitions);

    // Members
    nRF24L01P m_nrf_comm;
    static constexpr int TOTAL_MESSAGES_TO_TEST = 10'000;
};

#endif
