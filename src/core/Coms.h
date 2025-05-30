#ifndef __comical
#define __comical

#define GATESPI SPISettings(10000000, MSBFIRST, SPI_MODE0)
#define DAC8SPI SPISettings(20000000, MSBFIRST, SPI_MODE0)
#define DAC12SPI SPISettings(20000000, MSBFIRST, SPI_MODE0)
#define POTSPI SPISettings(10000000, MSBFIRST, SPI_MODE0)

#ifdef __cplusplus
extern "C" {
#endif

extern bool ccOutputs[5];
extern bool velOutputs[5];
extern bool ccChannelMode;
extern int16_t pbOffsets[5];

extern uint8_t gateCurrent;
void setGate(uint8_t value);

void setVel(uint8_t n, uint8_t value);
void setCCOut(uint8_t n, uint8_t value);
void setVelUnchecked(uint8_t n, uint8_t value);

uint8_t readVel(uint8_t n);
uint16_t readMod();

void _setNoteNorm(uint8_t n, uint8_t value);
void _setSubNote(uint8_t n, uint8_t value);
extern void (*setNote)(uint8_t, uint8_t);

void setPitchBend(uint8_t n, uint16_t value);
void setMod(uint16_t value);

void updatePitchBend(uint8_t n);

void configureGate();

// void configurePot(uint8_t pin);

// void setPot(uint8_t pin, bool bPot, uint8_t value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif