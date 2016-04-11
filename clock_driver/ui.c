/*
 * ui.c
 *
 * Created: 08/04/2016 12:46:40
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>

#include "hw_misc.h"
#include "display.h"
#include "gps.h"
#include "rtc.h"
#include "buttons.h"
#include "eeprom.h"

extern const __flash uint8_t bin_dec_lut[100][2];

struct {
	uint8_t		hour;
	uint8_t		minute;
	uint8_t		enabled;
	uint32_t	crc;
} alarm;

struct {
	uint8_t		high;
	uint8_t		low;
	uint8_t		start_hour;
	uint8_t		start_minute;
	uint8_t		end_hour;
	uint8_t		end_minute;
	uint8_t		threshold;
	uint32_t	crc;
} brightness;


/**************************************************************************************************
* Calculate CRC32
*/
uint32_t ui_crc32(void *buffer, uint16_t bytes)
{
	uint8_t *p = (uint8_t *)buffer;
	CRC.CTRL = CRC_RESET1_bm;
	CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_IO_gc;
	while (bytes--)
		CRC.DATAIN = *p++;
	CRC.STATUS = CRC_BUSY_bm;
	return (uint32_t)CRC.CHECKSUM0 | ((uint32_t)CRC.CHECKSUM1 << 8) | ((uint32_t)CRC.CHECKSUM2 << 16) | ((uint32_t)CRC.CHECKSUM3 << 24);
}

/**************************************************************************************************
* Set up UI
*/
void UI_init(void)
{
	// check alarm settings
	memcpy(&alarm, (void *)EEP_MAPPED_ADDR(0, 0), sizeof(alarm));
	if (alarm.crc != ui_crc32(&alarm, sizeof(alarm) - sizeof(uint32_t)))
	{
		// saved settings lost, restore default
		alarm.hour = 0;
		alarm.minute = 0;
		alarm.enabled = 0;
		alarm.crc = ui_crc32(&alarm, sizeof(alarm) - sizeof(uint32_t));

		EEP_WaitForNVM();
		memcpy((void *)EEP_MAPPED_ADDR(0, 0), &alarm, sizeof(alarm));
		EEP_AtomicWritePage(0);
		EEP_WaitForNVM();
	}

	// check alarm settings
	memcpy(&brightness, (void *)EEP_MAPPED_ADDR(1, 0), sizeof(brightness));
	if (brightness.crc != ui_crc32(&brightness, sizeof(brightness) - sizeof(uint32_t)))
	{
		brightness.high = 100;
		brightness.low = 50;
		brightness.start_hour = 8;
		brightness.start_minute = 0;
		brightness.end_hour = 23;
		brightness.end_minute = 0;
		brightness.threshold = 5;
		brightness.crc = ui_crc32(&brightness, sizeof(brightness) - sizeof(uint32_t));

		EEP_WaitForNVM();
		memcpy((void *)EEP_MAPPED_ADDR(1, 0), &brightness, sizeof(brightness));
		EEP_AtomicWritePage(0);
		EEP_WaitForNVM();
	}
}

/**************************************************************************************************
* Update clock display
*/
inline void ui_update_clock(void)
{
	RTC_time_t time;
	RTC_get_time(&time);
	DIS_set_digit(0, bin_dec_lut[time.hrs][0]);
	DIS_set_digit(1, bin_dec_lut[time.hrs][1]);
	DIS_set_digit(2, bin_dec_lut[time.mins][0]);
	DIS_set_digit(3, bin_dec_lut[time.mins][1]);
	DIS_set_digit(4, bin_dec_lut[time.secs][0]);
	DIS_set_digit(5, bin_dec_lut[time.secs][1]);
}

