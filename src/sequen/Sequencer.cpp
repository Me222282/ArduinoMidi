#include <Arduino.h>
#include <EEPROM.h>
#include "Sequencer.h"

#include "../../Callbacks.h"
#include "../../MenuFeedback.h"
#include "../../SpeicalOps.h"
#include "../../MemLocations.h"
#include "../core/Midi.h"
#include "../core/Globals.h"
#include "../notes/Channels.h"
#include "Track.h"
#include "TrackSeq.h"
#include "../core/Coms.h"

bool playMode = false;
// half the tempo time
uint32_t tempoTime = 250;
bool seqClocked = false;
uint32_t seqClockCount = 0;
bool playing = false;
uint16_t playStep = 0;

// in half semiquavers
uint8_t barSize = 32;
bool triggerOnBars = true;

TrackSequence sequences[5];

// track function
void pergeSlot(uint8_t slot)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        if (!sequences[i].current) { continue; }
        
        uint16_t size = sequences[i].size;
        TrackPart* tp = sequences[i].tracks;
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

void resetTrack(TrackSequence* seq)
{
    // does not exist
    if (!seq->current) { return; }
    seq->tPosition = 0;
    seq->lastNote = NOTEOFF;
    seq->position = 0;
    seq->currentCount = 0;
    seq->stepOffset = 0;
    seq->current = seq->tracks[0].track;
}
void restartTracks()
{
    resetTrack(&sequences[0]);
    resetTrack(&sequences[1]);
    resetTrack(&sequences[2]);
    resetTrack(&sequences[3]);
    resetTrack(&sequences[4]);
}

bool exitSeq = false;

bool tapTempo_S = false;
uint32_t tapTempoTime_S = 0;

bool setSeqTime = false;
bool setBarSize = false;
uint16_t lv_S = 120;

enum SlotState: uint8_t
{
    None,
    Flatten,
    Pull,
    Delete,
    Copy,
    Paste,
    BankCurrent,
    SaveCurrent,
    SaveSeq,
    LoadSeq
};

uint8_t trackCopySrc = 0;
SlotState trackSlotSelect = SlotState::None;

uint32_t playingTime = 0;
uint32_t elapsedTime = 0;

enum TrackState: uint8_t
{
    // KEEP ORDER
    SelectSlot,
    RangeBottom,
    RangeTop,
    AddNotes,
    SelectEditSlot,
    Edit
};

// create seqeunces
TrackPart* seqNewTracks;
uint8_t seqCreateChannel;
void sequenceManager(NoteName n);
bool exitSeqCreator();

// edit tracks
NoteName rangeBottom;
NoteName rangeTop;
TrackState trackSetState = TrackState::SelectSlot;
TrackSequence trackSet;
uint8_t trackSetSaveSlot = 255;
bool exitTrackEdit();

