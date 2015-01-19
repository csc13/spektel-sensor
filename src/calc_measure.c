/*
 * calc_measure.c
 *
 * Created: 18.01.2015 20:59:13
 *  Author: chris
 */ 

#include "calc_measure.h"

uint16_t adc_b = ADC_B;
uint16_t adc_usig_base = ADC_USIG_BASE;

	 /**
	 * \brief Initialize calculation bases by sensor values
	 *
	 * \param _adc_b LSB bits of ACS758 at 0A
	 * \param _adc_usig_base of ADC at 0V
	 */
	void init_calc(uint16_t _adc_b, uint16_t _adc_usig_base) {
		adc_b = _adc_b;
		adc_usig_base = _adc_usig_base;
	}
	
	

    /**
	 * \brief Calculate current in 10mA for installed ACS758 sensor
	 *
	 * \param res Resolution from unsigned ADC measurement
	 */	
	uint16_t calc_mA(uint16_t res) {
		if( adc_b >= res ) { 
			return 0; //clipping to zero
		}
		uint32_t _c = (( (uint32_t)( ((uint32_t)(res - adc_b)) * ((uint32_t)ACS_R_M1_O) ) ) >> ACS_SHIFT_M1);
		return ((uint16_t)_c);
	}
	
	 /**
	 * \brief Calculate used capacity between two measurements of installed ACS758 sensor
	 *
	 * \param res1 Resolution from first unsigned ADC measurement
	 * \param res2 Resolution from second unsigned ADC measurement
	 * \param time1 Time in ms from first unsigned ADC measurement
	 * \param time2 Time in ms from second unsigned ADC measurement
	 *
	 * \retval Used capacity in mAms
	 */	
	uint32_t calc_cap_mAms(uint32_t res1, uint32_t res2, uint32_t time1, uint32_t time2) {
		if( adc_b >= res1 ) res1 = adc_b; //clipping to zero
		if( adc_b >= res2 ) res2 = adc_b; //clipping to zero
		return (((uint32_t)(((res1 + res2 - (adc_b << 1)) >> 1)  * (time2 - time1) * ACS_R_M_O)) >> ACS_SHIFT_M); //for time in 1ms per bit
	}
	
	
	uint16_t calc_main_mV(uint16_t res) {
		if( res <= adc_usig_base ) { 
			return 0; 
		}
		else {
			double _v = (double)((double)(res - adc_usig_base) * (double)ADC_MV_12RES * (double)MAIN_RES_DIV / 10);
			return (uint16_t)_v; // ADC_MV_12RES * MAIN_RES_DIV / 10 (for 10mV)
		}
	}