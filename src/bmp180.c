/* 
 * bmp180.c
 */

#include <asf.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include "bmp180.h"

// This is the master structure for the BMP180 calculation
struct bmp180_t *bmp180;

/** Write a byte
 *
 * \param reg_addr the register address.
 * \param byte the data to be written.
 *
 * \return status of operation
 */
static uint8_t write_byte(const uint8_t reg_addr, uint8_t byte)
{
	twi_package_t w_packet = {
		.addr		 = { reg_addr },
		.addr_length = sizeof(reg_addr),
		.chip        = BMP180_ADDR_WRITE >> 1,
		.buffer      = &byte,
		.length      = sizeof(uint8_t),
		.no_wait     = false
	};

	return twi_master_write(&TWI_MASTER, &w_packet);
}

/** Read a byte
 *
 * \param reg_addr the register address.
 * \param byte the data to be read.
 *
 * \return status of read operation
 */
static uint8_t read_byte(const uint8_t reg_addr, uint8_t *byte)
{
	twi_package_t r_packet = {
		.addr		 = { reg_addr },
		.addr_length = 1,
		.chip        = BMP180_ADDR_READ >> 1,
		.buffer      = byte,
		.length      = 1,
		.no_wait     = false
	};
	return twi_master_read(&TWI_MASTER, &r_packet);
}

/** Read word (16bit)
 *
 * \param reg_addr the register address.
 * \param word the data to be read.
 *
 * \return status of operation
 */
static uint8_t read_word(const uint8_t reg_addr, uint16_t *word)
{
	uint8_t error;
	
	twi_package_t r_packet = {
		.addr		 = { reg_addr },
		.addr_length = sizeof(reg_addr),
		.chip        = BMP180_ADDR_READ >> 1,
		.buffer      = word,
		.length      = sizeof(uint16_t),
		.no_wait     = false
	};
	error = twi_master_read(&TWI_MASTER, &r_packet);
	
	uint8_t lsb = (*word) >> 8;
	(*word) = ((*word) << 8) | lsb;
	
	return error;
}

/** Read the calibration data.
 *
 * \bmp180 structure with calibration data
 */
uint8_t read_calibration_data()
{
	uint8_t error;
	uint16_t word;

	error = read_word(BMP180_REG_AC1, &word);
	bmp180->AC1 = (int16_t) word;

	if (!error) {
		error = read_word(BMP180_REG_AC2, &word);
		bmp180->AC2 = (int16_t) word;
	}
	
	if (!error) {
		error = read_word(BMP180_REG_AC3, &word);
		bmp180->AC3 = (int16_t) word;
	}
	
	if (!error)
		error = read_word(BMP180_REG_AC4, &bmp180->AC4);
		
	if (!error)
		error = read_word(BMP180_REG_AC5, &bmp180->AC5);
		
	if (!error)
		error = read_word(BMP180_REG_AC6, &bmp180->AC6);
		
	if (!error) {
		error = read_word(BMP180_REG_B1, &word);
		bmp180->B1 = (int16_t) word;
	}
	if (!error) {
		error = read_word(BMP180_REG_B2, &word);
		bmp180->B2 = (int16_t) word;
	}
	
	if (!error) {
		error = read_word(BMP180_REG_MB, &word);
		bmp180->MB = (int16_t) word;
	}
	if (!error) {
		error = read_word(BMP180_REG_MC, &word);
		bmp180->MC = (int16_t) word;
	}
	
	if (!error) {
		error = read_word(BMP180_REG_MD, &word);
		bmp180->MD = (int16_t) word;
	}

	return(error);
}


/** Start temperature measurement 
 *  After starting the measurement operation wait for 4.5ms to read
 *  See datasheet for more details.
 */
uint8_t bmp180_start_temperature_measurement()
{
	return write_byte(BMP180_REG_CTRL, 0x2e);
}

/** Read and calculate temperature measurement and save to BMP->T in 0.1C
 *  After starting the measurement operation wait for 4.5ms to read
 *  See datasheet for more details.
 */
uint8_t bmp180_calc_temperature()
{
	uint8_t error;
	uint16_t word;

	error = read_word(BMP180_REG_ADC, &word);
	bmp180->UT = (long)word;

	if (!error) {
		int32_t const x1 = ((bmp180->UT - bmp180->AC6) * bmp180->AC5) >> 15;
		int32_t const x2 = ((int32_t)bmp180->MC << 11) / (x1 + bmp180->MD);
		bmp180->B5 = (x1 + x2);
		bmp180->T = (bmp180->B5 + 8) >> 4;
	}

	return (error);
}

