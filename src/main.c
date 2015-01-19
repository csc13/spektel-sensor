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
#include <stdio.h>

#include "spektel.h"
#include "bmp180.h"
#include "floating_average.h"
#include "filter.h"
#include "filter_32.h"
#include "setup.h"
#include "calc_measure.h"

// ADC measurement data structures
volatile bool measure_cycle = false;
struct adc_channel_config adcch_conf;
uint16_t cur_mea_val = 0; //latest measurement
uint16_t vol_mea_val = 0; //latest measurement

// floating average data structures
tFloatAvgFilter cur_filter1 = { {0}, 0 };  //filter level 1
tFloatAvgFilter cur_filter2 = { {0}, 0 };  //filter level 2
tFloatAvgFilter cur_filter3 = { {0}, 0 };  //filter level 3
	
//simple lowpass filter	for voltage
tSimpleLowpassReg lowpass_reg_volt = { 0, 0 };
	
//simple lowpass filter for pressure	
tSimpleLowpassReg32 lowpass_reg_prs = { 0, 0 };
bool startup = true;
uint8_t sample_count = 0; //Counts samples in cycles of 255 for temperature measurements	
		
// variables for capacity calculation
uint16_t cur_adc_res[2] = { ADC_B, ADC_B }; // This is power = 0
uint32_t time[2] = { 0 };
bool act = 0;
uint32_t cap_mAms = 0;
uint16_t cap_mAh = 0;
uint16_t cap_min = 0;

// variables for climb rate calculation
int32_t cl_prs[2] = {0, 0};
int32_t alt_dif = 0;	
	
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

	/* Configure CCA to occur at half the period */
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

