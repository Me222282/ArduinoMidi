#include <Arduino.h>
#include "Sequencer.h"

#include "../../Callbacks.h"
#include "../menus/SpeicalOps.h"
#include "../../MemLocations.h"
#include "../core/Midi.h"
#include "../core/Globals.h"
#include "../notes/Channels.h"
#include "../core/NVData.h"
#include "Track.h"
#include "TrackSeq.h"
#include "../core/Coms.h"
#include "../../Arpeggio.h"
#include "../menus/CCMenu.h"

Menu _seq_Menu;
bool _playMode[5] = { false, false, false, false, false };
// half the tempo time
uint32_t _tempoTime = 250;
bool _seqClocked = false;
uint32_t _seqClockCount = 0;
bool _playing = false;
uint32_t _playStep = 0;
uint32_t _psBarOffset = 0;

// in half semiquavers
uint8_t _barSize = 32;
bool _triggerOnBars = true;
uint32_t _nextTempoTime = 0;
uint32_t _nextBarSize = 0;

TrackSequence _sequences[5];

// track function
void pergeSlot(uint8_t slot)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        if (!_sequences[i].current) { continue; }
        
        uint16_t size = _sequences[i].size;
        TrackPart* tp = _sequences[i].tracks;
        for (uint16_t j = 0; j < size; j++)
        {
            if (tp[i].track->saveSlot == slot)
            {
                tp[i].track->saveSlot = 255;
                return;
            }
        }
    }
}

void resetTrack(TrackSequence* seq, bool ln = true)
{
    // does not exist
    if (!seq->current) { return; }
    seq->tPosition = 0;
    if (ln)
    {
        seq->lastNote = NOTEOFF;
    }
    seq->skip = 0;
    seq->position = 0;
    seq->currentCount = 0;
    seq->stepOffset = 0;
    seq->current = seq->tracks[0].track;
}
void restartTracks()
{
    resetTrack(&_sequences[0], true);
    resetTrack(&_sequences[1], true);
    resetTrack(&_sequences[2], true);
    resetTrack(&_sequences[3], true);
    resetTrack(&_sequences[4], true);
}

bool _tapTempo_S = false;
uint32_t _tapTempoTime_S = 0;

bool _setSeqTime = false;
bool _setBarSize = false;
uint16_t _lv_S = 120;

enum SlotState: uint8_t
{
    None = 0U,
    Flatten,
    Pull,
    Delete,
    Query,
    Copy,
    Paste,
    BankCurrent,
    SaveCurrent,
    SaveSeq,
    LoadSeq
};

uint8_t _trackCopySrc = 0;
SlotState _trackSlotSelect = SlotState::None;

uint32_t _playingTime = 0;
uint32_t _elapsedTime = 0;

enum TrackState: uint8_t
{
    // KEEP ORDER
    Not = 0U,
    SelectSlot,
    RangeBottom,
    RangeTop,
    AddNotes,
    SelectEditSlot,
    Edit
};

// create seqeunces
TrackPart* _seqNewTracks;
uint8_t _seqCreateChannel;
void sequenceManager(NoteName n);
bool exitSeqCreator();

// edit tracks
NoteName _rangeBottom;
NoteName _rangeTop;
TrackState _trackSetState = TrackState::Not;
TrackSequence _trackSet;
uint8_t _trackSetSaveSlot = 255;
bool exitTrackEdit();

void onParamChange()
{
    _playMode[0] = false;
    _playMode[1] = false;
    _playMode[2] = false;
    _playMode[3] = false;
    _playMode[4] = false;
    if (_trackSlotSelect)
    {
        _trackSlotSelect = SlotState::None;
        playNote(NOTEFAIL_S, MF_DURATION);
        return;
    }
    if (_setBarSize || _setSeqTime)
    {
        _setBarSize = false;
        _setSeqTime = false;
        // ensure digit is 0 again
        getEnteredValue(0);
        playNote(NOTEFAIL_S, MF_DURATION);
        return;
    }
    
    Track* c = _trackSet.current;
    // trackset is creating or editing a track
    if (c && !exitTrackEdit())
    {
        deleteTrack(c);
        _trackSetState = TrackState::Not;
        _trackSetSaveSlot = 255;
    }
    else if (_seqNewTracks && !exitSeqCreator())
    {
        free(_seqNewTracks);
        _seqNewTracks = nullptr;
    }
}

void triggerTracks();
// must come after triggerTracks
void modTracks(uint32_t pt);

void pauseSequence()
{
    clearNotes(&channels[0]);
    clearNotes(&channels[1]);
    clearNotes(&channels[2]);
    clearNotes(&channels[3]);
    clearNotes(&channels[4]);
    gate = 0;
    setGate(0);
}
void pauseSomeSequence(uint8_t channel)
{
    onNoteOff(channel, _sequences[channel].lastNote.key);
    _sequences[channel].oneShot = false;
}
void resetSequence()
{
    _playStep = 0;
    _psBarOffset = 0;
    _seqClockCount = 0;
    _playingTime = 0;
    restartTracks();
}

