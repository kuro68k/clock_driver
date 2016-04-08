/*
 * gps.c
 *
 * Created: 04/04/2016 11:55:26
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include "debug.h"
#include "hw_misc.h"
#include "xmega_baud.h"
#include "rtc.h"
#include "gps.h"

// debug output
#define GPS_ECHO


volatile char		gps_buffer_AT[256];			// !! ATOMIC !! incoming sentence buffer
volatile uint8_t	gps_rx_index_AT = 0;		// !! ATOMIC !! read head
volatile int16_t	gps_rx_sol_index_AT = -1;	// !! ATOMIC !! index of the last found SOL, or -1 if none
volatile int16_t	gps_rx_eol_index_AT = -1;	// !! ATOMIC !! index of the last found EOL, or -1 if none


/**************************************************************************************************
** Set up GPS hardware
*/
void GPS_init(void)
{
	GPS_USART.CTRLA = 0;
	GPS_USART.CTRLB = 0;
	GPS_USART.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;
	GPS_USART.CTRLD = 0;
	GPS_USART.BAUDCTRLA = BSEL(F_CPU, GPS_BAUDRATE) & 0xff;
	GPS_USART.BAUDCTRLB = (BSCALE(F_CPU, GPS_BAUDRATE) << USART_BSCALE0_bp) | (BSEL(F_CPU, GPS_BAUDRATE) >> 8);

	GPS_USART.CTRLB = USART_RXEN_bm;
	GPS_USART.CTRLA = USART_RXCINTLVL_LO_gc;
}

/**************************************************************************************************
** Convert a two digit hex string to an integer. Returns -1 if characters are invalid.
*/
uint16_t gps_convert_hex_digit(uint8_t idx)
{
	char c1 = gps_buffer_AT[idx++];
	char c2 = gps_buffer_AT[idx];
	uint8_t i = 0;
	
	if ((c1 >= '0') && (c1 <= '9'))
		i = c1 - '0';
	else if ((c1 >= 'A') && (c1 <= 'F'))
		i = c1 - 'A' + 10;
	else
		return -1;
	i <<= 4;
	
	if ((c2 >= '0') && (c2 <= '9'))
		i = c2 - '0';
	else if ((c2 >= 'A') && (c2 <= 'F'))
		i = c2 - 'A' + 10;
	else
		return -1;

	return i;
}

/**************************************************************************************************
** Convert a two digit decimal string to an integer. Returns -1 if characters are invalid.
*/
int8_t gps_convert_dec_digit(uint8_t idx)
{
	char c1 = gps_buffer_AT[idx++];
	char c2 = gps_buffer_AT[idx];

	if ((c1 < '0') || (c1 > '9') || (c2 < '0') || (c2 > '9'))
		return -1;
	
	uint8_t i;
	i = (c1 - '0') * 10;
	i += c2 - '0';
	return i;
}

/**************************************************************************************************
** Find next character in circular buffer. Returns -1 if end reached.
*/
uint16_t gps_skip_char(char c, uint8_t start, uint8_t end)
{
	while (start != end)
	{
		if (gps_buffer_AT[start] == c)
		{
			start++;
			if (start == end)
				return -1;
			return start;
		}
		start++;
	}
	
	return -1;
}

/**************************************************************************************************
** Check for and decode sentences periodically
*/
void GPS_task(void)
{
	int16_t sol;
	int16_t eol;
	ATOMIC(sol = gps_rx_sol_index_AT);
	ATOMIC(eol = gps_rx_eol_index_AT);
	if ((eol == -1) || (sol == -1))
		return;
	// line found, decode it
	ATOMIC(gps_rx_sol_index_AT = -1);
	ATOMIC(gps_rx_eol_index_AT = -1);

	uint8_t idx = sol & 0xFF;
	idx++;	// skip '$'
	
	// check for "GPRMC,"
	if ((gps_buffer_AT[idx++] != 'G') ||
		(gps_buffer_AT[idx++] != 'P') ||
		(gps_buffer_AT[idx++] != 'R') ||
		(gps_buffer_AT[idx++] != 'M') ||
		(gps_buffer_AT[idx++] != 'C') ||
		(gps_buffer_AT[idx++] != ','))
		return;

	// checksum
	uint8_t checksum = 'G' ^ 'P' ^ 'R' ^ 'M' ^ 'C' ^ ',';
	uint8_t cidx = idx;
	while (cidx != eol)
	{
		if (gps_buffer_AT[cidx] == '*')
			break;
		checksum ^= gps_buffer_AT[cidx++];
	}
	if (cidx == eol)	// no '*' found
		return;
	cidx++;				// skip '*'
	uint8_t comp = gps_convert_hex_digit(cidx);
	if (comp != checksum)
	{
		DBG_print_P("Bad checksum\r\n");
		return;
	}
	
	// decode time
	// $GPRMC,225446,A,4916.45,N,12311.12,W,000.5,054.7,191194,020.3,E*68
	//       1      2 3       4 5        6 7     8     9      10
	//        time   * lat     l long     l speed cmg   date   mag   m
	uint16_t idec = idx;
	idec = gps_skip_char(',', idec, eol);	// 1
	if (idec == -1)
		return;
	int hrs, mins, secs;
	hrs = gps_convert_dec_digit(idec);
	idec = (idec + 2) & 0xFF;
	mins = gps_convert_dec_digit(idec);
	idec = (idec + 2) & 0xFF;
	secs = gps_convert_dec_digit(idec);
	
	if ((hrs < 0) || (hrs > 23) ||
		(mins < 0) || (mins > 59) ||
		(secs < 0) || (secs > 59))
		return;
	
	// decode date
	idec = gps_skip_char(',', idec, eol);	// 2
	idec = gps_skip_char(',', idec, eol);	// 3
	idec = gps_skip_char(',', idec, eol);	// 4
	idec = gps_skip_char(',', idec, eol);	// 5
	idec = gps_skip_char(',', idec, eol);	// 6
	idec = gps_skip_char(',', idec, eol);	// 7
	idec = gps_skip_char(',', idec, eol);	// 8
	idec = gps_skip_char(',', idec, eol);	// 9
	if (idec == -1)
		return;
	int day, month, year;
	day = gps_convert_dec_digit(idec);
	idec = (idec + 2) & 0xFF;
	month = gps_convert_dec_digit(idec);
	idec = (idec + 2) & 0xFF;
	year = gps_convert_dec_digit(idec);

	if ((day < 0) || (month < 0) || (year < 0) || (year > 99))
		return;
	
	RTC_time_t time;
	time.d = day;
	time.m = month;
	time.y = year;
	time.hrs = hrs;
	time.mins = mins;
	time.secs = secs;
	if (!RTC_validate(&time))
		return;
}

/**************************************************************************************************
** USART RX interrupt handler, stores value in a circular buffer
*/
ISR(GPS_USART_RXC_vect)
{
	uint8_t c = GPS_USART.DATA;		// clears interrupt flag

	if (c == '$')
		gps_rx_sol_index_AT = gps_rx_index_AT;
	if (c == '\r')
		gps_rx_eol_index_AT = gps_rx_index_AT;
	gps_buffer_AT[gps_rx_index_AT++] = c;

#ifdef GPS_ECHO
	DBG_putc(c);
#endif
}
