#ifndef __seqqq
#define __seqqq

#include "../../Callbacks.h"
#include "Track.h"

#ifdef __cplusplus
extern "C" {
#endif

void onParamChange();
bool seqLoopInvoke();

void resetSeqValues();

void saveSeqState();
void loadSeqState();

#ifdef __cplusplus
} // extern "C"
#endif

#endif