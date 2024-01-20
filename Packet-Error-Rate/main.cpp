#include "mbed.h"
#include "nRF24L01P.h"

#define MOSI_PIN    D11
#define MISO_PIN    D12
#define SCK_PIN     D13
#define CSN_PIN     D8 // D10 <- MCP
#define CE_PIN      D9
#define IRQ_PIN     D7

Serial pc(USBTX, USBRX); // tx, rx
nRF24L01P my_nrf24l01p(MOSI_PIN, MISO_PIN, SCK_PIN, CSN_PIN, CE_PIN, IRQ_PIN);

DigitalOut myled1(LED1);

int main() {
    // NOTE: this is to fix the powerup, getting stuck
    // Don't know why, but is a fix :)
    ////////////////////////////////////////////
    SPI spi(MOSI_PIN, MISO_PIN, SCK_PIN);
    spi.format(8, 3);
    ////////////////////////////////////////////

    pc.printf("Starting Syncronized Leds \r\n");

    // The nRF24L01+ supports transfers from 1 to 32 bytes, but Sparkfun's
    // "Nordic Serial Interface Board" (http://www.sparkfun.com/products/9019)
    // only handles 4 byte transfers in the ATMega code.
    #define TRANSFER_SIZE   4

    char txData[TRANSFER_SIZE], rxData[TRANSFER_SIZE];
    int txDataCnt = 0;
    int rxDataCnt = 0;

    my_nrf24l01p.powerUp();

    pc.printf("Powered the NRF24 \r\n");

    // Display the (default) setup of the nRF24L01+ chip
    pc.printf( "nRF24L01+ Frequency    : %d MHz\r\n",  my_nrf24l01p.getRfFrequency() );
    pc.printf( "nRF24L01+ Output power : %d dBm\r\n",  my_nrf24l01p.getRfOutputPower() );
    pc.printf( "nRF24L01+ Data Rate    : %d kbps\r\n", my_nrf24l01p.getAirDataRate() );
    pc.printf( "nRF24L01+ TX Address   : 0x%010llX\r\n", my_nrf24l01p.getTxAddress() );
    pc.printf( "nRF24L01+ RX Address   : 0x%010llX\r\n", my_nrf24l01p.getRxAddress() );

    pc.printf( "Type keys to test transfers:\r\n  (transfers are grouped into %d characters)\r\n", TRANSFER_SIZE );

    my_nrf24l01p.setTransferSize( TRANSFER_SIZE );

    my_nrf24l01p.setReceiveMode();
    my_nrf24l01p.enable();

    while (1) {

        // If we've received anything over the host serial link...
        if ( pc.readable() ) {

            // ...add it to the transmit buffer
            txData[txDataCnt++] = pc.getc();

            // If the transmit buffer is full
            if ( txDataCnt >= sizeof( txData ) ) {

                // Send the transmitbuffer via the nRF24L01+
                my_nrf24l01p.write( NRF24L01P_PIPE_P0, txData, txDataCnt );

                txDataCnt = 0;
            }

            // Toggle LED1 (to help debug Host -> nRF24L01+ communication)
            myled1 = !myled1;
        }

        // If we've received anything in the nRF24L01+...
        if ( my_nrf24l01p.readable() ) {

            // ...read the data into the receive buffer
            rxDataCnt = my_nrf24l01p.read( NRF24L01P_PIPE_P0, rxData, sizeof( rxData ) );

            // Display the receive buffer contents via the host serial link
            for ( int i = 0; rxDataCnt > 0; rxDataCnt--, i++ ) {

                pc.putc( rxData[i] );
            }
        }
    }
}
