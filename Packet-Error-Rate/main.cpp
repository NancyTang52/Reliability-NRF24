#include "mbed.h"
#include "nrf/nrf.h"
#include <vector>

#define TRANSMITTER     false
#define REPEAT_TESTCASE 4

void run_test_cases(NRF24& nrf24) {
    printf("start \r\n");

    for (size_t i = 0; i < TOTAL_TEST_CASES; i++)
    {
        for (size_t j = 0; j < REPEAT_TESTCASE; j++)
        {    
            nrf24.run_packet_error_rate_test();
        }

        nrf24.send_config_update();
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

    if(TRANSMITTER) 
    {
        auto config = nrf24.get_current_config();
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
