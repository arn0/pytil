#include <stdio.h>

#define PYPI_SIZE 4

typedef union { uint32_t packed;
                uint8_t byte[4];
                struct p { uint8_t command;
                          uint16_t value;
                          uint8_t crc
                        }p;     // clearup error
} pypi_packet;


bool pypi_put_rx_packet( pypi_packet packet );
bool pypi_get_rx_packet( pypi_packet *packet );
bool pypi_put_tx_packet( pypi_packet packet );
bool pypi_get_tx_packet( pypi_packet *packet );