void onParamChange()
{
    playMode = false;
    Track* c = trackSet.current;
    // trackset is creating or editing a track
    if (c && !exitTrackEdit())
    {
        deleteTrack(c);
        trackSetState = TrackState::SelectSlot;
        trackSetSaveSlot = 255;
    }
    else if (seqNewTracks && !exitSeqCreator())
    {
        free(seqNewTracks);
        seqNewTracks = nullptr;
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
    onNoteOff(channel, sequences[channel].lastNote.key);
    sequences[channel].oneShot = false;
}
void resetSequence()
{
    playStep = 0;
    seqClockCount = 0;
    playingTime = 0;
    restartTracks();
}

void trackManager(NoteName n, uint8_t vel);
void playingFunc(NoteName n, uint8_t channel)
{
    switch (n)
    {
        case NoteName::C4:
            resetSequence();
            return;
        // case NoteName::_D4:
        //     playing = true;
        //     return;
        case NoteName::E4:
            playing = false;
            pauseSequence();
            return;
            
        case NoteName::C5:
        {
            bool v = sequences[0].playing;
            if (triggerOnBars) { sequences[0].nextBar = !sequences[0].nextBar; }
            else
            {
                sequences[0].playing = !v;
                if (v) { pauseSomeSequence(0); }
            }
            return;
        }
        case NoteName::Db5:
            sequences[0].skip++;
            return;
        case NoteName::_D5:
        {
            bool v = sequences[1].playing;
            if (triggerOnBars) { sequences[1].nextBar = !sequences[1].nextBar; }
            else
            {
                sequences[1].playing = !v;
                if (v) { pauseSomeSequence(1); }
            }
            return;
        }
        case NoteName::Eb5:
            sequences[1].skip++;
            return;
        case NoteName::E5:
        {
            bool v = sequences[2].playing;
            if (triggerOnBars) { sequences[2].nextBar = !sequences[2].nextBar; }
            else
            {
                sequences[2].playing = !v;
                if (v) { pauseSomeSequence(2); }
            }
            return;
        }
        case NoteName::F5:
        {
            bool v = sequences[3].playing;
            if (triggerOnBars) { sequences[3].nextBar = !sequences[3].nextBar; }
            else
            {
                sequences[3].playing = !v;
                if (v) { pauseSomeSequence(3); }
            }
            return;
        }
        case NoteName::Gb5:
            sequences[2].skip++;
            return;
        case NoteName::G5:
        {
            bool v = sequences[4].playing;
            if (triggerOnBars) { sequences[4].nextBar = !sequences[4].nextBar; }
            else
            {
                sequences[4].playing = !v;
                if (v) { pauseSomeSequence(4); }
            }
            return;
        }
        case NoteName::Ab5:
            sequences[3].skip++;
            return;
        case NoteName::Bb5:
            sequences[4].skip++;
            return;
        case NoteName::C6:
            sequences[0].oneShot = true;
            if (triggerOnBars)  { sequences[0].nextBar = true; }
            else                { sequences[0].playing = true; }
            return;
        case NoteName::_D6:
            sequences[1].oneShot = true;
            if (triggerOnBars)  { sequences[1].nextBar = true; }
            else                { sequences[1].playing = true; }
            return;
        case NoteName::E6:
            sequences[2].oneShot = true;
            if (triggerOnBars)  { sequences[2].nextBar = true; }
            else                { sequences[2].playing = true; }
            return;
        case NoteName::F6:
            sequences[3].oneShot = true;
            if (triggerOnBars)  { sequences[3].nextBar = true; }
            else                { sequences[3].playing = true; }
            return;
        case NoteName::G6:
            sequences[4].oneShot = true;
            if (triggerOnBars)  { sequences[4].nextBar = true; }
            else                { sequences[4].playing = true; }
            return;
        
        case NoteName::_D3:
            playMode = true;
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
    SlotState action = trackSlotSelect;
    trackSlotSelect = SlotState::None;
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
            trackSlotSelect = SlotState::Delete;
            break;
        case SlotState::Copy:
            if (!loadMemBank(slot))
            {
                playNote(NOTEFAIL_S, MF_DURATION);
            }
            trackCopySrc = slot;
            trackSlotSelect = SlotState::Paste;
            break;
        case SlotState::Paste:
            deleteTrack(loadMemBank(slot));
            copyTrack(loadMemBank(trackCopySrc), loadMemBank(slot));
            break;
        case SlotState::BankCurrent:
        {
            // current exits at this point
            TrackSequence* ts = &sequences[channel];
            pushTrackToSlot(slot, ts->current);
            ts->current = loadMemBank(slot);
            ts->tracks[ts->position].track = loadMemBank(slot);
            playNumberC(n, channel);
            return;
        }
        case SlotState::SaveCurrent:
        {
            // current exits at this point
            TrackSequence* ts = &sequences[channel];
            saveTrack(ts->current, slot);
            playNumberC(n, channel);
            return;
        }
        case SlotState::SaveSeq:
            // only 16 seq slots
            if (slot < 16 && saveSequence(&sequences[channel], slot))
            {
                playNumberC(n, channel);
                return;
            }
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        case SlotState::LoadSeq:
            // only 16 seq slots
            if (slot < 16 && loadSequence(&sequences[channel], slot, channel))
            {
                playNumberC(n, channel);
                return;
            }
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
    }
    playNumber(n);
}

void manageSeqNote(NoteName n, uint8_t vel, uint8_t channel)
{
    if (tapTempo_S)
    {
        tempoTime = (millis() - tapTempoTime_S) >> 1;
        tapTempo_S = false;
        playNote(NOTEOPTION_S, MF_DURATION_SHORT);
        return;
    }
    if (playing)
    {
        playingFunc(n, channel);
        return;
    }
    if (trackSet.current)
    {
        trackManager(n, vel);
        return;
    }
    if (seqNewTracks)
    {
        sequenceManager(n);
        return;
    }
    
    // enter time - digit must start at 0
    if (setSeqTime)
    {
        // valid digit
        bool v = addDigit(n, 4);
        if (v) { return; }
        if (n != NoteName::C3)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    if (setBarSize)
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
    if (trackSlotSelect)
    {
        manageSlotSelection(n, channel);
        return;
    }

    switch (n)
    {
        case NoteName::Bb3:
            resetSeqValues();
            resetSequence();
            return;
        case NoteName::B3:
            exitSeq = true;
            return;
        case NoteName::C4:
            resetSequence();
            playing = true;
            if (!seqClocked)
            {
                triggerTracks();
                modTracks(0);
            }
            return;
        case NoteName::Db4:
            // save current track to memory bank
            if (!sequences[channel].current || sequences[channel].current->memBank)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            }
            trackSlotSelect = SlotState::BankCurrent;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::_D4:
            playing = true;
            return;
        case NoteName::Eb4:
            // save current track
            if (!sequences[channel].current)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            }
            trackSlotSelect = SlotState::SaveCurrent;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        // case NoteName::E4:
        //     resetSequence();
        //     return;
        case NoteName::F4:
        {
            resetSequence();
            seqNewTracks = CREATE_ARRAY(TrackPart, 256);
            seqCreateChannel = channel;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        }
        case NoteName::Gb4:
        {
            resetSequence();
            createSequence(&trackSet, nullptr, 0, channel);
            Track* tac = CREATE(Track);
            createTrack(tac);
            trackSet.current = tac;
            trackSetState = TrackState::RangeBottom;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        }
        case NoteName::G4:
            resetSequence();
            createSequence(&trackSet, nullptr, 0, channel);
            trackSetState = TrackState::SelectSlot;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::Ab4:
            // no current track
            if (!sequences[channel].current)
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                return;
            }
            resetSequence();
            createSequence(&trackSet, nullptr, 0, channel);
            trackSet.current = sequences[channel].current;
            trackSetState = TrackState::Edit;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::_A4:
            resetSequence();
            createSequence(&trackSet, nullptr, 0, channel);
            trackSetState = TrackState::SelectEditSlot;
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
            bool v = sequences[0].playing;
            if (triggerOnBars)  { sequences[0].nextBar = !sequences[0].nextBar; }
            else                { sequences[0].playing = !v; }
            triggerFeedbackC(!v, 0);
            return;
        }
        case NoteName::Db5:
            nextTrack(&sequences[0]);
            return;
        case NoteName::_D5:
        {
            bool v = sequences[1].playing;
            if (triggerOnBars)  { sequences[1].nextBar = !sequences[1].nextBar; }
            else                { sequences[1].playing = !v; }
            triggerFeedbackC(!v, 1);
            return;
        }
        case NoteName::Eb5:
            nextTrack(&sequences[1]);
            return;
        case NoteName::E5:
        {
            bool v = sequences[2].playing;
            if (triggerOnBars)  { sequences[2].nextBar = !sequences[2].nextBar; }
            else                { sequences[2].playing = !v; }
            triggerFeedbackC(!v, 2);
            return;
        }
        case NoteName::F5:
        {
            bool v = sequences[3].playing;
            if (triggerOnBars)  { sequences[3].nextBar = !sequences[3].nextBar; }
            else                { sequences[3].playing = !v; }
            triggerFeedbackC(!v, 3);
            return;
        }
        case NoteName::Gb5:
            nextTrack(&sequences[2]);
            return;
        case NoteName::G5:
        {
            bool v = sequences[4].playing;
            if (triggerOnBars)  { sequences[4].nextBar = !sequences[4].nextBar; }
            else                { sequences[4].playing = !v; }
            triggerFeedbackC(!v, 4);
            return;
        }
        case NoteName::Ab5:
            nextTrack(&sequences[3]);
            return;
        case NoteName::Bb5:
            nextTrack(&sequences[4]);
            return;
        case NoteName::C6:
            sequences[0].oneShot = true;
            if (triggerOnBars)  { sequences[0].nextBar = true; }
            else                { sequences[0].playing = true; }
            triggerFeedbackC(true, 0);
            return;
        case NoteName::_D6:
            sequences[1].oneShot = true;
            if (triggerOnBars)  { sequences[1].nextBar = true; }
            else                { sequences[1].playing = true; }
            triggerFeedbackC(true, 1);
            return;
        case NoteName::E6:
            sequences[2].oneShot = true;
            if (triggerOnBars)  { sequences[2].nextBar = true; }
            else                { sequences[2].playing = true; }
            triggerFeedbackC(true, 2);
            return;
        case NoteName::F6:
            sequences[3].oneShot = true;
            if (triggerOnBars)  { sequences[3].nextBar = true; }
            else                { sequences[3].playing = true; }
            triggerFeedbackC(true, 3);
            return;
        case NoteName::G6:
            sequences[4].oneShot = true;
            if (triggerOnBars)  { sequences[4].nextBar = true; }
            else                { sequences[4].playing = true; }
            triggerFeedbackC(true, 4);
            return;
        
        case NoteName::_A2:
        {
            triggerOnBars = !triggerOnBars;
            triggerFeedback(triggerOnBars);
            return;
        }
        case NoteName::Bb2:
            // copy track
            trackSlotSelect = SlotState::Copy;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::B2:
        {
            setBarSize = !setBarSize;
            playNote(NOTESELECT_S, MF_DURATION);
            // set arp time value
            if (!setBarSize)
            {
                // x2 for half time
                barSize = getEnteredValue(barSize >> 1) << 1;
            }
            return;
        }
        case NoteName::C3:
        {
            setSeqTime = !setSeqTime;
            playNote(NOTESELECT_S, MF_DURATION);
            // set arp time value
            if (!setSeqTime)
            {
                lv_S = getEnteredValue(lv_S);
                tempoTime = 30000 / lv_S;
            }
            return;
        }
        case NoteName::Db3:
            tapTempo_S = true;
            tapTempoTime_S = millis();
            playNote(NOTEOPTION_S, MF_DURATION_SHORT);
            return;
        case NoteName::_D3:
            playMode = true;
            return;
        case NoteName::Eb3:
            seqClocked = !seqClocked;
            triggerFeedback(seqClocked);
            resetSequence();
            return;
        case NoteName::E3:
            // save
            trackSlotSelect = SlotState::SaveSeq;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::F3:
            // load
            trackSlotSelect = SlotState::LoadSeq;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case NoteName::Gb3:
            // pull track
            trackSlotSelect = SlotState::Pull;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::G3:
            // try flatten sequence
            if (flattenSequence(&sequences[channel]))
            {
                triggerFeedbackC(true, channel);
                return;
            }
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        case NoteName::Ab3:
            // flatten track
            trackSlotSelect = SlotState::Flatten;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::_A3:
            // delete saved track
            trackSlotSelect = SlotState::Delete;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
    }
}

