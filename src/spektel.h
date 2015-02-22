/*
 * spektel.h
 *
 * Library for Spektrum RC sensor protocol via TM1000
 * Thanks to mukenukem and many others for re-engineering and informations
 *
 * Created: 03.10.2014 11:50:26
 *  Author: chris
 *
 * 
 *************************************************************** 
 *  The Spektrum RC TM1000 X-Bus is a 100 kHz I2C Bus
 *  
 *  The Pins [ SDA | SCL | Ubatt | GND ]
 *  Power: Ubatt comes from the receiver (e.g. 5V to 6V or more) SCA and SCL need to be operated at 3.3V
 *  Startup sequence: 70ms after the power is stable, the telemetry module enumerates the bus. 
 *                    It reads each address (0x01 to 0x7D) twice every 13ms. If no answer it will not ask again.
 *                    The enumeration takes around 3.4 seconds
 *                    
 *  After startup: The polls the sensors every 94ms
 *  Data: Every sensor must transfer 16 bytes data. First byte must be the address, 
 *        the second must be 0x00 and then 14 bytes of data.
 *  Addresses: 0x01 and 0x02 must not be used      
*****************************************************************/


#ifndef SPEKTEL_H_
#define SPEKTEL_H_

#include "twis.h"
#include "conf_spektel.h"

// Spektrum TM1000 Sensor Addresses
#define VOLTAGE_SENS 0x01  // don't use
#define TEMP_SENS 0x02 // don't use
#define CURRENT_SENS 0x03
#define POWERBOX_SENS 0x0a
#define AIRSPEED_SENS 0x11
#define ALTITUDE_SENS 0x12
#define GFORCE_SENS 0x14
#define JETCAT_SENS 0x15
#define GPS1_SENS 0x16
#define GPS2_SENS 0x17  // Second half of GPS signal
#define RX_CAP_SENS 0x18
#define ESC_SENS 0x20
#define FLIGHT_CAP_SENS 0x34 //Flight pack cap
#define VARIO_SENS 0x40

#define DATA_LENGTH     16

typedef struct spektel_sensor_data_t {
	uint8_t byte[16]; //16 byte sensor data
} spektel_sensor_data_t;

/*! Spektrum Telemetry options
typedef struct spektel_options_t {
	//! define used telemetry
	bool current_tel;
	bool powerbox_tel;
	bool airspeed_tel;
	bool altitude_tel;
	bool gforce_tel;
	bool jetcat_tel;
	bool gps_tel;
	bool vario_tel;
	
	//! Interrupt level
	spektel_int_level_t int_lvl;
	
} spektel_sensor_current_t; */

//! SPEKTEL interrupt levels
enum spektel_int_level_t {
	SPEKTEL_INT_LVL_OFF = 0x00,
	SPEKTEL_INT_LVL_LO = 0x01,
	SPEKTEL_INT_LVL_MED = 0x02,
	SPEKTEL_INT_LVL_HI = 0x03,
};


//! Structures for sensors

//! Current sensor
typedef struct spektel_sensor_current_t {
	//! Set current in 10mA
	uint16_t current;

} spektel_sensor_current_t;

//! Powerbox sensor
typedef struct spektel_sensor_powerbox_t {
  // voltage in 10 mV
  // capacity in 1 mAh
  
  //! Set Volt1 in 10 mV
  uint16_t volt1;
  
  //! Set Volt2 in 10 mV
  uint16_t volt2;
	
  //! Set Cpacity1 in 1 mAh
  uint16_t cap1;
	  
  //! Set Capacity1 in 1 mAh
  uint16_t cap2;
		
  //! Set Volt1 Alarm
  bool volt1_alarm;
  
  //! Set Volt2 Alarm
  bool volt2_alarm;
  
  //! Set Cpacity1 Alarm
  bool cap1_alarm;
  
  //! Set Capacity2 Alarm
  bool cap2_alarm;

} spektel_sensor_powerbox_t;


//! Vario sensor
typedef struct spektel_sensor_vario_t {
 // altitude in 0.1m
/*

 40,00,altLSB,altMSB,250MSB,250LSB,500MSB,500LSB,10 00MSB,1000LSB,1500MSB,1500LSB,2000MSB,2000LSB,3000 MSB,3000LSB

 alt is altitude in 0.1m
 250, 500, etc is altitude change rate in 0.1m, during the last 250ms, 500ms, 1s, 1.5s, 2s, 3s. So filtering is done in the sensor. 
 If you only have one value, either use only 250ms or transmit it for all rates. If you transmit it for the 250ms rate, you have to 
 set the radio also to 250ms in order to get vario tones.

 All altitude variables are Int16
*/ 
   //! Set Altitude in 0.1m
   int16_t altitude;
   
   //! Set Climb Rate average in 0.1m/s by last 250ms
   int16_t climb_rate_250ms;
   
   //! Set Climb Rate average in 0.1m/s by last 500ms
   int16_t climb_rate_500ms;
	
   //! Set Climb Rate average in 0.1m/s by last 1s
   int16_t climb_rate_1s;	
   
   //! Set Climb Rate average in 0.1m/s by last 1.5s
   int16_t climb_rate_1_5s;     

   //! Set Climb Rate average in 0.1m/s by last 2s
   int16_t climb_rate_2s;
	
   //! Set Climb Rate average in 0.1m/s by last 3s
   int16_t climb_rate_3s;	  	  
   	
} spektel_sensor_vario_t;

typedef struct spektel_sensor_flight_cap_t {
	//! Set current in 0.1A
	uint16_t current;
	
	//! Set capacity in mAh
	uint16_t cap;
	
	//! Set temp in 0.1C
	int16_t temp;

} spektel_sensor_flight_cap_t;

