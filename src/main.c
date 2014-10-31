/**
 * \file
 *
 * \brief Spektrum RC Telemetry AllinOne Sensor
 *
 */

/**
 * \mainpage Spektrum RC Telemetry All-In-One Sensor
 *
 * \par Target functionality
 *
 * The Spektrum RC Telemetry All-In-One Sensor is a Atmel ATXMEGA32E5-U
 * based sensor board using the Spektrum XBus to communicate with the 
 * Spektrum RC telemetry module TM1000. The TM1000 is used to send the
 * telemetry informations to the RC transmitter.
 * Thanks to contributions in a RC-Goups forum (special thanks to Mukenukem),
 * the I2C/TWI based XBus protocol is known. This software and the corresponding
 * hardware was inspired by the ideas of many forum contributors.
 *
 * The target is to create a platform for DIY Spektrum RC multi-sensors, combining
 * several sensor functions in a space saving application. An important target
 * was to provide a flexible capacity sensor and alarm for relative small models.
 *
 * \par Features
 *
 * -# XBus / TWI slave support for TM1000
 * -# eFuel Gauge - Used capacity sensor (as Spektrum RC Powerbox sensor)
 * -# Capacity alarm - underun battery 80% capacity
 * -# Main battery voltage
 * -# Voltage alarm by low watermark
 * -# Ampere meter (as Sketrum Ampere sensor)
 * -# Altitude meter (as Spektrum RC Vario sensor)
 * -# Climb rate sensor (as Spektrum RC Vario sensor)
 * -# Basic USART connector
 *
 * \par Possible future features
 * -# Programmable via USB
 * -# XBus In and Out connector for daisy chaining connectors (TODO: software support)
 * -# Cell voltage measurements, control and alarm
 * -# Other sensor support
 *
 *  \par Modules
 *
 * -# spektel - Spektrum RC TM1000 XBus/TWI based communication
 * -# adc_sens - Analog signal based sensor inputs (like ACS758 hall sensor, battery main voltage, etc.)
 * -# bmp180_sens - Bosch BMP180 air pressure sensor
 *
 *  \par Configuration
 *
 * -# conf_board.h is used to configure the used board revision and hardware
 * -# conf_spektel.h is used to configure the used Spektrum RC sensors based on the transmitter
 */

/*
 * The application uses the Atmel ASF framework.
 * The project is a DIY sample and has no commercial background.
Disclaimer for this application
The use of this software and is intended for hobby use only and no liability for any loss or damage is taken in any aspect.
The use is totally on own risk.

Disclaimer and Credits for ASF
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided with the distribution.

3. The name of Atmel may not be used to endorse or promote products derived from this software without specific
prior written permission.

4. This software may only be redistributed and used in connection with an Atmel microcontroller product.

THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN
NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <asf.h>
//#include <stdio.h>

#include "spektel.h"
#include "floating_average.h"
#include "filter.h"

// ADC measurement data structures
volatile bool measure_cycle = false;
struct adc_channel_config adcch_conf;
uint16_t cur_mea_val = 0; //latest measurement
uint16_t vol_mea_val = 0; //latest measurement

// floating average data structures
tFloatAvgFilter cur_filter1 = { {0}, 0 };  //filter level 1
tFloatAvgFilter cur_filter2 = { {0}, 0 };  //filter level 2
tFloatAvgFilter cur_filter3 = { {0}, 0 };  //filter level 3
	
//simple lowpass filter	
tSimpleLowpassReg lowpass_reg = { 0, 0 };
	
// variables for capacity calculation
uint16_t cur_adc_res[2] = { ADC_B, ADC_B }; // This is power = 0
uint32_t time[2] = { 0 };
bool act = 0;
uint32_t cap_mAms = 0;
uint16_t cap_mAh = 0;
uint16_t cap_min = 0;

	
// Spektrum RC data structures	
volatile spektel_sensor_current_t cur = { 0 };
volatile spektel_sensor_powerbox_t power = { 0 };
volatile spektel_sensor_vario_t vario = { 0 };

static void evsys_init(void) {
	sysclk_enable_module(SYSCLK_PORT_GEN, SYSCLK_EVSYS);
}

static void tc_error_callback(void) {
	ioport_set_pin_level(LED_GREEN, 0);
}

static void tc_overflow_callback(void) {
	tc45_clear_overflow(&TIMER_SENS);
}

/*
static void tc_cca_callback(void) {
} */

static void tc_init(void)
{
	/* Unmask clock for TCC4 */
	tc45_enable(&TIMER_SENS);

	/* Configure TC in normal mode */
	tc45_set_wgm(&TIMER_SENS, TC45_WG_NORMAL);

	/* Configure period*/
	tc45_write_period(&TIMER_SENS, TIMER_SENS_RESOLUTION / TIMER_SENS_PER);

	/* Configure CCA to occur at 250Hz */
	tc45_write_cc(&TIMER_SENS, TC45_CCA, TIMER_SENS_RESOLUTION / TIMER_SENS_PER / 2);

	/* Enable CCA channels */
	tc45_enable_cc_channels(&TIMER_SENS, TC45_CCACOMP);
	
	tc45_set_error_interrupt_callback(&TIMER_SENS, *tc_error_callback);
	tc45_set_overflow_interrupt_callback(&TIMER_SENS, *tc_overflow_callback);
	//tc45_set_cca_interrupt_callback(&TIMER_SENS, *tc_cca_callback);
	
	tc45_set_error_interrupt_level(&TIMER_SENS, TC45_INT_LVL_LO);
	tc45_set_overflow_interrupt_level(&TIMER_SENS, TC45_INT_LVL_LO);
	tc45_set_cca_interrupt_level(&TIMER_SENS, TC45_INT_LVL_LO);
}

static void adc_cur_callback(ADC_t *adc, uint8_t ch_mask, adc_result_t res) {
	cur_mea_val = res;
	act = !act;
	time[act] = rtc_get_time();
	
	measure_cycle = true;	
}

static void adcch_set_cur_measure(void) {
	// Set ADC to ACS current pin and interrupt
	adcch_set_input(&adcch_conf, ADC_MAIN_CURRENT_PIN, ADCCH_NEG_NONE, 1);
	adcch_enable_averaging(&adcch_conf, ADC_CUR_AVERAG_SAMP);
	adcch_set_interrupt_mode(&adcch_conf, ADCCH_MODE_COMPLETE); //complete conversion
	//adcch_conf.intctrl |= 0x10; //set via conf_adc.h to MED
	adcch_enable_interrupt(&adcch_conf);
}

static void adcch_set_volt_measure(void) {
	// Set ADC to ACS current pin and interrupt
	adcch_set_input(&adcch_conf, ADC_MAIN_VOLTAGE_PIN, ADCCH_NEG_NONE, 1);
	adcch_enable_averaging(&adcch_conf, ADC_VOLT_AVERAG_SAMP);
	//adcch_set_interrupt_mode(&adcch_conf, ADCCH_MODE_COMPLETE); //complete conversion
	//adcch_conf.intctrl |= 0x10; //set via conf_adc.h to MED
	adcch_disable_interrupt(&adcch_conf);
}

static void adc_init(void) {
	static struct adc_config adc_conf;
	adc_read_configuration(&ADC_MAIN, &adc_conf);
	adcch_read_configuration(&ADC_MAIN,ADC_MAIN_CH, &adcch_conf);
	 
	// Set Unsigned mode, 12bit resolution REF Voltage to Vcc / 1.6 = 2.06V
	adc_set_conversion_parameters(&adc_conf, ADC_SIGN_OFF,ADC_RES_MT12 , ADC_REF_VCC);  // ADC_RES_12
	adc_set_conversion_trigger(&adc_conf, ADC_TRIG_EVENT_SYNCSWEEP,1, 3 );
	adc_set_clock_rate(&adc_conf, ADC_CLOCK); // ADC clock 1.8MHz
	adc_set_callback(&ADC_MAIN, *adc_cur_callback);
	
	adcch_set_cur_measure();
	
	adc_write_configuration(&ADC_MAIN, &adc_conf);
	adcch_write_configuration(&ADC_MAIN, ADC_MAIN_CH, &adcch_conf);
	
	adc_enable(&ADC_MAIN);
}

int main (void)
{
	board_init();
	sysclk_init();
	sleepmgr_init();
	rtc_init();
	
	irq_initialize_vectors();
	cpu_irq_enable();
	
	cli();
	
	spektel_init();
	evsys_init();
	tc_init();
	adc_init();
		
	EVSYS.CH3MUX = EVSYS_CHMUX_TCC4_CCA_gc;  //Connect TCC4 Compare interrupt to event channel 3 (used to trigger ADC)
	tc45_set_resolution(&TIMER_SENS, TIMER_SENS_RESOLUTION);
	
	sei();
		
	// set LEDs
	ioport_set_pin_level(LED_GREEN, 1);
	ioport_set_pin_level(LED_RED, 0);
	
	// Initialize capacity output
	power.cap1 = BAT_CAP;
	spektel_write_powerbox_sens(power);
	cap_min = BAT_CAP * 0.2;  //calculate bat_mi alarm level to 20% capacity left
	
	// Initialize floating average filters
	InitFloatAvg(&cur_filter1, ACS_B);
	InitFloatAvg(&cur_filter2, ACS_B);
	InitFloatAvg(&cur_filter3, ACS_B);
	
	// Initialize simple lowpass filter
	init_simple_lowpass(&lowpass_reg, 2);
		
	while(1) {
		
		sleepmgr_enter_sleep(); //sleep up to next interrupt
		
		// Check if this is a wake up from the ADC current measurement. There will be a lot of TWI interrupts
		if(measure_cycle) {
			//filter
			AddToFloatAvg(&cur_filter1, (cur_mea_val));
			AddToFloatAvg(&cur_filter2, GetOutputValue(&cur_filter1));
			AddToFloatAvg(&cur_filter3, GetOutputValue(&cur_filter2));
			cur_adc_res[act] = (GetOutputValue(&cur_filter3));
			
			//filter
			power.volt1 = simple_lowpass(&lowpass_reg, cur_adc_res[act]);
			
			// finish capacity and current calculations
			cap_mAms += calc_cap_mAms( cur_adc_res[!act], cur_adc_res[act], time[!act], time[act] );
			cur.current = calc_mA(cur_adc_res[act]);
			power.cap2 = cur.current * 10;
			power.volt2 = cur_adc_res[act];
			
			//Check for capacity overflow
			if( cap_mAms >= 3600000 ) {
				cap_mAms -= 3600000;  //- one mAh
				cap_mAh++;			  //+ one mAh
				power.cap1--;         //subtract from capacity output
				//Check for alarm: under BAT_MIN
				if( power.cap1 <= cap_min ) {
					power.cap1_alarm = true;
				}
			}
	
			// Set ADC channel to voltage pin measurement
			adcch_set_volt_measure();
			adcch_write_configuration(&ADC_MAIN, ADC_MAIN_CH, &adcch_conf);
			
			// Start ADC measurement
			adc_start_conversion(&ADC_MAIN, ADC_MAIN_CH);
			adc_wait_for_interrupt_flag(&ADC_MAIN, ADC_MAIN_CH);
			power.volt1 = calc_main_mV(adc_get_unsigned_result(&ADC_MAIN, ADC_MAIN_CH));
		
			// ADC channel back to current measurement and enable interrupt
			adcch_set_cur_measure();
			adcch_write_configuration(&ADC_MAIN, ADC_MAIN_CH, &adcch_conf);
	
			measure_cycle = false;
		
			// write the values out
			spektel_write_powerbox_sens(power);
			spektel_write_current_sens(cur);
		
			//ioport_toggle_pin_level(LED_GREEN);
		}	

/* code for Spektrum monitoring via LED
		ioport_set_pin_level(LED_GREEN, !spektel_getStatus());
		r = spektel_getResult();
		if( r == TWIS_RESULT_OK ) {
			ioport_set_pin_level(LED_RED, 0);
		}
		else {
			ioport_set_pin_level(LED_RED, 1);
			if( r != rlast) {
				printf("Result status changed: %x", r);
				rlast = r;
			}
		}
*/		
			
		
	}		
}