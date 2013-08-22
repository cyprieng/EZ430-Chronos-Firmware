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
//******************************************************************************
//   eZ430-Chronos
//
//   Description: This is the standard software for the eZ430-Chronos
//
//   Version    1.7
//   Texas Instruments, Inc
//   March 2013
//   Known working builds:
//     IAR Embedded Workbench (Version: 5.51)
//     Code Composer Studio (Version  5.2.0.00069)
//******************************************************************************
//Change Log (More detailed information can be found in change_record.txt):
//******************************************************************************
//Version:  1.7
//Comments: Support for new hardware version with Bosch Sensortec sensors added.
//
//Version:  1.6
//Comments: Several bugs were fixed.
//          LCD shows "done" after successfully received data
//          rfBSL requires two button presses in order to update watch
//          New method detects a long button press
//          Removed file display1.c. The content is now in display.c
//          Backlight of Chronos stays on for 3 sec after backlight button was pushed.
//          After timeout the accelerometer menu shows "----"
//
//Version:  1.5
//Comments: Changed XT1 drive level to highest
//          Modified key lock procedure.
//          Negative �C are now converted correctly to Kelvin
//          Enabled fast mode when changing altitude offset
//          Disabled stopwatch stop when watch buttons are locked
//          Added SimpliciTI sources to project. Upgraded to Version 1.1.1.
//          Changed button names from M1/M2/S1/S2/BL to STAR/NUM/UP/DOWN/BACKLIGHT
//
//Version:  1.4
//Comments: Initial Release Version
//**********************************************************************************

// *************************************************************************************************
// Initialization and control of application.
// *************************************************************************************************

// *************************************************************************************************
// Include section

// system
#include "project.h"
#include <string.h>

// driver
#include "clock.h"
#include "display.h"
#include "cma_as.h"
#include "bmp_as.h"
#include "as.h"
#include "cma_ps.h"
#include "bmp_ps.h"
#include "ps.h"
#include "radio.h"
#include "buzzer.h"
#include "ports.h"
#include "timer.h"
#include "pmm.h"
#include "rf1a.h"

// logic
#include "menu.h"
#include "date.h"
#include "alarm.h"
#include "stopwatch.h"
#include "battery.h"
#include "temperature.h"
#include "altitude.h"
#include "battery.h"
#include "acceleration.h"
#include "rfsimpliciti.h"
#include "simpliciti.h"
#include "rfbsl.h"
#include "test.h"
#include "totp.h"
#include "counter.h"

// *************************************************************************************************
// Prototypes section
void init_application(void);
void init_global_variables(void);
void wakeup_event(void);
void process_requests(void);
void display_update(void);
void idle_loop(void);
void configure_ports(void);
void read_calibration_values(void);

// *************************************************************************************************
// Defines section

// Number of calibration data bytes in INFOA memory
#define CALIBRATION_DATA_LENGTH         (13u)

// *************************************************************************************************
// Global Variable section

// Variable holding system internal flags
volatile s_system_flags sys;

// Variable holding flags set by logic modules
volatile s_request_flags request;

// Variable holding message flags
volatile s_message_flags message;

// Global flag set if Bosch sensors are used
u8 bmp_used;

// Global radio frequency offset taken from calibration memory
// Compensates crystal deviation from 26MHz nominal value
u8 rf_frequoffset;

// Function pointers for LINE1 and LINE2 display function
void (*fptr_lcd_function_line1)(u8 line, u8 update);
void (*fptr_lcd_function_line2)(u8 line, u8 update);

// *************************************************************************************************
// Extern section

extern void start_simpliciti_sync(void);

// *************************************************************************************************
// @fn          main
// @brief       Main routine
// @param       none
// @return      none
// *************************************************************************************************
int main(void)
{
    // Init MCU
    init_application();

    // Assign initial value to global variables
    init_global_variables();

    // Branch to welcome screen
    test_mode();

    // Main control loop: wait in low power mode until some event needs to be processed
    while (1)
    {
        // When idle go to LPM3
        idle_loop();

        // Process wake-up events
        if (button.all_flags || sys.all_flags)
            wakeup_event();

        // Process actions requested by logic modules
        if (request.all_flags)
            process_requests();

        // Before going to LPM3, update display
        if (display.all_flags)
            display_update();
    }
}

