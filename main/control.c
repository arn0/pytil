#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include <inttypes.h>
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "wifi.h"
#include "tcp_client.h"
#include "control.h"


#include "pypi_comm.h"

static const char *TAG = "control_loop():";


enum commands {COMMAND_CODE=0xc7, RESET_CODE=0xe7, MAX_CURRENT_CODE=0x1e, MAX_CONTROLLER_TEMP_CODE=0xa4, MAX_MOTOR_TEMP_CODE=0x5a, RUDDER_RANGE_CODE=0xb6, RUDDER_MIN_CODE=0x2b, RUDDER_MAX_CODE=0x4d, REPROGRAM_CODE=0x19, DISENGAGE_CODE=0x68, MAX_SLEW_CODE=0x71, EEPROM_READ_CODE=0x91, EEPROM_WRITE_CODE=0x53, CLUTCH_PWM_AND_BRAKE_CODE=0x36};

enum results {CURRENT_CODE=0x1c, VOLTAGE_CODE=0xb3, CONTROLLER_TEMP_CODE=0xf9, MOTOR_TEMP_CODE=0x48, RUDDER_SENSE_CODE=0xa7, FLAGS_CODE=0x8f, EEPROM_VALUE_CODE=0x9a};

enum {SYNC=1, OVERTEMP_FAULT=2, OVERCURRENT_FAULT=4, ENGAGED=8, INVALID=16, PORT_PIN_FAULT=32, STARBOARD_PIN_FAULT=64, BADVOLTAGE_FAULT=128, MIN_RUDDER_FAULT=256, MAX_RUDDER_FAULT=512, CURRENT_RANGE=1024, BAD_FUSES=2048, /* PORT_FAULT=4096  STARBOARD_FAULT=8192 */ REBOOTED=32768};



struct struct_control_settings { unsigned int low_current;
                          unsigned int max_current;
                          unsigned int max_controller_temp;
                          unsigned int max_motor_temp;
                          unsigned int rudder_min;
                          unsigned int rudder_max;
                          unsigned int max_slew_speed;
                          unsigned int max_slew_slow;
                          unsigned char clutch_pwm;
                          unsigned char use_brake;
};


uint16_t flags = 0;
pypi_packet packet;
struct struct_control_settings control_settings;

void stop( void ){}

void process_packet( pypi_packet packet )
{
    flags |= SYNC;

    switch( packet.p.command ) {
        case REPROGRAM_CODE:
            ESP_LOGI(TAG, "Received REPROGRAM_CODE, value %d:", packet.p.value);
            break;
 
         case RESET_CODE:
                                                                // reset overcurrent flag
            flags &= ~OVERCURRENT_FAULT;
            break;
        
        case COMMAND_CODE:
            //  timeout = 0;
            //  if(serialin < 12)
            //      serialin+=4; // output at input rate
            if( packet.p.value > 2000 );
                                                                // unused range, invalid!!!
            else if(flags & (OVERTEMP_FAULT | OVERCURRENT_FAULT | BADVOLTAGE_FAULT));
                                                                // no command because of overtemp or overcurrent or badvoltage
            else if((flags & (PORT_PIN_FAULT | MAX_RUDDER_FAULT)) && packet.p.value > 1000)
                stop();                                         // no forward command if port fault
            else if((flags & (STARBOARD_PIN_FAULT | MIN_RUDDER_FAULT)) && packet.p.value < 1000)
                stop();                                         // no starboard command if port fault
            else {
                // brake_on = use_brake;
                // command_value = packet.p.value;
                //engage();
            }
            break;

        case MAX_CURRENT_CODE:                                  // current in units of 10mA
            unsigned int max_max_current = control_settings.low_current ? 2000 : 5000;
            if( packet.p.value > max_max_current)               // maximum is 20 or 50 amps
                packet.p.value = max_max_current;
            control_settings.max_current = packet.p.value;
            break;
    
        case MAX_CONTROLLER_TEMP_CODE:
            if( packet.p.value > 10000 )            // maximum is 100C
                packet.p.value = 10000;
            control_settings.max_controller_temp = packet.p.value;
            break;

        case MAX_MOTOR_TEMP_CODE:
            if( packet.p.value > 10000)                       // maximum is 100C
                packet.p.value = 10000;
            control_settings.max_motor_temp = packet.p.value;
            break;

        case RUDDER_MIN_CODE:
            control_settings.rudder_min = packet.p.value;
            break;

        case RUDDER_MAX_CODE:
            control_settings.rudder_max = packet.p.value;
            break;

        case DISENGAGE_CODE:
            // if(serialin < 12)
                // serialin+=4; // output at input rate
            //disengage();
            break;

        case MAX_SLEW_CODE:
            control_settings.max_slew_speed = packet.byte[1];
            control_settings.max_slew_slow = packet.byte[2];

            // if set at the end of range (up to 255)  no slew limit
            if( control_settings.max_slew_speed > 250 )
                control_settings.max_slew_speed = 250;
            if( control_settings.max_slew_slow > 250 )
                control_settings.max_slew_slow = 250;

            // must have some slew
            if( control_settings.max_slew_speed < 1 )
                control_settings.max_slew_speed = 1;
            if( control_settings.max_slew_slow < 1 )
                control_settings.max_slew_slow = 1;
            break;

        case EEPROM_READ_CODE:
            ESP_LOGI(TAG, "Received EEPROM_READ_CODE, value %d:", packet.p.value);
            break;

        case EEPROM_WRITE_CODE:
            ESP_LOGI(TAG, "Received EEPROM_WRITE_CODE, value %d:", packet.p.value);
            break;

        case CLUTCH_PWM_AND_BRAKE_CODE:
            control_settings.clutch_pwm = packet.byte[1];
            if( control_settings.clutch_pwm < 30 )
                control_settings.clutch_pwm = 30;
            else if( control_settings.clutch_pwm > 250 )
                control_settings.clutch_pwm = 255;
            control_settings.use_brake = packet.byte[2];
            break;
    }
}


void control_loop(void){

    control_settings.low_current = 2000;

    if( pypi_get_tx_packet( &packet ) ){
        process_packet( packet );
    }
    vTaskDelete(NULL);
}
