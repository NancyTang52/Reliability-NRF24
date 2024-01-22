#ifndef NRF24_H
#define NRF24_H

#include "nRF24L01P.h"
#include <string>


#define OUTPUT_POWERS           {-12, 0, -18}
#define FREQUENCY_CHANNELS      {2525, 2463, 2400}
#define DATA_RATES              {1000, 250, 2000}
#define DELAYS                  {250, 1875, 4000}
#define COUNTS                  {8, 0, 16}
#define TOTAL_TEST_CASES        3 * 3 * 3 * 3 * 3

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
    static constexpr char MESSAGE[] = { 'C', 'o', 'd', '\0'};
    static constexpr char NEXT_CONFIG_MESSAGE[] = { 'N', 'N', 'N', '\0'};
    // The nRF24L01+ supports transfers from 1 to 32 bytes, but Sparkfun's
    // "Nordic Serial Interface Board" (http://www.sparkfun.com/products/9019)
    static constexpr int TRANSFER_SIZE = sizeof(MESSAGE);
    static constexpr int MAX_ACKNOWLEDGMENT_TIMEOUT_MS = 25;

    NRF24();

    static NRF24Config generate_test_case(int8_t test_case_number);

    void set_receiver();

    NRF24Config get_current_config();

    void run_packet_error_rate_test();

    void send_test_message();

    void ensure_connection(NRF24Config& config);

    void send_config_update();

    bool did_receive_acknowledgement(NRF24Config& config, char* message);
    
    void acknowledge_package();

    void write_new_config(NRF24Config config);

private:
    void next_config_index();

    // void print_nrf_info();

    // void print_stats(NRF24Config config, int successful_transitions);

    void print_csv_stats(NRF24Config config, int successful_transitions);

    void print_nrf_info();

    // Members
    nRF24L01P m_nrf_comm;
    int8_t m_test_case_index = 0;
    bool m_did_receive_other_message = false;
    static constexpr int TOTAL_MESSAGES_TO_TEST = 100;
};

#endif
