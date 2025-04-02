#ifndef __arpeg
#define __arpeg

#include "src/core/Globals.h"
#include "src/notes/Channels.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    // dont change order
    NoteList* start;
    NoteList* end;
    NoteList* current;
    uint32_t timeOut;
    uint32_t ct;
    uint8_t mode;
    uint8_t noteCount;
    bool freeCurrent;
    bool sort;
    bool halfTime;
} Arpeggio;

extern Arpeggio arps[5];
extern bool arpClocked;

void clockedArp();
void invokeArps();

void clearArp(uint8_t channel);
void clearArps();
void initArps();
void loadArps();
void setArpTimeOut(uint8_t channel, uint32_t time);
void setArpMode(uint8_t channel, uint8_t mode);
void setArpSort(uint8_t channel, bool sort);
void setArpHT(uint8_t channel, bool halfTime);

void arpAddNote(uint8_t channel, Note n);

void arpRemoveNote(uint8_t channel, uint8_t key);

#ifdef __cplusplus
} // extern "C"
#endif

#endif