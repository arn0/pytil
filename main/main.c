#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

#include "esp_chip_info.h"
#include "esp_flash.h"

#include "wifi.h"
#include "tcp_client.h"
#include "control.h"

static const char *TAG = ">>> main";


void app_main( void ){

    uint8_t status = 0;

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "Erase nvs_flash");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

/*
Wi-Fi NVS Flash

If the Wi-Fi NVS flash is enabled, all Wi-Fi configurations set via the Wi-Fi APIs will be stored into flash, and the Wi-Fi driver will
start up with these configurations the next time it powers on/reboots. However, the application can choose to disable the Wi-Fi NVS flash
if it does not need to store the configurations into persistent memory, or has its own persistent storage, or simply due to debugging reasons, etc.
*/



    //xTaskCreate( control_loop, "control_loop", 4096, NULL, 5, NULL );

#define WIFI_INIT_OK BIT0
#define WIFI_AP_FOUND BIT1
#define WIFI_CONNECTED BIT2
#define TCP_SOCKET_OK BIT3
#define TCP_CONNECTED BIT5
#define SERVO_CONNECTED BIT5

    // main loop
    // here we make sure all that is needed is kept alive
    // ontherwise we perserve energy and sleep while waiting
    
    status |= WIFI_AP_FOUND;  //debug

    while( true ){
    //    char buf[32];
    //    size_t len = 32;

        ESP_LOGI(TAG, "Start main loop, status = %x", status);
        if( !( status & WIFI_INIT_OK )){
            ESP_LOGI(TAG, "Need wifi_init(), status = %x", status);
            if( wifi_init() ){
                status |= WIFI_INIT_OK;
            }
            ESP_LOGI(TAG, "Did wifi_init(), status = %x", status);
            // Rest is handled by event loop handlers
        }
//        else if( !( status & WIFI_AP_FOUND ) ){
//            ESP_LOGI(TAG, "Need wifi_scan(), status = %x", status);
//            esp_log_level_set( "*", ESP_LOG_INFO );
//            if( wifi_scan() ){
//                status |= WIFI_AP_FOUND;
//                ESP_LOGI(TAG, "WIFI_AP_FOUND, status = %x", status);
//            }
//            else {
                // lets save some energy and go to sleep
//                ESP_LOGI(TAG, "Want to go to sleep, status = %x", status);
//            }
//        }
//        else if( !( status & WIFI_CONNECTED ) ){
//            ESP_LOGI(TAG, "Need wifi_connect(), status = %x", status);
//            if( wifi_connect() ){
//                status |= WIFI_CONNECTED;
//                ESP_LOGI(TAG, "WIFI_CONNECTED, status = %x", status);
//            }
//        }
//        else if( !( status & TCP_SOCKET_OK ) ){
//            ESP_LOGI(TAG, "Need socket, status = %x", status);
//            if( tcp_get_socket() ) {
//                status |= TCP_SOCKET_OK;
//                ESP_LOGI(TAG, "TCP_SOCKET_OK, status = %x", status);
//            }
//        }
//        else if( !( status & TCP_CONNECTED ) ){
//            ESP_LOGI(TAG, "Need tcp_client(), status = %x", status);
//            tcp_connect_sock();
//        }
        vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
}