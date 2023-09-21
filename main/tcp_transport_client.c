/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"
#include "esp_transport_socks_proxy.h"

#include "../../secret.h"


static const char *TAG = "tcp_transport_client";
static const char *payload = "Message from ESP32\n";

void tcp_transport_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char host_ip[] = SECRET_ADDR;
    esp_transport_handle_t transport = esp_transport_tcp_init();

  while (1) {
        if (transport == NULL) {
            ESP_LOGE(TAG, "Error occurred during esp_transport_proxy_init()");
            break;
        }
        int err = esp_transport_connect( transport, SECRET_ADDR, SECRET_PORT, -1 );
        if (err != 0) {
            ESP_LOGE(TAG, "Client unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

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