#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"

#include "tcp_client.h"
#include "pypi_comm.h"


volatile int sock = -1;

static const char *TAG = "tcp_client():";


void tcp_client(void)
{
    char rx_buffer[128];
    char host_ip[] = SECRET_IP;
    int addr_family = 0;
    int ip_protocol = 0;
    pypi_packet packet;
    int i;

    while (1) {
        struct sockaddr_in dest_addr;
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(SECRET_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
    
        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %d.%d.%d.%d:%d", host_ip[0], host_ip[1], host_ip[2], host_ip[3], SECRET_PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
            if( pypi_get_tx_packet( &packet ) ){
                int err = send(sock, &packet, PYPI_SIZE, 0);
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            if( len >= 0 ) {
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                while( len >= 4 ){
                    packet.byte[0] = rx_buffer[0];
                    packet.byte[1] = rx_buffer[1];
                    packet.byte[2] = rx_buffer[2];
                    packet.byte[3] = rx_buffer[3];
                    pypi_put_rx_packet( packet );
                
                    for( i = 0; i < len; i++ ){
                        rx_buffer[i] = rx_buffer[i+4];
                    }
                    len =- 4;
                }
            } else {
            // Error occurred during receiving
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

bool tcp_open( void ) {
    char host_ip[] = SECRET_IP;

    struct sockaddr_in dest_addr;
    inet_pton( AF_INET, host_ip, &dest_addr.sin_addr );
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons( SECRET_PORT );

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return( false );
    }
    ESP_LOGI(TAG, "Socket created (%d), connecting to %d.%d.%d.%d:%d", sock, host_ip[0], host_ip[1], host_ip[2], host_ip[3], SECRET_PORT);
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        return( false );
    }
    ESP_LOGI(TAG, "Successfully connected");
    return( true );
}