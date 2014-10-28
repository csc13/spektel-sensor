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
	// climb rate in 0.1 m/s
	vario_data.byte[0] = VARIO_SENS; // Vario
	vario_data.byte[2] = (vario.altitude >> 8);
	vario_data.byte[3] = vario.altitude;
	vario_data.byte[4] = (vario.climb_rate >> 8);
	vario_data.byte[5] = vario.climb_rate;
}


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
			}
		}
	}
	if(++toggle > 2) toggle = 0;
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

bool spektel_init() { //TWI_t *twis) {
	uint8_t twi_slave_address = 0x00;
	status_code_t status_code = 0x00;
	//TWIC.SLAVE.CTRLA.PMEN register set, so no address match logic required
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
	
	// Initialize ports
	TWI_MASTER_PORT.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc;
	TWI_MASTER_PORT.PIN1CTRL = PORT_OPC_WIREDANDPULL_gc;

	irq_initialize_vectors();

	sysclk_enable_peripheral_clock(&TWI_MASTER);

	twi_bridge_enable(&TWI_MASTER);
	//twi_fast_mode_enable(&TWI_MASTER);
	//twi_slave_fast_mode_enable(&TWI_SLAVE);

// check if needed
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
	
	//TWI address for Current, Powerbox and Vario 
	TWIC.SLAVE.ADDRMASK = (0x4B << 1) ;  //mask (1) all address bits, which are different between all used sensors

	for (uint8_t i = 0; i < TWIS_SEND_BUFFER_SIZE; i++) {
		slave.receivedData[i] = 0;
	}

	//cpu_irq_enable();

	return true;
}