void trackManager(NoteName n, uint8_t vel);
void playingFunc(NoteName n, uint8_t channel)
{
    switch (n)
    {
        case NoteName::C4:
        {
            Note l0 = _sequences[0].lastNote;
            Note l1 = _sequences[1].lastNote;
            Note l2 = _sequences[2].lastNote;
            Note l3 = _sequences[3].lastNote;
            Note l4 = _sequences[4].lastNote;
            resetSequence();
            _sequences[0].lastNote = l0;
            _sequences[1].lastNote = l1;
            _sequences[2].lastNote = l2;
            _sequences[3].lastNote = l3;
            _sequences[4].lastNote = l4;
            return;
        }
        // case NoteName::_D4:
        //     playing = true;
        //     return;
        case NoteName::E4:
            _playing = false;
            pauseSequence();
            return;
            
        case NoteName::C5:
        {
            bool v = _sequences[0].playing;
            if (_sequences[0].oneShot)
            {
                _sequences[0].oneShot = false;
                return;
            }
            if (_triggerOnBars) { _sequences[0].nextBar = !_sequences[0].nextBar; }
            else
            {
                _sequences[0].playing = !v;
                if (v) { pauseSomeSequence(0); }
            }
            return;
        }
        case NoteName::Db5:
            _sequences[0].skip++;
            return;
        case NoteName::_D5:
        {
            bool v = _sequences[1].playing;
            if (_sequences[1].oneShot)
            {
                _sequences[1].oneShot = false;
                return;
            }
            if (_triggerOnBars) { _sequences[1].nextBar = !_sequences[1].nextBar; }
            else
            {
                _sequences[1].playing = !v;
                if (v) { pauseSomeSequence(1); }
            }
            return;
        }
        case NoteName::Eb5:
            _sequences[1].skip++;
            return;
        case NoteName::E5:
        {
            bool v = _sequences[2].playing;
            if (_sequences[2].oneShot)
            {
                _sequences[2].oneShot = false;
                return;
            }
            if (_triggerOnBars) { _sequences[2].nextBar = !_sequences[2].nextBar; }
            else
            {
                _sequences[2].playing = !v;
                if (v) { pauseSomeSequence(2); }
            }
            return;
        }
        case NoteName::F5:
        {
            bool v = _sequences[3].playing;
            if (_sequences[3].oneShot)
            {
                _sequences[3].oneShot = false;
                return;
            }
            if (_triggerOnBars) { _sequences[3].nextBar = !_sequences[3].nextBar; }
            else
            {
                _sequences[3].playing = !v;
                if (v) { pauseSomeSequence(3); }
            }
            return;
        }
        case NoteName::Gb5:
            _sequences[2].skip++;
            return;
        case NoteName::G5:
        {
            bool v = _sequences[4].playing;
            if (_sequences[4].oneShot)
            {
                _sequences[4].oneShot = false;
                return;
            }
            if (_triggerOnBars) { _sequences[4].nextBar = !_sequences[4].nextBar; }
            else
            {
                _sequences[4].playing = !v;
                if (v) { pauseSomeSequence(4); }
            }
            return;
        }
        case NoteName::Ab5:
            _sequences[3].skip++;
            return;
        case NoteName::Bb5:
            _sequences[4].skip++;
            return;
        case NoteName::C6:
            _sequences[0].oneShot = true;
            if (_triggerOnBars && !_sequences[0].playing)
            {
                _sequences[0].nextBar = true;
                resetTrack(&_sequences[0]);
            }
            else { _sequences[0].playing = true; }
            return;
        case NoteName::Db6:
            resetTrack(&_sequences[0], false);
            playNoteC(NOTEOPTION_S, 0, MF_DURATION);
            return;
        case NoteName::_D6:
            _sequences[1].oneShot = true;
            if (_triggerOnBars && !_sequences[1].playing)
            {
                _sequences[1].nextBar = true;
                resetTrack(&_sequences[1]);
            }
            else { _sequences[1].playing = true; }
            return;
        case NoteName::Eb6:
            resetTrack(&_sequences[1], false);
            playNoteC(NOTEOPTION_S, 1, MF_DURATION);
            return;
        case NoteName::E6:
            _sequences[2].oneShot = true;
            if (_triggerOnBars && !_sequences[2].playing)
            {
                _sequences[2].nextBar = true;
                resetTrack(&_sequences[2]);
            }
            else { _sequences[2].playing = true; }
            return;
        case NoteName::F6:
            _sequences[3].oneShot = true;
            if (_triggerOnBars && !_sequences[3].playing)
            {
                _sequences[3].nextBar = true;
                resetTrack(&_sequences[3]);
            }
            else { _sequences[3].playing = true; }
            return;
        case NoteName::Gb6:
            resetTrack(&_sequences[2], false);
            playNoteC(NOTEOPTION_S, 2, MF_DURATION);
            return;
        case NoteName::G6:
            _sequences[4].oneShot = true;
            if (_triggerOnBars && !_sequences[4].playing)
            {
                _sequences[4].nextBar = true;
                resetTrack(&_sequences[4]);
            }
            else { _sequences[4].playing = true; }
            return;
        case NoteName::Ab6:
            resetTrack(&_sequences[3], false);
            playNoteC(NOTEOPTION_S, 3, MF_DURATION);
            return;
        case NoteName::Bb6:
            resetTrack(&_sequences[4], false);
            playNoteC(NOTEOPTION_S, 4, MF_DURATION);
            return;
        
        case NoteName::G2:
            _playMode[0] = false;
            _playMode[1] = false;
            _playMode[2] = false;
            _playMode[3] = false;
            _playMode[4] = false;
            return;
        case NoteName::_A2:
        {
            _triggerOnBars = !_triggerOnBars;
            return;
        }
        case NoteName::B2:
        {
            _setBarSize = !_setBarSize;
            if (_setBarSize) { return; }
            uint16_t input = getEnteredValue(_barSize >> 1);
            if (input > 127) { return; }
            uint8_t bs = max(input << 1, 2);
            if (_triggerOnBars)
            {
                _nextBarSize = bs;
                return;
            }
            _barSize = bs;
            _psBarOffset = _playStep;
        }
        case NoteName::C3:
        {
            _setSeqTime = !_setSeqTime;
            if (_setSeqTime) { return; }
            // set tempo value
            uint16_t v = getEnteredValue(_lv_S);
            // min tempo
            if (v < 10) { return; }
            _lv_S = v;
            if (_triggerOnBars)
            {
                _nextTempoTime = 30000 / v;
                return;
            }
            _tempoTime = 30000 / v;
            _playingTime %= _tempoTime;
            return;
        }
        case NoteName::Db3:
            _tapTempo_S = true;
            _tapTempoTime_S = millis();
            return;
        case NoteName::_D3:
            _playMode[channel] = true;
            return;
    }
    return;
}
uint8_t getSaveSlot(NoteName n)
{
    switch (n)
    {
        case NoteName::_A0:
            return 0;
        case NoteName::_B0:
            return 1;
        case NoteName::C1:
            return 2;
        case NoteName::_D1:
            return 3;
        case NoteName::E1:
            return 4;
        case NoteName::F1:
            return 5;
        case NoteName::G1:
            return 6;
        case NoteName::_A1:
            return 7;
        case NoteName::_B1:
            return 8;
        case NoteName::C2:
            return 9;
        case NoteName::_D2:
            return 10;
        case NoteName::E2:
            return 11;
        case NoteName::F2:
            return 12;
        case NoteName::G2:
            return 13;
        case NoteName::_A2:
            return 14;
        case NoteName::B2:
            return 15;
        case NoteName::C3:
            return 16;
        case NoteName::_D3:
            return 17;
        case NoteName::E3:
            return 18;
        case NoteName::F3:
            return 19;
        case NoteName::G3:
            return 20;
        case NoteName::_A3:
            return 21;
        case NoteName::B3:
            return 22;
        case NoteName::C4:
            return 23;
        case NoteName::_D4:
            return 24;
        case NoteName::E4:
            return 25;
        case NoteName::F4:
            return 26;
        case NoteName::G4:
            return 27;
        case NoteName::_A4:
            return 28;
        case NoteName::B4:
            return 29;
        case NoteName::C5:
            return 30;
        case NoteName::_D5:
            return 31;
    }
    return 128;
}
void manageSlotSelection(NoteName n, uint8_t channel)
{
    uint8_t slot = getSaveSlot(n);
    SlotState action = _trackSlotSelect;
    _trackSlotSelect = SlotState::None;
    // invalid slot
    if (slot >= 32)
    {
        playNote(NOTEFAIL_S, MF_DURATION);
        return;
    }
    
    switch (action)
    {
        case SlotState::Flatten:
        {
            Track* tk = loadMemBank(slot);
            // empty track
            if (!tk->notes)
            {
                playNote(NOTEFAIL_S, MF_DURATION);
                return;
            }
            saveTrack(tk, slot);
            break;
        }
        case SlotState::Pull:
        {
            Track* newT = loadTrack(slot);
            // no saved track
            if (!newT)
            {
                playNote(NOTEFAIL_S, MF_DURATION);
                return;
            }
            pushTrackToSlot(slot, newT);
            break;
        }
        case SlotState::Delete:
            deleteSave(slot);
            _trackSlotSelect = SlotState::Delete;
            break;
        case SlotState::Query:
            _trackSlotSelect = SlotState::Query;
            if (!isTrackSaved(slot))
            {
                playNote(NOTEFAIL_S, MF_DURATION);
                return;
            }
            break;
        case SlotState::Copy:
            if (!loadMemBank(slot)->notes)
            {
                playNote(NOTEFAIL_S, MF_DURATION);
                return;
            }
            _trackCopySrc = slot;
            _trackSlotSelect = SlotState::Paste;
            break;
        case SlotState::Paste:
            deleteTrack(loadMemBank(slot));
            copyTrack(loadMemBank(_trackCopySrc), loadMemBank(slot));
            break;
        case SlotState::BankCurrent:
        {
            // current exits at this point
            TrackSequence* ts = &_sequences[channel];
            pushTrackToSlot(slot, ts->current);
            ts->current = loadMemBank(slot);
            ts->tracks[ts->position].track = loadMemBank(slot);
            playSlotC(n, channel);
            return;
        }
        case SlotState::SaveCurrent:
        {
            // current exits at this point
            TrackSequence* ts = &_sequences[channel];
            saveTrack(ts->current, slot);
            playSlotC(n, channel);
            return;
        }
        case SlotState::SaveSeq:
            // only 16 seq slots
            if (slot < 16 && saveSequence(&_sequences[channel], slot))
            {
                playSlotC(n, channel);
                return;
            }
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        case SlotState::LoadSeq:
            // only 16 seq slots
            if (slot < 16 && loadSequence(&_sequences[channel], slot, channel))
            {
                playSlotC(n, channel);
                return;
            }
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
    }
    playSlot(n);
}

