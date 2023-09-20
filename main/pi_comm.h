#include <stdint.h>

#define PYCOMM_BUFFER_SIZE 16

typedef union { uint32_t packed;
                uint8_t byte[4];
                struct { uint8_t command;
                          uint16_t value;
                          uint8_t crc
                        } p;
} pypi_packet;


bool pypi_put_rx_packet( pypi_packet packet );
bool pypi_get_rx_packet( pypi_packet *packet );
bool pypi_put_tx_packet( pypi_packet packet );
bool pypi_get_tx_packet( pypi_packet *packet );