#include "mbed.h"
#include "nrf/nrf.h"

void run_test_cases(const NRF24& nrf24) {
    int output_powers = {};
    int frequency_channels[] = {};
    int data_rates[] = {};
    int delays[] = {};
    int counts[] = {};

    // NOTE: this is a programming war crime, but it works :)
    for (size_t output_power = 0; output_power < output_powers; output_power++)
    {
        for (size_t frequency_channel = 0; frequency_channel < frequency_channels; frequency_channel++)
        {
            for (size_t data_rate = 0; data_rate < data_rates; data_rate++)
            {
                for (size_t delay = 0; delay < delays; delay++)
                {
                    for (size_t count = 0; count < counts; count++)
                    {
                        auto config = NRF24Config {
                            /* test_name: */ "Hello world",
                            /* output_power: */ -12,
                            /* frequency_channel: */ 2400,
                            /* data_rate: */ 10,
                            /* auto_retransmission_delay: */ 5, 
                            /* auto_retransmission_count: */ 6,
                        };
                    }
                }
            }
        }
    }
}

int main() {
    NRF24 nrf24;

    run_test_cases(nrf24);
}
