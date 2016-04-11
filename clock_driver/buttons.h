/*
 * buttons.h
 *
 * Created: 11/04/2016 10:18:52
 *  Author: paul.qureshi
 */ 


#ifndef BUTTONS_H_
#define BUTTONS_H_


#define BUTTON_COUNT			3
#define BUTTON_SET				0
#define	BUTTON_UP				1
#define BUTTON_DOWN				2


#define BUTTON_TC				TCC4
#define BUTTON_TC_DIV_gc		TC45_CLKSEL_DIV256_gc
#define BUTTON_TC_DIV			256
#define BUTTON_TC_OVR_vect		TCC4_OVF_vect

#define BUTTON_HZ				10
#define BUTTON_REPEAT_START		8
#define BUTTON_REPEAT_MODULO	3


extern volatile uint8_t	BTN_hold_SIG[BUTTON_COUNT];
extern volatile uint8_t	BTN_press_SIG[BUTTON_COUNT];



#endif /* BUTTONS_H_ */