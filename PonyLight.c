/*
 * PonyLight.c
 *
 * Created by Gregor Ottmann, zaphod@bluephod.net
 * Based on tinyRGB by Paul Rogalinski, paul@paul.vc
 *
 * Pins:
 *  2: RX  -> USB TX
 *  3: TX  -> USB RX
 * 14: PB2 / OC0A -> Blue
 * 15: PB3 / OC1A -> Green
 * 16: PB4 / OC1B -> RED
 */

#include "PonyLight.h"

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdlib.h>

#include "lib/pwm.h"
#include "lib/usart.h"
#include "lib/colors.h"
#include "lib/sense.h"

uint8_t mode = MODE_FADING;
int wait = 1;
int currentWait = 100;

RgbColor cRgb;   // current rgb values
RgbColor tRgb;   // target rgb values for fading

bool autosaveEnabled = false;

/*
 * take a guess.
 */
void printHelp() {
	writePgmStringToSerial(PSTR("\f\aPonyLight 1.0 by Zappes, based on Pulsar's tinyRGB - http://github.com/pulsar256/tinyrgb\r\n"));
	writePgmStringToSerial(PSTR("Commands: \r\n"));
	writePgmStringToSerial(PSTR("  help\r\n"));
	writePgmStringToSerial(PSTR("  status\r\n"));
	writePgmStringToSerial(PSTR("  RGB:rrrgggbbb (rgb values 0-255)\r\n"));
	writePgmStringToSerial(PSTR("  MODE:mmm (001 immediate, 002 fading)\r\n"));
	writePgmStringToSerial(PSTR("  DELAY:ddd (000 fastest, 999 slowest)\r\n"));
	writePgmStringToSerial(PSTR("  SAVE:sss (000 disables autosave, all else enables it. default is disabled.)\r\n"));
}

/*
 * Parses an integer value from a triple of chars. advances the pointer to the string afterwards. Clips the return value at "max".
 */
int parseNextInt(char** buffer, int max) {
	char val[4];
	strncpy(val, *buffer, 3);
	val[3] = '\0';
	*buffer = *buffer + 3;
	
	int res = atoi(val);
	return res <= max ? res : max;
}

/*
 * dumps the contents of the RgbColor structure to serial.
 */
void dumpRgbColorToSerial(RgbColor* color) {
	char buffer[4];
	itoa(color->r, buffer, 10);
	writeStringToSerial(buffer);
	writeStringToSerial("/");
	itoa(color->g, buffer, 10);
	writeStringToSerial(buffer);
	writeStringToSerial("/");
	itoa(color->b, buffer, 10);
	writeStringToSerial(buffer);
}

/*
 * Parses the command buffer and changes the current state accordingly.
 *
 * Command Syntax:
 * "RGB:RRRGGGBBB"      set R/G/B values
 * "MODE:MMM"             set mode:
 *                        001 - immediate RGB
 *                        002 - fading RGB
 * "DELAY:DDD"              set delay for fade / flash modes.
 * "SAVE:VVV"             enables (VVV > 0) or disables (VVV == 0) the eeprom autosave function. default is disabled.
 * "status"              get current RGB values and mode
 * "help"                help screen.
 */
bool handleCommands(char* commandBuffer) {
	char *bufferCursor;
	
	// Set RGB values and switch to MODE_FIXED (001)
	bufferCursor = strstr(commandBuffer, "RGB:");
	if (bufferCursor != NULL) {
		bufferCursor += 4;
		tRgb.r = parseNextInt(&bufferCursor, 255);
		tRgb.g = parseNextInt(&bufferCursor, 255);
		tRgb.b = parseNextInt(&bufferCursor, 255);
		
		if (mode == MODE_IMMEDIATE) {
			cRgb.r = tRgb.r;
			cRgb.g = tRgb.g;
			cRgb.b = tRgb.b;
			setRgb(tRgb.r, tRgb.g, tRgb.b);
		}
		
		return true;
	}
	
	// set delay / wait cycles between color changes.
	bufferCursor = strstr(commandBuffer, "DELAY:");
	if (bufferCursor != NULL) {
		bufferCursor += 6;
		wait = parseNextInt(&bufferCursor, 999);
		return true;
	}
	
	// Sets the mode
	bufferCursor = strstr(commandBuffer, "MODE:");
	if (bufferCursor != NULL) {
		bufferCursor += 5;
		mode = parseNextInt(&bufferCursor, 255);
		return true;
	}
	
	// Dumps all status registers
	bufferCursor = strstr(commandBuffer, "status");
	if (bufferCursor != NULL) {
		char buffer[4];
		
		writePgmStringToSerial(PSTR("Mode: "));
		itoa(mode, buffer, 10);
		writeStringToSerial(buffer);
		writeNewLine();
		
		writePgmStringToSerial(PSTR("Delay: "));
		itoa(wait, buffer, 10);
		writeStringToSerial(buffer);
		writeNewLine();
		
		writePgmStringToSerial(PSTR("Autosave: "));
		if (autosaveEnabled)
			writePgmStringToSerial(PSTR("enabled"));
		else
			writePgmStringToSerial(PSTR("disabled"));
		writeNewLine();
		
		writePgmStringToSerial(PSTR("Strip power: "));
		if (powerSupplied())
			writePgmStringToSerial(PSTR("enabled"));
		else
			writePgmStringToSerial(PSTR("disabled"));
		writeNewLine();

		writePgmStringToSerial(PSTR("Current RGB Values: "));
		dumpRgbColorToSerial(&cRgb);
		writeNewLine();
		
		writePgmStringToSerial(PSTR("Target RGB Values: "));
		dumpRgbColorToSerial(&tRgb);
		writeNewLine();
		
		return true;
	}
	
	// Dumps all status registers
	bufferCursor = strstr(commandBuffer, "help");
	if (bufferCursor != NULL) {
		printHelp();
		return true;
	}
	
	// Enables (value > 0) / Disables (value == 0) the autosave function.
	bufferCursor = strstr(commandBuffer, "SAVE:");
	if (bufferCursor != NULL) {
		bufferCursor += 5;
		uint8_t value = parseNextInt(&bufferCursor, 999);
		if (value > 0)
			autosaveEnabled = true;
		else
			autosaveEnabled = false;
		return true;
	}
	
	return false;
}

