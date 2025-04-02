#ifndef __comical
#define __comical

#include "Globals.h"

#define GATESPI SPISettings(10000000, MSBFIRST, SPI_MODE0)
#define DAC8SPI SPISettings(20000000, MSBFIRST, SPI_MODE0)
#define DAC12SPI SPISettings(20000000, MSBFIRST, SPI_MODE0)
#define POTSPI SPISettings(10000000, MSBFIRST, SPI_MODE0)

void setGate(uint8_t value);

void setVel(uint8_t n, uint8_t value);

void _setNoteNorm(uint8_t n, uint8_t value);
void _setSubNote(uint8_t n, uint8_t value);
extern void (*setNote)(uint8_t, uint8_t);

void setPitchBend(uint8_t n, uint16_t value);
void setMod(uint16_t value);

// void configurePot(uint8_t pin);

// void setPot(uint8_t pin, bool bPot, uint8_t value);

#endif