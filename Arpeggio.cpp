#include <Arduino.h>
#include "Arpeggio.h"
#include "src/core/NVData.h"
// #include "src/notes/Channels.h"
#include "src/notes/Notes.h"
#include "Callbacks.h"
#include "MemLocations.h"
#include "src/menus/CCMenu.h"

Arpeggio arps[5];
uint32_t _tElapsed;
bool arpClocked = false;

typedef struct
{
    uint32_t timeOut;
    uint8_t mode;
    bool sort;
    bool halfTime;
} ArpSave;

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
    removeNoteList((NoteList**)a, nl);
    a->noteCount--;
}
void _triggerNextArp(uint8_t i, Arpeggio* a)
{
    NoteList* previous = a->current;
    if (!previous) { previous = a->start; }
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
    
    NoteList* rem = previous;
    if (a->halfTime || !a->current) { rem = nullptr; }
    a->current = nextN;
    onArp(i);
    swap(i, rem, nextN);
    
    if (a->freeCurrent)
    {
        _removeArpNote(a, previous);
        a->freeCurrent = false;
    }
}
uint32_t arpClockCount;
void clockedArp()
{
    uint32_t acc = arpClockCount;
    arpClockCount++;
    
    if (acc % 6 == 0)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            Arpeggio* a = &arps[i];
            if (!a->start) { continue; }
            _triggerNextArp(i, a);
        }
        return;
    }
    // half time
    if (acc % 3 == 0)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            Arpeggio* a = &arps[i];
            if (!a->current || !a->halfTime) { continue; }
            onNoteOff(i, a->current->value.key);
        }
        return;
    }
}
void invokeArps()
{
    if (arpClocked) { return; }
    
    // change in time
    uint32_t dt = _tElapsed;
    _tElapsed = millis();
    dt = _tElapsed - dt;
    
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
void resetArps()
{
    clearArps();
    arps[0] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    arps[1] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    arps[2] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    arps[3] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    arps[4] = { nullptr, nullptr, nullptr, 250, 0, 0, 0, false, true, false };
    
    // for (uint8_t i = 0; i < 5; i++)
    // {
    //     uint8_t si = i * 7;
    //     setSpaceValue(si, 0);
    //     setSpaceValue(si + ARP_TIMEOUT_B, 0);
    //     setSpaceValue(si + ARP_TIMEOUT_C, 0);
    //     setSpaceValue(si + ARP_TIMEOUT_D, 250);
    //     setSpaceValue(si + ARP_MODE, 0);
    //     setSpaceValue(si + ARP_SORT, true);
    //     setSpaceValue(si + ARP_HALFTIME, false);
    // }
}
void loadArps()
{
    openDataSpace(DataSpace::Arps, false);
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 7;
        ArpSave dat = getSpaceData<ArpSave>(si);
        arps[i] =
        {
            nullptr, nullptr, nullptr,
            dat.timeOut,
            0, dat.mode, 0, false, dat.sort, dat.halfTime
        };
    }
    
    closeDataSpace();
}
void saveArps()
{
    openDataSpace(DataSpace::Arps, true);
    
    for (uint8_t i = 0; i < 5; i++)
    {
        Arpeggio* a = &arps[i];
        uint8_t si = i * 7;
        setSpaceData<ArpSave>(si, {
            a->timeOut,
            a->mode,
            a->sort,
            a->halfTime
        });
    }
    
    commitSpace();
    closeDataSpace();
}
// void setArpTimeOut(uint8_t channel, uint32_t time)
// {
//     uint8_t si = channel * 7;
//     arps[channel].timeOut = time;
    
//     setSpaceValue(si, time >> 24);
//     setSpaceValue(si + ARP_TIMEOUT_B, (time >> 16) & 0xFF);
//     setSpaceValue(si + ARP_TIMEOUT_C, (time >> 8) & 0xFF);
//     setSpaceValue(si + ARP_TIMEOUT_D, time & 0xFF);
//     EEPROM.commit();
// }
// void setArpMode(uint8_t channel, uint8_t mode)
// {
//     uint8_t si = channel * 7;
//     arps[channel].mode = mode;
//     setSpaceValue(si + ARP_MODE, mode);
//     EEPROM.commit();
// }
// void setArpSort(uint8_t channel, bool sort)
// {
//     uint8_t si = channel * 7;
//     arps[channel].sort = sort;
//     setSpaceValue(si + ARP_SORT, sort);
//     EEPROM.commit();
// }
// void setArpHT(uint8_t channel, bool halfTime)
// {
//     uint8_t si = channel * 7;
//     arps[channel].halfTime = halfTime;
//     setSpaceValue(si + ARP_HALFTIME, halfTime);
//     EEPROM.commit();
// }

void arpAddNote(uint8_t channel, Note n)
{
    if (channel >= 5) { return; }
    // Add to link list
    NoteList* nl = CREATE(NoteList);
    nl->value = n;
    nl->index = -1;
    
    Arpeggio* a = &arps[channel];
    
    if (a->sort) { insertNoteList((NoteList**)a, nl); }
    else { appendNoteList((NoteList**)a, nl); }
    
    a->noteCount++;
    // if no current note
    if (!arpClocked && !a->current)
    {
        a->current = nl;
        // instant start
        a->ct = 0;
        swap(channel, nullptr, nl);
    }
}

void arpRemoveNote(uint8_t channel, uint8_t key)
{
    if (channel >= 5) { return; }
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