#ifndef __cccccccc
#define __cccccccc

#include "../core/Midi.h"
#include "../../MenuFeedback.h"

#ifdef __cplusplus
extern "C" {
#endif

// void ccMenuFunction();
Menu* getCCMenu();

void invokePulse();
void onSequence();
void onTrack(uint8_t channel);
void onArp(uint8_t channel);

void resetCCMValues();

void saveCCMState();
void loadCCMState();

void updateCCOuts();

#ifdef __cplusplus
} // extern "C"
#endif

#endif