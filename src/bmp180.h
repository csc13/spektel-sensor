/*
 * bmp180.h
 *
 * Library for Bosch BMP180 pressure sensor
 * See the datasheet for the details
 *
 * Created: 03.01.2015
 *  Author: chris
 *
 *   
*****************************************************************/

#ifndef BMP180
#define BMP180

#include <asf.h>
#include "twim.h"
#include "conf_bmp180.h"

#define BMP180_ADDR_READ 0xEF
#define BMP180_ADDR_WRITE 0xEE

#define BMP180_REG_AC1 0xAA
#define BMP180_REG_AC2 0xAC
#define BMP180_REG_AC3 0xAE
#define BMP180_REG_AC4 0xB0
#define BMP180_REG_AC5 0xB2
#define BMP180_REG_AC6 0xB4
#define BMP180_REG_B1  0xB6
#define BMP180_REG_B2  0xB8
#define BMP180_REG_MB  0xBA
#define BMP180_REG_MC  0xBC
#define BMP180_REG_MD  0xBE

#define BMP180_REG_ID 0xd0
#define BMP180_REG_CTRL 0xf4
#define BMP180_REG_ADC 0xf6
#define BMP180_REG_ADCMSB 0xf6
#define BMP180_REG_ADCLSB 0xf7
#define BMP180_REG_ADC_XLSB 0xf8

#define BMP180_RES_LOW 0
#define BMP180_RES_STD 1
#define BMP180_RES_HIGH 2
#define BMP180_RES_ULTRAHIGH 3

#define BMP180_PRESSURE_SEELEVEL 101325 //average pressure on see level

struct bmp180_t {
	// ID of the device, alway 0x55. Used for testing the connection
	uint8_t ID;
	
	// Resolution of the measurements
	uint8_t OSS;

	// Calibration values as by the Bosch datasheet
	int16_t AC1;
	int16_t AC2;
	int16_t AC3;
	uint16_t AC4;
	uint16_t AC5;
	uint16_t AC6;
	int16_t B1;
	int16_t B2;
	int16_t MB;
	int16_t MC;
	int16_t MD;

	// Temp value needed between calculations
	int32_t B5;	

	// Result values as by datasheet
	int32_t UT;
	int32_t UP;
	int32_t T;			//Temperatur in 0.1C
	int32_t P;			//Pressure in 0.01mbar
	int32_t P0;			//Pressure at start (airfield) level in 0.01mbar
	int16_t altitude;	//Altitude in 0.1m
	int16_t altitude0;	//Altitude of start/airfield level in 0.1m
};

/**
 * \brief Read calibration coefficients from Bosch BMP180 pressure sensor
 *
 * \return status
 */
uint8_t read_calibration_data(void);

/**
 * \brief Initialize I2C/TWI connection to Bosch BMP180 pressure sensor
 *
 * \return status
 */
uint8_t bmp180_init(void);

/** Start temperature measurement 
 *  After starting the measurement operation wait for 4.5ms to read
 *  See datasheet for more details.
 *
 * \return status
 */
uint8_t bmp180_start_temperature_measurement(void);

/** Read and calculate temperature measurement and save to BMP->T in 0.1C
 *  After starting the measurement operation wait for 4.5ms to read
 *  See datasheet for more details.
 *
 * \return status
 */
uint8_t bmp180_calc_temperature(void);

/**
 * \brief Return temperature
 *
 * \return temperature
 */
uint32_t bmp180_get_temperature(void);

/**
 * \brief Start a pressure measurement on the Bosch BMP180 pressure sensor in 0.01mbar
 *        A temperature measurement is required before
 *
 * \return status
 */
uint8_t bmp180_start_pressure_measurement(void);

/**
 * \brief Read and calculate the pressure, etc. from the Bosch BMP180 pressure sensor in 0.01mbar
 *        A temperature measurement is required before
 *
 * \return status
 */
uint8_t bmp180_calc_pressure(void);

/**
 * \brief Calculate altitude in 0.1m based on start/airfield level
 *
 * \return altitude in 0.1m relative to start level
 */
int16_t calc_altitude(int32_t _p);

/**
 * \brief Get the bmp180 calculation data structure
 *
 * \return bmp180 data structure
 */
struct bmp180_t *get_bmp180(void);

/**
 * \brief Return pressure
 *
 * \return pressure
 */
int32_t get_pressure(void);

/**
 * \brief Set pressure at base (airfield) level
 *
 * \param _p pressure
 */
void set_base_pressure(uint32_t _p);

#endif
