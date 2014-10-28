/**
 * \file
 *
 * \brief User board initialization template
 *
 */

#include <asf.h>
#include <board.h>
#include <conf_board.h>

void board_init(void)
{
	/* This function is meant to contain board-specific initialization code
	 * for, e.g., the I/O pins. The initialization can rely on application-
	 * specific board configuration, found in conf_board.h.
	 */
	
	ioport_init();
	#ifdef BOARD_REV_A
	// USART
		PORTD_REMAP = USARTD_REMAP;
		PORTD_DIRSET = PIN7_bm;
	
	//LEDs
		ioport_set_pin_dir(LED_GREEN, IOPORT_DIR_OUTPUT);
		ioport_set_pin_dir(LED_RED, IOPORT_DIR_OUTPUT);
	#endif

	#ifdef ENABLE_USART	
	// startup USART
	static usart_rs232_options_t USART_SERIAL_OPTIONS = {
		.baudrate = USART_SERIAL_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT
	};
	sysclk_enable_module(SYSCLK_PORT_D, PR_USART0_bm);
	//usart_init_rs232(USART_SERIAL, &USART_SERIAL_OPTIONS);
	stdio_serial_init(USART_SERIAL, &USART_SERIAL_OPTIONS);
	#endif
	
}
