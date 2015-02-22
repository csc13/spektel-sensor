---
layout: post
title: "XMEGA, ASF and TWI/I2C"
description: "Getting TWI and I2C running with the Atmel ASF and XMEGA"
date: 2015-01-03
category: articles
tags: [Atmel, AVR, microcontroller, XMEGA, TWI, I2C, XMEGA32E5]
image:
  feature: Board_REV_B.jpg
---
One of the main XMEGA features the sensor uses is the I2C (Inter-Integrated Circut) or TWI
(Two-Wire-Interface) serial data bus. It is used for the `Spektrum RC Telemetry X-Bus` as 
a Slave and for the `Bosch BMP180` pressure sensor as a Master. In both cases the XMEGA
hardware capabilities are used.

While the first prototyping with an Arduino was quite simple, it took me some time to
figure things out using the XMEGA and ASF (Atmel Software Framework). In my opinion it is
because the XMEGA is far less used by makers and hobbyists compared to the ATMEGA series.
And the ASF functions have some undocumented unexpected behaviour. The main source I used
have been the XMEGA datasheet and AFS Quickstart examples, which you can find on the Atmel
website and at the end of some library header files as comments.

I will try to give an overview, pitfalls and solutions in this article. May it help others
to save some time.

### XMEGA TWI Bridge Mode
The `ATXMEGA32E5` offers a feature called Bridge mode. And I didn't found much help on this.
If you go to the ATXMEGA32E5 datasheet it lists the following Alternate Pin Functions 
(in chapter 32.2) for TWI:

- PORT C - PC0 (Pin 16) - SDA
- PORT C - PC1 (Pin 15) - SCL
- PORT D - PD0 (Pin 28) - SDA (TWID (Bridge))
- PORT D - PD1 (Pin 27) - SCL (TWID (Bridge))

Chapter 22 of the Atmel datasheet gives some more hints:

- Bridge mode with independent and simultaneous master and slave operation
- Independent timeout counters in master and slave (Bridge mode support)
- By using the bridge option, the slave can be mapped to different pin
locations
- The master and slave can support 100kHz, 400kHz and 1MHz bus frequency
- It is also possible to enable the bridge mode. In this mode, the slave I/O pins are 
selected from an alternative port, enabling independent and simultaneous master and slave operation
- PORTC has one TWI. Notation of this peripheral is TWIC
- Alternative TWI Slave location in bridge mode is on PORTD

Chapter 18 of the `Atmel ATXMEGA Guide` adds the following:

"When enabling the bridge mode, both master and slave can be active at the same time, each 
with its specific IO pins. Refer to the device datasheet to see which actual I/O port is 
used as alternative port selection for the slave in bridge mode."

**Following this we have to use Port D as TWI slave and PORT C as TWI master.**

So the setup look like the following:
`conf_board.h`
{% highlight html linenos %}
//Part of conf_board.h
// TWIM for BMP180
#define TWI_MASTER       TWIC
#define TWI_MASTER_PORT  PORTC
#define TWI_MASTER_ADDR  0xEF
#define TWI_SPEED        1000000
	 
// TWIS for XBUS
#define TWI_SLAVE        TWIC
#define TWI_SLAVE_PORT  PORTD
{% endhighlight %}

`conf_twim.h` looks like this:
{% highlight html linenos %}
#define CONF_TWIM_INTLVL        TWI_MASTER_INTLVL_MED_gc
#define CONF_PMIC_INTLVL        PMIC_MEDLVLEN_bm
{% endhighlight %}

In the sensor coding, the TWI initialization code is in the `spektel.c` file:

