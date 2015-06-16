/*
 * debug.c
 *
 * Created: 16/06/2015 11:50:31
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "debug.h"


#ifdef DEBUG_OUTPUT


/**************************************************************************************************
** Output a character to the debug USART
*/
int dbg_putc(char c, FILE *stream)
{
	while(!(DBG_USART.STATUS & USART_DREIF_bm));
	DBG_USART.DATA = c;
	return 0;
}

FILE usart_str = FDEV_SETUP_STREAM(dbg_putc, NULL, _FDEV_SETUP_WRITE);

/**************************************************************************************************
** Set up debug output USART
*/
void DBG_init(void)
{
	DBG_PORT.DIRSET = DBG_TX_PIN_bm;
	
	int		bsel = DBG_USART_BSEL;
	uint8_t	bscale = DBG_USART_BSCALE;

	DBG_USART.BAUDCTRLA = (uint8_t)bsel;
	DBG_USART.BAUDCTRLB = (bscale << 4) | (bsel >> 8);
	DBG_USART.CTRLA = 0;
	DBG_USART.CTRLB = USART_TXEN_bm | DBG_USART_CLK2X;
	DBG_USART.CTRLC = USART_CHSIZE_8BIT_gc;
	DBG_USART.CTRLD = 0;
	
	stdout = &usart_str;
}

/**************************************************************************************************
** Output string from flash memory. Much faster than printf_P() with no conversions.
*/
void DBG_print_P(const __flash char *s)
{
	while (*s != '\0')
	{
		while(!(DBG_USART.STATUS & USART_DREIF_bm));
		DBG_USART.DATA = *s++;
	}
}

#endif
