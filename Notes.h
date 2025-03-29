#ifndef __notes
#define __notes

#include "Channels.h"
#include "Globals.h"

int8_t _findNextIndex(Channel* c);
NoteList* _getNextNote(Channel* c);
NoteList* _getLosableNote(Channel* c);

bool retriggerNew = true;
bool retriggerOld = false;

void _add(Channel* c, NoteList* nl)
{
    NoteList* end = c->noteEnd;
    if (!end)
    {
        c->noteStart = nl;
    }
    else
    {
        end->next = nl;
        nl->last = end;
    }
    
    c->noteCount++;
    c->noteEnd = nl;
}
int8_t pushNote(Channel* c, uint8_t chI, Note n)
{
    // Add to link list
    NoteList* nl = CREATE(NoteList);
    // // out of memory
    // if (!nl) { return -1; }
    nl->value = n;
    nl->index = -1;
    
    int8_t hole = _findNextIndex(c);
    // not all slots are filled yet
    if (hole >= 0)
    {
        // add to list after slot chosen
        _add(c, nl);
        nl->index = hole;
        c->locations[hole] = nl;
        // trigger note
        gateChannelNote(chI, hole, true);
        return hole;
    }
    
    // take a use slot
    NoteList* take = _getLosableNote(c);
    // add to list after slot chosen
    _add(c, nl);
    if (take == nullptr) { return -1; }
    
    hole = take->index;
    take->index = -1;
    nl->index = hole;
    c->locations[hole] = nl;
    if (retriggerNew)
    {
        // retrigger note
        reTrigChannelNote(chI, hole);
    }
    return hole;
}
void _remove(Channel* c, NoteList* nl)
{
    NoteList* nE = nl->last;
    NoteList* nS = nl->next;
    
    if (c->noteEnd == nl)
    {
        c->noteEnd = nE;
    }
    if (c->noteStart == nl)
    {
        c->noteStart = nS;
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
    c->noteCount--;
}
int8_t removeNote(Channel* c, uint8_t chI, uint8_t key)
{
    NoteList* nl;
    // prioritises removing old notes first - important for arpeggios
    for (nl = c->noteStart; nl; nl = nl->next)
    {
        if (nl->value.key != key) { continue; }
        break;
    }
    
    // could not find note
    if (!nl) { return -1; }
    
    int8_t hole = nl->index;
    _remove(c, nl);
    // DONT use nl;
    
    // not a note being displayed
    if (hole < 0) { return -1; }
    
    // fill hole if can
    NoteList* replace = _getNextNote(c);
    // no more
    if (replace == nullptr)
    {
        c->locations[hole] = nullptr;
        if (!mode1)
        {
            // loop mode slot history
            pushHistory(&c->history, hole);
        }
        // no more note
        gateChannelNote(chI, hole, false);
        return -1;
    }
    replace->index = hole;
    c->locations[hole] = replace;
    if (retriggerOld)
    {
        // retrigger note
        reTrigChannelNote(chI, hole);
    }
    return hole;
}

// finding next open slot
int8_t _findNextIndex(Channel* c)
{
    if (!mode1)
    {
        return pullHistory(&c->history);
    }
    
    NoteList** list = c->locations;
    uint8_t size = c->voices;
    for (int8_t i = 0; i < size; i++)
    {
        NoteList* nl = list[i];
        if (nl) { continue; }
         
        return i;
    }
    
    return -1;
}
// for when slot opens - the note that should fill it again
NoteList* _getNextNote(Channel* c)
{
    // all notes in use
    if (c->noteCount < c->voices)
    {
        return nullptr;
    }
    
    NoteList* nl;
    
    if (mode2 == 1)
    {
        // for newest note priority
        for (nl = c->noteEnd; nl; nl = nl->last)
        {
            if (nl->index < 0) { return nl; }
        }
        
        return nullptr;
    }
    // take 1st
    if (mode2 == 2)
    {
        nl = c->noteEnd;
        if (nl->index < 0)
        {
            return nl;
        }
    }
        
    // for ignore new
    for (nl = c->noteStart; nl; nl = nl->next)
    {
        if (nl->index < 0) { return nl; }
    }
    
    return nullptr;
}
// for taking a used slot
NoteList* _getLosableNote(Channel* c)
{   
    if (mode2 == 0) { return nullptr; }
    
    // for newest note priority
    if (mode2 == 1)
    {
        NoteList* nl;
        
        // start from bottom of stack
        if (c->noteCount < (c->voices * 2))
        {
            for (nl = c->noteStart; nl; nl = nl->next)
            {
                if (nl->index >= 0) { return nl; }
            }
            
            return nullptr;
        }
        
        // start from top of stack
        for (nl = c->noteEnd; nl; nl = nl->last)
        {
            if (nl->index < 0) { return nl->next; }
        }
        
        return nullptr;
    }
    // mode 2 - take most recent
    return c->noteEnd;
}

#endif