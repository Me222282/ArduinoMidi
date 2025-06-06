#ifndef __chan_chan
#define __chan_chan

#include "../core/Globals.h"
#include "History.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _noteList
{
    _noteList* next;
    _noteList* last;
    Note value;
    int8_t index;
} NoteList;
typedef struct
{
    // dont change order
    // list is ordered by start time (oldest - newest)
    NoteList* noteStart;
    NoteList* noteEnd;
    NoteList** locations;
    // stored as bitwise not of values
    uint8_t* oldNotes;
    History history;
    
    uint16_t pitchBend;
    uint16_t modulation;
    
    uint8_t noteCount;
    uint8_t voices;
    uint8_t places;
    int8_t position;
} Channel;

extern Channel channels[5];
extern uint8_t* slotAllocation;

extern bool allChannelMode;
extern bool altAllocations;

void setChannels(uint8_t c, uint8_t v);

// void initChannel(Channel* c, uint8_t voices, uint8_t index, uint8_t places);
void clearNotes(Channel* c);
void clearChannel(Channel* c);

void gateChannelNote(uint8_t ci, uint8_t vi, bool v);
void reTrigChannelNote(uint8_t ci, uint8_t vi);

void appendNoteList(NoteList** list, NoteList* nl);
void insertNoteList(NoteList** list, NoteList* nl);
void removeNoteList(NoteList** list, NoteList* nl);

#ifdef __cplusplus
} // extern "C"
#endif

#endif