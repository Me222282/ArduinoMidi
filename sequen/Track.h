#ifndef __tracking
#define __tracking

#include "../core/Globals.h"
#include "Cubic.h"

typedef struct
{
    Note* notes;
    uint16_t* mods;
    Cubic cub;
    
    uint8_t size;
    uint8_t position;
    bool playing;
    bool useMod;
} Track;

Track* createTrack();
void deleteTrack(Track* t);

bool addTrackValue(Track* t, Note n, uint16_t m);
void finaliseTrack(Track* t);

#endif