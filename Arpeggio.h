#ifndef __arpeg
#define __arpeg

#include "Callbacks.h"

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

Arpeggio arps[5];
uint32_t tElapsed;

void swap(uint8_t channel, NoteList* out, NoteList* in)
{
    if (in)
    {
        Note n = in->value;
        onNoteOn(channel, n.key, n.vel);
    }
    if (out)
    {
        onNoteOff(channel, out->value.key);
    }
}

void _removeArpNote(Arpeggio* a, NoteList* nl)
{
    removeNote((NoteList**)a, nl);
    a->noteCount--;
}
void _triggerNextArp(uint8_t i, Arpeggio* a)
{
    NoteList* previous = a->current;
    NoteList* nextN;
    
    switch (a->mode)
    {
        case 0:
            nextN = previous->next;
            if (!nextN) { nextN = a->start; }
            break;
        case 1:
            nextN = previous->last;
            if (!nextN) { nextN = a->end; }
            break;
        // 2 and 3 together
        case 2:
            nextN = previous->next;
            if (!nextN)
            {
                nextN = previous->last;
                a->mode = 3;
            }
            break;
        case 3:
            nextN = previous->last;
            if (!nextN)
            {
                nextN = previous->next;
                a->mode = 2;
            }
            break;
    }
    // retrigger same note if theres only 1
    if (a->noteCount == 1 && a->freeCurrent) { nextN = nullptr; }
    // should not be called if freeCurrent && noteCount > 1
    else if (!nextN) { nextN = previous; }
    a->current = nextN;
    
    NoteList* rem = previous;
    if (a->halfTime) { rem = nullptr; }
    swap(i, rem, nextN);
    
    if (a->freeCurrent)
    {
        _removeArpNote(a, previous);
        a->freeCurrent = false;
    }
}
void invokeArps()
{
    // change in time
    uint32_t dt = tElapsed;
    tElapsed = millis();
    dt = tElapsed - dt;
    
    for (uint8_t i = 0; i < 5; i++)
    {
        Arpeggio* a = &arps[i];
        if (!a->start) { continue; }
        // there must be a current note here
        a->ct += dt;
        if (a->ct >= a->timeOut)
        {
            a->ct -= a->timeOut;
            // trigger
            _triggerNextArp(i, a);
            continue;
        }
        // turn off at half time
        if (a->halfTime && ((a->ct * 2) >= a->timeOut))
        {
            onNoteOff(i, a->current->value.key);
        }
    }
}

void clearArp(uint8_t channel)
{
    Arpeggio* a = &arps[channel];
    
    if (a->start)
    {
        // free by last so we still have index to next
        for (NoteList* nl = a->start->next; nl; nl = nl->next)
        {
            free(nl->last);
        }
        free(a->end);
        a->start = nullptr;
        a->current = nullptr;
        a->end = nullptr;
        a->noteCount = 0;
    }
}
void clearArps()
{
    clearArp(0);
    clearArp(1);
    clearArp(2);
    clearArp(3);
    clearArp(4);
}
void initArps()
{
    clearArps();
    arps[0] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    arps[1] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    arps[2] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    arps[3] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    arps[4] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 7;
        eeWrite(si, 0);
        eeWrite(si + 1, 0);
        eeWrite(si + 2, 0);
        eeWrite(si + 3, 250);
        eeWrite(si + 4, 0);
        eeWrite(si + 5, true);
        eeWrite(si + 6, false);
    }
}
void loadArps()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 7;
        uint32_t timeA = EEPROM.read(si) << 24;
        uint32_t timeB = EEPROM.read(si + 1) << 16;
        uint32_t timeC = EEPROM.read(si + 2) << 8;
        uint32_t timeD = EEPROM.read(si + 3);
        uint8_t mode = EEPROM.read(si + 4);
        uint8_t sort = EEPROM.read(si + 5);
        uint8_t ht = EEPROM.read(si + 6);
        arps[i] =
        {
            nullptr, nullptr, nullptr,
            timeA + timeB + timeC + timeD,
            0, mode, 0, false, sort, ht
        };
    }
}
void setArpTimeOut(uint8_t channel, uint32_t time)
{
    uint8_t si = channel * 7;
    arps[channel].timeOut = time;
    
    eeWrite(si, time >> 24);
    eeWrite(si + 1, (time >> 16) & 0xFF);
    eeWrite(si + 2, (time >> 8) & 0xFF);
    eeWrite(si + 3, time & 0xFF);
    EEPROM.commit();
}
void setArpMode(uint8_t channel, uint8_t mode)
{
    uint8_t si = channel * 7;
    arps[channel].mode = mode;
    eeWrite(si + 4, mode);
    EEPROM.commit();
}
void setArpSort(uint8_t channel, bool sort)
{
    uint8_t si = channel * 7;
    arps[channel].sort = sort;
    eeWrite(si + 5, sort);
    EEPROM.commit();
}
void setArpHT(uint8_t channel, bool halfTime)
{
    uint8_t si = channel * 7;
    arps[channel].halfTime = halfTime;
    eeWrite(si + 6, halfTime);
    EEPROM.commit();
}

void arpAddNote(uint8_t channel, Note n)
{
    // Add to link list
    NoteList* nl = CREATE(NoteList);
    nl->value = n;
    nl->index = -1;
    
    Arpeggio* a = &arps[channel];
    
    if (a->sort) { insertNote((NoteList**)a, nl); }
    else { appendNote((NoteList**)a, nl); }
    
    a->noteCount++;
    // if no current note
    if (!a->current)
    {
        a->current = nl;
        // instant start
        a->ct = 0;
        swap(channel, nullptr, nl);
    }
}

void arpRemoveNote(uint8_t channel, uint8_t key)
{
    Arpeggio* a = &arps[channel];
    
    for (NoteList* nl = a->start; nl; nl = nl->next)
    {
        if (nl->value.key != key) { continue; }
        if (a->current == nl)
        {
            a->freeCurrent = true;
            return;
        }
        
        _removeArpNote(a, nl);
        return;
    }
}

#endif