void manageSeqNote(NoteName n, uint8_t vel, uint8_t channel)
{
    if (_tapTempo_S)
    {
        uint32_t tv = (millis() - _tapTempoTime_S) >> 1;
        _tapTempo_S = false;
        if (_triggerOnBars) { _nextTempoTime = tv; }
        else                { _tempoTime = tv; }
        if (_playing) { return; }
        playNote(NOTEOPTION_S, MF_DURATION_SHORT);
        return;
    }
    // enter time - digit must start at 0
    if (_setSeqTime)
    {
        // valid digit
        bool v = addDigit(n, 4);
        if (v) { return; }
        if (n != NoteName::C3)
        {
            if (_playing) { return; }
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    if (_setBarSize)
    {
        // valid digit
        bool v = addDigit(n, 3);
        if (v) { return; }
        if (n != NoteName::B2)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    if (_playing)
    {
        playingFunc(n, channel);
        return;
    }
    if (_trackSetState)
    {
        trackManager(n, vel);
        return;
    }
    if (_seqNewTracks)
    {
        sequenceManager(n);
        return;
    }
    
    if (_trackSlotSelect)
    {
        manageSlotSelection(n, channel);
        return;
    }

    switch (n)
    {
        case NoteName::Bb3:
            resetSeqValues();
            resetSequence();
            triggerFeedback(true);
            return;
        case NoteName::B3:
            _seq_Menu.active = false;
            return;
        case NoteName::C4:
            resetSequence();
            _playing = true;
            if (!_seqClocked)
            {
                triggerTracks();
                modTracks(0);
            }
            return;
        case NoteName::Db4:
            // save current track to memory bank
            if (!_sequences[channel].current || _sequences[channel].current->memBank)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            }
            _trackSlotSelect = SlotState::BankCurrent;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::_D4:
            _playing = true;
            return;
        case NoteName::Eb4:
            // save current track
            if (!_sequences[channel].current)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            }
            _trackSlotSelect = SlotState::SaveCurrent;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        // case NoteName::E4:
        //     resetSequence();
        //     return;
        case NoteName::F4:
        {
            resetSequence();
            _seqNewTracks = CREATE_ARRAY(TrackPart, 256);
            _seqCreateChannel = channel;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        }
        case NoteName::Gb4:
        {
            resetSequence();
            createSequence(&_trackSet, nullptr, 0, channel);
            Track* tac = CREATE(Track);
            createTrack(tac);
            _trackSet.current = tac;
            _trackSetState = TrackState::RangeBottom;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        }
        case NoteName::G4:
            resetSequence();
            createSequence(&_trackSet, nullptr, 0, channel);
            _trackSetState = TrackState::SelectSlot;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::Ab4:
            // no current track
            if (!_sequences[channel].current)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                return;
            }
            resetSequence();
            createSequence(&_trackSet, nullptr, 0, channel);
            _trackSet.current = _sequences[channel].current;
            _trackSetState = TrackState::Edit;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::_A4:
            resetSequence();
            createSequence(&_trackSet, nullptr, 0, channel);
            _trackSetState = TrackState::SelectEditSlot;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::B4:
            saveSeqState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::Bb4:
            loadSeqState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        
        case NoteName::C5:
        {
            bool v = _sequences[0].playing;
            // if (_triggerOnBars)  { _sequences[0].nextBar = !_sequences[0].nextBar; }
            // else                { _sequences[0].playing = !v; }
            _sequences[0].oneShot = false;
            _sequences[0].playing = !v;
            triggerFeedbackC(!v, 0);
            return;
        }
        case NoteName::Db5:
            nextTrack(&_sequences[0]);
            playNoteC(NOTEOPTION_S, 0, MF_DURATION);
            return;
        case NoteName::_D5:
        {
            bool v = _sequences[1].playing;
            // if (_triggerOnBars)  { _sequences[1].nextBar = !_sequences[1].nextBar; }
            // else                { _sequences[1].playing = !v; }
            _sequences[1].oneShot = false;
            _sequences[1].playing = !v;
            triggerFeedbackC(!v, 1);
            return;
        }
        case NoteName::Eb5:
            nextTrack(&_sequences[1]);
            playNoteC(NOTEOPTION_S, 1, MF_DURATION);
            return;
        case NoteName::E5:
        {
            bool v = _sequences[2].playing;
            // if (_triggerOnBars)  { _sequences[2].nextBar = !_sequences[2].nextBar; }
            // else                { _sequences[2].playing = !v; }
            _sequences[2].oneShot = false;
            _sequences[2].playing = !v;
            triggerFeedbackC(!v, 2);
            return;
        }
        case NoteName::F5:
        {
            bool v = _sequences[3].playing;
            // if (_triggerOnBars)  { _sequences[3].nextBar = !_sequences[3].nextBar; }
            // else                { _sequences[3].playing = !v; }
            _sequences[3].oneShot = false;
            _sequences[3].playing = !v;
            triggerFeedbackC(!v, 3);
            return;
        }
        case NoteName::Gb5:
            nextTrack(&_sequences[2]);
            playNoteC(NOTEOPTION_S, 3, MF_DURATION);
            return;
        case NoteName::G5:
        {
            bool v = _sequences[4].playing;
            // if (_triggerOnBars)  { _sequences[4].nextBar = !_sequences[4].nextBar; }
            // else                { _sequences[4].playing = !v; }
            _sequences[4].oneShot = false;
            _sequences[4].playing = !v;
            triggerFeedbackC(!v, 4);
            return;
        }
        case NoteName::Ab5:
            nextTrack(&_sequences[3]);
            playNoteC(NOTEOPTION_S, 3, MF_DURATION);
            return;
        case NoteName::Bb5:
            nextTrack(&_sequences[4]);
            playNoteC(NOTEOPTION_S, 4, MF_DURATION);
            return;
        case NoteName::C6:
            _sequences[0].oneShot = true;
            // if (_triggerOnBars && !_sequences[0].playing)
            // {
            //     _sequences[0].nextBar = true;
            //     resetTrack(&_sequences[0]);
            // }
            // else { _sequences[0].playing = true; }
            _sequences[0].playing = true;
            triggerFeedbackC(true, 0);
            return;
        case NoteName::Db6:
            resetTrack(&_sequences[0]);
            playNoteC(NOTEOPTION_S, 0, MF_DURATION);
            return;
        case NoteName::_D6:
            _sequences[1].oneShot = true;
            // if (_triggerOnBars && !_sequences[1].playing)
            // {
            //     _sequences[1].nextBar = true;
            //     resetTrack(&_sequences[1]);
            // }
            // else { _sequences[1].playing = true; }
            _sequences[1].playing = true;
            triggerFeedbackC(true, 1);
            return;
        case NoteName::Eb6:
            resetTrack(&_sequences[1]);
            playNoteC(NOTEOPTION_S, 1, MF_DURATION);
            return;
        case NoteName::E6:
            _sequences[2].oneShot = true;
            // if (_triggerOnBars && !_sequences[2].playing)
            // {
            //     _sequences[2].nextBar = true;
            //     resetTrack(&_sequences[2]);
            // }
            // else { _sequences[2].playing = true; }
            _sequences[2].playing = true;
            triggerFeedbackC(true, 2);
            return;
        case NoteName::F6:
            _sequences[3].oneShot = true;
            // if (_triggerOnBars && !_sequences[3].playing)
            // {
            //     _sequences[3].nextBar = true;
            //     resetTrack(&_sequences[3]);
            // }
            // else { _sequences[3].playing = true; }
            _sequences[3].playing = true;
            triggerFeedbackC(true, 3);
            return;
        case NoteName::Gb6:
            resetTrack(&_sequences[2]);
            playNoteC(NOTEOPTION_S, 2, MF_DURATION);
            return;
        case NoteName::G6:
            _sequences[4].oneShot = true;
            // if (_triggerOnBars && !_sequences[4].playing)
            // {
            //     _sequences[4].nextBar = true;
            //     resetTrack(&_sequences[4]);
            // }
            // else { _sequences[4].playing = true; }
            _sequences[4].playing = true;
            triggerFeedbackC(true, 4);
            return;
        case NoteName::Ab6:
            resetTrack(&_sequences[3]);
            playNoteC(NOTEOPTION_S, 3, MF_DURATION);
            return;
        case NoteName::Bb6:
            resetTrack(&_sequences[4]);
            playNoteC(NOTEOPTION_S, 4, MF_DURATION);
            return;
        
        case NoteName::G2:
            _playMode[0] = false;
            _playMode[1] = false;
            _playMode[2] = false;
            _playMode[3] = false;
            _playMode[4] = false;
            triggerFeedback(false);
            return;
        case NoteName::Ab2:
            // query existing tracks
            _trackSlotSelect = SlotState::Query;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::_A2:
        {
            _triggerOnBars = !_triggerOnBars;
            triggerFeedback(_triggerOnBars);
            return;
        }
        case NoteName::Bb2:
            // copy track
            _trackSlotSelect = SlotState::Copy;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::B2:
        {
            _setBarSize = !_setBarSize;
            // set arp time value
            if (!_setBarSize)
            {
                uint16_t input = getEnteredValue(_barSize >> 1);
                if (input > 127)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                
                // x2 for half time
                _barSize = max(input << 1, 2);
            }
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
        case NoteName::C3:
        {
            _setSeqTime = !_setSeqTime;
            playNote(NOTESELECT_S, MF_DURATION);
            // set arp time value
            if (!_setSeqTime)
            {
                uint16_t v = getEnteredValue(_lv_S);
                // min tempo
                if (v < 10)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                _lv_S = v;
                _tempoTime = 30000 / v;
            }
            return;
        }
        case NoteName::Db3:
            _tapTempo_S = true;
            _tapTempoTime_S = millis();
            playNote(NOTEOPTION_S, MF_DURATION_SHORT);
            return;
        case NoteName::_D3:
            _playMode[channel] = true;
            return;
        case NoteName::Eb3:
            _seqClocked = !_seqClocked;
            triggerFeedback(_seqClocked);
            resetSequence();
            return;
        case NoteName::E3:
            // save
            _trackSlotSelect = SlotState::SaveSeq;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::F3:
            // load
            _trackSlotSelect = SlotState::LoadSeq;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::Gb3:
            // pull track
            _trackSlotSelect = SlotState::Pull;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::G3:
            // try flatten sequence
            if (flattenSequence(&_sequences[channel]))
            {
                triggerFeedbackC(true, channel);
                return;
            }
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        case NoteName::Ab3:
            // flatten track
            _trackSlotSelect = SlotState::Flatten;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::_A3:
            // delete saved track
            _trackSlotSelect = SlotState::Delete;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
    }
}

void seqLoopInvoke()
{
    uint32_t time = millis();
    uint32_t dt = time - _elapsedTime;
    _elapsedTime = time;
    
    if (_playing && !_seqClocked)
    {
        _playingTime += dt;
        
        if (_playingTime >= _tempoTime)
        {
            _playingTime -= _tempoTime;
            triggerTracks();
        }
        
        modTracks(_playingTime);
    }
    
    if (!_playing) { invokeMF(); }
    
    if (MIDI.read())
    {
        MidiCode type = MIDI.getType();
        uint8_t channel = MIDI.getChannel();
        if (channel >= 5) { _seq_Menu.active = false; return; }
        switch (type)
        {
            case MidiCode::NoteON:
            case MidiCode::NoteOFF:
            {
                uint8_t vel = MIDI.getData2();
                
                if (!_playMode[channel])
                {
                    if (vel != 0 && type == MidiCode::NoteON)
                    {
                        manageSeqNote(MIDI.getNote(), vel, channel);
                    }
                    return;
                }
                
                uint8_t n = MIDI.getData1();
                if (type == MidiCode::NoteOFF || vel == 0)
                {
                    if (channel < 5 && channelArps[channel])
                    {
                        arpRemoveNote(channel, n);
                        return;
                    }
                    onNoteOff(channel, n);
                    return;
                }
                
                if (channel < 5 && channelArps[channel])
                {
                    arpAddNote(channel, { n, vel });
                    return;
                }
                onNoteOn(channel, n, vel);
                return;
            }
            case MidiCode::CC:
            {
                TrackSequence* ts = &_sequences[channel];
                if (_playing && ts->playing && ts->current &&
                    ts->current->useMod &&
                    ts->current->playing)
                    { return; }
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return;
            }
            case MidiCode::PitchWheel:
                onPitchBend(channel, MIDI.getCombinedData());
                return;
            case MidiCode::TimingClock:
            {
                if (_playing && _seqClocked)
                {
                    uint32_t acc = _seqClockCount;
                    _seqClockCount++;
                    if (acc % 3 == 0)
                    {
                        triggerTracks();
                    }
                    modTracks(acc);
                }
                return;
            }
            case MidiCode::Start:
                _playing = true;
                resetSequence();
                if (!_seqClocked)
                {
                    triggerTracks();
                    modTracks(0);
                }
                return;
            case MidiCode::Continue:
                _playing = true;
                return;
            case MidiCode::Stop:
                _playing = false;
                pauseSequence();
                return;
        }
    }
    
    return;
}

void triggerTracks()
{
    bool onBar = (_playStep - _psBarOffset) % _barSize == 0;
    if (onBar && _nextTempoTime)
    {
        _tempoTime = _nextTempoTime;
        _nextTempoTime = 0;
        _playingTime %= _tempoTime;
    }
    if (onBar && _nextBarSize)
    {
        _barSize = _nextBarSize;
        _nextBarSize = 0;
        _psBarOffset = _playStep;
    }
    
    onSequence();
    for (uint8_t i = 0; i < 5; i++)
    {
        TrackSequence* seq = &_sequences[i];
        if (onBar && seq->nextBar)
        {
            seq->playing = !seq->playing;
            seq->nextBar = false;
            if (!seq->playing)
            {
                seq->oneShot = false;
                pauseSomeSequence(i);
                continue;
            }
        }
        
        triggerTrackSeq(seq, _playStep);
    }
    _playStep++;
}
void modTracks(uint32_t pt)
{
    CubicInput ci;
    if (!_seqClocked)
    {
        // second half of step - inverted as it comes after
        if (!(bool)(_playStep & 1U))
        {
            pt += _tempoTime;
        }
        
        ci = getInput(pt / (float)(_tempoTime << 1));
    }
    else
    {
        ci = getInput((float)(pt % 6) / 6.0f);
    }
    
    for (uint8_t i = 0; i < 5; i++)
    {
        modTrackSeq(&_sequences[i], ci);
    }
}

bool _clockDivSet = false;
uint16_t _lastClockDiv = 1;
bool _tkInc = false;
bool _offsetNoteSet = false;
bool _selectTKOKey = false;
uint8_t _lastNoteOffset = 1;
Notes _nOKey = Notes::C;
bool _maxTrackNotes = false;
void trackManager(NoteName n, uint8_t vel)
{
    uint8_t channel = _trackSet.channel;
    
    switch (_trackSetState)
    {
        case TrackState::SelectEditSlot:
        case TrackState::SelectSlot:
        {
            uint8_t slot = getSaveSlot(n);
            if (slot >= 31)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                return;
            }
            Track* tk = loadMemBank(slot);
            // track does not exist
            if (_trackSetState == TrackState::SelectEditSlot && !tk->notes)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                return;
            }
            if (_trackSetState == TrackState::SelectSlot)
            {
                // override whats there
                deleteTrack(tk);
                createTrack(tk);
            }
            _trackSetSaveSlot = slot;
            _trackSet.current = tk;
            // next mode must be next in list
            _trackSetState = (TrackState)(_trackSetState + 1);
            playSlotC(n, channel);
            // playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        }
        case TrackState::RangeBottom:
        {
            _rangeBottom = n;
            _trackSetState = TrackState::RangeTop;
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        }
        case TrackState::RangeTop:
        {
            _trackSetState = TrackState::AddNotes;
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            if (n < _rangeBottom)
            {
                _rangeTop = _rangeBottom;
                _rangeBottom = n;
                return;
            }
            _rangeTop = n;
            return;
        }
    }
    
    Notes q = (Notes)(n % 12);
    
    if (_offsetNoteSet)
    {
        // valid digit
        if (addDigit(n, 3)) { return; }
        _offsetNoteSet = false;
        bool stkok = _selectTKOKey;
        _selectTKOKey = false;
        uint16_t v = getEnteredValue(_lastNoteOffset);
        if (v > 127)
        {
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        }
        _lastNoteOffset = v;
        int8_t offset = _tkInc ? v : -v;
        uint16_t range = _trackSetState == TrackState::AddNotes ? _trackSet.tPosition : _trackSet.current->size;
        if (stkok)
        {
            transposeTrackKey(_trackSet.current, offset, _nOKey, range);
        }
        else
        {
            transposeTrack(_trackSet.current, offset, range);
        }
        playNoteC(NOTESELECT_S, channel, MF_DURATION);
        return;
    }
    // but not offsetNoteSet
    if (_selectTKOKey)
    {
        uint8_t fk = n - NoteName::C1;
        if (fk < 12)
        {
            _nOKey = (Notes)fk;
            playNumberC(n, channel);
            _offsetNoteSet = true;
            return;
        }
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return;
    }
    
    if (_clockDivSet)
    {
        // valid digit
        if (addDigit(n, 3)) { return; }
        if (q != Notes::A)
        {
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        }
    }
    
    uint16_t mod = channels[channel].modulation;
    // in range
    if (_trackSetState == TrackState::AddNotes && _rangeBottom <= n && _rangeTop >= n)
    {
        // filter keys
        if (filterKeys && notInKey(n, filter)) { return; }
        if (_maxTrackNotes)
        {
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        }
        
        _maxTrackNotes = addTrackValue(&_trackSet, { n, vel }, mod);
        _trackSet.lastNote = { n, vel };
        playNoteC(n, channel, MF_DURATION);
        return;
    }
    
    Track* tk = _trackSet.current;
    
    switch (q)
    {
        case Notes::C:
            exitTrackEdit();
            return;
        case Notes::Db:
            _selectTKOKey = true;
            _tkInc = false;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::D:
            if (_trackSetState == TrackState::Edit) { return; }
            if (_maxTrackNotes)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                return;
            }
            _maxTrackNotes = addTrackValue(&_trackSet, NOTEOFF, mod);
            _trackSet.lastNote = NOTEOFF;
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case Notes::Eb:
            _selectTKOKey = true;
            _tkInc = true;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::E:
        {
            if (_trackSetState == TrackState::Edit) { return; }
            if (_maxTrackNotes)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                return;
            }
            uint8_t pos = _trackSet.tPosition;
            _maxTrackNotes = addTrackValue(&_trackSet, NOTEHOLD, mod);
            if (noteEquals(_trackSet.lastNote, NOTEOFF))
            {
                playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            }
            else if (pos != 0)
            {
                playNoteC((NoteName)_trackSet.lastNote.key, channel, MF_DURATION);
            }
            return;
        }
        case Notes::F:
            tk->useMod = !tk->useMod;
            triggerFeedbackC(tk->useMod, channel);
            return;
        case Notes::Gb:
            _selectTKOKey = false;
            _offsetNoteSet = true;
            _tkInc = false;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::G:
            tk->halfTime = !tk->halfTime;
            triggerFeedbackC(tk->halfTime, channel);
            return;
        case Notes::Ab:
            _selectTKOKey = false;
            _offsetNoteSet = true;
            _tkInc = true;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::A:
            _clockDivSet = !_clockDivSet;
            // set arp time value
            if (!_clockDivSet)
            {
                uint16_t v = getEnteredValue(_lastClockDiv);
                if (v > 255 || v == 0)
                {
                    playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                    return;
                }
                _lastClockDiv = v;
                tk->clockDivision = v;
            }
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::B:
            tk->playing = !tk->playing;
            triggerFeedbackC(tk->playing, channel);
            return;
    }
}
bool exitTrackEdit()
{
    uint8_t channel = _trackSet.channel;
    
    _clockDivSet = false;
    _selectTKOKey = false;
    _offsetNoteSet = false;
    getEnteredValue(0);
    
    bool create = _trackSetState == TrackState::AddNotes;
    if (create && !finaliseTrack(&_trackSet))
    {
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return false;
    }
    _trackSet.lastNote = NOTEOFF;
    _trackSetState = TrackState::Not;
    uint8_t slot = _trackSetSaveSlot;
    _trackSetSaveSlot = 255;
    triggerFeedbackC(true, channel);
    
    // not saving to slot or editing
    if (create && slot >= 32)
    {
        deleteSequence(&_sequences[channel]);
        TrackPart* tks = CREATE_ARRAY(TrackPart, 1);
        tks[0] = { _trackSet.current, 1 };
        createSequence(&_sequences[channel], tks, 1, channel);
    }
    
    deleteSequence(&_trackSet);
    return true;
}

