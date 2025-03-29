#include "Callbacks.h"

typedef struct
{
    NoteList* start;
    NoteList* end;
    NoteList* current;
    uint32_t timeOut;
    uint32_t ct;
    uint8_t mode;
    uint8_t noteCount;
    bool freeCurrent;
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
    // remove from linked list
    NoteList* nE = nl->last;
    NoteList* nS = nl->next;
    
    if (a->end == nl)
    {
        a->end = nE;
    }
    if (a->start == nl)
    {
        a->start = nS;
    }
    
    if (nE)
    {
        nE->next = nS;
    }
    if (nS)
    {
        nS->last = nE;
    }
    
    a->noteCount--;
    free(nl);
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
    swap(i, previous, nextN);
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
    }
}

void initArps()
{
    arps[0] = { nullptr, nullptr, nullptr, 500, 0, 0, false };
    arps[1] = { nullptr, nullptr, nullptr, 500, 0, 0, false };
    arps[2] = { nullptr, nullptr, nullptr, 500, 0, 0, false };
    arps[3] = { nullptr, nullptr, nullptr, 500, 0, 0, false };
    arps[4] = { nullptr, nullptr, nullptr, 500, 0, 0, false };
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
    }
}

void arpAddNote(uint8_t channel, Note n)
{
    // Add to link list
    NoteList* nl = CREATE(NoteList);
    nl->value = n;
    nl->index = -1;
    
    Arpeggio* a = &arps[channel];
    
    NoteList* end = a->end;
    if (!end)
    {
        a->start = nl;
    }
    else
    {
        end->next = nl;
        nl->last = end;
    }
    
    a->end = nl;
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