bool seqLoopInvoke()
{
    uint32_t time = millis();
    uint32_t dt = time - elapsedTime;
    elapsedTime = time;
    
    if (playing && !seqClocked)
    {
        playingTime += dt;
        
        if (playingTime >= tempoTime)
        {
            playingTime -= tempoTime;
            triggerTracks();
        }
        
        modTracks(playingTime);
    }
    
    if (!playing) { invokeMF(); }
    
    if (MIDI.read())
    {
        MidiCode type = MIDI.getType();
        uint8_t channel = MIDI.getChannel();
        if (channel >= 5) { return false; }
        switch (type)
        {
            case MidiCode::NoteON:
            case MidiCode::NoteOFF:
            {
                uint8_t vel = MIDI.getData2();
                
                if (!playMode)
                {
                    if (vel != 0 && type == MidiCode::NoteON)
                    {
                        manageSeqNote(MIDI.getNote(), vel, channel);
                    }
                    bool con = !exitSeq;
                    exitSeq = false;
                    return con;
                }
                
                uint8_t n = MIDI.getData1();
                if (type == MidiCode::NoteOFF || vel == 0)
                {
                    onNoteOff(channel, n);
                    return true;
                }
                
                onNoteOn(channel, n, vel);
                return true;
            }
            case MidiCode::CC:
                if (playing && sequences[channel].playing &&
                    sequences[channel].current->useMod &&
                    sequences[channel].current->playing)
                    { return true; }
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return true;
            case MidiCode::PitchWheel:
                onPitchBend(channel, MIDI.getCombinedData());
                return true;
            case MidiCode::TimingClock:
            {
                if (playing && seqClocked)
                {
                    uint32_t acc = seqClockCount;
                    seqClockCount++;
                    if (acc % 3 == 0)
                    {
                        triggerTracks();
                    }
                    modTracks(acc);
                }
                return true;
            }
            case MidiCode::Start:
                playing = true;
                resetSequence();
                if (!seqClocked)
                {
                    triggerTracks();
                    modTracks(0);
                }
                return true;
            case MidiCode::Continue:
                playing = true;
                return true;
            case MidiCode::Stop:
                playing = false;
                pauseSequence();
                return true;
        }
    }
    
    return true;
}