uint16_t _seqCreatePtr = 0;
uint8_t _seqCLastSlot = 255;
bool _forceLoadNVM = false;
void sequenceManager(NoteName n)
{
    uint8_t channel = _seqCreateChannel;
    
    if (n == NoteName::G5)
    {
        _forceLoadNVM = !_forceLoadNVM;
        triggerFeedbackC(_forceLoadNVM, channel);
        return;
    }
    if (n == NoteName::B5)
    {
        exitSeqCreator();
        return;
    }
    
    uint8_t slot = getSaveSlot(n);
    // invalid slot
    if (slot > 32) { return; }
    
    // too many
    if (_seqCreatePtr >= 256)
    {
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return;
    }
    
    if (slot == _seqCLastSlot)
    {
        TrackPart* tppp = &_seqNewTracks[_seqCreatePtr - 1];
        tppp->count++;
        // max number of repeats in data format
        if (tppp->count >= 255) { _seqCLastSlot = 255; }
        playSlotC(n, channel);
        return;
    }
    Track* tk;
    if (!_forceLoadNVM)
    {
        tk = loadMemBank(slot);
        if (tk->notes)
        {
            _seqNewTracks[_seqCreatePtr] = { tk, 1 };
            _seqCreatePtr++;
            playSlotC(n, channel);
            return;
        }
        // none in mem bank - check NVM
    }
    // from NVM
    tk = loadTrack(slot);
    // no available track
    if (!tk)
    {
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return;
    }
    
    _seqNewTracks[_seqCreatePtr] = { tk, 1 };
    _seqCreatePtr++;
    playSlotC(n, channel);
}
bool exitSeqCreator()
{
    uint8_t channel = _seqCreateChannel;
    _forceLoadNVM = false;
    
    if (_seqCreatePtr == 0)
    {
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return false;
    }
    
    TrackPart* tracks = RESIZE_ARRAY(TrackPart, _seqNewTracks, _seqCreatePtr);
    deleteSequence(&_sequences[channel]);
    createSequence(&_sequences[channel], tracks, _seqCreatePtr, channel);
    _seqCreatePtr = 0;
    _seqCLastSlot = 255;
    _seqNewTracks = nullptr;
    triggerFeedbackC(true, channel);
    return true;
}

