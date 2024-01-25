#include "mbed.h"
#include "nrf/nrf.h"
#include <cstdint>
#include <cstring>
#include <vector>

#define TRANSMITTER     true
#define REPEAT_TESTCASE 4

void run_test_case(NRF24& nrf24) {
    printf("start \r\n");

    for (size_t j = 0; j < REPEAT_TESTCASE; j++)
    {    
        nrf24.run_troughput_test();
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

    uint32_t value = 1000;

    char* data = reinterpret_cast<char*>(&value);

    MBED_ASSERT(sizeof(data) == sizeof(uint32_t));

    char arr[sizeof(data)];

    memcpy(&arr, data, sizeof(uint32_t));

    uint32_t number;
    
    memcpy(&number, arr, sizeof(number));

    printf("%d \r\n", number);


    if(TRANSMITTER) 
    {
        auto config = nrf24.get_current_config();
        nrf24.ensure_connection(config);
        printf("Transmitter \r\n");
        run_test_case(nrf24);
    } 
    else 
    {
        printf("receiver \r\n");
        receive_test_cases(nrf24);
    }
}
