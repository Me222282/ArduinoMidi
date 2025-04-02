#ifndef __spec
#define __spec

#ifdef __cplusplus
extern "C" {
#endif

extern bool invokeArp;
extern bool channelArps[5];

void specialOptions();

void resetValues();

void saveSate();
void loadSate();

#ifdef __cplusplus
} // extern "C"
#endif

#endif