void resetSeqValues()
{
    _tempoTime = 250;
    _triggerOnBars = true;
    _barSize = 32;
    _seqClocked = false;
}

void saveSeqState()
{
    openDataSpace(DataSpace::SeqGlobals, true);
    
    setSpaceInt(SEQ_TIMEOUT_A, _tempoTime);
    setSpaceByte(SEQ_CLOCKED, _seqClocked);
    setSpaceByte(SEQ_BAR_TRIGGER, _triggerOnBars);
    setSpaceByte(SEQ_BAR_SIZE, _barSize);
    
    commitSpace();
    closeDataSpace();
}
void loadSeqState()
{
    openDataSpace(DataSpace::SeqGlobals, true);
    
    _tempoTime = getSpaceInt(SEQ_TIMEOUT_A);
    if (_tempoTime == 0)
    {
        _tempoTime = 250;
        setSpaceInt(SEQ_TIMEOUT_A, 250);
    }
    
    _seqClocked = getSpaceByte(SEQ_CLOCKED);
    _triggerOnBars = getSpaceByte(SEQ_BAR_TRIGGER);
    _barSize = getSpaceByte(SEQ_BAR_SIZE);
    if (_barSize == 0)
    {
        _barSize = 32;
        setSpaceByte(SEQ_BAR_SIZE, 32);
    }
    
    closeDataSpace();
}

void seqOpen()
{
    playNote(NOTESELECT_S, MF_DURATION);
}

Menu* getSeqMenu()
{
    _seq_Menu = { true, seqLoopInvoke, onParamChange, seqOpen };
    return &_seq_Menu;
}