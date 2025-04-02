#ifndef __calls
#define __calls

#include "src/core/Midi.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool alwaysDelay;
extern bool filterKeys;
extern Notes filter;

void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
void onNoteOff(uint8_t channel, uint8_t note);
void onPitchBend(uint8_t channel, uint16_t bend);
void onControlChange(uint8_t channel, CCType number, uint8_t value);
void updateAllNotes();
void updateAllPBs();
void updateOtherPorts();

#ifdef __cplusplus
} // extern "C"
#endif

#endif