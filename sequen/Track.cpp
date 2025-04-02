#include "Track.h"
#include "../core/Globals.h"

Track* createTrack()
{
    Track* t = CREATE(Track);
    t->notes = CREATE_ARRAY(Track, 256);
    t->mods = CREATE_ARRAY(uint16_t, 256);
    t->size = 256;
    
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
    t->mods = m;
    t->position = i + 1;
    return i == 255;
}
void finaliseTrack(Track* t)
{
    uint8_t size = t->position;
    if (size == 0) { return; }
    
    t->notes = realloc(t->notes, size);
    t->mods = realloc(t->mods, size);
    t->position = 0;
}