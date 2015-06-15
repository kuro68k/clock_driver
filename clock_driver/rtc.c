/*
 * rtc.c
 *
 * Created: 15/06/2015 14:52:59
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>

#include "hw_misc.h"
#include "rtc.h"


volatile RTC_time_t	time_AT;
volatile uint8_t	RTC_second_tick_SIG = 0;		// !! SIGNAL !! set to 0xFF when second ticks over

const __flash uint8_t days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


/**************************************************************************************************
** Set up RTC
*/
void RTC_init(void)
{
	memset((void *)&time_AT, 0, sizeof(time_AT));
	time_AT.d = 1;
	time_AT.m = 1;

	CLK.RTCCTRL = CLK_RTCSRC_EXTCLK_gc | CLK_RTCEN_bm;		// 32768Hz from TCXO
	
	while(RTC.STATUS & RTC_SYNCBUSY_bm);
	RTC.CNT = 0;
	RTC.PER = 32768;
	RTC.CTRL = RTC_PRESCALER_DIV1_gc;
	RTC.INTCTRL = RTC_OVFINTLVL_LO_gc;
}

/**************************************************************************************************
** Get current time
*/
void RTC_get_time(RTC_time_t *time)
{
	cli();
	memcpy(time, (void*)time_AT, sizeof(RTC_time_t));
	sei();
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
		time_AT.secs = 0;
		time_AT.mins++;
		if (time_AT.mins > 59)
		{
			time_AT.mins = 0;
			time_AT.hrs++;
			if (time_AT.hrs > 23)
			{
				time_AT.hrs = 0;
				time_AT.d++;
				
				uint8_t max_d = days_in_month[time_AT.m];
				if ((time_AT.y % 4) == 0)	// leap year calculation will break on century years, meh
					max_d++;
				
				if (time_AT.d > max_d)
				{
					time_AT.d = 0;
					time_AT.m++;
					if (time_AT.m > 12)
					{
						time_AT.m = 0;
						time_AT.y++;
						if (time_AT.y > 99)
							time_AT.y = 0;
					}
				}
			}
		}
	}
}