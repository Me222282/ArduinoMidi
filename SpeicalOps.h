#ifndef __spec
#define __spec

#include "src/core/Midi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool invokeArp;
extern bool channelArps[5];

void specialOptions();

void resetValues();

void saveSate();
void loadSate();

uint8_t getDigit(NoteName n);
inline void triggerFeedbackC(bool v, uint8_t c);
inline void triggerFeedback(bool v);

#ifdef __cplusplus
} // extern "C"
#endif

#endif