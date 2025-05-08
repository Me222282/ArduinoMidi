#ifndef __vvvvvvvvvvvvv
#define __vvvvvvvvvvvvv

#include "../core/Midi.h"
#include "../../MenuFeedback.h"

#ifdef __cplusplus
extern "C" {
#endif

// void ttMenuFunction();
Menu* getVMenu();

void invokeVibrato();

void resetVVMValues();

void saveVVMState();
void loadVVMState();

#ifdef __cplusplus
} // extern "C"
#endif

#endif