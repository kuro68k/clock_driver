/*
 * display.c
 *
 * Created: 11/06/2015 17:09:16
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <string.h>
#include "hw_misc.h"
#include "display.h"

// Timer runs at F_CPU
//#define DIS_TC_PER		0x01387		// 3200Hz row clock, 200Hz frame clock
#define FRAME_RATE		200
#define DIS_TC_PER		(F_CPU / (FRAME_RATE * DIS_NUM_DIGITS))

// TLC5928 sink
// MIC5891 source

// 16 seg modules, 6 digits + 1 dots
// 7 current sources
// 16 current sinks

extern const __flash uint16_t segbmp[];

volatile uint16_t	bitmap_AT[DIS_NUM_DIGITS];


#pragma region Serial Interaces

/**************************************************************************************************
** Wait for the last byte to be shifted out the SPI port to TLC5928
*/
inline void dis_sink_wait(void)
{
	while (!(SINK_SPI.STATUS & SPI_IF_bm))
		;
}

/**************************************************************************************************
** Send a byte out on the sink SPI port TLC5928
*/
inline void dis_sink_spi(uint8_t byte)
{
	dis_sink_wait();
	SINK_SPI.DATA = byte;
}

/**************************************************************************************************
** Latch TLC5928 shift register to sink outputs
*/
inline void dis_sink_latch(void)
{
	SINK_PORT.OUTSET = SINK_LAT_PIN_bm;
	SINK_PORT.OUTCLR = SINK_LAT_PIN_bm;
}

/**************************************************************************************************
** Clock one bit into MIC5891 shift register
*/
inline void dis_source_clock(void)
{
	SRC_PORT.OUTSET = SRC_CLK_PIN_bm;
	SRC_PORT.OUTCLR = SRC_CLK_PIN_bm;
}

/**************************************************************************************************
** Latch MIC5891 shift register to source outputs
*/
inline void dis_source_latch(void)
{
	SRC_PORT.OUTSET = SRC_STROBE_PIN_bm;
	SRC_PORT.OUTCLR = SRC_STROBE_PIN_bm;
}

#pragma endregion

#pragma region Display Updating

/**************************************************************************************************
** Set 16 seg digit by ASCII value
*/
void DIS_set_digit(uint8_t digit, char c)
{
	bitmap_AT[digit] = segbmp[(uint8_t)c];
}

/**************************************************************************************************
** Display a string from flash
*/
void DIS_set_string_P(uint8_t digit, const __flash char *s)
{
	while (digit < DIS_NUM_DIGITS)
		DIS_set_digit(digit++, *s++);
}

/**************************************************************************************************
** Set 16 seg digit by ASCII value
*/
void DIS_set_digit_bitmap(uint8_t digit, uint16_t bitmap)
{
	bitmap_AT[digit] = bitmap;
}

/**************************************************************************************************
** Display a string from RAM
*/
void DIS_set_string(uint8_t digit, const char *s)
{
	while (digit < DIS_NUM_DIGITS)
		DIS_set_digit(digit++, *s++);
}

/**************************************************************************************************
** Set display brightness, 0-100%
*/
void DIS_set_brightness(uint8_t brightness)
{
	if (brightness >= 100)
	{
		DIS_TC.CCA = 0xFFFF;	// never trigger
		return;
	}
	
	uint32_t cca = DIS_TC_PER * brightness;
	cca /= 100;
	DIS_TC.CCA = cca;
}

#pragma endregion

/**************************************************************************************************
** Set up display driver
*/
void DIS_init(void)
{
	memset((void *)bitmap_AT, 0, sizeof(bitmap_AT));

	// port setup in hw_misc.c/HW_init()

	SINK_SPI.CTRL = SPI_CLK2X_bm | SPI_MASTER_bm | SPI_MODE_0_gc | SPI_PRESCALER_DIV4_gc;
	SINK_SPI.CTRLB = SPI_BUFMODE_OFF_gc | SPI_SSD_bm;				// disable slave select
	SINK_SPI.INTCTRL = 0;
	SINK_SPI.CTRL |= SPI_ENABLE_bm;

	SINK_SPI_PORT.OUTCLR = SINK_CLK_PIN_bm;
	SINK_PORT.OUTCLR = SINK_LAT_PIN_bm;
	SINK_PORT.OUTSET = SINK_BLANK_PIN_bm;

	// clear sink shift register
	dis_sink_spi(0x00);
	dis_sink_spi(0x00);
	dis_sink_wait();
	dis_sink_latch();

	// clear source shift register
	SRC_PORT.OUTCLR = SRC_SIN_PIN_bm;
	for (uint8_t i = 0; i < 8; i++)
		dis_source_clock();
	dis_source_latch();
	SRC_PORT.OUTCLR = SRC_OE_PIN_bm;								// sources always enabled

	DIS_TC.CTRLA = 0;
	DIS_TC.CTRLB = 0;
	DIS_TC.CTRLC = 0;
	DIS_TC.CTRLD = 0;
	DIS_TC.CTRLE = 0;
	DIS_TC.INTCTRLA = TC45_OVFINTLVL_HI_gc;
	DIS_TC.INTCTRLB = 0;
	DIS_TC.PER = DIS_TC_PER;
	DIS_TC.CCA = 0xFFFF;											// start at max brightness
	DIS_TC.CNT = 0;
	DIS_TC.CTRLA = TC45_CLKSEL_DIV1_gc;
}

/**************************************************************************************************
** Display update interrupt
*/
ISR(DIS_TC_OVF_vect)
{
	static uint8_t	digit = 0;
	
	DIS_TC.INTFLAGS = TC5_OVFIF_bm;			// clear interrupt flag

	SINK_PORT.OUTSET = SINK_BLANK_PIN_bm;	// blank output while updating
	
	// set up next common sink
	digit++;
	if (digit >= DIS_NUM_DIGITS)
	{
		digit = 0;
		SRC_PORT.OUTSET = SRC_SIN_PIN_bm;
	}
	dis_source_clock();
	SRC_PORT.OUTCLR = SRC_SIN_PIN_bm;
	
	// load sink outputs
	dis_sink_spi(bitmap_AT[digit]);
	dis_sink_spi(bitmap_AT[digit]);
	dis_sink_wait();
	dis_sink_latch();
	
	SINK_PORT.OUTCLR = SINK_BLANK_PIN_bm;
}

/**************************************************************************************************
** Display brightness control interrupt
*/
ISR(DIS_TC_CCA_vect)
{
	DIS_TC.INTFLAGS = TC5_CCAIF_bm;			// clear interrupt flag
	SINK_PORT.OUTSET = SINK_BLANK_PIN_bm;
}