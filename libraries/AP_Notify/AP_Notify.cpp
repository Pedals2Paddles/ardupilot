/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AP_Notify.h"

#include "AP_BoardLED.h"
#include "PixRacerLED.h"
#include "Buzzer.h"
#include "Display.h"
#include "ExternalLED.h"
#include "NavioLED_I2C.h"
#include "OreoLED_PX4.h"
#include "RCOutputRGBLed.h"
#include "ToneAlarm_Linux.h"
#include "ToneAlarm_PX4.h"
#include "ToshibaLED.h"
#include "ToshibaLED_I2C.h"
#include "VRBoard_LED.h"
#include "DiscreteRGBLed.h"
#include "DiscoLED.h"
#include <stdio.h>

#define CONFIG_NOTIFY_DEVICES_COUNT 5

// table of user settable parameters
const AP_Param::GroupInfo AP_Notify::var_info[] = {

    // @Param: LED_BRIGHT
    // @DisplayName: LED Brightness
    // @Description: Select the RGB LED brightness level. When USB is connected brightness will never be higher than low regardless of the setting.
    // @Values: 0:Off,1:Low,2:Medium,3:High
    // @User: Advanced
    AP_GROUPINFO("LED_BRIGHT", 0, AP_Notify, _rgb_led_brightness, RGB_LED_HIGH),

    // @Param: BUZZ_ENABLE
    // @DisplayName: Buzzer enable
    // @Description: Enable or disable the buzzer. Only for Linux and PX4 based boards.
    // @Values: 0:Disable,1:Enable
    // @User: Advanced
    AP_GROUPINFO("BUZZ_ENABLE", 1, AP_Notify, _buzzer_enable, BUZZER_ON),

    // @Param: LED_OVERRIDE
    // @DisplayName: Setup for MAVLink LED override
    // @Description: This sets up the board RGB LED for override by MAVLink. Normal notify LED control is disabled
    // @Values: 0:Disable,1:Enable
    // @User: Advanced
    AP_GROUPINFO("LED_OVERRIDE", 2, AP_Notify, _rgb_led_override, 0),

    // @Param: DISPLAY_TYPE
    // @DisplayName: Type of on-board I2C display
    // @Description: This sets up the type of on-board I2C display. Disabled by default.
    // @Values: 0:Disable,1:ssd1306,2:sh1106
    // @User: Advanced
    AP_GROUPINFO("DISPLAY_TYPE", 3, AP_Notify, _display_type, 0),

    // @Param: BRD_TYPE
    // @DisplayName: Board type for notify devices
    // @Description: This sets up the notification devices based on board, Defaults to PX4
    // @Values: 0:None,1:PX4,2:PX4_V4,3:Solo,4:VRBrain,5:VRBrain_45,6:Linux_Default,7:Navio,8:Navio3,9:BBBMini,10:Blue,11:RASPilot,12:MinLure,13:ERLEBrain2,14:PXFMini,15:BH,16:Disco
    // @User: Advanced
    AP_GROUPINFO("BRD_TYPE", 4, AP_Notify, _board_type, 1),
    
    // @Param: OLED_THEME
    // @DisplayName: OreoLED Theme
    // @Description: Enable/Disable Oreo LED driver, and set the theme for aircraft or rover. Default is zero for off.
    // @Values: 0:Disabled,1:Aircraft, 2:Rover
    // @User: Advanced
    AP_GROUPINFO("OLED_THEME", 5, AP_Notify, _oled_theme, 0),

    AP_GROUPEND
};

// Default constructor
AP_Notify::AP_Notify()
{
	AP_Param::setup_object_defaults(this, var_info);
}

// static flags, to allow for direct class update from device drivers
struct AP_Notify::notify_flags_and_values_type AP_Notify::flags;
struct AP_Notify::notify_events_type AP_Notify::events;

NotifyDevice *AP_Notify::_devices[] = {nullptr, nullptr, nullptr, nullptr, nullptr};

