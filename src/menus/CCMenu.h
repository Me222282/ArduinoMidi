#ifndef __cccccccc
#define __cccccccc

#include "../core/Midi.h"
#include "../../MenuFeedback.h"

#ifdef __cplusplus
extern "C" {
#endif

// void ccMenuFunction();
Menu* getCCMenu();

void resetCCMValues();

void saveCCMState();
void loadCCMState();

#ifdef __cplusplus
} // extern "C"
#endif

#endif