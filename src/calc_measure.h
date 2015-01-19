/*
 * calc_measure.h
 *
 * Created: 18.01.2015 20:59:37
 *  Author: chris
 */ 


#ifndef CALC_MEASURE_H_
#define CALC_MEASURE_H_

#include <asf.h>
#include "conf_board.h"

	 /**
	 * \brief Initialize calculation bases by sensor values
	 *
	 * \param _adc_b LSB bits of ACS758 at 0A
	 * \param _adc_usig_base of ADC at 0V
	 */
	void init_calc(uint16_t _adc_b, uint16_t _adc_usig_base);
		
	/**
	* \brief Calculate current in 10mA for installed ACS758 sensor
	*
	* \param res Resolution from unsigned ADC measurement
	*/	
	uint16_t calc_mA(uint16_t res);

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
	uint32_t calc_cap_mAms(uint32_t res1, uint32_t res2, uint32_t time1, uint32_t time2);
	
	/**
	* \brief Calculate voltage in 0.01V for main battery
	*
	* \param res Resolution from unsigned ADC measurement
	*/	
	uint16_t calc_main_mV(uint16_t res);

#endif /* CALC_MEASURE_H_ */