/*
 * display.c
 *
 * Created: 11/06/2015 17:09:16
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include "hw_misc.h"
#include "display.h"

// div 1, 16MHz
#define DIS_TC_PER		0x01387		// 3200Hz row clock, 200Hz frame clock

#define SINK_BYTE_WIDTH		4
#define SINK_BIT_WIDTH		(SINK_BYTE_WIDTH * 8)
#define SRC_BYTE_WIDTH		16
#define SRC_BIT_WIDTH		(SRC_BYTE_WIDTH * 8)

volatile uint8_t	bitmap[SINK_BYTE_WIDTH][SRC_BYTE_WIDTH];
volatile uint8_t	*frame_ptr_AT = (uint8_t *)bitmap;			// !! ATOMIC !!	pointer to the next frame bitmap
uint8_t brightness_SIG = 5;


/**************************************************************************************************
** Set up display driver
*/
void DIS_init(void)
{
	memset(bitmap, 0, sizeof(bitmap));
	
	SRC_USART.CTRLA = 0;											// no interrupts
	SRC_USART.BAUDCTRLA = 0;										// clock = fCPU/2
	SRC_USART.BAUDCTRLB = 0;
	SRC_USART.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
	SRC_USART.CTRLC = USART_CMODE_MSPI_gc | USART_CHSIZE_8BIT_gc;	// master SPI mode
	SRC_USART.CTRLD = 0;

	/*
	SINK_SPI.CTRL = SPI_CLK2X_bm | SPI_MASTER_bm | SPI_MODE_0_gc | SPI_PRESCALER_DIV4_gc;
	SINK_SPI.CTRLB = SPI_BUFMODE_OFF_gc | SPI_SSD_bm;				// disable slave select
	SINK_SPI.INTCTRL = 0;
	SINK_SPI.CTRL |= SPI_ENABLE_bm;
	*/
	SINK_SPI_PORT.OUTCLR = SINK_CLK_PIN_bm;
	SINK_PORT.OUTCLR = SINK_LAT_PIN_bm;
	SINK_PORT.OUTSET = SINK_BLANK_PIN_bm;
	// clear sink shift register
	SINK_SPI_PORT.OUTCLR = SINK_SIN_PIN_bm;
	for (uint8_t i = 0; i < SINK_BIT_WIDTH; i++)
	{
		SINK_SPI_PORT.OUTSET = SINK_CLK_PIN_bm;
		SINK_SPI_PORT.OUTCLR = SINK_CLK_PIN_bm;
	}
	
	SRC_PORT.OUTCLR = SRC_OE_PIN_bm;								// sources always enabled

	DIS_TC.CTRLA = 0;
	DIS_TC.CTRLB = 0;
	DIS_TC.CTRLC = 0;
	DIS_TC.CTRLD = 0;
	DIS_TC.CTRLE = 0;
	DIS_TC.INTCTRLA = TC45_OVFINTLVL_HI_gc;
	DIS_TC.INTCTRLB = 0;
	DIS_TC.PER = DIS_TC_PER;
	DIS_TC.CNT = 0;
	DIS_TC.CTRLA = TC45_CLKSEL_DIV1_gc;
}

/**************************************************************************************************
** Display update interrupt
*/
ISR(DIS_TC_OVF_vect)
{
	static uint8_t	sink = 0;
	static uint8_t	*ptr = (uint8_t *)bitmap;
	
	DIS_TC.INTFLAGS = TC5_OVFIF_bm;			// clear interrupt flag
	
	// set up next common sink
	sink++;
	if (sink >= SINK_BIT_WIDTH)
	{
		ptr = (uint8_t *)frame_ptr_AT;		// reload frame pointer
		sink = 0;
		SINK_SPI_PORT.OUTSET = SINK_SIN_PIN_bm;
	}
	SINK_SPI_PORT.OUTSET = SINK_CLK_PIN_bm;
	SINK_SPI_PORT.OUTCLR = SINK_CLK_PIN_bm | SINK_SIN_PIN_bm;
	SINK_PORT.OUTSET = SINK_LAT_PIN_bm;
	SINK_PORT.OUTCLR = SINK_LAT_PIN_bm;
	
	
	// load a row of source bytes
	for (uint8_t i = 0; i < SRC_BYTE_WIDTH; i++)
	{
		while(!(SRC_USART.STATUS & USART_DREIF_bm));
		//SRC_USART.DATA = bitmap[sink][i];
		SRC_USART.DATA = *ptr++;
	}
	while(!(SRC_USART.STATUS & USART_TXCIF_bm));
	SRC_PORT.OUTSET = SRC_STROBE_PIN_bm;
	SRC_PORT.OUTCLR = SRC_STROBE_PIN_bm;
	
	// light row up
	SINK_PORT.OUTCLR = SINK_BLANK_PIN_bm;
	for (uint8_t i = 0; i < brightness_SIG; i++)
		_delay_us(10);
	SINK_PORT.OUTSET = SINK_BLANK_PIN_bm;
}