// *************************************************************************************************
// @fn          init_application
// @brief       Initialize the microcontroller.
// @param       none
// @return      none
// *************************************************************************************************
void init_application(void)
{
    volatile unsigned char *ptr;

    // ---------------------------------------------------------------------
    // Enable watchdog

    // Watchdog triggers after 16 seconds when not cleared
#ifdef USE_WATCHDOG
    WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK;
#else
    WDTCTL = WDTPW + WDTHOLD;
#endif

    // ---------------------------------------------------------------------
    // Configure PMM
    SetVCore(3);

    // Set global high power request enable
    PMMCTL0_H = 0xA5;
    PMMCTL0_L |= PMMHPMRE;
    PMMCTL0_H = 0x00;

    // ---------------------------------------------------------------------
    // Enable 32kHz ACLK
    P5SEL |= 0x03;              // Select XIN, XOUT on P5.0 and P5.1
    UCSCTL6 &= ~XT1OFF;         // XT1 On, Highest drive strength
    UCSCTL6 |= XCAP_3;          // Internal load cap

    UCSCTL3 = SELA__XT1CLK;     // Select XT1 as FLL reference
    UCSCTL4 = SELA__XT1CLK | SELS__DCOCLKDIV | SELM__DCOCLKDIV;

    // ---------------------------------------------------------------------
    // Configure CPU clock for 12MHz
    _BIS_SR(SCG0);              // Disable the FLL control loop
    UCSCTL0 = 0x0000;           // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_5;        // Select suitable range
    UCSCTL2 = FLLD_1 + 0x16E;   // Set DCO Multiplier
    _BIC_SR(SCG0);              // Enable the FLL control loop

    // Worst-case settling time for the DCO when the DCO range bits have been
    // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
    // UG for optimization.
    // 32 x 32 x 12 MHz / 32,768 Hz = 375000 = MCLK cycles for DCO to settle
    __delay_cycles(375000);

    // Loop until XT1 & DCO stabilizes, use do-while to insure that
    // body is executed at least once
    do
    {
        UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
        SFRIFG1 &= ~OFIFG;      // Clear fault flags
    }
    while ((SFRIFG1 & OFIFG));

    // ---------------------------------------------------------------------
    // Configure port mapping

    // Disable all interrupts
    __disable_interrupt();
    // Get write-access to port mapping registers:
    PMAPPWD = 0x02D52;
    // Allow reconfiguration during runtime:
    PMAPCTL = PMAPRECFG;

    // P2.7 = TA0CCR1A or TA1CCR0A output (buzzer output)
    ptr = &P2MAP0;
    *(ptr + 7) = PM_TA1CCR0A;
    P2OUT &= ~BIT7;
    P2DIR |= BIT7;

    // P1.5 = SPI MISO input
    ptr = &P1MAP0;
    *(ptr + 5) = PM_UCA0SOMI;
    // P1.6 = SPI MOSI output
    *(ptr + 6) = PM_UCA0SIMO;
    // P1.7 = SPI CLK output
    *(ptr + 7) = PM_UCA0CLK;

    // Disable write-access to port mapping registers:
    PMAPPWD = 0;
    // Re-enable all interrupts
    __enable_interrupt();

    // ---------------------------------------------------------------------
    // Configure ports

    // ---------------------------------------------------------------------
    // Reset radio core
    radio_reset();
    radio_powerdown();

    // ---------------------------------------------------------------------
    // Init acceleration sensor
    as_init();

    // ---------------------------------------------------------------------
    // Init LCD
    lcd_init();

    // ---------------------------------------------------------------------
    // Init buttons
    init_buttons();

    // ---------------------------------------------------------------------
    // Configure Timer0 for use by the clock and delay functions
    Timer0_Init();
    // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------
    // Init pressure sensor
    bmp_ps_init();
    // Bosch sensor not found?
    if (!ps_ok)
    {
        bmp_used = 0;
        cma_ps_init();
    }
    else
    {
    	bmp_used = 1;
    }
}