void triggerTracks()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        TrackSequence* seq = &sequences[i];
        if (seq->nextBar && (playStep % barSize == 0))
        {
            seq->playing = !seq->playing;
            if (!seq->playing)
            {
                seq->oneShot = false;
                pauseSomeSequence(i);
                continue;
            }
        }
        
        triggerTrackSeq(seq, playStep);
    }
    
    playStep++;
}
void modTracks(uint32_t pt)
{
    CubicInput ci;
    if (!seqClocked)
    {
        // second half of step - inverted as it comes after
        if (!(bool)(playStep & 1U))
        {
            pt += tempoTime;
        }
        
        ci = getInput(pt / (float)(tempoTime << 1));
    }
    else
    {
        ci = getInput((float)(pt % 6) / 6.0f);
    }
    
    for (uint8_t i = 0; i < 5; i++)
    {
        modTrackSeq(&sequences[i], ci);
    }
}

bool clockDivSet = false;
uint16_t lastClockDiv = 1;
bool tkInc = false;
bool offsetNoteSet = false;
bool selectTKOKey = false;
uint8_t lastNoteOffset = 1;
Notes nOKey = Notes::C;
void trackManager(NoteName n, uint8_t vel)
{
    uint8_t channel = trackSet.channel;
    
    switch (trackSetState)
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
            trackSetSaveSlot = slot;
            trackSet.current = loadMemBank(slot);
            if (trackSetState == TrackState::SelectSlot)
            {
                // override whats there
                deleteTrack(trackSet.current);
                createTrack(trackSet.current);
            }
            // next mode must be next in list
            trackSetState = (TrackState)(trackSetState + 1);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        }
        case TrackState::RangeBottom:
        {
            rangeBottom = n;
            trackSetState = TrackState::RangeBottom;
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        }
        case TrackState::RangeTop:
        {
            trackSetState = TrackState::AddNotes;
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            if (n < rangeBottom)
            {
                rangeTop = rangeBottom;
                rangeBottom = n;
                return;
            }
            rangeTop = n;
            return;
        }
    }
    
    Notes q = (Notes)(n % 12);
    
    if (offsetNoteSet)
    {
        // valid digit
        if (addDigit(n, 3)) { return; }
        offsetNoteSet = false;
        uint16_t v = getEnteredValue(lastNoteOffset);
        if (v > 127)
        {
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        }
        lastNoteOffset = v;
        int8_t offset = tkInc ? v : -v;
        uint16_t range = trackSetState == TrackState::AddNotes ? trackSet.position : trackSet.current->size;
        if (selectTKOKey)
        {
            transposeTrackKey(trackSet.current, offset, nOKey, range);
        }
        else
        {
            transposeTrack(trackSet.current, offset, range);
        }
        playNoteC(NOTESELECT_S, channel, MF_DURATION);
        return;
    }
    // but not offsetNoteSet
    if (selectTKOKey)
    {
        uint8_t fk = n - NoteName::C1;
        if (fk < 12)
        {
            nOKey = (Notes)fk;
            playNumberC(n, channel);
            offsetNoteSet = true;
            return;
        }
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return;
    }
    
    if (clockDivSet)
    {
        // valid digit
        if (addDigit(n, 3)) { return; }
        if (q != Notes::A)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    
    uint16_t mod = channels[channel].modulation;
    // in range
    if (trackSetState == TrackState::AddNotes && rangeBottom <= n && rangeTop >= n)
    {
        // filter keys
        if (filterKeys && notInKey(n, filter)) { return; }
        
        addTrackValue(&trackSet, { n, vel }, mod);
        trackSet.lastNote = { n, vel };
        playNoteC(n, channel, MF_DURATION);
        return;
    }
    
    Track* tk = trackSet.current;
    
    switch (q)
    {
        case Notes::C:
            exitTrackEdit();
            return;
        case Notes::Db:
            selectTKOKey = true;
            tkInc = false;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::D:
            if (trackSetState == TrackState::Edit) { return; }
            addTrackValue(&trackSet, NOTEOFF, mod);
            return;
        case Notes::Eb:
            selectTKOKey = true;
            tkInc = true;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::E:
        {
            if (trackSetState == TrackState::Edit) { return; }
            uint8_t pos = trackSet.position;
            addTrackValue(&trackSet, NOTEHOLD, mod);
            if (pos != 0)
            {
                playNoteC((NoteName)trackSet.lastNote.key, channel, MF_DURATION);
            }
            return;
        }
        case Notes::F:
            tk->useMod = !tk->useMod;
            triggerFeedbackC(tk->useMod, channel);
            return;
        case Notes::Gb:
            selectTKOKey = false;
            offsetNoteSet = true;
            tkInc = false;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::G:
            tk->halfTime = !tk->halfTime;
            triggerFeedbackC(tk->halfTime, channel);
            return;
        case Notes::Ab:
            selectTKOKey = false;
            offsetNoteSet = true;
            tkInc = true;
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        case Notes::A:
            clockDivSet = !clockDivSet;
            // set arp time value
            if (!clockDivSet)
            {
                uint16_t v = getEnteredValue(lastClockDiv);
                if (v > 255)
                {
                    playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                    return;
                }
                lastClockDiv = v;
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
    uint8_t channel = trackSet.channel;
    
    bool create = trackSetState == TrackState::AddNotes;
    if (create && !finaliseTrack(&trackSet))
    {
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return false;
    }
    trackSet.lastNote = NOTEOFF;
    trackSetState = TrackState::SelectSlot;
    uint8_t slot = trackSetSaveSlot;
    trackSetSaveSlot = 255;
    triggerFeedbackC(true, channel);
    
    // not saving to slot or editing
    if (create && slot >= 32)
    {
        deleteSequence(&sequences[channel]);
        TrackPart* tks = CREATE_ARRAY(TrackPart, 1);
        createSequence(&sequences[channel], tks, 1, channel);
        tks[0] = { trackSet.current, 1 };
        sequences[channel].current = trackSet.current;
    }
    
    deleteSequence(&trackSet);
    return true;
}

uint16_t seqCreatePtr = 0;
uint8_t seqCLastSlot = 255;
bool forceLoadNVM = false;
void sequenceManager(NoteName n)
{
    uint8_t channel = seqCreateChannel;
    
    if (n == NoteName::G5)
    {
        forceLoadNVM = !forceLoadNVM;
        triggerFeedbackC(forceLoadNVM, channel);
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
    if (seqCreatePtr >= 256)
    {
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return;
    }
    
    if (slot == seqCLastSlot)
    {
        seqNewTracks[seqCreatePtr - 1].count++;
        playNumberC(n, channel);
        return;
    }
    Track* tk;
    if (!forceLoadNVM)
    {
        tk = loadMemBank(slot);
        if (tk->notes)
        {
            seqNewTracks[seqCreatePtr] = { tk, 1 };
            seqCreatePtr++;
            playNumberC(n, channel);
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
    
    seqNewTracks[seqCreatePtr] = { tk, 1 };
    seqCreatePtr++;
    playNumberC(n, channel);
}
bool exitSeqCreator()
{
    uint8_t channel = seqCreateChannel;
    forceLoadNVM = false;
    
    if (seqCreatePtr == 0)
    {
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return false;
    }
    
    TrackPart* tracks = (TrackPart*)realloc(seqNewTracks, seqCreatePtr);
    deleteSequence(&sequences[channel]);
    createSequence(&sequences[channel], tracks, seqCreatePtr, channel);
    seqCreatePtr = 0;
    seqCLastSlot = 255;
    seqNewTracks = nullptr;
    triggerFeedbackC(true, channel);
    return true;
}

void resetSeqValues()
{
    tempoTime = 250;
    triggerOnBars = true;
    barSize = 32;
}

void saveSeqState()
{
    eeWrite(SEQ_TIMEOUT_A, tempoTime >> 24);
    eeWrite(SEQ_TIMEOUT_B, (tempoTime >> 16) & 0xFF);
    eeWrite(SEQ_TIMEOUT_C, (tempoTime >> 8) & 0xFF);
    eeWrite(SEQ_TIMEOUT_D, tempoTime & 0xFF);
    eeWrite(SEQ_CLOCKED, seqClocked);
    eeWrite(SEQ_BAR_TRIGGER, triggerOnBars);
    eeWrite(SEQ_BAR_SIZE, barSize);
    EEPROM.commit();
}
void loadSeqState()
{
    uint32_t timeA = EEPROM.read(SEQ_TIMEOUT_A) << 24;
    uint32_t timeB = EEPROM.read(SEQ_TIMEOUT_B) << 16;
    uint32_t timeC = EEPROM.read(SEQ_TIMEOUT_C) << 8;
    uint32_t timeD = EEPROM.read(SEQ_TIMEOUT_D);
    tempoTime = timeA + timeB + timeC + timeD;
    
    seqClocked = EEPROM.read(SEQ_CLOCKED);
    triggerOnBars = EEPROM.read(SEQ_BAR_TRIGGER);
    barSize = EEPROM.read(SEQ_BAR_SIZE);
}