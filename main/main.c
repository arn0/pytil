#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_transport.h"
#include "esp_transport_tcp.h"


#include "../../secret.h"

#define ESP_MAXIMUM_RETRY  8
#define EXAMPLE_H2E_IDENTIFIER ""

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = ">>> main";        // For debugging

static int s_retry_num = 0;

static void event_handler_wifi( void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
static const char *TAG = ">>> event_handler_wifi";

    if( event_base == WIFI_EVENT ) {
        switch( event_id ) {
            case WIFI_EVENT_WIFI_READY:           /**< WiFi ready */
                ESP_LOGI( TAG, "WIFI_EVENT_WIFI_READY" );
            break;
            case WIFI_EVENT_SCAN_DONE:            /**< Finished scanning AP */
                ESP_LOGI( TAG, "WIFI_EVENT_SCAN_DONE" );
            break;
            case WIFI_EVENT_STA_START:            /**< Station start */
                ESP_LOGI( TAG, "WIFI_EVENT_STA_START" );
                esp_wifi_connect();
                ESP_LOGI( TAG, "done esp_wifi_connect()" );
            break;
            case WIFI_EVENT_STA_STOP:             /**< Station stop */
                ESP_LOGI( TAG, "WIFI_EVENT_STA_STOP" );
            break;
            case WIFI_EVENT_STA_CONNECTED:        /**< Station connected to AP */
                ESP_LOGI( TAG, "WIFI_EVENT_STA_CONNECTED" );
            break;
            case WIFI_EVENT_STA_DISCONNECTED:     /**< Station disconnected from AP */
                ESP_LOGI( TAG, "WIFI_EVENT_STA_DISCONNECTED" );
                if (s_retry_num < ESP_MAXIMUM_RETRY) {
                    esp_wifi_connect();
                    s_retry_num++;
                    ESP_LOGI( TAG, "done esp_wifi_connect(), retry %d", s_retry_num );
                    if ( s_retry_num > 1 ) {
                        ESP_LOGI( TAG, "delay by %d s, retry %d", s_retry_num * 4 + 4, s_retry_num );
                        vTaskDelay( s_retry_num * 4000 + 4000 / portTICK_PERIOD_MS );
                    }
                    ESP_LOGI( TAG, "retry to connect to the AP" );
                } else {
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                    ESP_LOGI(TAG,"connect to the AP failed");
                }
            break;
            case WIFI_EVENT_STA_AUTHMODE_CHANGE:  /**< the auth mode of AP connected by device's station changed */
                ESP_LOGI( TAG,  "WIFI_EVENT_STA_AUTHMODE_CHANGE" );
            break;
            default:
                ESP_LOGI( TAG,  "event_id = %lx", event_id );
        }
    }
    else {
        ESP_LOGI( TAG,  "unknown event! event_base = %hhd event_id = %ld", *event_base, event_id );
    }
}

static void event_handler_ip( void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
static const char *TAG = ">>> event_handler_ip";
    switch( event_id ) {
        case IP_EVENT_STA_GOT_IP:               /*!< station got IP from connected AP */
            ip_event_got_ip_t* event = ( ip_event_got_ip_t* ) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR( &event->ip_info.ip ));
            s_retry_num = 0;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            ESP_LOGI( TAG, "IP_EVENT_STA_GOT_IP" );
        break;
        case IP_EVENT_STA_LOST_IP:              /*!< station lost IP and the IP is reset to 0 */
            ESP_LOGI( TAG, "IP_EVENT_STA_LOST_IP" );
        break;
        case IP_EVENT_AP_STAIPASSIGNED:         /*!< soft-AP assign an IP to a connected station */
            ESP_LOGI( TAG, "IP_EVENT_AP_STAIPASSIGNED" );
        break;
        case IP_EVENT_GOT_IP6:                  /*!< station or ap or ethernet interface v6IP addr is preferred */
            ESP_LOGI( TAG, "IP_EVENT_GOT_IP6" );
        break;
        case IP_EVENT_ETH_GOT_IP:               /*!< ethernet got IP from connected AP */
            ESP_LOGI( TAG, "IP_EVENT_ETH_GOT_IP" );
        break;
        case IP_EVENT_ETH_LOST_IP:              /*!< ethernet lost IP and the IP is reset to 0 */
            ESP_LOGI( TAG, "IP_EVENT_ETH_LOST_IP" );
        break;
        case IP_EVENT_PPP_GOT_IP:               /*!< PPP interface got IP */
            ESP_LOGI( TAG, "IP_EVENT_PPP_GOT_IP" );
        break;
        case IP_EVENT_PPP_LOST_IP:              /*!< PPP interface lost IP */
            ESP_LOGI( TAG, "IP_EVENT_PPP_LOST_IP" );
        break;
        default:
            ESP_LOGI( TAG, "event_base = %hhd event_id %ld", *event_base, event_id );
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK( esp_netif_init() );                    // Create an LwIP core task and initialize LwIP-related work
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );               // This call triggers efuse and NVS read

    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;
    ESP_ERROR_CHECK( esp_event_handler_instance_register( WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler_wifi, NULL, &instance_wifi_event ) );
    ESP_ERROR_CHECK( esp_event_handler_instance_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler_ip, NULL, &instance_ip_event ) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SECRET_SSID,
            .password = SECRET_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_STA ) );
    ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_STA, &wifi_config ) );
    ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_LOGI( TAG, "wifi_init_sta finished." );

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if ( bits & WIFI_CONNECTED_BIT ) {
        ESP_LOGI( TAG, "connected to ap SSID:%s password:%s", SECRET_SSID, SECRET_PASS );
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI( TAG, "Failed to connect to SSID:%s, password:%s", SECRET_SSID, SECRET_PASS );
    } else {
        ESP_LOGE( TAG, "UNEXPECTED EVENT");
    }
}

static const char *payload = "Message from ESP32\n";


static void tcp_transport_client_task(void *pvParameters)
{


    static const char* TAG = ">>> tcp";
    char rx_buffer[128];
    char host_ip[] = SECRET_IP;
    esp_transport_handle_t transport = esp_transport_tcp_init();

    while (1) {
        if (transport == NULL) {
            ESP_LOGE(TAG, "Error occurred during esp_transport_proxy_init()");
            break;
        }
        int err = esp_transport_connect(transport, SECRET_IP, SECRET_PORT, -1);
        if (err != 0) {
            ESP_LOGE(TAG, "Client unable to connect: errno %d", errno);
            break;
            ESP_LOGE(TAG, "Below break");
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
    ESP_LOGE(TAG, "Exit first while(1)");
    esp_transport_destroy(transport);

    ESP_LOGE(TAG, "At vTaskDelete");
    vTaskDelete(NULL);
}



void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if ( ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
      ESP_ERROR_CHECK( nvs_flash_erase() );
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    vTaskDelay( 5000 / portTICK_PERIOD_MS );

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG( SECRET_IP );
    esp_netif_sntp_init( &config );
    if (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(10000)) != ESP_OK) {
        printf("Failed to update system time within 10s timeout/n");
}

    xTaskCreate(tcp_transport_client_task, "tcp_transport_client", 4096, NULL, 5, NULL);

    vTaskDelay( 10000 / portTICK_PERIOD_MS );
}