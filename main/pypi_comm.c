// Communication between TCP and pypilot control.




/*
The program uses a simple protocol to ensure only
correct data can be received and to ensure that
false/incorrect or random data is very unlikely to
produce motor movement.

The input and output over uart has 4 byte packets

The first byte is the command or register, the next
two bytes is a 16bit value (signed or unsigned)
the last byte is a crc8 of the first 3 bytes

If incoming data has the correct crc for a few frames
the command can be recognized.
*/

#include <inttypes.h>
#include <stdbool.h>

#include "pypi_comm.h"
#include "crc.h"

static const char *TAG = ">>> pypi_comm";

pypi_packet pypi_rx_buffer[4];
pypi_packet pypi_tx_buffer[4];

int8_t pypi_rx_len = 0;
int8_t pypi_tx_len = 0;


bool pypi_put_rx_packet( pypi_packet packet ){
    if( crc8( packet.byte, 3 ) == packet.byte[3] ){
        if( pypi_rx_len < 4 ) {
            pypi_rx_buffer[pypi_rx_len] = packet;
            pypi_rx_len++;
            return( true );
        }
    }
    return( false );    //error
}

 bool pypi_get_rx_packet( pypi_packet *packet ){
    if( pypi_rx_len > 0 ){
        *packet = pypi_rx_buffer[pypi_rx_len-1];
        for( int i = 0; i < pypi_rx_len-1; i++){
            pypi_rx_buffer[i] = pypi_rx_buffer[i+1];
        }
        pypi_rx_len--;
        return( true );
    }
    return( false );    //error

}

bool pypi_put_tx_packet( pypi_packet packet ){
    if( pypi_tx_len < 4 ) {
        packet.byte[3] = crc8( packet.byte, 3 );
        pypi_tx_buffer[pypi_tx_len] = packet;
        pypi_tx_len++;
        return( true );
    }
    return( false );    //error
}

bool pypi_get_tx_packet( pypi_packet *packet ){
    if( pypi_tx_len > 0 ){
        *packet = pypi_tx_buffer[pypi_tx_len-1];
        for( int i = 0; i < pypi_tx_len-1; i++){
            pypi_tx_buffer[i] = pypi_tx_buffer[i+1];
        }
        pypi_tx_len--;
        return( true );
    }
    return( false );    //error
}