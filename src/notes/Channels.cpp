#include <Arduino.h>
#include "Channels.h"

uint8_t _ss0[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t _ss1[5] = { 0x00, 0x00, 0x00, 0x01, 0x01 };
uint8_t _ss2[5] = { 0x00, 0x00, 0x01, 0x01, 0x02 };
uint8_t _ss3[5] = { 0x00, 0x00, 0x01, 0x02, 0x03 };
uint8_t _ss4[5] = { 0x00, 0x01, 0x02, 0x03, 0x04 };
uint8_t _ss5[5] = { 0x00, 0x00, 0x00, 0x10, 0x10 };
uint8_t _ss6[5] = { 0x00, 0x00, 0x01, 0x10, 0x11 };
uint8_t _ss7[5] = { 0x00, 0x01, 0x02, 0x10, 0x11 };
uint8_t _ss8[5] = { 0x00, 0x01, 0x02, 0x03, 0x10 };
uint8_t _ss9[5] = { 0x00, 0x00, 0x10, 0x10, 0x20 };
uint8_t _ss10[5] = { 0x00, 0x01, 0x10, 0x11, 0x20 };
uint8_t _ss11[5] = { 0x00, 0x01, 0x02, 0x10, 0x20 };
uint8_t _ss12[5] = { 0x00, 0x00, 0x10, 0x20, 0x30 };
uint8_t _ss13[5] = { 0x00, 0x01, 0x10, 0x20, 0x30 };
uint8_t _ss14[5] = { 0x00, 0x10, 0x20, 0x30, 0x40 };
uint8_t* _slotSettings[25] =
{
    // voice priority
    // // 1 Channel
    // _ss0, _ss1, _ss2, _ss3, _ss4,
    // // 2 Channels
    // _ss5, _ss6, _ss7, _ss8, _ss4,
    // // 3 Channels
    // _ss9, _ss10, _ss11, _ss8, _ss4,
    // // 4 Channels
    // _ss12, _ss13, _ss11, _ss8, _ss4,
    // // 5 Channels
    // _ss14, _ss13, _ss11, _ss8, _ss4
    // channel priority
    // 1 Channel
    _ss0, _ss1, _ss2, _ss3, _ss4,
    // 2 Channels
    _ss5, _ss6, _ss7, _ss8, _ss8,
    // 3 Channels
    _ss9, _ss10, _ss11, _ss11, _ss11,
    // 4 Channels
    _ss12, _ss13, _ss13, _ss13, _ss13,
    // 5 Channels
    _ss14, _ss14, _ss14, _ss14, _ss14
};

uint8_t _ss15[5] = { 0x00, 0x01, 0x00, 0x01, 0x00 };
uint8_t _ss16[5] = { 0x00, 0x01, 0x02, 0x00, 0x01 };
uint8_t _ss17[5] = { 0x00, 0x01, 0x02, 0x03, 0x00 };
uint8_t _ss18[5] = { 0x00, 0x01, 0x00, 0x10, 0x11 };

Channel channels[5];
uint8_t* slotAllocation;

bool allChannelMode = false;
bool altAllocations = false;

void initChannel(Channel* c, uint8_t voices, uint8_t index, uint8_t places);

void setChannels(uint8_t c, uint8_t v)
{
    slotAllocation = _slotSettings[((c - 1) * 5) + v - 1];
    
    if (altAllocations)
    {
        if (c == 0)
        {
            if (v == 1)             { slotAllocation = _ss15; }
            else if (v == 2)        { slotAllocation = _ss16; }
            else if (v == 3)        { slotAllocation = _ss17; }
        }
        else if (c == 1 && v == 1)  { slotAllocation = _ss18; }
    }
    
    uint8_t c0 = 0;
    uint8_t v0 = 0;
    uint8_t p0 = 0;
    uint8_t li = 0;
    for (int i = 0; i < 5; i++)
    {
        clearChannel(&channels[i]);
        
        uint8_t sa = slotAllocation[i];
        uint8_t chI = sa >> 4;
        uint8_t vI = sa & 0b00001111;
        
        // init 0 when chI == 1, init 1 when chI == 2, etc
        if (chI != c0)
        {
            // init previous channel
            // v0 is auto counted
            initChannel(&channels[c0], v0 + 1, li, p0);
            // this index is the start of the next channel
            li = i;
            p0 = 0;
        }
        c0 = chI;
        v0 = vI;
        p0++;
    }
    // init last channel
    initChannel(&channels[c0], v0 + 1, li, p0);
}

void initChannel(Channel* c, uint8_t voices, uint8_t index, uint8_t places)
{
    // will be null
    // c->noteStart = nullptr;
    // c->noteEnd = nullptr;
    c->locations = CREATE_ARRAY(NoteList*, voices);
    c->voices = voices;
    c->pitchBend = 0x2000;
    c->modulation = 0;
    c->position = index;
    c->places = places;
    
    c->history = allocateHistory(voices);
}
void clearNotes(Channel* c)
{
    if (c->noteStart)
    {
        // free by last so we still have index to next
        for (NoteList* nl = c->noteStart->next; nl; nl = nl->next)
        {
            free(nl->last);
        }
        free(c->noteEnd);
        c->noteStart = nullptr;
        c->noteEnd = nullptr;
        c->noteCount = 0;
    }
    
    clearHistory(&c->history);
    c->history = allocateHistory(c->voices);
}
void clearChannel(Channel* c)
{
    clearNotes(c);
    if (c->locations)
    {
        free(c->locations);
        c->locations = nullptr;
    }
    c->position = 0;
    c->places = 0;
    c->voices = 0;
}

void gateChannelNote(uint8_t ci, uint8_t vi, bool v)
{
    uint8_t com = (ci << 4) | vi;
    
    for (uint8_t i = 0; i < 5; i++)
    {
        if (slotAllocation[i] == com)
        {
            setGateNote(i, v);
        }
    }
}
void reTrigChannelNote(uint8_t ci, uint8_t vi)
{
    uint8_t com = (ci << 4) | vi;
    
    for (uint8_t i = 0; i < 5; i++)
    {
        if (slotAllocation[i] == com)
        {
            reTrigNote(i);
        }
    }
}

void appendNoteList(NoteList** list, NoteList* nl)
{
    NoteList* end = list[1];
    if (!end)
    {
        list[0] = nl;
    }
    else
    {
        end->next = nl;
        nl->last = end;
    }
    list[1] = nl;
}
void insertNoteList(NoteList** list, NoteList* nl)
{
    NoteList* start = list[0];
    uint8_t key = nl->value.key;
    
    // add in order
    NoteList* ins;
    for (ins = start; ins && ins->value.key < key; ins = ins->next) { }
    // none found - add to end like normal
    if (!ins)
    {
        appendNoteList(list, nl);
        return;
    }
    // insert into list before ins
    nl->last = ins->last;
    nl->next = ins;
    ins->last = nl;
    if (nl->last) { nl->last->next = nl; }
    else { list[0] = nl; }
}
void removeNoteList(NoteList** list, NoteList* nl)
{
    NoteList* nE = nl->last;
    NoteList* nS = nl->next;
    
    if (list[1] == nl)
    {
        list[1] = nE;
    }
    if (list[0] == nl)
    {
        list[0] = nS;
    }
    
    if (nE)
    {
        nE->next = nS;
    }
    if (nS)
    {
        nS->last = nE;
    }
    
    free(nl);
}