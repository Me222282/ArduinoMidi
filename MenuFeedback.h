#ifndef __MFG
#define __MFG

#include "src/core/Midi.h"

#define NOTEFAIL_S NoteName::_B1
#define NOTEON_S NoteName::C4
#define NOTEOFF_S NoteName::G3
#define NOTESELECT_S NoteName::C3
#define NOTEOPTION_S NoteName::G4

#define MF_DURATION 125
#define MF_DURATION_SHORT 75

#ifdef __cplusplus
extern "C" {
#endif

extern bool isMenuFeedback;

extern uint8_t digit;
extern uint8_t digits[4];

bool addDigit(NoteName n, uint8_t max);
uint16_t getEnteredValue(uint16_t last);

void playNote(NoteName n, uint32_t duration);
void playNoteC(NoteName n, uint8_t channel, uint32_t duration);
void invokeMF();

inline void triggerFeedback(bool v)
{
    playNote(v ? NOTEON_S : NOTEOFF_S, MF_DURATION);
}
inline void triggerFeedbackC(bool v, uint8_t c)
{
    playNoteC(v ? NOTEON_S : NOTEOFF_S, c, MF_DURATION);
}
inline void playNumber(NoteName n)
{
    playNote((NoteName)(n + (NoteName::_A3 - NoteName::_A0)), MF_DURATION);
}
inline void playNumberC(NoteName n, uint8_t channel)
{
    playNoteC((NoteName)(n + (NoteName::_A3 - NoteName::_A0)), channel, MF_DURATION);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif