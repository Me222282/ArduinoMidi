#ifndef __tracking
#define __tracking

#include "../core/Globals.h"
#include "../core/Midi.h"

#define NOTEHOLD (Note){ 0xFF, 0xFF }
#define NOTEOFF (Note){ 0xFF, 0x00 }

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    Note* notes;
    uint16_t* mods;
    
    uint16_t size;
    uint8_t saveSlot;
    uint8_t clockDivision;
    bool playing;
    bool useMod;
    bool halfTime;
    
    bool memBank;
} Track;

typedef struct
{
    uint8_t size;
    uint8_t clockDiv;
    uint8_t useMod;
    uint8_t ht;
} TrackSave;

void createTrack(Track* t);
void deleteTrack(Track* t);
void freeTrack(Track* t);

void pushTrackToSlot(uint8_t slot, Track* src);
void copyTrack(Track* src, Track* dest);

void saveTrack(Track* t, uint8_t slot);
void deleteSave(uint8_t slot);
Track* loadTrack(uint8_t slot);
Track* loadMemBank(uint8_t slot);

void transposeTrack(Track* t, int8_t semiTones, uint16_t range);
void transposeTrackKey(Track* t, int8_t offset, Notes key, uint16_t range);

void pergeSlot(uint8_t slot);

#ifdef __cplusplus
} // extern "C"
#endif

#endif