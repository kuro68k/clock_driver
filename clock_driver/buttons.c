/*
 * buttons.c
 *
 * Created: 11/04/2016 10:18:39
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include "buttons.h"

const struct {
	PORT_t	*port;
	uint8_t	mask;
	bool	repeat;
} btn_port_map[BUTTON_COUNT] = {	{ &PORTA, PIN0_bm, false },
									{ &PORTA, PIN1_bm, true },
									{ &PORTA, PIN2_bm, true },
								};

volatile uint8_t	BTN_hold_SIG[BUTTON_COUNT];
volatile uint8_t	BTN_press_SIG[BUTTON_COUNT];

/**************************************************************************************************
* Set up button input
*/
void BTN_init(void)
{
	BUTTON_TC.CTRLA = 0;	// stopped while we configure
	BUTTON_TC.CTRLB = 0;
	BUTTON_TC.CTRLC = 0;
	BUTTON_TC.CTRLD = 0;
	BUTTON_TC.CTRLE = 0;
	BUTTON_TC.CTRLF = 0;
	BUTTON_TC.INTCTRLA = TC45_OVFINTLVL_LO_gc;
	BUTTON_TC.CNT = 0;
	BUTTON_TC.PER = ((16000000L / BUTTON_TC_DIV) / BUTTON_HZ) - 1;	// check this is 0x1869 @ 16MHz/10Hz
	BUTTON_TC.CTRLA = BUTTON_TC_DIV_gc;
}

/**************************************************************************************************
* Button timer interrupt
*/
ISR(BUTTON_TC_OVR_vect)
{
	static uint8_t hold_timer[BUTTON_COUNT];

	for (uint8_t i = 0; i < BUTTON_COUNT; i++)
	{
		if (btn_port_map[i].port->IN & btn_port_map[i].mask)
		{
			if (hold_timer[i] < 255)
				hold_timer[i]++;
			if (hold_timer[i] == 1)
				BTN_press_SIG[i] = 0xFF;
			if ((hold_timer[i] > BUTTON_REPEAT_START) &&
				((hold_timer[i] % BUTTON_REPEAT_MODULO) == 0))
				{
					BTN_hold_SIG[i] = 0xFF;
					if (btn_port_map[i].repeat)
						BTN_press_SIG[i] = 0xFF;
				}
		}
	}
}