// initialisation
void AP_Notify::init(bool enable_external_leds)
{
    // Select Board LED Type based on board
    switch (_board_type) {
        case Board_Type_PX4:
        case Board_Type_Solo:
        case Board_Type_VRBrain_45:
        case Board_Type_Blue:
        case Board_Type_ERLEBrain2:
        case Board_Type_PXFMini:
        case Board_Type_BH:
        case Board_Type_BBBMini:
            _devices[0] = new AP_BoardLED();
            break;
            
        
        case Board_Type_RASPilot:
            _devices[0] = nullptr;
            break;

#if CONFIG_HAL_BOARD == HAL_BOARD_VRBRAIN
        case Board_Type_VRBrain:
            _devices[0] = new VRBoard_LED();
            break;
#endif
            
#if CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_PX4_V4
        case Board_Type_PX4_V4:
            _devices[0] = new PixRacerLED();
            break;

#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_NAVIO
        case Board_Type_Navio:
            _devices[0] = new NavioLED_I2C();
            break;

#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_NAVIO2
        case Board_Type_Navio2:
            _devices[0] = new DiscreteRGBLed(4, 27, 6, false);
            break;

#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_DISCO
        case Board_Type_Disco:
            _devices[0] = new DiscoLED();
            break;

#elif CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_MINLURE
        case Board_Type_MinLure:
            _devices[0] = new RCOutputRGBLedOff(15, 13, 14, 255);
            break;
#endif
            
        default:
            _devices[0] = new AP_BoardLED();
            break;
    }
    
    // Select Toshiba Led based on board
    switch (_board_type) {
        case Board_Type_BBBMini:
        case Board_Type_Blue:
        case Board_Type_MinLure:
        case Board_Type_ERLEBrain2:
        case Board_Type_PXFMini:
        case Board_Type_Disco:
            _devices[1] = nullptr;
            
#if CONFIG_HAL_BOARD_SUBTYPE == HAL_BOARD_SUBTYPE_LINUX_BH
        case Board_Type_BH:
            _devices[1] = new RCOutputRGBLed(HAL_RCOUT_RGBLED_RED, HAL_RCOUT_RGBLED_GREEN, HAL_RCOUT_RGBLED_BLUE);
            break;
#endif

        default:
           _devices[1] = new ToshibaLED_I2C();
           break;
    }
    
    // Select Display based on board
    switch (_board_type) {
        case Board_Type_PX4:
        case Board_Type_PX4_V4:
        case Board_Type_Solo:
        case Board_Type_BBBMini:
            _devices[4] = new Display();
            break;
        default:
            _devices[4] = nullptr;
            break;
    }

    // Select Tone Alarm Type based on board
    switch (_board_type) {
#if CONFIG_HAL_BOARD == HAL_BOARD_PX4
        case Board_Type_PX4:
        case Board_Type_PX4_V4:
        case Board_Type_VRBrain:
        case Board_Type_VRBrain_45:
        case Board_Type_Solo:
            _devices[2] = new ToneAlarm_PX4();
            break;
#endif
            
        case Board_Type_BBBMini:
            _devices[2] = new Buzzer();
            break;
 
#if CONFIG_HAL_BOARD == HAL_BOARD_LINUX
        case Board_Type_RASPilot:
        case Board_Type_Disco:
            _devices[2] = new ToneAlarm_Linux();
            break;
#endif
            
        default:
#if CONFIG_HAL_BOARD == HAL_BOARD_LINUX
            _devices[2] = new ToneAlarm_Linux();
#else
            _devices[2] = nullptr;
#endif
            break;
    }

    // Enable/Disable the OreoLED driver based on NTF_OREO_LED not being zero
    // Only allowed to enable on a non-minimized PX4
    // Only the Pixhawk 2 running px4v3 has enough memory
#if CONFIG_HAL_BOARD == HAL_BOARD_PX4
#if !HAL_MINIMIZE_FEATURES
    if (_oled_theme) {
        _devices[3] = new OreoLED_PX4(_oled_theme); // Enabled if non-zero
    } else {
        _devices[3] = nullptr;                      // Disabled if zero
    }
#else
    _devices[3] = nullptr;                          // Disabled if minimized px4
#endif
#else
    _devices[3] = nullptr;                          // Disabled if not px4 at all
#endif

    // clear all flags and events
    memset(&AP_Notify::flags, 0, sizeof(AP_Notify::flags));
    memset(&AP_Notify::events, 0, sizeof(AP_Notify::events));

    // clear flight mode string and text buffer
    memset(_flight_mode_str, 0, sizeof(_flight_mode_str));
    memset(_send_text, 0, sizeof(_send_text));
    _send_text_updated_millis = 0;

    AP_Notify::flags.external_leds = enable_external_leds;

    for (uint8_t i = 0; i < CONFIG_NOTIFY_DEVICES_COUNT; i++) {
        if (_devices[i] != nullptr) {
            _devices[i]->pNotify = this;
            _devices[i]->init();
        }
    }
}

// main update function, called at 50Hz
void AP_Notify::update(void)
{
    for (uint8_t i = 0; i < CONFIG_NOTIFY_DEVICES_COUNT; i++) {
        if (_devices[i] != nullptr) {
            _devices[i]->update();
        }
    }

    //reset the events
    memset(&AP_Notify::events, 0, sizeof(AP_Notify::events));
}

// handle a LED_CONTROL message
void AP_Notify::handle_led_control(mavlink_message_t *msg)
{
    for (uint8_t i = 0; i < CONFIG_NOTIFY_DEVICES_COUNT; i++) {
        if (_devices[i] != nullptr) {
            _devices[i]->handle_led_control(msg);
        }
    }
}

// handle a PLAY_TUNE message
void AP_Notify::handle_play_tune(mavlink_message_t *msg)
{
    for (uint8_t i = 0; i < CONFIG_NOTIFY_DEVICES_COUNT; i++) {
        if (_devices[i] != nullptr) {
            _devices[i]->handle_play_tune(msg);
        }
    }
}

// set flight mode string
void AP_Notify::set_flight_mode_str(const char *str)
{
    strncpy(_flight_mode_str, str, 4);
    _flight_mode_str[sizeof(_flight_mode_str)-1] = 0;
}

void AP_Notify::send_text(const char *str)
{
    strncpy(_send_text, str, sizeof(_send_text));
    _send_text[sizeof(_send_text)-1] = 0;
    _send_text_updated_millis = AP_HAL::millis();
}