uint32_t bmp180_get_temperature() {
	return bmp180->T;
}

uint8_t bmp180_start_pressure_measurement()
{
	uint8_t error = 0;

	// Start pressure measurement
	error = write_byte(BMP180_REG_CTRL, (0x34 + (bmp180->OSS << 6)));
	
	// Wait based on the oversampling (see datasheet)
	if (!error) {
		switch (bmp180->OSS) {
			case BMP180_RES_LOW:
			return 5;
			break;
			case BMP180_RES_STD:
			return 8;
			break;
			case BMP180_RES_HIGH:
			return 14;
			break;
			case BMP180_RES_ULTRAHIGH:
			return 26;
			break;
			default:
			return 5;
		}
	}
	
	return error;
}

uint8_t bmp180_calc_pressure()
{
	uint8_t error = 0, byte;
	uint16_t word;

	error = read_word(BMP180_REG_ADC, &word);
	bmp180->UP = word;
	bmp180->UP <<= 8;

	if (!error) {
		error = read_byte(BMP180_REG_ADC_XLSB, &byte);
		bmp180->UP |= byte;
		bmp180->UP >>= (8 - bmp180->OSS);
	}

	if (!error) {
		int32_t const b6 = bmp180->B5 - 4000L;
		int32_t x1 = (bmp180->B2 * ((b6 * b6) >> 12)) >> 11;
		int32_t x2 = (bmp180->AC2 * b6) >> 11;
		int32_t x3 = x1 + x2;

		int32_t const b3 = ((((int32_t)bmp180->AC1 * 4 + x3) << bmp180->OSS) + 2) >> 2;
		
		x1 = (bmp180->AC3 * b6) >> 13;
		x2 = (bmp180->B1 * ((b6 * b6) >> 12)) >> 16;
		x3 = ((x1 + x2) + 2) >> 2;
		uint32_t const b4 = (bmp180->AC4 * (uint32_t)(x3 + 32768L)) >> 15;
		uint32_t const b7 = (uint32_t)(bmp180->UP - b3) * (50000L >> bmp180->OSS);

		if (b7 < 0x80000000UL)
		bmp180->P = (b7 << 1) / b4;
		else
		bmp180->P = (b7 / b4) << 1;

		x1 = (bmp180->P >> 8) * (bmp180->P >> 8);
		x1 = (x1 * 3038) >> 16;
		x2 = (-7357L * bmp180->P) >> 16;
		bmp180->P += ((x1 + x2 + 3791L) >> 4);
	}
	return(error);
}

/** Init
 */
uint8_t bmp180_init()
{
	uint8_t error;
	// Allocate memory for the main BMP180 calculation data structure 
	bmp180 = malloc(sizeof(struct bmp180_t));
	bmp180->altitude0 = BMP180_START_ALTITUDE; //Set start altitude
	bmp180->altitude =  BMP180_START_ALTITUDE;
	bmp180->P0 = BMP180_PRESSURE_SEELEVEL;
	
	error = read_byte(BMP180_REG_ID, &bmp180->ID);

	if (!error && (bmp180->ID == 0x55)) {
		error = write_byte(BMP180_REG_CTRL, ((uint8_t)BMP180_RESOLUTION) << 6);
		
		error = read_byte(BMP180_REG_CTRL, &bmp180->OSS);
		bmp180->OSS >>= 6;

		if (!error) {
			error = read_calibration_data();}
	}
	
	//Now start the initial temperature reading, since this is needed
	bmp180_start_temperature_measurement();
	_delay_ms(5);
	bmp180_calc_temperature();

	return (error);
}

struct bmp180_t *get_bmp180() {
	return bmp180;
}

int16_t calc_altitude(int32_t _p) {
	// Given a pressure measurement P (mb) and the pressure at a baseline P0 (mb),
	// return altitude (in 0.1m) relative to the start altitude.
	//return (bmp180->P0 - _p); //*0.843
	
	float altitude;

	float pressure = (float)_p;
	float baselevelPressure = bmp180->P0;
	
	altitude = 443300 * (1.0 - pow(pressure / baselevelPressure,0.1903));

	return altitude;	
}

int32_t get_pressure() {
	return bmp180->P;
}

void set_base_pressure(uint32_t _p) {
	bmp180->P0 = _p;
}