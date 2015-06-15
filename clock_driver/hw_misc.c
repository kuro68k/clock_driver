/*
 * hw_misc.c
 *
 * Created: 27/03/2015 15:19:04
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "hw_misc.h"
#include "display.h"

/**************************************************************************************************
* Set up hardware
*/
void HW_init(void)
{
	SLEEP.CTRL	= SLEEP_SMODE_IDLE_gc | SLEEP_SEN_bm;

	// set 16MHz CPU clock
	OSC.CTRL |= OSC_RC32MEN_bm;											// enable 32MHz RC oscillator
	while(!(OSC.STATUS & OSC_RC32MRDY_bm));								// wait for oscillator to become ready
	HW_CCPWrite(&CLK.PSCTRL, CLK_PSADIV_2_gc | CLK_PSBCDIV_1_1_gc);		// 2:1:1 dividers, resulting in 16MHz
	HW_CCPWrite(&CLK.CTRL, CLK_SCLKSEL_RC32M_gc);						// switch to 16MHz clock
	OSC.CTRL = OSC_RC32MEN_bm;											// disable all other clock sources

	// PORT A, display current sink
	PORTA.OUT = SINK_BLANK_PIN_bm;
	PORTA.DIR = 0xFF;
	
	// Port C, I2C, USART, display current sink
	PORTC.OUT = 0;
	PORTC.DIR = 0x1F | SINK_CLK_PIN_bm | SINK_SIN_PIN_bm;
	ENABLE_PULLUP(SINK_SOUT_PINCTRL);
	
	// Port D, display current source
	PORTD.OUT = SRC_OE_PIN_bm | SRC_STROBE_PIN_bm;
	PORTD.DIR = PIN0_bm |
				SRC_CLK_PIN_bm | SRC_SIN_PIN_bm | SRC_OE_PIN_bm | SRC_STROBE_PIN_bm |
				PIN6_bm | PIN7_bm;
	ENABLE_PULLUP(PORTD.PIN2CTRL);		// SOUT, USART RX
	
	// PORT R, xtal
	PORTR.OUT = 0;
	PORTR.DIR = 0xFF;
}

/**************************************************************************************************
** Write a CCP protected register.
**
** Interrupts are temporarily disabled during the write. Code mostly adapted/stolen from Atmel's
** clksys_driver.c and avr_compiler.h.
*/
void HW_CCPWrite(volatile uint8_t *address, uint8_t value)
{
        uint8_t	saved_sreg;

        // disable interrupts if running
		saved_sreg = SREG;
		cli();
		
		volatile uint8_t * tmpAddr = address;
        RAMPZ = 0;

        asm volatile(
                "movw r30,  %0"       "\n\t"
                "ldi  r16,  %2"       "\n\t"
                "out   %3, r16"       "\n\t"
                "st     Z,  %1"       "\n\t"
                :
                : "r" (tmpAddr), "r" (value), "M" (CCP_IOREG_gc), "i" (&CCP)
                : "r16", "r30", "r31"
                );

        SREG = saved_sreg;
}

/**************************************************************************************************
* Calculate a CRC16 for given buffer
*/
uint16_t HW_crc16(const void *buffer, uint8_t length)
{
	CRC.CTRL = CRC_RESET1_bm;
	NOP();
	CRC.CTRL = CRC_SOURCE_IO_gc;
	
	while(length--)
		CRC.DATAIN = *(uint8_t *)buffer++;
	return ((CRC.CHECKSUM1 << 8) | CRC.CHECKSUM0);
}
