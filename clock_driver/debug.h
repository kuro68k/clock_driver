/*
 * debug.h
 *
 * Created: 16/06/2015 11:50:39
 *  Author: paul.qureshi
 */ 


#ifndef DEBUG_H_
#define DEBUG_H_


#define DEBUG_OUTPUT



#define DBG_USART				USARTC0
#define DBG_USART_BSEL			0
#define DBG_USART_BSCALE		0
#define DBG_USART_CLK2X			USART_CLK2X_bm
#define DBG_PORT				PORTC
#define DBG_TX_PIN_bm			PIN3_bm



#ifdef DEBUG_OUTPUT

#include <stdio.h>
#include <avr/pgmspace.h>

extern void DBG_init(void);
extern void DBG_print_P(const __flash char *s)	__attribute__((nonnull));
#define DBG_printf_P(...)	printf_P(__VA_ARGS__)

#else

#define DBG_init()
#define DBG_print_P(...)
#define DBG_printf_P(...)

#endif




#endif /* DEBUG_H_ */