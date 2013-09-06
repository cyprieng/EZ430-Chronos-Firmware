// system
#include "project.h"

// driver
#include "counter.h"
#include "ports.h"
#include "display.h"
#include "as.h"
#include "adc12.h"
#include "timer.h"

// logic
#include "user.h"
#include "acceleration.h"
#include "bmp_as.h"
#include "cma_as.h"

// Global Variable section
struct counter sCounter;

void reset_counter(void)
{
	sCounter.state = MENU_ITEM_NOT_VISIBLE;
	sCounter.count = 0;
	sCounter.style = 0;
}

void mx_counter(u8 line){
	// reset counter
	sCounter.count = 0;
	display.flag.update_counter = 1;
}

void sx_counter(u8 line){
	sCounter.style++;
	if(sCounter.style > 2) sCounter.style = 0;
	display_counter(NULL, DISPLAY_LINE_UPDATE_PARTIAL);
}

void display_counter(u8 line, u8 update)
{
	u8 * str;
	// Redraw line
	switch( update ) {
		case DISPLAY_LINE_UPDATE_FULL:
			sCounter.data[0] = sCounter.data[1] = sCounter.data[2] = 0;
		sCounter.sum = 0;
		sCounter.rise_state = 0;

		// Start sensor
		if (bmp_used)
		{
			bmp_as_start();
		}
		else
		{
			cma_as_start();
		}

		// Menu item is visible
		sCounter.state = MENU_ITEM_VISIBLE;

		case DISPLAY_LINE_UPDATE_PARTIAL:
			if(sCounter.style == 0){
				// Display result in xxxxx format
				str = int_to_array(sCounter.count, 5, 0);
				display_chars(LCD_SEG_L2_4_0, str, SEG_ON);
			}
			else if(sCounter.style == 1){
				// Display result in percent
				s8 percent = sCounter.count / 150;
				str = int_to_array(percent, 3, 0);
				display_chars(LCD_SEG_L2_2_0, str, SEG_ON);
			}
			else{
				// Display result in progress bar
				s8 squareNum = sCounter.count / 3000;
				if(squareNum > 5) squareNum = 5;

				switch (squareNum)
				{
					case 5:
						display_char(LCD_SEG_L2_4, '-', SEG_ON);
					case 4:
						display_char(LCD_SEG_L2_3, '-', SEG_ON);
					case 3:
						display_char(LCD_SEG_L2_2, '-', SEG_ON);
					case 2:
						display_char(LCD_SEG_L2_1, '-', SEG_ON);
					case 1:
						display_char(LCD_SEG_L2_0, '-', SEG_ON);
				}
			}
			display.flag.update_counter = 0;
		break;
		case DISPLAY_LINE_CLEAR:
			// Stop acceleration sensor
			if (bmp_used)
			{
				bmp_as_stop();
			}
			else
			{
				cma_as_stop();
			}

			// Menu item is not visible
			sCounter.state = MENU_ITEM_NOT_VISIBLE;
		break;
	}
}

u8 is_counter_measurement(void)
{
	return sCounter.state == MENU_ITEM_VISIBLE;
}

#define WALK_COUNT_THRESHOLD  200
#define RUN_COUNT_THRESHOLD  150
void do_count(void)
{
	u16 threshold = (sCounter.style ? RUN_COUNT_THRESHOLD: WALK_COUNT_THRESHOLD );
	if (( sCounter.high - sCounter.low) > threshold ) {
		sCounter.count ++;
		//display.flag.update_counter = 1;
		display_counter(NULL, DISPLAY_LINE_UPDATE_PARTIAL);
	}
}

void do_counter_measurement(void)
{
	extern u16 convert_acceleration_value_to_mgrav(u8 value);
	u8 i;
	u16 accel_data, sum1;

	// Get data from sensor
	if (bmp_used)
	{
		bmp_as_get_data(sCounter.xyz);
	}
	else
	{
		cma_as_get_data(sCounter.xyz);
	}

	for ( i = 0, sum1 = 0; i < 3; i ++ ) {
		accel_data = convert_acceleration_value_to_mgrav(sCounter.xyz[i]);

		// Filter acceleration
		#if 0
			accel_data = (u16)((accel_data * 0.2) + (sCounter.data[i] * 0.8));
		#endif
			accel_data = (u16)((accel_data * 0.1) + (sCounter.data[i] * 0.9));

		sum1 += accel_data;

		// Store average acceleration
		sCounter.data[i] = accel_data;
	}
	if ( sCounter.sum == 0 ) {
		sCounter.low = sum1;
		sCounter.high = sum1;
	} else {
		switch ( sCounter.rise_state) { // init state
			case 0:
				if ( sum1 > sCounter.sum ) {
					sCounter.rise_state = 1;
					sCounter.low = sCounter.sum;
				} else {
					sCounter.rise_state = 2;
					sCounter.high = sCounter.sum;
				}
			break;
			case 1:
				if ( sum1 < sCounter.sum ) { // change direction
					sCounter.high = sCounter.sum;
					sCounter.rise_state = 2;
				}
			break;
			case 2:
				if ( sum1 > sCounter.sum ) {
					sCounter.low = sCounter.sum;
					sCounter.rise_state = 1;
					do_count();
				}
			break;
		}
	}
	sCounter.sum = sum1;
}