// *************************************************************************************************
// @fn          init_global_variables
// @brief       Initialize global variables.
// @param       none
// @return      none
// *************************************************************************************************
void init_global_variables(void)
{
    // --------------------------------------------
    // Apply default settings

    // set menu pointers to default menu items
    ptrMenu_L1 = &menu_L1_Time;
    //      ptrMenu_L1 = &menu_L1_Alarm;
    //      ptrMenu_L1 = &menu_L1_Temperature;
    //      ptrMenu_L1 = &menu_L1_Altitude;
    //      ptrMenu_L1 = &menu_L1_Acceleration;
    ptrMenu_L2 = &menu_L2_Date;
    //      ptrMenu_L2 = &menu_L2_Stopwatch;
    //      ptrMenu_L2 = &menu_L2_Counter;
    //      ptrMenu_L2 = &menu_L2_Totp;
    //      ptrMenu_L2 = &menu_L2_Rf;
    //      ptrMenu_L2 = &menu_L2_Ppt;
    //      ptrMenu_L2 = &menu_L2_Sync;
    //      ptrMenu_L2 = &menu_L2_Battery;
    //      ptrMenu_L2 = &menu_L2_RFBSL;

    // Assign LINE1 and LINE2 display functions
    fptr_lcd_function_line1 = ptrMenu_L1->display_function;
    fptr_lcd_function_line2 = ptrMenu_L2->display_function;

    // Init system flags
    button.all_flags = 0;
    sys.all_flags = 0;
    request.all_flags = 0;
    display.all_flags = 0;
    message.all_flags = 0;

    // Force full display update when starting up
    display.flag.full_update = 1;

#ifndef ISM_US
    // Use metric units when displaying values
    sys.flag.use_metric_units = 1;
#endif

    // Read calibration values from info memory
    read_calibration_values();

    // reset counter
     reset_counter();

    // Set system time to default value
    reset_clock();

    // Set date to default value
    reset_date();

    // Set alarm time to default value
    reset_alarm();

    // Set buzzer to default value
    reset_buzzer();

    // Reset stopwatch
    reset_stopwatch();

    // Reset altitude measurement
    reset_altitude_measurement();

    // Reset acceleration measurement
    reset_acceleration();

    // Reset SimpliciTI stack
    reset_rf();

    // Reset temperature measurement
    reset_temp_measurement();

    // Reset battery measurement
    reset_batt_measurement();
    battery_measurement();
}

