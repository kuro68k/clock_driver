/*
 * display.h
 *
 * Created: 11/06/2015 17:09:24
 *  Author: paul.qureshi
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_


// current sources
#define SRC_PORT				PORTD
#define SRC_OE_PIN_bm			PIN4_bm			// sources off when high, ext. pull-up
#define SRC_STROBE_PIN_bm		PIN5_bm

#define SRC_USART				USARTD0
#define SRC_CLK_PIN_bm			PIN1_bm
#define SRC_SIN_PIN_bm			PIN3_bm


// current sinks
#define SINK_PORT				PORTA
#define SINK_BLANK_PIN_bm		PIN6_bm			// sinks off when high, ext. pull-up
#define SINK_LAT_PIN_bm			PIN7_bm

#define SINK_SPI				SPIC
#define SINK_SPI_PORT			PORTC
#define SINK_CLK_PIN_bm			PIN5_bm
#define SINK_SOUT_PIN_bm		PIN6_bm			// output from shift register, input to MCU
#define SINK_SOUT_PINCTRL		SINK_SPI_PORT.PIN6CTRL
#define SINK_SIN_PIN_bm			PIN7_bm			// input to shift register, outpit from MCU


// display refresh timer
#define DIS_TC					TCC5
#define DIS_TC_OVF_vect			TCC5_OVF_vect



extern void DIS_init(void);



#endif /* DISPLAY_H_ */