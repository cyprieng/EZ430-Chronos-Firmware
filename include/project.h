// *************************************************************************************************
//
//      Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
//
//
//        Redistribution and use in source and binary forms, with or without
//        modification, are permitted provided that the following conditions
//        are met:
//
//          Redistributions of source code must retain the above copyright
//          notice, this list of conditions and the following disclaimer.
//
//          Redistributions in binary form must reproduce the above copyright
//          notice, this list of conditions and the following disclaimer in the
//          documentation and/or other materials provided with the
//          distribution.
//
//          Neither the name of Texas Instruments Incorporated nor the names of
//          its contributors may be used to endorse or promote products derived
//          from this software without specific prior written permission.
//
//        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//        "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//        LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//        A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//        OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//        SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//        LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//        DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//        THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//        (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//        OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// *************************************************************************************************

#ifndef PROJECT_H_
#define PROJECT_H_

// *************************************************************************************************
// Include section
#include <cc430x613x.h>
#include <bm.h>

// *************************************************************************************************
// Defines section

// Comment this to not use the LCD charge pump
//#define USE_LCD_CHARGE_PUMP

// Comment this define to build the application without watchdog support
//#define USE_WATCHDOG

// Use/not use filter when measuring physical values
#define FILTER_OFF                                              (0u)
#define FILTER_ON                                               (1u)

// *************************************************************************************************
// Macro section

// Conversion from usec to ACLK timer ticks
#define CONV_US_TO_TICKS(usec)                          (((usec) * 32768) / 1000000)

// Conversion from msec to ACLK timer ticks
#define CONV_MS_TO_TICKS(msec)                          (((msec) * 32768) / 1000)

// *************************************************************************************************
// Typedef section

typedef enum
{
    MENU_ITEM_NOT_VISIBLE = 0,            // Menu item is not visible
    MENU_ITEM_VISIBLE                     // Menu item is visible
} menu_t;

// Set of system flags
typedef union
{
    struct
    {
        u16 idle_timeout : 1;             // Timeout after inactivity
        u16 idle_timeout_enabled : 1;     // When in set mode, timeout after a given period
        u16 lock_buttons : 1;             // Lock buttons
        u16 mask_buzzer : 1;              // Do not output buzz for next button event
        u16 up_down_repeat_enabled : 1;   // While in set_value(), create virtual UP/DOWN button
                                          // events
        u16 low_battery : 1;              // 1 = Battery is low
        u16 use_metric_units : 1;         // 1 = Use metric units, 0 = use English units
        u16 delay_over : 1;               // 1 = Timer delay over
    } flag;
    u16 all_flags;                        // Shortcut to all system flags (for reset)
} s_system_flags;
extern volatile s_system_flags sys;

// Set of request flags
typedef union
{
    struct
    {
        u16 temperature_measurement : 1;  // 1 = Measure temperature
        u16 voltage_measurement : 1;      // 1 = Measure voltage
        u16 altitude_measurement : 1;     // 1 = Measure air pressure
        u16 acceleration_measurement : 1; // 1 = Measure acceleration
        u16 buzzer : 1;                   // 1 = Output buzzer
        u16 counter_measurement : 1;      // 1 = measure counter from acceleration
    } flag;
    u16 all_flags;                        // Shortcut to all request flags (for reset)
} s_request_flags;
extern volatile s_request_flags request;

// Set of message flags
typedef union
{
    struct
    {
        u16 prepare : 1;                  // 1 = Wait for clock tick, then set
                                          // display.flag.show_message flag
        u16 show : 1;                     // 1 = Display message now
        u16 erase : 1;                    // 1 = Erase message
        u16 type_locked : 1;              // 1 = Show "buttons are locked" in Line2
        u16 type_unlocked : 1;            // 1 = Show "buttons are unlocked" in Line2
        u16 type_lobatt : 1;              // 1 = Show "lobatt" text in Line2
        u16 type_alarm_on : 1;            // 1 = Show "  on" text in Line1
        u16 type_alarm_off : 1;           // 1 = Show " off" text in Line1
    } flag;
    u16 all_flags;                        // Shortcut to all message flags (for reset)
} s_message_flags;
extern volatile s_message_flags message;

// *************************************************************************************************
// Global Variable section

// Global flag set if Bosch sensors are used
extern u8 bmp_used;

#endif                                    /*PROJECT_H_ */
