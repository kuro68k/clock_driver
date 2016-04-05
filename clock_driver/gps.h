/*
 * gps.h
 *
 * Created: 04/04/2016 11:55:37
 *  Author: paul.qureshi
 */ 


#ifndef GPS_H_
#define GPS_H_


#define GPS_USART				USARTC0
#define GPS_USART_RXC_vect		USARTC0_RXC_vect
#define GPS_BAUDRATE			9600

#define GPS_PORT				PORTF
#define GPS_PWR_EN_PIN_bm		PIN0_bm
#define GPS_1PPS_PIN_bm			PIN1_bm
#define GPS_RX_PIN_bm			PIN6_bm
#define	GPS_TX_PIN_bm			PIN7_bm



#endif /* GPS_H_ */