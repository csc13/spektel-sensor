/*
* spektel.c
*
* Created: 03.10.2014 12:27:31
*  Author: chris
*/

#include <asf.h>
/*
#include <twim.h>
#include <twis.h>
#include "conf_board.h"
*/

#include "spektel.h"
#include "conf_spektel.h"

TWI_Slave_t slave;

uint8_t toggle = 0;
spektel_sensor_data_t current_data = { {CURRENT_SENS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
spektel_sensor_data_t powerbox_data = { {POWERBOX_SENS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
spektel_sensor_data_t vario_data = { {VARIO_SENS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };
spektel_sensor_data_t flight_cap_data = { {FLIGHT_CAP_SENS, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} };				


void spektel_write_current_sens(spektel_sensor_current_t current) {
	// current in 10mA	
	current.current /= 19.67; // / 19.67 = current in 10mA

	current_data.byte[0] = CURRENT_SENS; // Current
	current_data.byte[2] =  (current.current >> 8); //highByte
	current_data.byte[3] =  current.current; //lowByte
}

void spektel_write_powerbox_sens(spektel_sensor_powerbox_t powerbox) {
  // voltage in 10 mV
  // voltage in 10 mV
  // capacity in 1 mAh
  powerbox_data.byte[0] = POWERBOX_SENS; // Powerbox
  powerbox_data.byte[2] = (powerbox.volt1 >> 8);
  powerbox_data.byte[3] = powerbox.volt1;
  powerbox_data.byte[4] = (powerbox.volt2 >> 8);
  powerbox_data.byte[5] = powerbox.volt2;
  powerbox_data.byte[6] = (powerbox.cap1 >> 8);
  powerbox_data.byte[7] = powerbox.cap1;
  powerbox_data.byte[8] = (powerbox.cap2 >> 8);
  powerbox_data.byte[9] = powerbox.cap2;
  // Alarm last byte bits: volt1, volt2, cap1, cap2
  powerbox_data.byte[15] |= (powerbox.volt1_alarm << 0);
  powerbox_data.byte[15] |= (powerbox.volt2_alarm << 1);
  powerbox_data.byte[15] |= (powerbox.cap1_alarm << 2);
  powerbox_data.byte[15] |= (powerbox.cap2_alarm << 3);	
}

void spektel_write_vario_sens(spektel_sensor_vario_t vario) {
	// altitude in 0.1m
	// climb rates in 0.1 m/s with different averages
	vario_data.byte[0] = VARIO_SENS; // Vario
	vario_data.byte[2] = (vario.altitude >> 8);
	vario_data.byte[3] = vario.altitude;
	vario_data.byte[4] = (vario.climb_rate_250ms >> 8);
	vario_data.byte[5] = vario.climb_rate_250ms;
	vario_data.byte[6] = (vario.climb_rate_500ms >> 8);
	vario_data.byte[7] = vario.climb_rate_500ms;
	vario_data.byte[8] = (vario.climb_rate_1s >> 8);
	vario_data.byte[9] = vario.climb_rate_1s;
	vario_data.byte[10] = (vario.climb_rate_1_5s >> 8);
	vario_data.byte[11] = vario.climb_rate_1_5s;
	vario_data.byte[12] = (vario.climb_rate_2s >> 8);
	vario_data.byte[13] = vario.climb_rate_2s;
	vario_data.byte[14] = (vario.climb_rate_3s >> 8);
	vario_data.byte[15] = vario.climb_rate_3s;		
}

void spektel_write_flight_cap_sens(spektel_sensor_flight_cap_t flight_cap) {
	// current in 0.1A
	// capacity in mAh (used capacity, NOT left battery capacity)
	// temperature in 0.1C
	flight_cap_data.byte[0] = FLIGHT_CAP_SENS; // Flight pack capacity
	flight_cap_data.byte[2] = flight_cap.current;
	flight_cap_data.byte[3] = (flight_cap.current >> 8);
	flight_cap_data.byte[4] = flight_cap.cap;
	flight_cap_data.byte[5] = (flight_cap.cap >> 8);
	flight_cap_data.byte[6] = flight_cap.temp;
	flight_cap_data.byte[7] = (flight_cap.temp >> 8);
}

/**
 * \brief Write the values of a sensor to the (I2C/TWI) X-Bus. Since the TM1000 doesn't match
 *        the requested address to the delivered address this function implements a round robin.	
 */
void spektel_write_sensor_data() {
	uint8_t i = 0;
	if(slave.status == TWIS_STATUS_READY) {
		for(i = 0; i < DATA_LENGTH && i < TWIS_SEND_BUFFER_SIZE; i++) {
			if(toggle == 0) {
				slave.sendData[i] = powerbox_data.byte[i];
			} else if (toggle == 1) {
				slave.sendData[i] = current_data.byte[i];
			} else if (toggle == 2) {
				slave.sendData[i] = vario_data.byte[i];
			} else if (toggle == 3) {
				slave.sendData[i] = flight_cap_data.byte[i];
			}	
		}
	}
	if(++toggle > 3) toggle = 0;
}

register8_t spektel_getResult(void) {
	return slave.result;
}

register8_t spektel_getStatus(void) {
	return slave.status;
}


ISR(TWIC_TWIS_vect) {
	TWI_SlaveInterruptHandler(&slave);
}

static void slave_process(void) {
	spektel_write_sensor_data();
}

bool spektel_init() {
	uint8_t twi_slave_address = 0x00;
	status_code_t status_code = 0x00;
	// calculate slave address for single sensor (two TWI addresses are support by XMEGA)
	if( CURRENT_TEL ) {
		twi_slave_address |= CURRENT_SENS;
	}
	if( POWERBOX_TEL ) {
		twi_slave_address |= POWERBOX_SENS;
	}
	if( VARIO_TEL ) {
		twi_slave_address |= VARIO_SENS;
	}
	if( FLIGHT_CAP_TEL ) {
		twi_slave_address |= FLIGHT_CAP_SENS;
	}

	
	// Initialize ports
	TWI_MASTER_PORT.PIN0CTRL = PORT_OPC_WIREDAND_gc;
	TWI_MASTER_PORT.PIN1CTRL = PORT_OPC_WIREDAND_gc;

	irq_initialize_vectors();

	sysclk_enable_peripheral_clock(&TWI_MASTER);

	twi_bridge_enable(&TWI_MASTER);
	//twi_fast_mode_enable(&TWI_MASTER);
	//twi_slave_fast_mode_enable(&TWI_SLAVE);

	twi_options_t m_options = {
		.speed     = TWI_SPEED,
		.chip      = TWI_MASTER_ADDR,
		.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), TWI_SPEED)
	};
	 
	status_code = twi_master_init(&TWI_MASTER, &m_options);
	if(status_code != STATUS_OK) printf("spektel_init.twi_master_init status: %x", status_code);
	twi_master_enable(&TWI_MASTER);

	sysclk_enable_peripheral_clock(&TWI_SLAVE);
	TWI_SlaveInitializeDriver(&slave, &TWI_SLAVE, *slave_process);
	TWI_SlaveInitializeModule(&slave, twi_slave_address, TWI_SLAVE_INTLVL_MED_gc);
	//TWIC.SLAVE.CTRLA |= 0x02; // PMEN: Promiscuous Mode Enable - address match logic disabled, react to everything
	
	//TWI address for Current, Powerbox, Vario and Flight pack capacity
	TWIC.SLAVE.ADDRMASK = (0x7F << 1) ;  //mask (1) all address bits, which are different between all used sensors

	for (uint8_t i = 0; i < TWIS_SEND_BUFFER_SIZE; i++) {
		slave.receivedData[i] = 0;
	}

	//cpu_irq_enable();

	return true;
}



