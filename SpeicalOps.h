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

void resetValues();

void saveSate();
void loadSate();

uint8_t getDigit(NoteName n);
inline void triggerFeedback(bool v)
{
    playNote(v ? NOTEON_S : NOTEOFF_S, MF_DURATION);
}
inline void triggerFeedbackC(bool v, uint8_t c)
{
    playNoteC(v ? NOTEON_S : NOTEOFF_S, c, MF_DURATION);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif