/*
 * display.h
 *
 * Created: 11/06/2015 17:09:24
 *  Author: paul.qureshi
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_


#define DIS_NUM_DIGITS			7


// current sources, MIC5891
#define SRC_PORT				PORTD
#define SRC_CLK_PIN_bm			PIN1_bm
#define SRC_SIN_PIN_bm			PIN3_bm
#define SRC_OE_PIN_bm			PIN4_bm			// sources off when high, ext. pull-up
#define SRC_STROBE_PIN_bm		PIN5_bm


// current sinks, TLC5928
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
#define DIS_TC_CCA_vect			TCC5_CCA_vect



extern void DIS_init(void);
extern void DIS_set_digit(uint8_t digit, char c);
extern void DIS_set_string(uint8_t digit, const char *s);
extern void DIS_set_digit_bitmap(uint8_t digit, uint16_t bitmap);
extern void DIS_set_brightness(uint8_t brightness);
extern void DIS_set_digit_brightness(uint8_t digit, uint8_t brightness);



#endif /* DISPLAY_H_ */