// *************************************************************************************************
// @fn          wakeup_event
// @brief       Process external / internal wakeup events.
// @param       none
// @return      none
// *************************************************************************************************
void wakeup_event(void)
{
    // Enable idle timeout
    sys.flag.idle_timeout_enabled = 1;

    // If buttons are locked, only display "buttons are locked" message
    if (button.all_flags && sys.flag.lock_buttons)
    {
        // Show "buttons are locked" message synchronously with next second tick
        if (!(BUTTON_NUM_IS_PRESSED && BUTTON_DOWN_IS_PRESSED))
        {
            message.flag.prepare = 1;
            message.flag.type_locked = 1;
        }
        // Clear buttons
        button.all_flags = 0;
    }
    // Process long button press event (while button is held)
    else if (button.flag.star_long)
    {
        // Clear button event
        button.flag.star_long = 0;

        // Call sub menu function
        ptrMenu_L1->mx_function(LINE1);

        // Set display update flag
        display.flag.full_update = 1;
    }
    else if (button.flag.num_long)
    {
        // Clear button event
        button.flag.num_long = 0;

        // Call sub menu function
        ptrMenu_L2->mx_function(LINE2);

        // Set display update flag
        display.flag.full_update = 1;
    }
    // Process single button press event (after button was released)
    else if (button.all_flags)
    {
        // M1 button event ---------------------------------------------------------------------
        // (Short) Advance to next menu item
        if (button.flag.star)
        {
            // Clean up display before activating next menu item
            fptr_lcd_function_line1(LINE1, DISPLAY_LINE_CLEAR);

            // Go to next menu entry
            ptrMenu_L1 = ptrMenu_L1->next;

            // Assign new display function
            fptr_lcd_function_line1 = ptrMenu_L1->display_function;

            // Set Line1 display update flag
            display.flag.line1_full_update = 1;

            // Clear button flag
            button.flag.star = 0;
        }
        // NUM button event ---------------------------------------------------------------------
        // (Short) Advance to next menu item
        else if (button.flag.num)
        {
            // Clear rfBSL confirmation flag
            rfBSL_button_confirmation = 0;

            // Clean up display before activating next menu item
            fptr_lcd_function_line2(LINE2, DISPLAY_LINE_CLEAR);

            // Go to next menu entry
            ptrMenu_L2 = ptrMenu_L2->next;

            // Assign new display function
            fptr_lcd_function_line2 = ptrMenu_L2->display_function;

            // Set Line2 display update flag
            display.flag.line2_full_update = 1;

            // Clear button flag
            button.flag.num = 0;
        }
        // UP button event ---------------------------------------------------------------------
        // Activate user function for Line1 menu item
        else if (button.flag.up)
        {
            // Call direct function
            ptrMenu_L1->sx_function(LINE1);

            // Set Line1 display update flag
            display.flag.line1_full_update = 1;

            // Clear button flag
            button.flag.up = 0;
        }
        // DOWN button event ---------------------------------------------------------------------
        // Activate user function for Line2 menu item
        else if (button.flag.down)
        {
            if (ptrMenu_L2 == &menu_L2_RFBSL)
            {

            }

            // Call direct function
            ptrMenu_L2->sx_function(LINE2);

            // Set Line1 display update flag
            display.flag.line2_full_update = 1;

            // Clear button flag
            button.flag.down = 0;
        }
    }
    // Process internal events
    if (sys.all_flags)
    {
        // Idle timeout ---------------------------------------------------------------------
        if (sys.flag.idle_timeout)
        {
            // Clear timeout flag
            sys.flag.idle_timeout = 0;

            // Clear display
            clear_display();

            // Set display update flags
            display.flag.full_update = 1;
        }
    }
    // Disable idle timeout
    sys.flag.idle_timeout_enabled = 0;
}

// *************************************************************************************************
// @fn          process_requests
// @brief       Process requested actions outside ISR context.
// @param       none
// @return      none
// *************************************************************************************************
void process_requests(void)
{
    // Do temperature measurement
    if (request.flag.temperature_measurement)
        temperature_measurement(FILTER_ON);

    // Do pressure measurement
    if (request.flag.altitude_measurement)
        do_altitude_measurement(FILTER_ON);

    // Do acceleration measurement
    if (request.flag.acceleration_measurement)
        do_acceleration_measurement();

    // Do voltage measurement
    if (request.flag.voltage_measurement)
        battery_measurement();

    // Generate alarm (two signals every second)
    if (request.flag.buzzer)
        start_buzzer(2, BUZZER_ON_TICKS, BUZZER_OFF_TICKS);

    if (request.flag.counter_measurement)
    	do_counter_measurement();

    // Reset request flag
    request.all_flags = 0;
}

