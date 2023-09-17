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

static const char *TAG = "main";


void app_main( void ){

    uint8_t status = 0;

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());


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
#define TCP_CONNECTED BIT3
#define SERVO_CONNECTED BIT4

    // main loop
    // here we make sure all that is needed is kept alive
    // ontherwise we perserve energy and sleep while waiting
    
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
            // Need to wait for wifi to start up ???
            //vTaskDelay( 4000 );
        }
        else if( !( status & WIFI_AP_FOUND ) ){
            ESP_LOGI(TAG, "Need wifi_scan(), status = %x", status);
            esp_log_level_set( "*", ESP_LOG_INFO );
            if( wifi_scan() ){
                status |= WIFI_AP_FOUND;
                ESP_LOGI(TAG, "WIFI_AP_FOUND, status = %x", status);
            }
            else {
                // lets save some energy and go to sleep
                ESP_LOGI(TAG, "Want to go to sleep, status = %x", status);
            }
        }
        else if( !( status & WIFI_CONNECTED ) ){
            ESP_LOGI(TAG, "Need wifi_connect(), status = %x", status);
            if( wifi_connect() ){
                status |= WIFI_CONNECTED;
                ESP_LOGI(TAG, "WIFI_CONNECTED, status = %x", status);
            }
        }
        else if( !( status & TCP_CONNECTED ) ){
            ESP_LOGI(TAG, "Need tcp_client(), status = %x", status);
            tcp_open();
        }
    }
}