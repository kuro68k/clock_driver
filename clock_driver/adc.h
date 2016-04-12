/*
 * adc.h
 *
 * Created: 16/06/2015 10:51:35
 *  Author: paul.qureshi
 */ 


#ifndef ADC_H_
#define ADC_H_


/**************************************************************************************************
** Hardware
*/

#define ADC_REFERENCE_MV		1000.0
#define ADC_MV_PER_LSB			(ADC_REFERENCE_MV/2048.0)	// 11 bit signed result


// optional GND reference pin
//#define ADC_GND				// uncomment to enable
#define ADC_GND_PIN_bm			PIN1_bm
#define ADC_GND_MUXPOS			ADC_CH_MUXPOS_PIN1_gc
#define ADC_GND_MUXNEG			ADC_CH_MUXNEGL_PIN1_gc
#define ADC_GND_INPUTMODE		ADC_CH_INPUTMODE_DIFFWGAINL_gc

// optional LDR
#define ADC_LDR_PIN_bm			PIN2_bm
#define ADC_LDR_MUXPOS			ADC_CH_MUXPOS_PIN2_gc

// optional NTC
#define ADC_NTC					// uncomment to enable
#define ADC_NTC_PIN_bm			PIN2_bm
#define ADC_NTC_MUXPOS			ADC_CH_MUXPOS_PIN2_gc
#ifndef ADC_GND
#define ADC_NTC_MUXNEG			ADC_CH_MUXNEGL_GND_gc
#else
#define ADC_NTC_MUXNEG			ADC_GND_MUXNEG
#endif
#define ADC_NTC_INPUTMODE		ADC_CH_INPUTMODE_DIFFWGAINL_gc

#define ADC_NTC_NOMINAL_RES		10000		// 10k at 25°C
#define ADC_NTC_NOMINAL_TEMP_C	25			//
#define ADC_NTC_B_COEFFICIENT	3977		// B coefficient
#define ADC_NTC_SERIES_RES		150000		// 150k series resistor


/**************************************************************************************************
** Public functions and variable
*/

extern void		ADC_init(void);
extern float	ADC_read_internal_temperature(void);
#ifdef ADC_NTC
extern float	ADC_read_ntc_temperature(void);
#endif
extern uint16_t ADC_read_input(uint8_t muxpos);



#endif /* ADC_H_ */