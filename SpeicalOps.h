#ifndef __spec
#define __spec

#include "src/core/Midi.h"
#include "MenuFeedback.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool invokeArp;
extern bool channelArps[5];

void specialOptions();

void resetOpsValues();

void saveOpsState();
void loadOpsState();

uint8_t getDigit(NoteName n);

#ifdef __cplusplus
} // extern "C"
#endif

#endif