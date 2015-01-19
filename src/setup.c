/*
 * setup.c
 *
 * Procedures to setup the sensor
 *
 * Created: 28.12.2014 17:08:07
 *  Author: chris
 */ 

#include <asf.h>
#include "setup.h"

void button_setup(void) {
	uint16_t c = 0;
	ioport_set_pin_level(LED_RED, 1);
	c = button_setup_cap();
	ioport_set_pin_level(LED_RED, 1);
	if(saveCapa(c)) {
		ioport_set_pin_level(LED_RED, 0);
	}
}

uint16_t button_setup_cap(void) {
	//Set the capacity of the main battery
	//Set the hundreds, thousands and ten thousands digit, max 65535mAh
	uint16_t i = 0;
	uint8_t digit = 0;
	uint16_t setup_capa = 0;
	volatile spektel_sensor_powerbox_t s_power = { 0 };
	s_power.cap1 = 0;
	
	for(i = 0; i < 4; i++) {
		_delay_ms(1000);
		ioport_toggle_pin_level(LED_RED);
		ioport_toggle_pin_level(LED_GREEN);
	}	
	
	for(digit = 0; digit < 3; digit++) {
		for(i = 0; i <= 10; i++) {
			if((digit == 2 && i > 5) || i > 9 ) {
				i = 0; }
			s_power.cap1 = i * digits(digit) + setup_capa;
			spektel_write_powerbox_sens(s_power);
			_delay_ms(1000);
			if(ioport_get_pin_level(BUT_1)) {
				ioport_toggle_pin_level(LED_RED);
				setup_capa = i * digits(digit) + setup_capa;
				break;
			}
		}
	}
	return setup_capa;
}

uint16_t digits(uint8_t i) {
	switch(i) {
		case 0: return 100; break;
		case 1: return 1000; break;
		case 2: return 10000; break;
	}
	return 100;
}

void fill_page_with_zeroes(uint8_t (*page)[EEPROM_PAGE_SIZE])
{
	for(int j = 0; j < EEPROM_PAGE_SIZE; j++) {
		(*page)[j] = 0;
	}
}

bool saveCapa(uint16_t capa) {
	uint8_t write_page[EEPROM_PAGE_SIZE];
	uint8_t read_page[EEPROM_PAGE_SIZE];
	
	write_page[0] = (capa >> 8);
	write_page[1] = capa;
	
	fill_page_with_zeroes(&read_page);
	
	nvm_eeprom_load_page_to_buffer(write_page);
	nvm_eeprom_atomic_write_page(CONFIG_PAGE);
	
	if( readCapa() == capa ) {
		return true; }
	else {
		return false; }
}

uint16_t readCapa(void) {
	uint8_t read_page[EEPROM_PAGE_SIZE];
	uint16_t r_capa = 0;
	
	nvm_eeprom_read_buffer(CONFIG_ADDR, read_page, EEPROM_PAGE_SIZE);
	r_capa = read_page[0] << 8;
	r_capa = r_capa | read_page[1];
	return r_capa;
}

bool saveCalibration(uint16_t _adc_b, uint16_t _adc_usig_base) {
	uint8_t write_page[EEPROM_PAGE_SIZE];
	uint8_t read_page[EEPROM_PAGE_SIZE];
	
	write_page[0] = (_adc_b >> 8);
	write_page[1] = _adc_b;
	write_page[2] = (_adc_usig_base >> 8);
	write_page[3] = _adc_usig_base;
	
	fill_page_with_zeroes(&read_page);
	
	nvm_eeprom_load_page_to_buffer(write_page);
	nvm_eeprom_atomic_write_page(CALIBRATION_PAGE);
	
	if( (read_adc_b() == _adc_b) && (read_adc_usig_base() == _adc_usig_base) ){
		return true; }
	else {
		return false; }
}

uint16_t read_adc_b(void) {
	uint8_t read_page[EEPROM_PAGE_SIZE];
	uint16_t r_calibration = 0;
	
	nvm_eeprom_read_buffer(CALIBRATION_ADDR, read_page, EEPROM_PAGE_SIZE);
	r_calibration = read_page[0] << 8;
	r_calibration |= read_page[1];
	return r_calibration;
}

uint16_t read_adc_usig_base(void) {
	uint8_t read_page[EEPROM_PAGE_SIZE];
	uint16_t r_calibration = 0;
	
	nvm_eeprom_read_buffer(CALIBRATION_ADDR, read_page, EEPROM_PAGE_SIZE);
	r_calibration = read_page[2] << 8;
	r_calibration |= read_page[3];
	return r_calibration;
}


