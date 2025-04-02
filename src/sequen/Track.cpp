#include <Arduino.h>
#include "Track.h"
#include "../core/Globals.h"

Track* createTrack(uint8_t channel)
{
    Track* t = CREATE(Track);
    t->notes = CREATE_ARRAY(Note, 256);
    t->mods = CREATE_ARRAY(uint16_t, 256);
    t->size = 256;
    t->channel = channel;
    
    return t;
}
void deleteTrack(Track* t)
{
    if (!t) { return; }
    free(t->notes);
    free(t->mods);
    free(t);
}

bool addTrackValue(Track* t, Note n, uint16_t m)
{
    uint8_t i = t->position;
    t->notes[i] = n;
    t->mods[i] = m;
    t->position = i + 1;
    return i == 255;
}
void finaliseTrack(Track* t)
{
    uint8_t size = t->position;
    if (size == 0) { return; }
    
    t->notes = (Note*)realloc(t->notes, size);
    t->mods = (uint16_t*)realloc(t->mods, size);
    t->position = 0;
}