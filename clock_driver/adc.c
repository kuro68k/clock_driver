/*
 * adc.c
 *
 * Created: 16/06/2015 10:51:43
 *  Author: paul.qureshi
 */ 

#include <avr/io.h>
#include <stddef.h>
#include <math.h>
#include "debug.h"
#include "hw_misc.h"
#include "adc.h"


// temperature sensor calibration data
float	kelvin_per_lsb = 0;
float	temp_offset = 0;


/**************************************************************************************************
** Perform a single conversion with the ADC
*/
inline int16_t adc_sample(void)
{
	ADCA.CH0.CTRL |= ADC_CH_START_bm;
	while (!(ADCA.CH0.INTFLAGS | ADC_CH_IF_bm));
	ADCA.CH0.INTFLAGS = ADC_CH_IF_bm;
	return ADCA.CH0.RES;
}

/**************************************************************************************************
** Calibrate ADC offset error
*/
void adc_calibrate(void)
{
	// offset calibration
#ifdef ADC_GND
	ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_GND_INPUTMODE;
	ADCA.CH0.MUXCTRL = ADC_GND_MUXPOS | ADC_GND_MUXNEG;
	
	int32_t	ave = 0;
	for (uint8_t i = 0; i < 16; i++)
		ave += adc_sample();
	ave /= 16;
	
	ADCA.CH0.OFFSETCORR0 = ave & 0xFF;
	ADCA.CH0.OFFSETCORR1 = (ave >> 8) & 0xFF;
	ADCA.CH0.CORRCTRL = ADC_CH_CORREN_bm;
#else
	ADCA.CH0.CORRCTRL = 0;
#endif

	// temperature sensor calibration
	int16_t hot_ref;
	float	hot_temp_k;
	hot_ref = HW_read_signature_byte(offsetof(NVM_PROD_SIGNATURES_t, TEMPSENSE0));
	hot_ref |= HW_read_signature_byte(offsetof(NVM_PROD_SIGNATURES_t, TEMPSENSE1)) << 8;
	hot_temp_k = HW_read_signature_byte(offsetof(NVM_PROD_SIGNATURES_t, HOTTEMP)) + 273.15;
	
	int16_t room_ref;
	float	room_temp_k;
	room_ref = HW_read_signature_byte(offsetof(NVM_PROD_SIGNATURES_t, TEMPSENSE2));
	room_ref |= HW_read_signature_byte(offsetof(NVM_PROD_SIGNATURES_t, TEMPSENSE3)) << 8;
	room_temp_k = HW_read_signature_byte(offsetof(NVM_PROD_SIGNATURES_t, ROOMTEMP)) + 273.15;

	// calculate offset and slope
	kelvin_per_lsb = (hot_temp_k - room_temp_k) / ((float)hot_ref - (float)room_ref);
	temp_offset = (float)room_ref - (room_temp_k / kelvin_per_lsb);

	DBG_printf_P(PSTR("kpl:\t%f\r\n"), kelvin_per_lsb);
	DBG_printf_P(PSTR("off:\t%f\r\n"), temp_offset);
}

/**************************************************************************************************
** Load calibration data for ADC and set up registers
*/
void ADC_init(void)
{
	// load ADC calibration values
	ADCA.CALL = HW_read_signature_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0));
	ADCA.CALH = HW_read_signature_byte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1));
	
	ADCA.CTRLA = 0;
	ADCA.CTRLB = ADC_CURRLIMIT_NO_gc | ADC_RESOLUTION_12BIT_gc;
	ADCA.REFCTRL = ADC_REFSEL_INT1V_gc | ADC_BANDGAP_bm | ADC_TEMPREF_bm;
	ADCA.EVCTRL = 0;
	ADCA.SAMPCTRL = 0;

#if F_CPU == 16000000
	ADCA.PRESCALER = ADC_PRESCALER_DIV256_gc;		// 62,500Hz
#elif FCPU == 8000000
	ADCA.PRESCALER = ADC_PRESCALER_DIV128_gc;		// 62,500Hz
#elif FCPU == 4000000
	ADCA.PRESCALER = ADC_PRESCALER_DIV64_gc;		// 62,500Hz
#elif FCPU == 2000000
	ADCA.PRESCALER = ADC_PRESCALER_DIV32_gc;		// 62,500Hz
#elif FCPU == 1000000
	ADCA.PRESCALER = ADC_PRESCALER_DIV16_gc;		// 62,500Hz
#else
	#error Unknown CPU frequency, unable to set ADC divider
#endif

	ADCA.CH0.INTCTRL = 0;
	ADCA.CH0.SCAN = 0;

	ADCA.CTRLA |= ADC_ENABLE_bm;
	adc_calibrate();
}

/**************************************************************************************************
** Read internal temperature sensor
*/
float ADC_read_internal_temperature(void)
{
	uint8_t i;
	float temperature_c = 0;
	
	ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_INTERNAL_gc;
	ADCA.CH0.MUXCTRL = ADC_CH_MUXINT_TEMP_gc;
	
	for (i = 0; i < 2; i++)			// discard a couple of samples
		adc_sample();
	
	for (i = 0; i < 16; i++)		// take 16 samples and average them
		temperature_c += adc_sample();
	temperature_c /= i;
	
	temperature_c -= temp_offset;
	temperature_c *= kelvin_per_lsb;
	temperature_c -= 273.15;		// kelvin to Celsius conversion
	return temperature_c;
}

/**************************************************************************************************
** Read NTC temperature
*/
float ADC_read_ntc_temperature(void)
{
	uint8_t i;
	float	mv = 0;
	float	ntc_res_ohms;
	float	steinhart;
	
	ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_NTC_INPUTMODE;
	ADCA.CH0.MUXCTRL = ADC_NTC_MUXPOS | ADC_NTC_MUXNEG;
	
	for (i = 0; i < 2; i++)			// discard a couple of samples
		adc_sample();
	
	for (i = 0; i < 16; i++)		// take 16 samples and average them
		mv += adc_sample();
	mv /= i;
	
	if (mv < 5) return -100;		// sensor disconnected/faulty
	
	ntc_res_ohms = (ADC_REFERENCE_MV / mv) - 1.0;			// (Vin/Vout) - 1
	ntc_res_ohms = ADC_NTC_SERIES_RES / ntc_res_ohms;		// R1/( (Vin/Vout)-1 )
	
	steinhart = ntc_res_ohms / ADC_NTC_NOMINAL_RES;			// R/Ro
	steinhart = log(steinhart);								// ln(R/Ro)
	steinhart /= ADC_NTC_B_COEFFICIENT;						// 1/B * ln(R/Ro)
	steinhart += 1.0 / (ADC_NTC_NOMINAL_TEMP_C + 273.15);	// + 1/To
	steinhart = 1.0 / steinhart;							// invert
	steinhart -= 273.15;									// convert to centigrade

	return steinhart;
}