//! Functions

/**
 * \brief Write values for the current sensor to the Spektrum TM1000 X-Bus connection
 *		  The values get transmitted in one of the next times, the TM1000 polls for data	
 *
 * \param current Spektel current sensor structure
 */
void spektel_write_current_sens(spektel_sensor_current_t current);

/**
 * \brief Write values for the Powerbox sensor to the Spektrum TM1000 X-Bus connection
 *		  The values get transmitted in one of the next times, the TM1000 polls for data	
 *
 * \param current Spektel Powerbox sensor structure
 */
void spektel_write_powerbox_sens(spektel_sensor_powerbox_t powerbox);

/**
 * \brief Write values for the Vario sensor to the Spektrum TM1000 X-Bus connection
 *		  The values get transmitted in one of the next times, the TM1000 polls for data	
 *
 * \param current Spektel Vario sensor structure
 */
void spektel_write_vario_sens(spektel_sensor_vario_t vario);

/**
 * \brief Write values for the flight pack capacity sensor to the Spektrum TM1000 X-Bus connection
 *		  The values get transmitted in one of the next times, the TM1000 polls for data	
 *
 * \param current Spektel flight pack capacity sensor structure
 */
void spektel_write_flight_cap_sens(spektel_sensor_flight_cap_t flight_cap);

/**
 * \brief Write the values of a sensor to the (I2C/TWI) X-Bus. Since the TM1000 doesn't match
 *        the requested address to the delivered address this function implements a round robin.	
 */
void spektel_write_sensor_data(void);

/**
 * \brief Initialize TI/I2C and connection to Spektrum RC TM1000
 *
 * \param twis The TWI module.
 * \param opt Spektel options.
 *
 * \return status
 */
bool spektel_init(void);// TWI_t *twis, const spektel_options_t *opt)

/**
* \brief Get TWIS result
*
* \return TWIS result
*/
register8_t spektel_getResult(void);

/**
* \brief Get TWIS status
*
* \return TWIS status
*/
register8_t spektel_getStatus(void);

/**
 * \brief Set data to send
 *
 * \param dat Data to send.
 */
void spektel_setData(spektel_sensor_data_t dat);

/**
 * \brief Set TWIS interrupt level.
 *
 * Sets the interrupt level on TWIS interrupt.
 *
 * \param twis Pointer to the TWI Slave module.
 * \param level Interrupt level of the TXD interrupt.
 */
static inline void spektel_set_interrupt_level(TWI_t *twis) /*	,enum spektel_int_level_t level) */
{
	return;
}


/*
Data type = 0x16 GPS Sensor (always second GPS packet)

0[00] 22(0x16)
1[01] 00
2[02] Altitude LSB (Decimal) //In 0.1m
3[03] Altitude MSB (Decimal) //Altitude = Altitude(0x17) * 10000 + Value (in 0.1m)
4[04] 1/10000 degree minutes latitude (Decimal) (DD MM.MMMM)
5[05] 1/100 degree minutes latitude (Decimal)
6[06] degree minutes latitude (Decimal)
7[07] degrees latitude (Decimal)
8[08] 1/100th of a degree second longitude (Decimal) (DD MM.MMMM)
9[09] degree seconds longitude (Decimal)
10[0A] degree minutes longitude (Decimal)
11[0B] degrees longitude (Decimal)
12[0C] Heading LSB (Decimal)
13[0D] Heading MSB (Decimal) Divide by 10 for Degrees
14[0E] Unknown (Decimal)
15[0F] First bit for latitude: 1=N(+), 0=S(-);
Second bit for longitude: 1=E(+), 0=W(-);
Third bit for longitude over 99 degrees: 1=+-100 degrees



Data type = 0x17 GPS Sensor (always first GPS packet)

0[00] 23(0x17)
1[01] 00
2[02] Speed LSB (Decimal)
3[03] Speed MSB (Decimal) Divide by 10 for Knots. Multiply by 0.185 for Kph and 0.115 for Mph
4[04] UTC Time LSB (Decimal) 1/100th sec. (HH:MM:SS.SS)
5[05] UTC Time (Decimal) = SS
6[06] UTC Time (Decimal) = MM
7[07] UTC Time MSB (Decimal) = HH
8[08] Number of Sats (Decimal)
9[09] Altitude in 1000m (Decimal)
10[0A]-15[0F] Unused (But contains Data left in buffer)
*/

/* New sensors
msg[ 0] = 0x34; // 0x34 Flight Pack Cap  0x18 RX Pack Cap  0x20 ESC
msg[ 1] = 0;
msg[ 2] = 0;    // A*10 low              mA/10 low         RPM/10 high
msg[ 3] = 0;    // A*10 high             mA/10 high        RPM/10 low
msg[ 4] = c;    // mAh low               mAh*10 low        V*100 high
msg[ 5] = c>>8; // mAh high              mAh*10 high       V*100 low
msg[ 6] = 0;    // Temp*10 low           V*100 low         FET Temp*10 low
msg[ 7] = 0;    // Temp*10 high          V*100 high        FET Temp*10 high
msg[ 8] = 0;    //                                         A*100 high
msg[ 9] = 0;    //                                         A*100 low
msg[10] = 0;    //                                         BEC Temp*10 high
msg[11] = 0;    //                                         BEC Temp*10 low
msg[12] = 0;    //                                         BEC A*10
msg[13] = 0;    //                                         BEC V*20
msg[14] = 0;    //                                         Throttle % * 2
msg[15] = 0;    //                                         Output % * 2

*/

#endif /* SPEKTEL_H_ */