{% highlight html linenos %}
bool spektel_init() { //TWI_t *twis) {
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
{% endhighlight %}
As you can see the bridge mode works without the fast modes.

Make sure you call `sysclk_init();` at the beginning of your code and change `conf_clock.h`
to the desired clock speed.

Initializing the master:
{% highlight html linenos %}
status_code = twi_master_init(&TWI_MASTER, &m_options);
if(status_code != STATUS_OK) printf("spektel_init.twi_master_init status: %x", status_code);
twi_master_enable(&TWI_MASTER);
{% endhighlight %}

###TWI X-Bus slave initialization
We need to calculate the TWI slave address, the XMEGA TWI hardware implementation
listens to. The TWI master always sends a 7-bit slave address to address one slave 
listening on the bus. There is a register for this address, but in our case the sensor
should behave like a multiple personality and react to three sensor addresses.

As muknukem (am rcgroups.com and rc-heli.de forums member) re-engineered, the Spektrum RC
X-Bus starts to enumerates the sensor addresses 70ms after the power is stable. It sends a 
request for each address between 0x01 to 0x7D, with pauses of 13ms in two cycles. Each
sensor the Spektrum transmitters know have one or more (the GPS sensor uses two, because
it needs to send more data) addresses. So the number of different sensors is theoretically
limited by the number of the requested addresses. On the other hand all is fixed and simple.  

The good thing is, the TM1000 telemetry module doesn't care, if the sensor reacts to a request for
lets say address 0x01 with 0x0A for the Powerbox sensor. It uses what the sensor sends back.
After the startup procedure, the TM1000 only asks for sensors we sent a response for
(again, the address sent back is the interesting one).

The XMEGA reacts to all addresses by setting this register:
{% highlight html linenos %}
TWIC.SLAVE.CTRLA |= 0x02; // PMEN: Promiscuous Mode Enable - address match logic disabled, react to everything
{% endhighlight %}

But to save the interrupt routines some work it could be useful to set the XMEGA hardware
to react not to all. On the ATMEGA and Arduinos the following code works:

{% highlight html linenos %}
uint8_t twi_slave_address = 0x00;
status_code_t status_code = 0x00;
// calculate slave address for single sensor (two TWI addresses are only support by XMEGA)
if( CURRENT_TEL ) {
	twi_slave_address |= CURRENT_SENS;
}
if( POWERBOX_TEL ) {
	twi_slave_address |= POWERBOX_SENS;
}
if( VARIO_TEL ) {
	twi_slave_address |= VARIO_SENS;
}
{% endhighlight %}

*In my experience, his is not working for the XMEGA!* And two addresses can be applied with
respect to the datasheet, but not three.

So the further code looks like this:
{% highlight html linenos %}	
sysclk_enable_peripheral_clock(&TWI_SLAVE);
TWI_SlaveInitializeDriver(&slave, &TWI_SLAVE, *slave_process);
{% endhighlight %}

Where `*slave_process` is a pointer to the a function reacting on requests from the master.
I will get into more detail later on.


{% highlight html linenos %}
TWI_SlaveInitializeModule(&slave, twi_slave_address, TWI_SLAVE_INTLVL_MED_gc);
	
//TWI address for Current, Powerbox and Vario and flight pack capacity
TWIC.SLAVE.ADDRMASK = (0x7F << 1) ;  //mask (1) all address bits, which are different between all used sensors

for (uint8_t i = 0; i < TWIS_SEND_BUFFER_SIZE; i++) {
	slave.receivedData[i] = 0;
}
{% endhighlight %}

The magic is done by  writing  0x4B to the upper 7bits of the Slave ADDRMASK Address Mask Register.
The datasheet says:

"If ADDREN is set to zero, ADDRMASK can be loaded with a 7-bit slave address mask. Each bit in ADDRMASK
can mask (disable) the corresponding address bit in the ADDR register. If the mask bit is one, the address match
between the incoming address bit and the corresponding bit in ADDR is ignored; i.e., masked bits will always
match."

We leave ADDREN (Bit 0 of the same register) to zero. The mask is calculated as following:
{% highlight html linenos %}
0000 0011	Current Sensor address
0000 1010	Powerbox Sensor address
0100 0000   Vario Sensor address
0011 0100   Flight Cap Sensor address
-----------------------------------
0111 1111   = 0x7F as Address Mask
{% endhighlight %}

The last part of the code initializes the receive buffer.

###TWI slave operation
Whenever the master request some data from the slave an interrupt routine within the ASF
is called and will ask some client code for the sensor values. And here is the first odd
part of the ASF TWI implementation in my humble opinion. The following was posted on avrfreaks.net
from me.

When looking on the twis.c implementation for TWI slaves it misses an important functionality
to me. Looking at the Arduino Wire library it provides a `onReceive()` and `onRequest()` 
where the first one is to receive data from the master and the second is to respond to 
the masters request.When looking into the ASF TWI slave implementation there is only one 
callback for the `onReceive()` case. So I didn't figure out what the way to handle master 
requests would be. By initializing the TWI slave module the following is used (same as above):

{% highlight html linenos %}
sysclk_enable_peripheral_clock(&TWI_SLAVE);
TWI_SlaveInitializeDriver(&slave, &TWI_SLAVE, *slave_process);
TWI_SlaveInitializeModule(&slave, twi_slave_address, TWI_SLAVE_INTLVL_MED_gc);
{% endhighlight %}

Where `*slave_process` is the callback. Looking in `twis.c` it looks like the following. 
`TWI_SlaveDataHandler(...)` is called (via two functions calls) in the TWIx Interrupt 
Service Routine.

{% highlight html linenos %}
void TWI_SlaveDataHandler(TWI_Slave_t *twi)
{
	if (twi->interface->SLAVE.STATUS & TWI_SLAVE_DIR_bm) {
		TWI_SlaveWriteHandler(twi);
	} else {
		TWI_SlaveReadHandler(twi);
	}
}

//and later

/* brief TWI slave read interrupt handler.
 *  Handles TWI slave read transactions and responses.
 *  param twi The TWI_Slave_t struct instance.
 */
void TWI_SlaveReadHandler(TWI_Slave_t *twi)
{
	/* Enable stop interrupt. */
	uint8_t currentCtrlA = twi->interface->SLAVE.CTRLA;
	twi->interface->SLAVE.CTRLA = currentCtrlA | TWI_SLAVE_PIEN_bm;

	/* If free space in buffer. */
	if (twi->bytesReceived < TWIS_RECEIVE_BUFFER_SIZE) {
		/* Fetch data */
		uint8_t data = twi->interface->SLAVE.DATA;
		twi->receivedData[twi->bytesReceived] = data;

// look here >>>>>>>
		/* Process data. */
		twi->Process_Data();

		twi->bytesReceived++;

		/* If application signalling need to abort (error occured),
		 * complete transaction and wait for next START. Otherwise
		 * send ACK and wait for data interrupt.
		 */
		if (twi->abort) {
			twi->interface->SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
			TWI_SlaveTransactionFinished(twi, TWIS_RESULT_ABORTED);
			twi->abort = false;
		} else {
			twi->interface->SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
		}
	}
	/* If buffer overflow, send NACK and wait for next START. Set
	 * result buffer overflow.
	 */
	else {
		twi->interface->SLAVE.CTRLB = TWI_SLAVE_ACKACT_bm |
		                              TWI_SLAVE_CMD_COMPTRANS_gc;
		TWI_SlaveTransactionFinished(twi, TWIS_RESULT_BUFFER_OVERFLOW);
	}
}
{% endhighlight %}

So you see, that for the read handler the Process_Data callback handler is placed. Looking
on the write handler this callback is missing. The following change of `twis.c` works 
for me (I only need the onRequest() so I used the same callback function):

{% highlight html linenos %}
/* brief TWI slave write interrupt handler.
 *  Handles TWI slave write transactions and responses.
 *
 *  param twi The TWI_Slave_t struct instance.
 */
void TWI_SlaveWriteHandler(TWI_Slave_t *twi)
{
	/* If NACK, slave write transaction finished. */
	if ((twi->bytesSent > 0) && (twi->interface->SLAVE.STATUS &
	                             TWI_SLAVE_RXACK_bm)) {						 

		twi->interface->SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
		TWI_SlaveTransactionFinished(twi, TWIS_RESULT_OK);
//>>>>>>>>>> added the next line for callback		
		twi->Process_Data();
	}
	/* If ACK, master expects more data. */
	else {
		if (twi->bytesSent < TWIS_SEND_BUFFER_SIZE) {
			uint8_t data = twi->sendData[twi->bytesSent];
			twi->interface->SLAVE.DATA = data;
			twi->bytesSent++;

			/* Send data, wait for data interrupt. */
			twi->interface->SLAVE.CTRLB = TWI_SLAVE_CMD_RESPONSE_gc;
		}
		/* If buffer overflow. */
		else {
			twi->interface->SLAVE.CTRLB = TWI_SLAVE_CMD_COMPTRANS_gc;
			TWI_SlaveTransactionFinished(twi, TWIS_RESULT_BUFFER_OVERFLOW);
		}
	}
}
{% endhighlight %}

I don't think such changes should be made to a standard library. But I invested to much time
to understand the framework, so that it was easier for me than a rewrite.

###Sending out data
If the slave_process function is called via the `twi->Process_Data()` function pointer,
`spektel_write_sensor_data()` is called sending out the data in a round robin pattern.

{% highlight html linenos %}
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
			}
		}
	}
	if(++toggle > 2) toggle = 0;
} 
{% endhighlight %}

