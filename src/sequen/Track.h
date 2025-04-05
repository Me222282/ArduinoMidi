#ifndef __tracking
#define __tracking

#include "../core/Globals.h"
#include "Cubic.h"

#define NOTEHOLD (Note){ 0xFF, 0xFF }
#define NOTEOFF (Note){ 0xFF, 0x00 }

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    Note* notes;
    uint16_t* mods;
    Cubic cub;
    
    uint16_t size;
    Note lastNote;
    uint8_t position;
    uint8_t channel;
    uint8_t clockDivision;
    bool playing;
    bool useMod;
    bool halfTime;
} Track;

void createTrack(Track* t, uint8_t channel);
void deleteTrack(Track* t);

bool addTrackValue(Track* t, Note n, uint16_t m);
bool finaliseTrack(Track* t);

void triggerTrack(Track* t, uint8_t channel, uint16_t playStep);
void modTrack(Track* t, uint8_t channel, CubicInput time);

void saveTrack(Track* t, uint8_t slot);
void loadTrack(Track* t, uint8_t slot, uint8_t channel);

#ifdef __cplusplus
} // extern "C"
#endif

#endif