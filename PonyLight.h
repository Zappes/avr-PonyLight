#ifndef PONYLIGHT_H_
#define PONYLIGHT_H_

#ifndef F_CPU
#define F_CPU 16000000UL     /* Set your clock speed here, mine is a 16 Mhz quartz*/
#endif

#define REG_RED                 OCR1B
#define REG_GRN                 OCR1A
#define REG_BLU                 OCR0A

#define MODE_IMMEDIATE          1
#define MODE_FADING             2
#define LOCAL_ECHO              1
#define	EEPStart                0x10

int parseNextInt(char** buffer, int max);
void updateEEProm();
void restoreFromEEProm();
void serialBufferHandler(char* commandBuffer);

#endif /* TINYRGB_H_ */
