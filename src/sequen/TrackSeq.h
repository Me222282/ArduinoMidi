#ifndef __tqc
#define __tqc

#include "../core/Globals.h"
#include "Cubic.h"
#include "Track.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    Track* track;
    uint8_t count;
} TrackPart;
typedef struct
{
    TrackPart* tracks;
    // must exist if sequence in use
    Track* current;
    Cubic cub;
    uint32_t stepOffset;
    
    uint16_t size;
    Note lastNote;
    uint8_t position;
    uint8_t tPosition;
    
    uint8_t currentCount;
    uint8_t skip;
    uint8_t channel;
    uint8_t saveSlot;
    bool playing;
    bool oneShot;
    bool nextBar;
} TrackSequence;

void createSequence(TrackSequence* seq, TrackPart* tracks, uint8_t size, uint8_t channel);
void deleteSequence(TrackSequence* seq);

bool addTrackValue(TrackSequence* t, Note n, uint16_t m);
bool finaliseTrack(TrackSequence* t);

void triggerTrackSeq(TrackSequence* t, uint16_t playStep);
void modTrackSeq(TrackSequence* t, CubicInput time);

void nextTrack(TrackSequence* t);

bool saveSequence(TrackSequence* seq, uint8_t slot);
bool loadSequence(TrackSequence* seq, uint8_t slot, uint8_t channel);

bool flattenSequence(TrackSequence* seq);

#ifdef __cplusplus
} // extern "C"
#endif

#endif