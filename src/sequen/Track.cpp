#include <Arduino.h>
#include "Track.h"
#include "../core/Globals.h"
#include "../notes/Channels.h"
#include "../notes/Notes.h"
#include "../../Callbacks.h"

#define NVMADDRESS 0x290000
const uint32_t nvmLocations[16]
{
    0, 1034, 2068, 3102, 4136, 5170, 6204, 7238,
    8272, 9306, 10340, 11374, 12408, 13442, 14476, 15510
};

void createTrack(Track* t, uint8_t channel)
{
    t->notes = CREATE_ARRAY(Note, 256);
    t->mods = CREATE_ARRAY(uint16_t, 256);
    t->cub = { 0, 0, 0, 0 };
    t->size = 256;
    t->channel = channel;
    t->position = 0;
    t->useMod = false;
    t->playing = false;
    t->clockDivision = 1;
    t->halfTime = false;
}
void deleteTrack(Track* t)
{
    if (t->notes) { free(t->notes); }
    if (t->mods) { free(t->mods); }
    t->size = 0;
}

bool addTrackValue(Track* t, Note n, uint16_t m)
{
    uint8_t i = t->position;
    t->notes[i] = n;
    t->mods[i] = m;
    t->position = i + 1;
    return i == 255;
}
bool finaliseTrack(Track* t)
{
    uint8_t size = t->position;
    if (size == 0) { return true; }
    if (size < 4) { return false; }
    
    t->notes = (Note*)realloc(t->notes, size);
    t->mods = (uint16_t*)realloc(t->mods, size);
    t->position = 0;
    t->playing = true;
    return true;
}

void noteOn(Track* t, uint8_t channel)
{
    uint8_t pos = t->position;
    Note n = t->notes[pos];
    if (noteEquals(n, NOTEHOLD)) { return; }
    if (!noteEquals(n, NOTEOFF))
    {
        onNoteOn(channel, n.key, n.vel);
    }
    if (t->halfTime) { return; }
    if (pos == 0)
    {
        onNoteOff(channel, t->notes[t->size - 1].key);
        return;
    }
    onNoteOff(channel, t->notes[pos - 1].key);
}

uint16_t wrapMods(Track* t, uint8_t pos, uint16_t size)
{
    if (pos >= size)
    {
        return t->mods[pos - size];
    }
    
    return t->mods[pos];
}
Note wrapNote(Track* t, uint8_t pos)
{
    uint8_t size = t->size;
    if (pos >= size)
    {
        return t->notes[pos - size];
    }
    
    return t->notes[pos];
}
// track size must be >= 4
void newCubic(Track* t)
{
    uint8_t pos = t->position;
    uint16_t size = t->size;
    
    Cubic c = generateCubic(
        wrapMods(t, pos, size),
        wrapMods(t, pos + 1, size),
        wrapMods(t, pos + 2, size),
        wrapMods(t, pos + 3, size));
    
    t->cub = c;
}

void triggerTrack(Track* t, uint8_t channel, uint16_t playStep)
{
    if (!t->playing) { return; }
    if (playStep & 1U)
    {
        uint8_t cPos = t->position;
        if (t->halfTime && !noteEquals(wrapNote(t, cPos + 1), NOTEHOLD))
        {
            onNoteOff(channel, t->notes[t->position].key);
        }
        return;
    }
    
    uint16_t div = playStep % t->clockDivision;
    if (div) { return; }
    // div == 0
    
    noteOn(t, channel);
    newCubic(t);
    uint8_t pos = t->position + 1;
    if (pos >= t->size) { pos = 0; }
    t->position = pos;
}

void modTrack(Track* t, uint8_t channel, CubicInput time)
{
    if (!t->useMod) { return; }
    float m = getCubicValue(t->cub, time);
    uint16_t act = (uint16_t)m;
    // set mod;
    onControlChange(channel, CCType::ModulationWheel_MSB, act >> 7);
    onControlChange(channel, CCType::ModulationWheel_LSB, act & 0x7F);
}

template<typename T>
void FlashWrite(uint32_t address, T value)
{
    ESP.flashEraseSector(address >> 12);
    ESP.flashWrite(address, (uint32_t*)&value, sizeof(T));
}
template<typename T>
void FlashWrite(uint32_t address, const T* value, uint16_t size)
{
    ESP.flashEraseSector(address >> 12);
    ESP.flashWrite(address, (uint32_t*)&value, sizeof(T) * size);
}
template<typename T>
T FlashRead(uint32_t address)
{
    T value;
    ESP.flashRead(address, (uint32_t*)&value, sizeof(T));
    return value;
}
template<typename T>
T* FlashRead(uint32_t address, uint16_t size)
{
    T* values = CREATE_ARRAY(T, size);
    ESP.flashRead(address, (uint32_t*)values, sizeof(T) * size);
    return values;
}

void saveTrack(Track* t, uint8_t slot)
{
    uint32_t address = NVMADDRESS + nvmLocations[slot];
    
    uint16_t size = t->size;
    FlashWrite<uint8_t>(address, t->clockDivision);
    FlashWrite<bool>(address + 1, t->useMod);
    FlashWrite<bool>(address + 2, t->halfTime);
    FlashWrite<uint16_t>(address + 3, size);
    FlashWrite<Note>(address + 4, t->notes, size);
    if (t->useMod)
    {
        FlashWrite<uint16_t>(address + 4 + 512, t->mods, size);
    }
}
void loadTrack(Track* t, uint8_t slot, uint8_t channel)
{
    uint32_t address = NVMADDRESS + nvmLocations[slot];
    
    t->position = 0;
    t->playing = true;
    t->channel = channel;
    
    uint16_t size = t->size;
    t->clockDivision = FlashRead<uint8_t>(address);
    t->useMod = FlashRead<bool>(address + 1);
    t->halfTime = FlashRead<bool>(address + 2);
    uint16_t size = FlashRead<uint16_t>(address + 3);
    t->size = size;
    t->notes = FlashRead<Note>(address + 4, size);
    if (t->useMod)
    {
        t->mods = FlashRead<uint16_t>(address + 4 + 512, size);
    }
}