All we need to do is to write the data to the `slave.sendData[]` array. Since each X-Bus
data package is 16 byte long starting with the slave address in the first byte, a zero byte
and then the data, we can send all data in one chunk. I just changed the buffer sizes in the
`twis.h` file to 16 byte:

{% highlight html linenos %}
/* Buffer size defines. */
#define TWIS_RECEIVE_BUFFER_SIZE         16
#define TWIS_SEND_BUFFER_SIZE            16
{% endhighlight %}

The format of each sensor data package is defined by the Spektrum TM1000 module and translated
to data structures in the `spektel.h` file. If the measuring code of the sensor has new data,
it writes it to these structures (for example `spektel_sensor_powerbox_t`) and calls the 
`spektel_write_pwerbox_sens(...)` function which translates it into an 16 byte array for the
next TWI `onRequest()` action.

I hope the whole process on the slave side gets clear now.

###Communication as a TWI Master
Setting up the connection to the BMP180, where the XMEGA is the Master and the BMP180 is the
slave took me some time to figure out. But one by one!

###Getting the hardware right
The I2C / TWI bus used to connect the Bosch BMP180 pressure sensor has two pull-up resistors.
This is the general design of each TWI bus and used on the X-Bus TM1000 side as well. One
resistor for each line (SDA/SCL) no matter how many slaves are connected to pull the open drain
signal lines to Vcc (3.3V in our case) if needed. If you work with a 5V µC you will need
an additional level shifter.

