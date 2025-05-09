#include <Arduino.h>
#include "TrackSeq.h"
#include "../notes/Channels.h"
#include "../notes/Notes.h"
#include "../../Callbacks.h"
#include "../../MemLocations.h"
#include "../core/NVData.h"
#include "../menus/CCMenu.h"

#define NVMADDRESS 0x2B0000

void createSequence(TrackSequence* seq, TrackPart* tracks, uint8_t size, uint8_t channel)
{
    seq->tracks = tracks;
    if (tracks)
    {
        seq->current = tracks[0].track;
    }
    else
    {
        seq->current = nullptr;
    }
    seq->channel = channel;
    seq->playing = true;
    seq->oneShot = false;
    seq->position = 0;
    seq->tPosition = 0;
    seq->lastNote = NOTEOFF;
    seq->currentCount = 0;
    seq->saveSlot = 255;
    seq->size = size;
    seq->skip = 0;
    seq->nextBar = false;
    seq->stepOffset = 0;
}
void deleteSequence(TrackSequence* seq)
{
    if (seq->tracks)
    {
        for (uint16_t i = 0; i < seq->size; i++)
        {
            freeTrack(seq->tracks[i].track);
        }
        seq->tracks = nullptr;
    }
    seq->size = 0;
    seq->current = nullptr;
    seq->playing = false;
}

bool addTrackValue(TrackSequence* t, Note n, uint16_t m)
{
    uint8_t i = t->tPosition;
    t->current->notes[i] = n;
    t->current->mods[i] = m;
    t->tPosition = i + 1;
    return i == 255;
}
bool finaliseTrack(TrackSequence* t)
{
    uint8_t size = t->tPosition;
    if (size == 0)
    {
        t->current->size = size;
        return true;
    }
    if (size < 4) { return false; }
    
    t->current->notes = RESIZE_ARRAY(Note, t->current->notes, size)
    t->current->mods = RESIZE_ARRAY(uint16_t, t->current->mods, size);
    t->tPosition = 0;
    t->current->size = size;
    return true;
}

Note noteOn(Track* t, uint8_t pos, uint8_t channel, Note last)
{
    if (!t->playing)
    {
        if (!noteEquals(last, NOTEOFF))
        {
            onNoteOff(channel, last.key);
        }
        return NOTEOFF;
    }
    Note n = t->notes[pos];
    if (noteEquals(n, NOTEHOLD)) { return last; }
    if (!noteEquals(n, NOTEOFF))
    {
        onTrack(channel);
        onNoteOn(channel, n.key, n.vel);
    }
    // always in case of track change
    // if (t->halfTime) { return n; }
    onNoteOff(channel, last.key);
    return n;
}

