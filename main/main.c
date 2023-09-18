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

static const char *TAG = "main(): ";

// `Keep track of the systemwide status of all subprocceses

#define WIFI_EVENT_OK BIT0
#define WIFI_NVS_OK BIT1
#define WIFI_INIT_OK BIT2
#define WIFI_AP_FOUND BIT3
#define WIFI_CONNECTED BIT4
#define SOCKET_OK BIT5
#define TCP_CONNECTED BIT6
#define SERVO_CONNECTED BIT7

volatile uint16_t esp_status = 0;


void app_main( void ){

/*
Wi-Fi NVS Flash

If the Wi-Fi NVS flash is enabled, all Wi-Fi configurations set via the Wi-Fi APIs will be stored into flash, and the Wi-Fi driver will
start up with these configurations the next time it powers on/reboots. However, the application can choose to disable the Wi-Fi NVS flash
if it does not need to store the configurations into persistent memory, or has its own persistent storage, or simply due to debugging reasons, etc.
*/

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGI(TAG, "Erase nvs_flash");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Reading NVS takes a while, so no point in checking here

    // main loop
    // here we make sure all that is needed is kept alive
    // ontherwise we perserve energy and sleep while waiting
    
    esp_log_level_set( "*", ESP_LOG_INFO );

    while( true ) {
        switch ( esp_status )
        {
        case 0:
            wifi_start();
            break;
        
        default:
            ESP_LOGI(TAG, "In loop, status = %x", esp_status);
            vTaskDelay( 1000 );
            break;
        }


    }
}