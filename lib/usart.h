/*
 * usart.h
 *
 * Created: 15.09.2013 03:05:59
 * Author: Paul Rogalinski, paul@paul.vc
 */

#ifndef USART_H_
#define USART_H_

#include<stdbool.h>

// @see 14.3.1 Internal Clock Generation � The Baud Rate Generator
#define UART_BAUD_RATE 9600L
#define UART_BAUD_CALC(UART_BAUD_RATE,F_CPU) ((F_CPU)/((UART_BAUD_RATE)*16l)-1)

typedef void (*serialBufferReadyCallbackType)(char* commandBuffer);
serialBufferReadyCallbackType serialBufferReadyCallback = 0x00;

char commandBuffer[16];
uint8_t bufferPos = 0;
const char* bufferReadyPrompt;
volatile bool bufferReady = false;

/*
 * Sets the callback reference for buffer ready events 
 */
void setCommandBufferCallback(serialBufferReadyCallbackType cb) {
	serialBufferReadyCallback = cb;
}

/*
 * Initializes the USART registers, enables the ISR
 */
void initSerial(void) {
	UCSRC |= (1 << UCSZ0) | (1 << UCSZ1); // 1 Stop-Bit, 8 Bits
	UCSRB = (1 << TXEN) | (1 << RXEN); // enable RX/TX
	UCSRB |= (1 << RXCIE); // Enable the USART Receive Complete interrupt (USART_RXC)
			
	/* UBRRL and UBRRH � USART Baud Rate Registers */
	UBRRH = (uint8_t) (UART_BAUD_CALC(UART_BAUD_RATE,F_CPU) >> 8);
	UBRRL = (uint8_t) (UART_BAUD_CALC(UART_BAUD_RATE, F_CPU));
	
	bufferReadyPrompt = PSTR("\r\n");
}

/*
 * Waits for the usart to become ready to send data and 
 * writes a single char.
 */
void writeCharToSerial(unsigned char c) {
	// UCSRA � USART Control and Status Register A
	// � Bit 5 � UDRE: USART Data Register Empty
	while (!(UCSRA & (1 << UDRE)))
		;
	UDR = c;
}

/*
 * Writes a PROGMEM string to the serial port
 */
void writePgmStringToSerial(const char *pgmString) {
	while (pgm_read_byte(pgmString) != 0x00)
		writeCharToSerial((char) pgm_read_byte(pgmString++));
}

/*
 * Writes a string/char[] to the serial port.
 */
void writeStringToSerial(char *str) {
	while (*str != 0x00)
		writeCharToSerial(*str++);
}

/*
 * Writes a linefeed and newline to the serial port
 */
void writeNewLine() {
	writePgmStringToSerial(PSTR("\r\n"));
}

/*
 * Handles bytes received from the serial port. Fills up the command buffer and calls
 * the bufferReadyCallback when the buffer overflows or a linefeed/newline has been
 * sent.
 */
ISR( USART_RX_vect) {
	char chrRead;
	chrRead = UDR;
	commandBuffer[bufferPos++] = chrRead;
	commandBuffer[bufferPos] = 0x00;
	UDR = chrRead;
	if ((bufferPos >= (sizeof(commandBuffer) - 1))
			|| ((chrRead == '\n' || chrRead == '\r'))) {
		bufferPos = 0;
		writePgmStringToSerial(bufferReadyPrompt);
		bufferReady = true;
		if (serialBufferReadyCallback != 0x00)
			serialBufferReadyCallback(commandBuffer);
	}
}

#endif /* USART_H_ */