I tried to be smarter than the datasheet and put 2.7K in. It turned out, that they are two
small to drive the SDA and SCL lines to GND fast enough. So use 4.7K for the TWI/I2Cs instead.
For the X-Bus 2.7K seems to work, but the connection using a 20cm cable is much longer as the
1cm data lines on the board for the BMP180.

I got myself a Saleae Logic8 logic analyser. Why it is probably not the most needed tool,
in this case it helps a lot. And it brings analog monitoring functionality as well. From the
signal shape below I figured out that something goes wrong. The XMEGA trys to pull down the 
signal resulting in a slow drop of the signal voltage.

<figure>
	<img src="/images/saleae_logic_i2c_bmp180_faulty.jpg">
	<figcaption>Monitoring the I2C bus to the BMP180</figcaption>
</figure>

It has to look this way. The sequence just reads the static device ID for test purposes.

<figure>
	<img src="/images/saleae_logic_i2c_bmp180.jpg">
	<figcaption>Monitoring the I2C bus to the BMP180</figcaption>
</figure>

###Initializing the Master
Here the code from the TWI initialization routine above again in short form: 
{% highlight html linenos %}
bool spektel_init() {
// Initialize ports
TWI_MASTER_PORT.PIN0CTRL = PORT_OPC_WIREDAND_gc;
TWI_MASTER_PORT.PIN1CTRL = PORT_OPC_WIREDAND_gc;
irq_initialize_vectors();
sysclk_enable_peripheral_clock(&TWI_MASTER);
twi_bridge_enable(&TWI_MASTER);
twi_options_t m_options = {
	.speed     = TWI_SPEED,
	.chip      = TWI_MASTER_ADDR,
	.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), TWI_SPEED)
};
{% endhighlight %}     

Now we can directly start writing to the slave devices. In our case, the BMP180
provides EEPROM style registers for the communication. So this is a standard style
for TWI communication.

Write: Write the 7-bit slave address and the LSB (most right bit) bit 1 for read and 0 for 
write. As it turns out this was the next pitfall the ASF provides for me. In all of the 
Atmel code, I worked with the 7-bit slave address more or less like with an 8-bit address 
(expect for direct access to the registers). But the ASF library functions 
`twi_master_write`and `twi_master_read` add the LSB dependent on the read or write function.
I guess this is part of the TWI-EEPROM access protocol. Where the Bosch datasheet talks
about 0xEF for read and 0xEE for write, the ASF functions need 0x77! With this we can read 
and write bytes:

Here the code from the TWI initialization routine above again in short form: 
{% highlight html linenos %}
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
{% endhighlight %} 

For the .chip you can use 0x77 or the datasheet address shifted to the right. On the bus
the XMEGA will call with 0xEF and 0XEF showing if it wants to read or write. 

###Reading many bytes
Another pitfall was, that the Atmel ASF seems to expect the MSB (in this case higher **byte** 
of a 16 bit value) and LSB (lower byte) order different than the Bosch BMP180. The 
library supports to read multiple bytes in one read. Doing so, the BMP180 respondes with 
the MSB, the Master sends an `ACK` requesting more data. Then the BMP180 sends the LSB
and a `NAK` follows to finish the communication and release the bus.
But the ASF gives back that values in the wrong order LSB-MSB. So a swap operation is needed
at the end. Either the BMP180 or the ASF gets it wrong!?  
 {% highlight html linenos %}
/** Read word (16bit)
 *
 * \param reg_addr the register address.
 * \param word the data to be read.
 *
 * \return status of operation
 */
static uint8_t read_word(const uint8_t reg_addr, uint16_t *word)
{
	uint8_t err;
	
	twi_package_t r_packet = {
		.addr		 = { reg_addr },
		.addr_length = sizeof(reg_addr),
		.chip        = BMP180_ADDR_READ >> 1,
		.buffer      = word,
		.length      = sizeof(uint16_t),
		.no_wait     = false
	};
	err = twi_master_read(&TWI_MASTER, &r_packet);
	
	uint8_t lsb = (*word) >> 8;
	(*word) = ((*word) << 8) | lsb;
	
	return err;
}
{% endhighlight %}
 







