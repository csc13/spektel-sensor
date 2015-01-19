/**
 * \file
 *
 * \brief User board configuration template
 *
 */

#ifndef CONF_BOARD_H
#define CONF_BOARD_H

#define BOARD_REV_B

#define REALTIME_PLOTTER true

//#define ARDUPLOT true

#if (ARDUPLOT || REALTIME_PLOTTER)
	#define ENABLE_USART true
#endif	
  
  #ifdef BOARD_REV_B
  /*** Changes to REV_B
	* Changed pins for LEDS
	* Changed ADC pins to add VREF
	* Voltage divider for the hall sensor
	* Additional 2048V external reference voltage
 */
// TWIM for BMP180
	 #define TWI_MASTER       TWIC
	 #define TWI_MASTER_PORT  PORTC
	 #define TWI_MASTER_ADDR  0x50
	 #define TWI_SPEED        50000
	 
// TWIS for XBUS
	 #define TWI_SLAVE        TWIC
	 #define TWI_SLAVE_PORT  PORTD


// USART
	#define PR_USART1_bm  0xC0  /* Port D USART1 bit mask. */
	#define PR_USART1_bp  128  /* Port D USART1 bit position. */
	#define USARTD_REMAP 0x10  /* USART0 Remap from Px[3:0] to Px[7:4], (for additional TC4D from Px3 to Px7 and TC4C from Px2 to Px6 --> set 0x1C)*/
	
// LEDs
	#define LED_GREEN IOPORT_CREATE_PIN(PORTD, 4)
	#define LED_RED IOPORT_CREATE_PIN(PORTD, 3)
	
// Button
	#define BUT_1 IOPORT_CREATE_PIN(PORTD, 2)
	
//EEPROM
	#define CONFIG_PAGE 1
	#define CONFIG_ADDR CONFIG_PAGE * EEPROM_PAGE_SIZE
	
	#define CALIBRATION_PAGE 2
	#define CALIBRATION_ADDR CALIBRATION_PAGE * EEPROM_PAGE_SIZE	
	
// ADCs
	#define ADC_MAIN    ADCA
	#define ADC_MAIN_CH ADC_CH0 // since the XMEGA_E has only one ADC channel
	//#define ADC_MAIN_VOLTAGE_CH ADC_CH1
	#define ADC_MAIN_REF_VOLTAGE_PIN ADCCH_POS_PIN0
	#define ADC_MAIN_CURRENT_PIN ADCCH_POS_PIN1
	#define ADC_MAIN_VOLTAGE_PIN ADCCH_POS_PIN2
	
	#define ADC_CLOCK      1800000  //1.8MHz 
	#define ADC_CUR_AVERAG_SAMP ADC_SAMPNUM_8X
	#define ADC_VOLT_AVERAG_SAMP ADC_SAMPNUM_8X
	#define ADC_REF_AVERAG_SAMP ADC_SAMPNUM_8X
	#define ADC_USIG_BASE  161  // ADC result for zero voltage at pin for unsigned mode, 205 by datasheet
	#define ADC_MV_12RES		0.4899  //mV per LSB at 12 bit resolution
	
	//REV_B has voltage dividers:
	//For current measurement (3K + 4.7K) / 4.7K = 1.64
	//#define CUR_RES_DIV		1.638 //Multiply voltage with
	//By measurement of resistors
	#define CUR_RES_DIV    1.684
	   
	//For voltage measurement(4.7K + 330) / 330
	#define MAIN_RES_DIV	15.24 // (R1+R2) / R2, R1 = 4K7, R2 = 330 = 15.24
    

	/* Calculation of  current is
	 I = (((RES - ADC_USIG_BASE) * ADC_MV_12RES) - ACS758_BASE) / ACS758_RATE
	 I = ((RES - ADC_USIG_BASE) - (ACS758_BASE / ADC_MV_12RES)) * (ADC_MV_12RES / ACS758_RATE)
	 I = ((RES - ADC_USIG_BASE) - ACS_B) * (ADC_MV_12RES / ACS758_RATE)
	 I = ((RES - ADC_B) * ACS_R(_M)
	 I = (((RES - ADC_B) * ACS_R(_M)_O) >> ACS_SHIFT(_M)
	*/ 
 	
	#define ACS758_U100 
	#ifdef  ACS758_U100  
		#define ACS758_BASE		383		// mV at 0mA current at Vcc 3.3V (396 by data sheet at Vcc 3.3V)
		#define ACS758_RATE		24	    // mV/A	(at 3.3V)
		#define ACS758_RATE_M1	0.24	// mV/10mA	(at 3.3V)
		#define ACS758_RATE_M	0.024	// mV/mA	(at 3.3V)
				
		#define ACS_B			471 	//(ACS758_BASE / ADC_MV_12RES / CUR_RES_DIV)  
		#define ADC_B			632 	//(ADC_USIG_BASE + ACS_B) // Measurement 632
				
		#define ACS_R			0.0312  //(ADC_MV_12RES / ACS758_RATE * CUR_RES_DIV)
		#define ACS_R_M1		3.12	//(ADC_MV_12RES / ACS758_RATE_M1 * CUR_RES_DIV)
		#define ACS_R_M			31.2	//(ADC_MV_12RES / ACS758_RATE_M * CUR_RES_DIV)
				
		#define ACS_R_O			8.279 	// ACS_R * 256
		#define ACS_R_M1_O		3199	// ACS_R_M1 * 1024
		#define ACS_R_M_O		15996	// ACS_R_M * 512  for 32bit and max 40ms sample time only
				
		#define ACS_SHIFT		8		// /256
		#define ACS_SHIFT_M1	10		// /1024
		#define ACS_SHIFT_M		9		// /512
	#endif
  #endif
  	

// Timer
	#define TIMER_SENS	   TCC4
	#define TIMER_SENS_RESOLUTION	31250
	#define TIMER_SENS_PER 25 // 25Hz Samples(time between samples 40ms)
	//#define TIMER_SENS_PER 50 // 50Hz Samples(time between samples 20ms)

#endif // CONF_BOARD_H
