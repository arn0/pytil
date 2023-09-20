#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"

#include "pi_transport.h"
#include "../../secret.h"


static const char *payload = "Message from ESP32\n";


void tcp_transport_client_task( void *pvParameters )
{
    static const char *TAG = ">>> tcp_transport_client_task";

    char rx_buffer[128];
    char host_ip[] = SECRET_IP;
    esp_transport_handle_t transport = esp_transport_tcp_init();

    ESP_LOGI( TAG, "started" );

    while ( true ) {
        if ( transport == NULL ) {
            ESP_LOGE( TAG, "Error occurred during esp_transport_init()" );
            break;
        }
        ESP_LOGI( TAG, "call esp_transport_connect()" );
        int err = esp_transport_connect( transport, SECRET_IP, SECRET_PORT, -1 );
        ESP_LOGI( TAG, "call esp_transport_connect() returned errno %d", errno );
        if ( err != 0 ) {
            ESP_LOGE( TAG, "Client unable to connect: errno %d", errno );
            break;
        }
        ESP_LOGI( TAG, "Successfully connected" );

        while (1) {
            int bytes_written = esp_transport_write(transport, payload, strlen(payload), 0);
            if (bytes_written < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: esp_transport_write() returned %d, errno %d", bytes_written, errno);
                break;
            }
            int len = esp_transport_read(transport, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: esp_transport_read() returned %d, errno %d", len, errno);
                break;
            }
            // Data received
            rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
            ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
            ESP_LOGI(TAG, "Received data : %s", rx_buffer);

            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        ESP_LOGE(TAG, "Shutting down transport and restarting...");
        esp_transport_close(transport);
    }
    esp_transport_destroy(transport);

  vTaskDelete(NULL);
}
