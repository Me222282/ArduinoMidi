#ifndef __spec
#define __spec

#include "../core/Midi.h"
#include "../../MenuFeedback.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool invokeArp;
extern bool channelArps[5];

void specialOptions();

void resetOpsValues();

void saveOpsState();
void loadOpsState();

#ifdef __cplusplus
} // extern "C"
#endif

#endif