uint16_t wrapMods(TrackSequence* t, uint8_t pos, uint16_t size)
{
    if (pos >= size)
    {
        uint8_t ncp = t->position + 1;
        if (ncp >= t->size) { ncp = 0; }
        return t->tracks[ncp].track->mods[pos - size];
    }
    
    return t->current->mods[pos];
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
void newCubic(TrackSequence* t, uint8_t pos)
{
    uint16_t size = t->current->size;
    
    Cubic c = generateCubic(
        wrapMods(t, pos, size),
        wrapMods(t, pos + 1, size),
        wrapMods(t, pos + 2, size),
        wrapMods(t, pos + 3, size));
    
    t->cub = c;
}

void nextTrack(TrackSequence* t)
{
    t->tPosition = 0;
    TrackPart tp = t->tracks[t->position];
    uint8_t ncc = t->currentCount + 1;
    // loop number of times
    if (ncc < tp.count) { t->currentCount = ncc; return; }
    
    // next track
    t->currentCount = 0;
    uint8_t ncp = t->position + 1;
FindNextTrack:
    if (ncp >= t->size)
    {
        // stop
        if (t->oneShot)
        {
            // reset
            t->playing = false;
            t->position = 0;
            t->tPosition = 0;
            t->skip = 0;
            t->stepOffset = 0;
            t->currentCount = 0;
            t->current = t->tracks[0].track;
            // t->oneShot = false;
            return;
        }
        ncp = 0;
    }
    // skip some track loops
    if (t->skip)
    {
        uint8_t c = t->tracks[ncp].count;
        if (t->skip >= c)
        {
            t->skip -= c;
            ncp++;
            goto FindNextTrack;
        }
        
        t->currentCount = t->skip;
        t->skip = 0;
    }
    t->position = ncp;
    t->current = t->tracks[ncp].track;
}

void triggerTrackSeq(TrackSequence* t, uint32_t playStep)
{
    Track* tk = t->current;
    if (!tk) { return; }
    if (!t->playing)
    {
        if (!t->oneShot || t->nextBar) { return; }
        // end one shot
        onNoteOff(t->channel, t->lastNote.key);
        t->oneShot = false;
        t->lastNote = NOTEOFF;
        return;
    }
    
    // plus 1 allows ht to occur the step before the next clockdiv note trigger
    uint32_t div = ((playStep - t->stepOffset + 1) >> 1) % tk->clockDivision;
    if (div) { return; }
    // div == 0
    
    uint8_t pos = t->tPosition;
    uint8_t ht = playStep & 1U;
    if (ht)
    {
        bool next = pos >= tk->size;
        if (next) { pos = 0; }
        
        if (tk->halfTime && !noteEquals(tk->notes[pos], NOTEHOLD) &&
        // if not playing but still triggering
            !noteEquals(t->lastNote, NOTEOFF))
        {
            onNoteOff(t->channel, t->lastNote.key);
        }
        
        // still on current track
        if (!next) { return; }
        
        // next track on ht before the new tracks first note
        // - allows for clock div and ht of previous track
        nextTrack(t);
        t->stepOffset = playStep + 1;
        return;
    }
    
    t->lastNote = noteOn(tk, pos, t->channel, t->lastNote);
    newCubic(t, pos);
    t->tPosition = pos + 1;
}

void modTrackSeq(TrackSequence* t, CubicInput time)
{
    if (!(t->current && t->current->useMod && t->current->playing)) { return; }
    float m = getCubicValue(t->cub, time);
    if (m < 0) { m = 0.0f; }
    else if (m > 0x3FFF) { m = 0x3FFF; }
    uint16_t act = (uint16_t)m;
    uint8_t channel = t->channel;
    // set mod
    onControlChange(channel, CCType::ModulationWheel_MSB, act >> 7);
    onControlChange(channel, CCType::ModulationWheel_LSB, act & 0x7F);
}

bool saveSequence(TrackSequence* seq, uint8_t slot)
{
    if (!seq->current) { return false; }
    // data already written
    if (seq->saveSlot == slot) { return true; }
    
    uint16_t size = seq->size;
    
    // reformat / is sequence flattened
    Note* array = CREATE_ARRAY(Note, size);
    for (uint16_t i = 0; i < size; i++)
    {
        TrackPart tp = seq->tracks[i];
        uint8_t slot = tp.track->saveSlot;
        
        if (slot >= 32)
        {
            free(array);
            return false;
        }
        
        array[i] = { slot, tp.count };
    }
    
    uint32_t address = NVMADDRESS + (4096 * (uint32_t)slot);
    uint8_t si = SEQS_SLOT_1 + slot;
    
    openDataSpace(DataSpace::Sequens, true);
    setSpaceByte(si, size - 1);
    setSpaceByte(si << 1, true);
    commitSpace();
    closeDataSpace();
    
    seq->saveSlot = slot;
    
    ESP.flashEraseSector(address >> 12);
    FlashWrite<Note>(address, array, size);
    
    free(array);
    return true;
}

bool loadSequence(TrackSequence* seq, uint8_t slot, uint8_t channel)
{
    uint32_t address = NVMADDRESS + (4096 * (uint32_t)slot);
    uint8_t si = SEQS_SLOT_1 + slot;
    
    openDataSpace(DataSpace::Sequens, false);
    uint16_t size = getSpaceByte(si) + 1;
    bool exists = getSpaceByte(si << 1);
    closeDataSpace();
    // cannot occur
    // if (size < 1) { return false; }
    
    // true with written sequence
    if (!exists) { return false; }
    
    // get loaded tracks
    Track** loadedTracks = CREATE_ARRAY(Track*, 32);
    for (uint8_t i = 0; i < 32; i++)
    {
        Track* tk = loadMemBank(i);
        if (tk->saveSlot >= 32) { continue; }
        loadedTracks[tk->saveSlot] = tk;
    }
    
    Note* array = FlashRead<Note>(address, size);
    
    TrackPart* tracks = CREATE_ARRAY(TrackPart, size);
    for (uint16_t i = 0; i < size; i++)
    {
        Note v = array[i];
        if (loadedTracks[v.key])
        {
            tracks[i] = { loadedTracks[v.key], v.vel };
            continue;
        }
        
        Track* tk = loadTrack(v.key);
        // no longer exists
        if (!tk)
        {
            free(loadedTracks);
            free(array);
            free(tracks);
            return false;
        }
        
        tracks[i] = { tk, v.vel };
    }
    
    deleteSequence(seq);
    createSequence(seq, tracks, size, channel);
    
    free(loadedTracks);
    free(array);
    return true;
}

uint8_t* getOpenSaves(uint8_t* size)
{
    uint8_t* array = CREATE_ARRAY(uint8_t, 32);
    uint8_t aPtr = 0;
    
    uint8_t si = TRACKS_SLOT_1;
    openDataSpace(DataSpace::Tracks, false);
    for (uint8_t i = 0; i < 32; i++)
    {
        si += (4 * i);
        TrackSave ts = getSpaceData<TrackSave>(si);
        // invalid
        if (ts.size > 0 && ts.size < 4)
        {
            array[aPtr] = i;
            aPtr++;
        }
    }
    closeDataSpace();
    
    *size = aPtr;
    return array;
}
Track** getUnsavedTracks(TrackSequence* seq, uint16_t* count)
{
    uint16_t size = seq->size;
    Track** array = CREATE_ARRAY(Track*, size);
    uint16_t aPtr = 0;
    
    for (uint16_t i = 0; i < size; i++)
    {
        Track* t = seq->tracks[i].track;
        // unsaved
        if (t->saveSlot >= 32)
        {
            array[aPtr] = t;
            aPtr++;
        }
    }
    
    *count = aPtr;
    return array;
}
bool flattenSequence(TrackSequence* seq)
{
    if (!seq->current) { return false; }
    
    uint8_t openSlots = 0;
    uint16_t unsavedTracks = 0;
    uint8_t* slots = getOpenSaves(&openSlots);
    Track** tracks = getUnsavedTracks(seq, &unsavedTracks);
    
    // not enough slots
    if (unsavedTracks > openSlots) { return false; }
    
    for (uint16_t i = 0; i < unsavedTracks; i++)
    {
        saveTrack(tracks[i], slots[i]);
    }
    
    free(slots);
    free(tracks);
    return true;
}