#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"

#include "protocol_examples_common.h"

#include "tcp_transport_client.h"
#include "../../secret.h"


void app_main(void)
{
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_proxy", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG( SECRET_ADDR );
    esp_netif_sntp_init( &config );
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
        printf("Failed to update system time within 10s timeout/n");
    }
    xTaskCreate(tcp_transport_client_task, "tcp_transport_client", 4096, NULL, 5, NULL);
}
