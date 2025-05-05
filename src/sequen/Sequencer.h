#ifndef __seqqq
#define __seqqq

#include "../../Callbacks.h"
#include "../../MenuFeedback.h"
#include "Track.h"

#ifdef __cplusplus
extern "C" {
#endif

// void onParamChange();
// bool seqLoopInvoke();

void resetSeqValues();

Menu* getSeqMenu();

void saveSeqState();
void loadSeqState();

#ifdef __cplusplus
} // extern "C"
#endif

#endif