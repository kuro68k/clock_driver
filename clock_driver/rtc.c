/*
 * rtc.c
 *
 * Created: 15/06/2015 14:52:59
 *  Author: paul.qureshi
 *
 * RTC with calendar function
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "hw_misc.h"
#include "gps.h"
#include "rtc.h"


volatile RTC_time_t	time_AT;
volatile uint8_t	RTC_second_tick_SIG = 0;		// !! SIGNAL !! set to 0xFF when second ticks over
volatile uint8_t	RTC_minute_tick_SIG = 0;		// !! SIGNAL !! set to 0xFF when second ticks over

const __flash uint8_t days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


/**************************************************************************************************
** Set up RTC
*/
void RTC_init(void)
{
	memset((void *)&time_AT, 0, sizeof(time_AT));
	time_AT.d = 1;
	time_AT.m = 1;
	time_AT.y = 16;

	CLK.RTCCTRL = CLK_RTCSRC_EXTCLK_gc | CLK_RTCEN_bm;		// 32768Hz from TCXO
	
	while(RTC.STATUS & RTC_SYNCBUSY_bm);
	RTC.CNT = 0;
	RTC.PER = 32768;
	RTC.CTRL = RTC_PRESCALER_DIV1_gc;
	RTC.INTCTRL = RTC_OVFINTLVL_LO_gc;
}

/**************************************************************************************************
** Wait for start of second
*/
void RTC_wait_for_second_tick(void)
{
	while (RTC_second_tick_SIG == 0)
	{
		sleep_cpu();
		WDR();
		GPS_task();
	}
}

/**************************************************************************************************
** Get current time
*/
void RTC_get_time(RTC_time_t *time)
{
	cli();
	memcpy(time, (void*)&time_AT, sizeof(RTC_time_t));
	sei();
}

/**************************************************************************************************
** Check if a year is a leap year
*/
bool rtc_is_leap_year(uint8_t year)
{
	if (year % 100 == 0)
	{
		//if (year % 400 == 0)
		if (year == 0)		// uint8, 2000 was not a leap year
			return true;
		else
			return false;
	}

	if (year % 4 == 0)
		return true;

	return false;
}

/**************************************************************************************************
** Return the number of days in a month
*/
uint8_t rtc_days_in_month(uint8_t month, uint8_t year)
{
	uint8_t d = days_in_month[month];
	if ((month == 2) && rtc_is_leap_year(year))	// February in a leap year
		d++;
	return d;
}

/**************************************************************************************************
** Validate time
*/
bool RTC_validate(RTC_time_t *time)
{
	if ((time->hrs > 23) ||
		(time->mins > 59) ||
		(time->secs > 59) ||	// can't handle leap seconds
		(time->m == 0) ||
		(time->m > 12) ||
		(time->d == 0) ||
		(time->d > rtc_days_in_month(time->m, time->y)))
		return false;
	return true;
}

/**************************************************************************************************
** Increment day
*/
void rtc_increment_day(RTC_time_t *time)
{
	time->d++;
				
	uint8_t max_d = days_in_month[time->m];
	if ((time->m == 2) && rtc_is_leap_year(time->y))	// February in a leap year
		max_d++;
				
	if (time->d > max_d)
	{
		time->d = 1;
		time->m++;
		if (time->m > 12)
		{
			time->m = 1;
			time->y++;
			if (time->y > 99)
				time->y = 0;
		}
	}
}

/**************************************************************************************************
** Decrement day
*/
void rtc_decrement_day(RTC_time_t *time)
{
	if (time->d > 1)
	{
		time->d--;
		return;
	}
				
	if ((time->y == 0) && (time->m == 1))	// can't go back any further
		return;
	
	time->m--;
	if (time->m == 0)
	{
		time->m = 12;
		time->y--;
	}

	time->d = days_in_month[time->m];
	if ((time->m == 2) && rtc_is_leap_year(time->y))	// February in a leap year
		time->d++;
}

/**************************************************************************************************
** Adjust time for timezone/DST. hours_offset in the range -23 to +23.
*/
void RTC_adjust_for_timezone(RTC_time_t *time, int8_t hours_offset)
{
	if (hours_offset > 0)		// wind clock forwards
	{
		time->hrs += hours_offset;
		if (time->hrs > 23)
		{
			time->hrs -= 24;
			rtc_increment_day(time);
		}
	}
	
	if (hours_offset < 0)		// wind clock backwards
	{
		if (time->hrs < abs(hours_offset))
		{
			time->hrs += 24;
			rtc_decrement_day(time);
		}
		time->hrs += hours_offset;
	}
}

/**************************************************************************************************
** Second tick interrupt
*/
ISR(RTC_OVF_vect)
{
	RTC_second_tick_SIG = 0xFF;
	
	time_AT.secs++;
	if (time_AT.secs > 59)
	{
		RTC_minute_tick_SIG = 0xFF;
		time_AT.secs = 0;
		time_AT.mins++;
		if (time_AT.mins > 59)
		{
			time_AT.mins = 0;
			time_AT.hrs++;
			if (time_AT.hrs > 23)
			{
				time_AT.hrs = 0;
				rtc_increment_day((RTC_time_t *)&time_AT);
			}
		}
	}
}