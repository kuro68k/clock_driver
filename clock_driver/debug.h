/*
 * debug.h
 *
 * Created: 16/06/2015 11:50:39
 *  Author: paul.qureshi
 */ 


#ifndef DEBUG_H_
#define DEBUG_H_


#define DEBUG_OUTPUT



#define DBG_USART				USARTD0
#define DBG_BAUDRATE			115200
#define DBG_USART_CLK2X			USART_CLK2X_bm
#define DBG_USART_SWAP_PINS
#define DBG_PORT				PORTD
#define DBG_TX_PIN_bm			PIN7_bm



#ifdef DEBUG_OUTPUT

#include <stdio.h>
#include <avr/pgmspace.h>

extern void DBG_init(void);
extern void DBG_print_P(const __flash char *s)	__attribute__((nonnull));
#define DBG_printf_P(...)	printf_P(__VA_ARGS__)

inline void DBG_putc(char c)
{
	while(!(DBG_USART.STATUS & USART_DREIF_bm));
	DBG_USART.DATA = c;
}

#else

#define DBG_init()
#define DBG_print_P(...)
#define DBG_printf_P(...)
#define DBG_putc(X)

#endif




#endif /* DEBUG_H_ */