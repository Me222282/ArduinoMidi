#include <Arduino.h>
#include "Track.h"
#include "../core/Globals.h"
#include "../notes/Channels.h"
#include "../notes/Notes.h"
#include "../../Callbacks.h"
#include "../../MemLocations.h"
#include <EEPROM.h>

#define NVMADDRESS 0x290000

void createTrack(Track* t)
{
    t->notes = CREATE_ARRAY(Note, 256);
    t->mods = CREATE_ARRAY(uint16_t, 256);
    t->size = 256;
    t->saveSlot = 255;
    t->useMod = false;
    t->playing = true;
    t->clockDivision = 1;
    t->halfTime = false;
}

void deleteTrack(Track* t)
{
    if (!t) { return; }
    if (t->notes) { free(t->notes); t->notes = nullptr; }
    if (t->mods) { free(t->mods); t->mods = nullptr; }
    if (!t->memBank)
    {
        free(t);
        return;
    }
    t->size = 0;
    t->saveSlot = 255;
}
void freeTrack(Track* t)
{
    if (!t || t->memBank) { return; }
    if (t->notes) { free(t->notes); t->notes = nullptr; }
    if (t->mods) { free(t->mods); t->mods = nullptr; }
    free(t);
}

Track memoryBank[32] = {
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true },
    { nullptr, nullptr, 0, 0xFF, 1, true, false, false, true }
};

Track* loadMemBank(uint8_t slot)
{
    return &memoryBank[slot];
}

void removeOldSlot(uint8_t slot)
{
    for (uint8_t i = 0; i < 32; i++)
    {
        if (memoryBank[i].saveSlot == slot)
        {
            memoryBank[i].saveSlot = 255;
            return;
        }
    }
    
    pergeSlot(slot);
}

void saveTrack(Track* t, uint8_t slot)
{
    uint32_t address = NVMADDRESS + (4096 * (uint32_t)slot);
    uint8_t si = TRACKS_SLOT_1 + (4 * slot);
    
    uint16_t size = t->size;
    eeWrite(si, t->clockDivision);
    eeWrite(si + TRACK_USEMOD, t->useMod);
    eeWrite(si + TRACK_HALFTIME, t->halfTime);
    eeWrite(si + TRACK_SIZE, size & 0xFF);
    EEPROM.commit();
    
    // data already written
    if (t->saveSlot == slot) { return; }
    // find track that was saved here
    removeOldSlot(slot);
    t->saveSlot = slot;
    
    ESP.flashEraseSector(address >> 12);
    FlashWrite<Note>(address, t->notes, size);
    if (t->useMod)
    {
        FlashWrite<uint16_t>(address + 512, t->mods, size);
    }
}
void deleteSave(uint8_t slot)
{
    uint8_t si = TRACKS_SLOT_1 + (4 * slot);
    
    eeWrite(si, 1);
    eeWrite(si + TRACK_USEMOD, 0);
    eeWrite(si + TRACK_HALFTIME, 0);
    // size of 1 is invalid
    eeWrite(si + TRACK_SIZE, 1);
    EEPROM.commit();
}
Track* loadTrack(uint8_t slot)
{   
    uint32_t address = NVMADDRESS + (4096 * (uint32_t)slot);
    uint8_t si = TRACKS_SLOT_1 + (4 * slot);
    
    uint16_t size = EEPROM.read(si + TRACK_SIZE);
    if (size == 0) { size = 256; }
    if (size < 4) { return nullptr; } 
    
    Track* t = CREATE(Track);
    t->memBank = false;
    
    t->clockDivision = EEPROM.read(si);
    t->useMod = EEPROM.read(si + TRACK_USEMOD);
    t->halfTime = EEPROM.read(si + TRACK_HALFTIME);
    t->playing = true;
    t->saveSlot = slot;
    
    t->size = size;
    t->notes = FlashRead<Note>(address, size);
    t->mods = FlashRead<uint16_t>(address + 512, size);
    
    return t;
}

void pushTrackToSlot(uint8_t slot, Track* src)
{
    Track* dest = loadMemBank(slot);
    deleteTrack(dest);
    dest->clockDivision = src->clockDivision;
    dest->halfTime = src->halfTime;
    dest->mods = src->mods;
    dest->notes = src->notes;
    dest->playing = src->playing;
    dest->saveSlot = src->saveSlot;
    dest->size = src->size;
    dest->useMod = src->useMod;
    free(src);
}
void copyTrack(Track* src, Track* dest)
{
    dest->clockDivision = src->clockDivision;
    dest->halfTime = src->halfTime;
    dest->playing = src->playing;
    dest->saveSlot = src->saveSlot;
    dest->size = src->size;
    dest->useMod = src->useMod;
    
    Note* notes = CREATE_ARRAY(Note, src->size);
    uint16_t* mods = CREATE_ARRAY(uint16_t, src->size);
    
    dest->notes = notes;
    dest->mods = mods;
    
    for (uint16_t i = 0; i < src->size; i++)
    {
        notes[i] = src->notes[i];
        mods[i] = src->mods[i];
    }
}

// t must be valid
void transposeTrack(Track* t, int8_t semiTones, uint16_t range)
{
    for (uint8_t i = 0; i < range; i++)
    {
        Note n = t->notes[i];
        if (noteEquals(n, NOTEOFF) || noteEquals(n, NOTEHOLD)) { continue; }
        t->notes[i].key += semiTones;
    }
}
typedef struct
{
    uint8_t note;
    bool offset;
} KeyNote;
KeyNote noteToKey(uint8_t n)
{
    switch (n)
    {
        case 0:
            return { 0, false };
        case 1:
            return { 0, true };
        case 2:
            return { 1, false };
        case 3:
            return { 1, true };
        case 4:
            return { 2, false };
        case 5:
            return { 3, false };
        case 6:
            return { 3, true };
        case 7:
            return { 4, false };
        case 8:
            return { 4, true };
        case 9:
            return { 5, false };
        case 10:
            return { 5, true };
        case 11:
            return { 6, false };
    }
    
    return { 255, true };
}
uint8_t keyToNote(KeyNote kn)
{
    uint8_t v = 0;
    
    switch (kn.note)
    {
        // case 0:
        //     break;
        case 1:
            v = 2;
            break;
        case 2:
            v = 4;
            break;
        case 3:
            v = 5;
            break;
        case 4:
            v = 7;
            break;
        case 5:
            v = 9;
            break;
        case 6:
            v = 11;
            break;
    }
    
    if (kn.offset) { return v + 1; }
    return v;
}

// t must be valid
void transposeTrackKey(Track* t, int8_t offset, Notes key, uint16_t range)
{
    for (uint16_t i = 0; i < range; i++)
    {
        Note nv = t->notes[i];
        if (noteEquals(nv, NOTEOFF) || noteEquals(nv, NOTEHOLD)) { continue; }
        uint8_t n = nv.key + 12;
        n -= key;
        // to key
        uint8_t octave = n / 12;
        KeyNote kn = noteToKey(n % 12);
        kn.note += (octave * 7) + offset;
        // back to note
        octave = kn.note / 7;
        kn.note %= 7;
        
        n = keyToNote(kn) + (octave * 12);
        t->notes[i].key = n + key - 12;
    }
}