static uint16_t adcch_get_pin_measure(enum adcch_positive_input pin, enum adcch_sampnum sample) {
	// Set ADC channel to voltage pin measurement
	// Set ADC to ACS current pin and interrupt
	adcch_set_input(&adcch_conf, pin, ADCCH_NEG_NONE, 1);
	adcch_enable_averaging(&adcch_conf, sample);
	//adcch_set_interrupt_mode(&adcch_conf, ADCCH_MODE_COMPLETE); //complete conversion
	//adcch_conf.intctrl |= 0x10; //set via conf_adc.h to MED
	adcch_disable_interrupt(&adcch_conf);
	adcch_write_configuration(&ADC_MAIN, ADC_MAIN_CH, &adcch_conf);

	// Start ADC measurement
	adc_start_conversion(&ADC_MAIN, ADC_MAIN_CH);
	adc_wait_for_interrupt_flag(&ADC_MAIN, ADC_MAIN_CH);
	
	return adc_get_unsigned_result(&ADC_MAIN, ADC_MAIN_CH);	
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

static void calibration(void) {
	// Initialize floating average filters
	InitFloatAvg(&cur_filter1,  adcch_get_pin_measure(ADC_MAIN_VOLTAGE_PIN, ADC_VOLT_AVERAG_SAMP));   // Read the value for 0V voltage
	//InitFloatAvg(&cur_filter2, adcch_get_pin_measure(ADC_MAIN_REF_VOLTAGE_PIN, ADC_REF_AVERAG_SAMP)); // Read the value for 2,048V voltage
	InitFloatAvg(&cur_filter3, adcch_get_pin_measure(ADC_MAIN_CURRENT_PIN, ADC_CUR_AVERAG_SAMP));     // Read the value for 0A current
	
	for(uint8_t i = 0; i < 40 ; i++) {
		AddToFloatAvg(&cur_filter1, adcch_get_pin_measure(ADC_MAIN_VOLTAGE_PIN, ADC_VOLT_AVERAG_SAMP)); // Read the value for 0V voltage
		//AddToFloatAvg(&cur_filter2, adcch_get_pin_measure(ADC_MAIN_REF_VOLTAGE_PIN, ADC_REF_AVERAG_SAMP)); // Read the value for 2,048V voltage
		AddToFloatAvg(&cur_filter3, adcch_get_pin_measure(ADC_MAIN_CURRENT_PIN, ADC_CUR_AVERAG_SAMP)); // Read the value for 0A current
	}
	
	uint16_t volt_zero = GetOutputValue(&cur_filter1);   // value for 0V voltage
	//uint16_t volt_ref = GetOutputValue(&cur_filter2);    // value for 2,048V voltage
	uint16_t ACS758_zero = GetOutputValue(&cur_filter3); // value for 0A current
	
	//double lsb_res = 2.048D / (double)(volt_ref - volt_zero);	
	saveCalibration(ACS758_zero + 1, volt_zero);
}


static void plotter_init(void) {
	#if ARDUPLOT
		printf("\nd %u %u %u %u %u %u %d %d\n", cur.current, power.volt1, power.volt2, power.cap1, power.cap2, cur_mea_val, vario.altitude, vario.climb_rate);
		printf("n current volt1 volt2 cap1 cap2 measure, altitude, climbrt\n");
		printf("r current 0 5000 volt1 0 2600 volt2 0 2600 cap1 0 3000 cap2 0 55000 measure 0 4096 altitude -2000 2000 climbrt -200 200\n");
		printf("c current 255 255 255 volt1 0 0 255 volt2 0 255 0 cap1 255 0 0 cap2 0 255 255 measure 150 150 0 altitude 255 150 150 climbrt 150 255 150\n");
	#endif
}

static void writeToPlotter(void) {
	#if ARDUPLOT
		printf("d %u %u %u %u %u %u %d %d\n", cur.current, power.volt1, power.volt2, power.cap1, power.cap2, cur_mea_val, vario.altitude, vario.climb_rate);
	#elif REALTIME_PLOTTER
		printf("%u %u %u %u %u %u %d %d\r", cur.current, power.volt1, power.volt2, power.cap1, power.cap2, cur_mea_val, vario.altitude, vario.climb_rate);
	#endif
}

int main (void)
{
	sysclk_init();
	board_init();
	sleepmgr_init();
	rtc_init();
	
	cli();
	
	spektel_init();
			
	evsys_init();
	tc_init();
	adc_init();
		
	EVSYS.CH3MUX = EVSYS_CHMUX_TCC4_CCA_gc;  //Connect TCC4 Compare interrupt to event channel 3 (used to trigger ADC)
	tc45_set_resolution(&TIMER_SENS, TIMER_SENS_RESOLUTION);

	sei();
	
	irq_initialize_vectors();
	cpu_irq_enable();
	
	// Initialize BMP180 module
	bmp180_init();
		
	// set LEDs
	ioport_set_pin_level(LED_GREEN, 1);
	ioport_set_pin_level(LED_RED, 0);
	
	// If button is pressed on startup enter setup mode
	 if( ioport_get_pin_level(BUT_1) == true ) { 
		 button_setup();
		 calibration();
	 }
	 
	 // Read calibration values
	 uint16_t _adc_b = read_adc_b();
	 uint16_t _adc_usig_base = read_adc_usig_base();
	 
	 if(_adc_b > 400 && _adc_b < 800 && _adc_usig_base > 80 && _adc_usig_base < 300) {
		init_calc(_adc_b, _adc_usig_base);
	 }
	
	// Initialize capacity output
	power.cap1 = readCapa();
	if(power.cap1 == 0 || power.cap1 == 0xFFFF) {
		power.cap1 = BAT_CAP; }
	
	spektel_write_powerbox_sens(power);
	cap_min = BAT_CAP * 0.2;  //calculate bat_mi alarm level to 20% capacity left
	
	// Initialize floating average filters
	InitFloatAvg(&cur_filter1, ACS_B);
	InitFloatAvg(&cur_filter2, ACS_B);
	InitFloatAvg(&cur_filter3, ACS_B);
	
	// Initialize simple lowpass filter for voltage
	init_simple_lowpass(&lowpass_reg_volt, 4);
	
	// Initialize simple lowpass filter for altitude
	init_simple_lowpass_32(&lowpass_reg_prs, 5);
	
	//Start first pressure measurement
	bmp180_start_pressure_measurement();
	
	// Initialize plotter
	plotter_init();

	while(1) {
				
		sleepmgr_enter_sleep(); //sleep up to next interrupt
		
		// Check if this is a wake up from the ADC current measurement. There will be a lot of TWI interrupts
		if(measure_cycle) {
			//filter
			AddToFloatAvg(&cur_filter1, (cur_mea_val));
			AddToFloatAvg(&cur_filter2, GetOutputValue(&cur_filter1));
			AddToFloatAvg(&cur_filter3, GetOutputValue(&cur_filter2));
			cur_adc_res[act] = (GetOutputValue(&cur_filter3));	
			
			// finish capacity and current calculations
			cap_mAms += calc_cap_mAms( cur_adc_res[!act], cur_adc_res[act], time[!act], time[act] );
			cur.current = calc_mA(cur_adc_res[act]);
			power.cap2 = cur.current * 10;
			//power.volt2 = cur_adc_res[act];
			
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

			// Get main battery voltage pin measure
			power.volt1 = calc_main_mV(simple_lowpass(&lowpass_reg_volt, adcch_get_pin_measure(ADC_MAIN_VOLTAGE_PIN, ADC_VOLT_AVERAG_SAMP)));
			
			// ADC channel back to current measurement and enable interrupt
			adcch_set_cur_measure();
			adcch_write_configuration(&ADC_MAIN, ADC_MAIN_CH, &adcch_conf);
			
			// The BMP180 requires measuring and reading the actual temperature from the device for temp. drift compensation
			// So we do a temperature reading every 32nd measurement. We use a rolling counter
			if((sample_count & 0x3F) == 0x3F) {
				bmp180_calc_temperature();
			}
			else {
				// Read and calculate pressure from BMP180
				bmp180_calc_pressure();  //Read and calculate last measurement from BMP180
				cl_prs[act] = simple_lowpass_32(&lowpass_reg_prs, get_pressure()); //Use a low pass for the pressure
				vario.altitude = calc_altitude(cl_prs[act]);
				
				if((sample_count == 0xFF) && startup) {
					set_base_pressure(cl_prs[act]);
					startup = false;
				}
				
				// Climb rate is
				// (Actual hight - Last hight) / time in seconds
				/*alt_dif = ((int32_t)(cl_prs[act] - cl_prs[!act])) * 1000;
				if( alt_dif != 0 ) {
					alt_dif = alt_dif / ((int32_t)(time[act] - time[!act]));
					vario.climb_rate = simple_lowpass(&lowpass_reg_clbrt, (int16_t)alt_dif );
				}*/
			}
			
			
			sample_count++; //overrun is wanted	
			// Trigger the next measurement of the BMP180
			if((sample_count & 0x3F) == 0x3F) {
				bmp180_start_temperature_measurement();
			}
			else {
				bmp180_start_pressure_measurement(); //Since the measure cycle is triggered every 40ms its enough time for the BMP180 in ultrahigh precision mode
			}
					
			measure_cycle = false;
		
			// write the values out
			spektel_write_powerbox_sens(power);
			spektel_write_current_sens(cur);
			spektel_write_vario_sens(vario);
			
			 writeToPlotter();
		}	
		
			 if( ioport_get_pin_level(BUT_1) == true ) {
				 set_base_pressure(cl_prs[act]);
			 }
	}
	
/* code for Spektrum XBus monitoring via LED
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
		
	}		*/
}