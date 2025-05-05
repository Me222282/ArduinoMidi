#ifndef __ttttttttttttt
#define __ttttttttttttt

#include "../core/Midi.h"
#include "../../MenuFeedback.h"

#ifdef __cplusplus
extern "C" {
#endif

// void ttMenuFunction();
Menu* getTMenu();

void invokeTremelo();

void resetTTMValues();

void saveTTMState();
void loadTTMState();

#ifdef __cplusplus
} // extern "C"
#endif

#endif