#ifndef __arpeg
#define __arpeg

#include "src/core/Midi.h"

#ifdef __cplusplus
extern "C" {
#endif

void playNote(NoteName n, uint32_t duration);
void playNoteC(NoteName n, uint8_t channel, uint32_t duration);
void invokeMF();

#ifdef __cplusplus
} // extern "C"
#endif

#endif