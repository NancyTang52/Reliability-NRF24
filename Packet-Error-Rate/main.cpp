#include "mbed.h"
#include "nrf/nrf.h"
#include <vector>
#include <string>

#define OUTPUT_POWERS           {0, -12, -18}
#define FREQUENCY_CHANNELS      {2400, 2463, 2525}
#define DATA_RATES              {250, 1000, 2000}
#define DELAYS                  {250, 1875, 4000}
#define COUNTS                  {0, 8, 16}
#define TOTAL_TEST_CASES        3 * 3 * 3 * 3 * 3

#define TRANSMITTER     true
#define REPEAT_TESTCASE 5

NRF24Config generate_test_case(int test_case_number) {
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

void run_test_cases(NRF24& nrf24) {
    printf("start \r\n");

    for (size_t i = 0; i < TOTAL_TEST_CASES; i++)
    {
        for (size_t j = 0; j < REPEAT_TESTCASE; j++)
        {
            auto test_case = generate_test_case(i);
            nrf24.run_packet_error_rate_test(test_case);
        }
    }
    
    printf("end \r\n");
}

void receive_test_cases(NRF24& nrf24) {
    nrf24.set_receiver();
    while(true) {
        nrf24.acknowledge_package();
    }
}

int main() {
    NRF24 nrf24;

    auto config = NRF24Config {
        /* test_name: */ "",
        /* output_power: */ -12,
        /* frequency_channel: */ 2500,
        /* data_rate: */ 1000,
        /* auto_retransmission_delay: */ 250,
        /* auto_retransmission_count: */ 5,
    };

    nrf24.write_new_config(config);

    if(TRANSMITTER) 
    {
        nrf24.ensure_connection(config);
        printf("Starting transmitter with %d testcases\r\n", TOTAL_TEST_CASES);
        run_test_cases(nrf24);
    } 
    else 
    {
        printf("receiver \r\n");
        receive_test_cases(nrf24);
    }
}
