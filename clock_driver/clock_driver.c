/*
 * clock_driver.c
 *
 * Created: 11/06/2015 16:22:01
 *  Author: paul.qureshi
 */ 


#include <avr/io.h>
#include <stdbool.h>
#include "debug.h"
#include "hw_misc.h"
#include "display.h"
#include "rtc.h"

int main(void)
{
	DBG_init();
	HW_init();
	DIS_init();
	RTC_init();

	DBG_print_P(PSTR("*** RESET ***\r\n"));

	for(;;);
}