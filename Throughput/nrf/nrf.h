#ifndef NRF24_H
#define NRF24_H

#include "nRF24L01P.h"
#include <string>


#define OUTPUT_POWERS           {-12}
#define FREQUENCY_CHANNELS      {2525}
#define DATA_RATES              {1000, 250, 2000}
#define DELAYS                  {1875}
#define COUNTS                  {8}
#define TOTAL_TEST_CASES        1 * 1 * 3 * 1 * 1

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
    static constexpr int TRANSFER_SIZE = sizeof(uint32_t);
    static constexpr int MAX_ACKNOWLEDGMENT_TIMEOUT_MS = 25;

    NRF24();

    static NRF24Config generate_test_case(int16_t test_case_number);

    void set_receiver();

    NRF24Config get_current_config();

    void run_troughput_test();

    void send_message(char* data, bool should_measure = true);

    void ensure_connection(NRF24Config& config);

    void acknowledge_package();

private:
    bool did_receive_acknowledgement(NRF24Config& config, char* message);

    void write_new_config(NRF24Config config);

    void print_csv_stats(NRF24Config config, us_timestamp_t duration, uint32_t message_send);

    void print_nrf_info();

    // Members
    nRF24L01P m_nrf_comm;
    int16_t m_test_case = 2;
    bool m_did_receive_other_message = false;

    us_timestamp_t m_total_duration = 0;
    static constexpr uint32_t TOTAL_MESSAGES_TO_TEST = 10'000;
};

#endif
