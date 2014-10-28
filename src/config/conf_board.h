/**
 * \file
 *
 * \brief User board configuration template
 *
 */

#ifndef CONF_BOARD_H
#define CONF_BOARD_H

#define BOARD_REV_A

//#define ENABLE_USART true

#ifdef BOARD_REV_A
// TWIM for BMP180
	 #define TWI_MASTER       TWIC
	 #define TWI_MASTER_PORT  PORTC
	 #define TWI_MASTER_ADDR  0x50
	 #define TWI_SPEED        1000000
	 
// TWIS for XBUS
	 #define TWI_SLAVE        TWIC
	 #define TWI_SLAVE_PORT  PORTD


// USART
	#define PR_USART1_bm  0xC0  /* Port D USART1 bit mask. */
	#define PR_USART1_bp  128  /* Port D USART1 bit position. */
	#define USARTD_REMAP 0x10  /* USART0 Remap from Px[3:0] to Px[7:4], (for additional TC4D from Px3 to Px7 and TC4C from Px2 to Px6 --> set 0x1C)*/
	
// LEDs
	#define LED_GREEN IOPORT_CREATE_PIN(PORTC, 2)
	#define LED_RED IOPORT_CREATE_PIN(PORTD, 4)
	
// ADCs
	#define ADC_MAIN    ADCA
	#define ADC_MAIN_CH ADC_CH0 // since the XMEGA_E has only one ADC channel
	//#define ADC_MAIN_VOLTAGE_CH ADC_CH1
	#define ADC_MAIN_CURRENT_PIN ADCCH_POS_PIN0
	#define ADC_MAIN_VOLTAGE_PIN ADCCH_POS_PIN1
	
	#define ADC_CLOCK      1800000  //1.8MHz 
	#define ADC_CUR_AVERAG_SAMP ADC_SAMPNUM_8X
	#define ADC_VOLT_AVERAG_SAMP ADC_SAMPNUM_8X
	#define ADC_USIG_BASE  165  // ADC result for zero voltage at pin for unsigned mode, 205 by datasheet
	#define ADC_MV_12RES		0.4899  //mV per LSB at 12 bit resolution

/* Calculation of  current is
 I = (((RES - ADC_USIG_BASE) * ADC_MV_12RES) - ACS758_BASE) / ACS758_RATE
 I = ((RES - ADC_USIG_BASE) - (ACS758_BASE / ADC_MV_12RES)) * (ADC_MV_12RES / ACS758_RATE)
 I = ((RES - ADC_USIG_BASE) - ACS_B) * (ADC_MV_12RES / ACS758_RATE)
 I = ((RES - ADC_B) * ACS_R(_M)
 I = (((RES - ADC_B) * ACS_R(_M)_O) >> ACS_SHIFT(_M)
*/ 
 	
	#define ACS758_U100 
	#ifdef  ACS758_U100  
		#define ACS758_BASE		410		// mV at 0mA current at Vcc 3.3V (396 by data sheet at Vcc 3.3V)	
		#define ACS758_RATE		26.4	// mV/A	(at 3.3V)
		#define ACS758_RATE_M1	0.264	// mV/10mA	(at 3.3V)
		#define ACS758_RATE_M	0.0264	// mV/mA	(at 3.3V)
		
		#define ACS_B			837 	//(ACS758_BASE / ADC_MV_12RES)
		#define ADC_B			1002 	//(ADC_USIG_BASE + ACS_B)
		
		#define ACS_R			0.018557//(ADC_MV_12RES / ACS758_RATE)
		#define ACS_R_M1		1.85568	//(ADC_MV_12RES / ACS758_RATE_M1)
		#define ACS_R_M			18.5568	//(ADC_MV_12RES / ACS758_RATE_M)
		
		#define ACS_R_O			9.5 	// ACS_R * 512
		#define ACS_R_M1_O		14.8	// ACS_R_M * 8
		#define ACS_R_M_O		4751	// ACS_R_M * 256  for 32bit and max 40ms sample time only
		
		#define ACS_SHIFT		9		// /512
		#define ACS_SHIFT_M1	3		// /8
		#define ACS_SHIFT_M		8		// /256
	#endif
	
	/**
	 * \brief Calculate current in 10mA for installed ACS758 sensor
	 *
	 * \param res Resolution from unsigned ADC measurement
	 */	
	static inline uint16_t calc_mA(uint16_t res) {
		if( ADC_B >= res ) { 
			return 0; //clipping to zero
		}
		return (((uint16_t)((res - ADC_B) * ACS_R_M1_O)) >> ACS_SHIFT_M1);
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
	static inline uint16_t calc_cap_mAms(uint16_t res1, uint16_t res2, uint32_t time1, uint32_t time2) {
		if( ADC_B >= res1 ) res1 = ADC_B; //clipping to zero
		if( ADC_B >= res2 ) res2 = ADC_B; //clipping to zero
		return (((uint32_t)(((res1 + res2 - (ADC_B * 2)) >> 1)  * (time2 - time1) * ACS_R_M_O)) >> ACS_SHIFT_M); //for time in 1ms per bit
	}
	
	#define MAIN_RES_DIV	21 // (R1+R2) / R2, R1 = 4K7, R2 = 235 (two 470 in parallel)
	
	static inline uint16_t calc_main_mV(uint16_t res) {
		if( res <= ADC_USIG_BASE  ) { 
			return 0; 
		}
		else {
			return ((res - ADC_USIG_BASE) * 10.28 / 10); // ADC_MV_12RES * MAIN_RES_DIV / 10 (for 10mV)
		}
	}
	
// Timer
	#define TIMER_SENS	   TCC4
	#define TIMER_SENS_RESOLUTION	31250
	#define TIMER_SENS_PER 50 // 50Hz Samples(time between samples 20ms)
#endif

#endif // CONF_BOARD_H
