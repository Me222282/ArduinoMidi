#ifndef __MFG
#define __MFG

#include "src/core/Midi.h"

#define NOTEFAIL_S NoteName::C2
#define NOTEON_S NoteName::_D4
#define NOTEOFF_S NoteName::C4
#define NOTESELECT_S NoteName::C3
#define NOTEOPTION_S NoteName::G3

#define MF_DURATION 125
#define MF_DURATION_SHORT 75

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