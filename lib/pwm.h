/*
 * rgb.h
 *
 * Created: 15.09.2013 03:03:59
 * Author: Paul Rogalinski, paul@paul.vc
 */

#ifndef RGB_H_
#define RGB_H_

/*
 * Initialize PWM output pins, timers and comparators.
 */
void initPwm(void) {
	// set PWM pins to output
	DDRB |= (1 << PB2) | (1 << PB3) | (1 << PB4);
	
	// setup timer 0
	// fast PWM mode
	// clock / 64 (~1.2khz @ 20mhz system clock)
	// enable both comparator outputs
	TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A0) | (1 << COM0A1)
			| (1 << COM0B0) | (1 << COM0B1);
	TCCR0B = (1 << CS00) | (1 << CS01);
	
	// setup timer 1
	// fast PWM mode @ 8 bit
	// clock / 64 (~1.2khz @ 20mhz system clock)
	// enable both comparator outputs
	TCCR1A = (1 << COM1A0) | (1 << COM1A1) | (1 << COM1B0) | (1 << COM1B1)
			| (1 << WGM10);
	TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12);
	
	// turn off the outputs
	REG_BLU = 255;
	REG_GRN = 255;
	REG_RED = 255;
}

/*
 * Loads the RGB value into the output comparator registers
 */
void setRgb(uint8_t r, uint8_t g, uint8_t b) {
	REG_RED = 255 - r;
	REG_BLU = 255 - b;
	REG_GRN = 255 - g;
}

#endif /* RGB_H_ */