/**************************************************************************************************
* Set alarm time
*/
void ui_set_alarm(void)
{
	// setup
	DIS_set_digit(0, 'A');
	DIS_set_digit(1, 'L');
	DIS_set_digit(2, 0);
	DIS_set_digit(3, 0);
	DIS_set_digit(4, 0);
	DIS_set_digit(5, 0);
	
	// enable/disable
	do 
	{
		if (alarm.enabled)
		{
			DIS_set_digit(3, 0);
			DIS_set_digit(4, 'O');
			DIS_set_digit(5, 'N');
		} else {
			DIS_set_digit(3, 'O');
			DIS_set_digit(4, 'F');
			DIS_set_digit(5, 'F');
		}
		
		if (BTN_press_SIG[BUTTON_UP] || BTN_press_SIG[BUTTON_DOWN])
		{
			BTN_press_SIG[BUTTON_UP] = 0;
			BTN_press_SIG[BUTTON_DOWN] = 0;
			alarm.enabled = !alarm.enabled;
		}
	} while (!BTN_press_SIG[BUTTON_SET] ||		// button pressed
			BTN_hold_SIG[BUTTON_SET]);			// button not held from time of entry
	BTN_press_SIG[BUTTON_SET] = 0;
	
	// display set time
	DIS_set_digit(2, bin_dec_lut[alarm.hour][0]);
	DIS_set_digit(3, bin_dec_lut[alarm.hour][1]);
	DIS_set_digit(4, bin_dec_lut[alarm.minute][0]);
	DIS_set_digit(5, bin_dec_lut[alarm.minute][1]);
	
	// set hours
	do 
	{
		uint8_t flash = 0;
		
		if (flash < 0x7F)
		{
			DIS_set_digit(2, bin_dec_lut[alarm.hour][0]);
			DIS_set_digit(3, bin_dec_lut[alarm.hour][1]);
		} else {
			DIS_set_digit(2, 0);
			DIS_set_digit(3, 0);
		}
		flash++;
		
		if (BTN_press_SIG[BUTTON_UP])
		{
			alarm.hour++;
			if (alarm.hour > 23)
				alarm.hour = 0;
		}
		
		if (BTN_press_SIG[BUTTON_DOWN])
		{
			if (alarm.hour > 1)
				alarm.hour--;
			else
				alarm.hour = 23;
		}
	} while (!BTN_press_SIG[BUTTON_SET]);
	BTN_press_SIG[BUTTON_SET] = 0;
	DIS_set_digit(2, bin_dec_lut[alarm.hour][0]);
	DIS_set_digit(3, bin_dec_lut[alarm.hour][1]);

	// set minutes
	do 
	{
		uint8_t flash = 0;
		
		if (flash < 0x7F)
		{
			DIS_set_digit(4, bin_dec_lut[alarm.minute][0]);
			DIS_set_digit(5, bin_dec_lut[alarm.minute][1]);
		} else {
			DIS_set_digit(4, 0);
			DIS_set_digit(5, 0);
		}
		flash++;
		
		if (BTN_press_SIG[BUTTON_UP])
		{
			alarm.minute++;
			if (alarm.minute > 59)
				alarm.minute = 0;
		}
		
		if (BTN_press_SIG[BUTTON_DOWN])
		{
			if (alarm.minute > 1)
				alarm.minute--;
			else
				alarm.minute = 23;
		}
	} while (!BTN_press_SIG[BUTTON_SET]);
	BTN_press_SIG[BUTTON_SET] = 0;
}

/**************************************************************************************************
* Main loop
*/
void UI_run(void)
{
	for(;;)
	{
		if (RTC_second_tick_SIG)
		{
			RTC_second_tick_SIG = 0;
			ui_update_clock();
		}
		
		if (RTC_minute_tick_SIG)
		{
			RTC_minute_tick_SIG = 0;
			RTC_time_t time;
			RTC_get_time(&time);
			if ((time.hrs == alarm.hour) && (time.mins == alarm.minute))
				NOP();	// sound alarm
		}
		
		if (BTN_hold_SIG[BUTTON_SET])
			ui_set_alarm();
	}
}