/*
 * Callback for the USART handler, executed each time the buffer is full or a 
 * newline has been submitted.
 */
void serialBufferHandler(char* commandBuffer) {
	cli();
	
	if (!handleCommands(commandBuffer)) {
		writePgmStringToSerial(PSTR("\r\nERR\r\n"));
	} else {
		writePgmStringToSerial(PSTR("\r\nOK\r\n"));
		if (autosaveEnabled)
			updateEEProm();
	}

	sei();
}

/*
 * writes an uint8 value into the eeprom at the offset position. 
 * the passed offset will be off set once again by the value of 
 * EEPStart
 */
void writeEepromByte(int offset, uint8_t value) {
	uint8_t* addr = (uint8_t*) (EEPStart + offset);
	eeprom_update_byte(addr, value);
}

/*
 * reads an uint8 value from the eeprom at the offset position. 
 * the passed offset will be off set once again by the value of 
 * EEPStart
 */
uint8_t readEepromByte(int offset) {
	uint8_t* addr = (uint8_t*) (EEPStart + offset);
	return eeprom_read_byte(addr);
}

/*
 * Saves current state into the eeprom
 */
void updateEEProm() {
	int c = 0;
	writeEepromByte(c++, mode);
	writeEepromByte(c++, tRgb.r);
	writeEepromByte(c++, tRgb.g);
	writeEepromByte(c++, tRgb.b);
	writeEepromByte(c++, wait);
}

/*
 * restores the current state from the eeprom 
 */
void restoreFromEEProm() {
	int c = 0;
	mode = readEepromByte(c++);
	if (mode == 0 || mode == 255) // eeprom uninitialized.
			{
		mode = 0;
		return;
	};
	tRgb.r = readEepromByte(c++);
	tRgb.g = readEepromByte(c++);
	tRgb.b = readEepromByte(c++);
	cRgb.r = tRgb.r;
	cRgb.g = tRgb.g;
	cRgb.b = tRgb.b;
	wait = readEepromByte(c++);
}

/*
 * Initialization routines, self test and welcome screen. Main loop is implemented in the ISR below
 */
int main(void) {
	initSerial();
	initPwm();
	initSense();
	restoreFromEEProm();
	
	if (mode == 0) {
		mode = MODE_FADING;
		setRgb(0, 0, 0);
		tRgb.r = 255;
		tRgb.g = 255;
		tRgb.b = 255;
		cRgb.r = tRgb.r;
		cRgb.g = tRgb.g;
		cRgb.b = tRgb.b;
	}
	
	printHelp();
	writePgmStringToSerial(PSTR("\r\nOK\r\n"));
	setCommandBufferCallback(serialBufferHandler);
	
	// our "main" loop is scheduled by timer1's overflow IRQ
	// @20MHz system clock and a /64 timer pre-scaler the ISR will run at 610.3515625 Hz / every 1.639ms
	TIMSK |= (1 << TOIE1);
	sei();
	
	while (1)
		;
}

uint8_t getNextValue(uint8_t current, uint8_t target) {
	if (current == target) {
		return current;
	} else if (current > target) {
		return current - 1;
	} else {
		return current + 1;
	}
}

/*
 * Scheduled to run every 1-2ms (depending on your system clock). Transfers the color values into
 * the PWM registers and, depending on the current mode, computes the colorcycling effects
 */
ISR(TIMER1_OVF_vect) {
	if (currentWait-- == 0) {
		currentWait = wait;
		
		if (mode == MODE_FADING) {
			cRgb.r = getNextValue(cRgb.r, tRgb.r);
			cRgb.g = getNextValue(cRgb.g, tRgb.g);
			cRgb.b = getNextValue(cRgb.b, tRgb.b);
		}
		
		setRgb(cRgb.r, cRgb.g, cRgb.b);
	}
}
