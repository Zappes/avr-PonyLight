/*
 * sense.h
 *
 *  Created on: 26.10.2014
 *      Author: zaphod
 */

#ifndef SENSE_H_
#define SENSE_H_

#include <avr/io.h>

// this pin goes high when 12V is supplied to the strip
#define PORT_SENSE_12V					PORTD
#define DDR_SENSE_12V						DDRD
#define PIN_SENSE_12V						PD6
#define INPUT_SENSE_12V					PIND

void initSense(void) {
	// set sense pin as input
	DDR_SENSE_12V &= ~_BV(PIN_SENSE_12V);
}

uint8_t powerSupplied(void) {
	return INPUT_SENSE_12V & _BV(PIN_SENSE_12V);
}

#endif /* SENSE_H_ */
