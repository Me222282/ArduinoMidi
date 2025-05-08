#include <Arduino.h>
#include "Callbacks.h"
#include "src/notes/Notes.h"
#include "src/notes/Channels.h"
#include "src/core/Coms.h"
#include "src/core/Globals.h"

void _updateSlot(uint8_t com, Note n)
{
    n.vel <<= 1;
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        if (v != com) { continue; }
        
        setNote(i, n.key);
        if (!option) { setVel(i, n.vel); }
    }
    
    // other velocity
    if (option && com == 0)
    {
        // 8bit to 12bit
        uint16_t vel = n.vel << 4;
        setMod(vel);
    }
}
uint8_t ccListeners[5];

bool alwaysDelay = false;
bool filterKeys = false;
Notes filter = Notes::C;

int8_t noteOffsets[5];
void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (channel < 5) { note += noteOffsets[channel]; }
    
    // key filter
    if (filterKeys && notInKey((NoteName)note, filter)) { return; }
    if (allChannelMode)
    {
        channel %= activeChannels;
    }
    else if (channel >= activeChannels) { return; }
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
    if (channel < 5) { note += noteOffsets[channel]; }
    
    if (allChannelMode)
    {
        channel %= activeChannels;
    }
    else if (channel >= activeChannels) { return; }
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
    if (allChannelMode)
    {
        channel %= activeChannels;
    }
    else if (channel >= activeChannels) { return; }
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
            setVel(i, mod);
        }
        return;
    }
    
    // cc 01 is mod wheel
    if (c->position != 0) { return; }
    // goes to mod
    mod >>= 2;
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

void ccUpdate(Channel* c, CCType number, uint8_t value, uint8_t channel)
{
    uint8_t offset = 0;
    if (ccListeners[0] == number)       { offset = 0; }
    else if (ccListeners[1] == number)  { offset = 1; }
    else if (ccListeners[2] == number)  { offset = 2; }
    else if (ccListeners[3] == number)  { offset = 3; }
    else if (ccListeners[4] == number)  { offset = 4; }
    else { return; }
    
    if (!ccChannelMode)
    {
        if (channel != 0) { return; }
        setCCOut(offset, value);
        return;
    }
    
    if (offset >= c->places || !ccOutputs[channel]) { return; }
    setCCOut(offset + c->position, value);
}
void onControlChange(uint8_t channel, CCType number, uint8_t value)
{
    if (allChannelMode)
    {
        channel %= activeChannels;
    }
    else if (channel >= activeChannels) { return; }
    Channel* c = &channels[channel];
    
    ccUpdate(c, number, value, channel);
    
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
        // changes to make
        setNote(i, n);
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
        setMod(vel);
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
        setVel(i, vel);
    }
    
    _updateMod(c, c->modulation);
}