// *************************************************************************************************
// @fn          display_update
// @brief       Process display flags and call LCD update routines.
// @param       none
// @return      none
// *************************************************************************************************
void display_update(void)
{
    u8 line;
    u8 string[8];

    // ---------------------------------------------------------------------
    // Call Line1 display function
    if (display.flag.full_update || display.flag.line1_full_update)
    {
        clear_line(LINE1);
        fptr_lcd_function_line1(LINE1, DISPLAY_LINE_UPDATE_FULL);
    } else if (ptrMenu_L1->display_update())
    {
        // Update line1 only when new data is available
        fptr_lcd_function_line1(LINE1, DISPLAY_LINE_UPDATE_PARTIAL);
    }

    // ---------------------------------------------------------------------
    // If message text should be displayed on Line2, skip normal update
    if (message.flag.show)
    {
        line = LINE2;

        // Select message to display
        if (message.flag.type_locked)
            memcpy(string, "  LO?T", 6);
        else if (message.flag.type_unlocked)
            memcpy(string, "  OPEN", 6);
        else if (message.flag.type_lobatt)
            memcpy(string, "LOBATT", 6);
        else if (message.flag.type_alarm_on)
        {
            memcpy(string, "  ON", 4);
            line = LINE1;
        } else if (message.flag.type_alarm_off)
        {
            memcpy(string, " OFF", 4);
            line = LINE1;
        }
        // Clear previous content
        clear_line(line);
        fptr_lcd_function_line2(line, DISPLAY_LINE_CLEAR);

        if (line == LINE2)
            display_chars(LCD_SEG_L2_5_0, string, SEG_ON);
        else
            display_chars(LCD_SEG_L1_3_0, string, SEG_ON);

        // Next second tick erases message and repaints original screen content
        message.all_flags = 0;
        message.flag.erase = 1;
    }
    // ---------------------------------------------------------------------
    // Call Line2 display function
    else if (display.flag.full_update || display.flag.line2_full_update)
    {
        clear_line(LINE2);
        fptr_lcd_function_line2(LINE2, DISPLAY_LINE_UPDATE_FULL);
    } else if (ptrMenu_L2->display_update() && !message.all_flags)
    {
        // Update line2 only when new data is available
        fptr_lcd_function_line2(LINE2, DISPLAY_LINE_UPDATE_PARTIAL);
    }

    // Clear display flag
    display.all_flags = 0;
}

// *************************************************************************************************
// @fn          to_lpm
// @brief       Go to LPM0/3.
// @param       none
// @return      none
// *************************************************************************************************
void to_lpm(void)
{
    // Go to LPM3
    _BIS_SR(LPM3_bits + GIE);
    __no_operation();
}

// *************************************************************************************************
// @fn          idle_loop
// @brief       Go to LPM. Service watchdog timer when waking up.
// @param       none
// @return      none
// *************************************************************************************************
void idle_loop(void)
{
    // To low power mode
    to_lpm();

#ifdef USE_WATCHDOG
    // Service watchdog
    WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK + WDTCNTCL;
#endif
}

// *************************************************************************************************
// @fn          read_calibration_values
// @brief       Read calibration values for temperature measurement, voltage measurement
//                              and radio from INFO memory.
// @param       none
// @return      none
// *************************************************************************************************
void read_calibration_values(void)
{
    u8 cal_data[CALIBRATION_DATA_LENGTH]; // Temporary storage for constants
    u8 i;
    u8 *flash_mem;                        // Memory pointer

    // Read calibration data from Info D memory
    flash_mem = (u8 *) 0x1800;
    for (i = 0; i < CALIBRATION_DATA_LENGTH; i++)
    {
        cal_data[i] = *flash_mem++;
    }

    if (cal_data[0] == 0xFF)
    {
        // If no values are available (i.e. INFO D memory has been erased by user), assign
        // experimentally derived values
        rf_frequoffset = 4;
        sTemp.offset = -250;
        sBatt.offset = -10;
        simpliciti_ed_address[0] = 0x79;
        simpliciti_ed_address[1] = 0x56;
        simpliciti_ed_address[2] = 0x34;
        simpliciti_ed_address[3] = 0x12;
        sAlt.altitude_offset = 0;
    } else
    {
        // Assign calibration data to global variables
        rf_frequoffset = cal_data[1];
        // Range check for calibrated FREQEST value (-20 .. + 20 is ok, else use default value)
        if ((rf_frequoffset > 20) && (rf_frequoffset < (256 - 20)))
        {
            rf_frequoffset = 0;
        }
        sTemp.offset = (s16) ((cal_data[2] << 8) + cal_data[3]);
        sBatt.offset = (s16) ((cal_data[4] << 8) + cal_data[5]);
        simpliciti_ed_address[0] = cal_data[6];
        simpliciti_ed_address[1] = cal_data[7];
        simpliciti_ed_address[2] = cal_data[8];
        simpliciti_ed_address[3] = cal_data[9];
        // S/W version byte set during calibration?
        if (cal_data[12] != 0xFF)
        {
            sAlt.altitude_offset = (s16) ((cal_data[10] << 8) + cal_data[11]);
        } else
        {
            sAlt.altitude_offset = 0;
        }
    }
}

