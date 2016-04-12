/*
 * clock_driver.c
 *
 * Created: 11/06/2015 16:22:01
 *  Author: paul.qureshi
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "debug.h"
#include "hw_misc.h"
#include "adc.h"
#include "display.h"
#include "rtc.h"
#include "gps.h"
#include "ui.h"

// 2kclk (2s) watchdog, continuous BOD at 3.0V
FUSES = {
	0xFF,		// fusebyte 0
	0x08,		// fusebyte 1
	0xFE,		// fusebyte 2
	0xFF,		// fusebyte	3
	0xF1,		// fusebyte 4
	0xE0,		// fusebyte 5
	0xFF,		// fusebyte 6
};

int main(void)
{
	DBG_init();
	HW_init();
	DIS_init();
	ADC_init();
	RTC_init();
	GPS_init();
	UI_init();

	// enable interrupts
	HW_CCPWrite(&PMIC.CTRL, PMIC_RREN_bm | PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm);
	sei();

	// print header
	DBG_print_P(PSTR("*** RESET ***\r\n"));
	DBG_print_P(PSTR("Flags: "));
	if (RST.STATUS & RST_SRF_bm)
		DBG_print_P(PSTR("SR "));
	if (RST.STATUS & RST_PDIRF_bm)
		DBG_print_P(PSTR("PDI "));
	if (RST.STATUS & RST_WDRF_bm)
		DBG_print_P(PSTR("WDR "));
	if (RST.STATUS & RST_BORF_bm)
		DBG_print_P(PSTR("BOR "));
	if (RST.STATUS & RST_EXTRF_bm)
		DBG_print_P(PSTR("EXT "));
	if (RST.STATUS & RST_SRF_bm)
		DBG_print_P(PSTR("POR"));
	DBG_print_P(PSTR("\r\n"));
	RST.STATUS = 0xFF;	// clear all flags
	
	DIS_set_brightness(100);
	uint8_t i = 1;
	for(;;)
	{
		for (uint8_t j = 0; j < DIS_NUM_DIGITS; j++)
			DIS_set_digit(j, i);
		i++;
		if (i > 17) i = 0;
		RTC_wait_for_second_tick();
	}
	
	UI_run();
}
