/*
 * setup.h
 *
 * Created: 28.12.2014 17:08:28
 *  Author: chris
 */ 


#ifndef SETUP_H_
#define SETUP_H_

#include <spektel.h>
#include <util/delay.h>

/**
 * \brief Setup the sensor using the hardware button on the sensor and the Spektrum transmitter display
 *
 */
void button_setup(void);

/**
 * \brief Setup the sensor using the hardware button on the sensor and the Spektrum transmitter display
 *
 */
uint16_t button_setup_cap(void);

uint16_t digits(uint8_t i);

void fill_page_with_zeroes(uint8_t (*page)[EEPROM_PAGE_SIZE]);

bool saveCapa(uint16_t capa);

uint16_t readCapa(void);

bool saveCalibration(uint16_t _adc_b, uint16_t _adc_usig_base);

uint16_t read_adc_b(void);

uint16_t read_adc_usig_base(void);

#endif /* SETUP_H_ */