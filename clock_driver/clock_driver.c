/*
 * clock_driver.c
 *
 * Created: 11/06/2015 16:22:01
 *  Author: paul.qureshi
 */ 


#include <avr/io.h>
#include "hw_misc.h"
#include "display.h"

int main(void)
{
	HW_init();
	DIS_init();
	RTC_init();

	for(;;);
}