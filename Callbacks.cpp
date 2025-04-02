#include "Callbacks.h"
#include "notes/Notes.h"
#include "notes/Channels.h"
#include "core/Coms.h"
#include "core/Globals.h"

void _updateSlot(uint8_t com, Note n)
{
    n.key += (octaveOffset * 12);
    n.vel <<= 1;
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        if (v != com) { continue; }
        
        Note old = oldValues[i];
        if (old.key != n.key)
        {
            setNote(i, n.key);
        }
        if (!option && old.vel != n.vel)
        {
            setVel(i, n.vel);
        }
        oldValues[i] = n;
    }
    
    // other velocity
    if (option && com == 0)
    {
        // 8bit to 12bit
        uint16_t vel = n.vel << 4;
        if (oldMod != vel)
        {
            oldMod = vel;
            setMod(vel);
        }
    }
}
bool alwaysDelay = false;
bool filterKeys = false;
Notes filter = Notes::C;
void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (channel >= activeChannels ||
        // key filter
        (filter && notInKey(note, filter))) { return; }
    
    Channel* c = &channels[channel];
    Note n = { note, velocity };
    int8_t vi = pushNote(c, channel, n);
    
    if (vi < 0) { return; }
    
    // set data
    if (reTrig)
    {
        // turn off retriggered notes
        setGate(gateCurrent & ~reTrig);
        reTrig = 0;
        delay(RETRIGTIME);
    }
    else if (alwaysDelay)
    {
        delay(RETRIGTIME);
    }
    
    uint8_t com = (channel << 4) | (uint8_t)vi;

    _updateSlot(com, n);
    
    // gate
    setGate(gate);
}
void onNoteOff(uint8_t channel, uint8_t note)
{
    if (channel >= activeChannels) { return; }
    Channel* c = &channels[channel];
    int8_t vi = removeNote(c, channel, note);
    
    if (reTrig)
    {
        // turn off retriggered notes
        setGate(gateCurrent & ~reTrig);
        reTrig = 0;
        delay(RETRIGTIME);
    }
    
    if (vi >= 0)
    {
        uint8_t com = (channel << 4) | (uint8_t)vi;
        Note n = c->locations[vi]->value;
        _updateSlot(com, n);
    }
    
    // gate
    setGate(gate);
}
void onPitchBend(uint8_t channel, uint16_t bend)
{
    if (channel >= activeChannels) { return; }
    Channel* c = &channels[channel];
    
    c->pitchBend = bend;
    
    // set data
    uint8_t end = c->position + c->places;
    for (uint8_t i = c->position; i < end; i++)
    {
        setPitchBend(i, bend >> 2);
    }
}
void _updateMod(Channel* c, uint16_t mod)
{
    // set data
    // goes to vel
    if (option)
    {
        mod >>= 6;
        
        uint8_t end = c->position + c->places;
        for (uint8_t i = c->position; i < end; i++)
        {
            if (oldValues[i].vel == mod) { continue; }
            oldValues[i].vel = mod;
            setVel(i, mod);
        }
        return;
    }
    
    if (c->position != 0) { return; }
    // goes to mod
    mod >>= 2;
    if (oldMod == mod) { return; }
    oldMod = mod;
    setMod(mod);
}
void _gateOff(Channel* c)
{
    uint8_t end = c->position + c->places;
    for (uint8_t i = c->position; i < end; i++)
    {
        setGateNote(i, false);
    }
    setGate(gate);
}
void onControlChange(uint8_t channel, CCType number, uint8_t value)
{
    // cc 01 is mod wheel
    if (channel >= activeChannels) { return; }
    Channel* c = &channels[channel];
    
    if (number == CCType::AllNotesOff)
    {
        clearNotes(c);
        _gateOff(c);
        return;
    }
    
    uint16_t mod = c->modulation;
    if (number == CCType::ModulationWheel_MSB)
    {
        c->modulation = (mod & 0x007F) | (value << 7);
        _updateMod(c, mod);
        return;
    }
    if (number != CCType::ModulationWheel_LSB) { return; }
    // fine mod control
    c->modulation = (mod & 0x3F80) | value;
    _updateMod(c, mod);
}
void updateAllNotes()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        uint8_t chI = v >> 4;
        uint8_t vI = v & 0b00001111;
        
        Channel* c = &channels[chI];
        NoteList* nl = c->locations[vI];
        if (!nl) { continue; }
        uint8_t n = nl->value.key;
        n += (octaveOffset * 12);
        uint8_t old = oldValues[i].key;
        // changes to make
        if (old != n)
        {
            setNote(i, n);
        }
        oldValues[i].key = n;
    }
}
void updateAllPBs()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        uint8_t chI = v >> 4;
        uint8_t vI = v & 0b00001111;
        
        Channel* c = &channels[chI];
        
        setPitchBend(i, c->pitchBend >> 2);
    }
}
void updateOtherPorts()
{
    Channel* c = &channels[0];
    
    if (option)
    {
        for (uint8_t i = 0; i < activeChannels; i++)
        {
            Channel* c = &channels[i];
            _updateMod(c, c->modulation);
        }
        
        NoteList* nl = c->locations[0];
        if (!nl) { return; }
        
        // 7bit to 12bit
        uint16_t vel = nl->value.vel << 4;
        if (oldMod != vel)
        {
            oldMod = vel;
            setMod(vel);
        }
        return;
    }
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        uint8_t chI = v >> 4;
        uint8_t vI = v & 0b00001111;
        
        Channel* c = &channels[chI];
        NoteList* nl = c->locations[vI];
        if (!nl) { continue; }
        uint16_t vel = nl->value.vel << 1;
        if (oldValues[i].vel != vel)
        {
            setVel(i, vel);
        }
    }
    
    _updateMod(c, c->modulation);
}