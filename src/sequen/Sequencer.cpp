#include <Arduino.h>
#include "Sequencer.h"

#include "../../Callbacks.h"
#include "../../MenuFeedback.h"
#include "../../SpeicalOps.h"
#include "../core/Midi.h"
#include "../core/Globals.h"
#include "../notes/Channels.h"
#include "Track.h"
#include "../core/Coms.h"

bool playMode = false;
// half the tempo time
uint32_t tempoTime = 250;
bool seqClocked = false;
bool playing = false;
uint16_t playStep = 0;

Track tracks[5];

void onParamChange() { playMode = false; }
void restartTracks()
{
    tracks[0].position = 0;
    tracks[1].position = 0;
    tracks[2].position = 0;
    tracks[3].position = 0;
    tracks[4].position = 0;
}

Track* trackSet = nullptr;
bool exitSeq = false;

bool tapTempo_S = false;
uint32_t tapTempoTime_S = 0;

bool setSeqTime = false;
uint8_t digit_S = 0;
uint32_t lv_S = 0;
uint8_t seqDigits[4] = { 0, 0, 0, 0 };

uint8_t trackSlotSelect = 0;

uint32_t playingTime = 0;
uint32_t elapsedTime = 0;

// edit tracks
NoteName rangeBottom;
NoteName rangeTop;
uint8_t trackSetState = 0;

void triggerTracks();
void modTracks(uint32_t pt);

void resetSequence()
{
    clearNotes(&channels[0]);
    clearNotes(&channels[1]);
    clearNotes(&channels[2]);
    clearNotes(&channels[3]);
    clearNotes(&channels[4]);
    
    playStep = 0;
    playing = false;
    tracks[0].position = 0;
    tracks[1].position = 0;
    tracks[2].position = 0;
    tracks[3].position = 0;
    tracks[4].position = 0;
    gate = 0;
    setGate(0);
}

void trackManager(NoteName n, uint8_t vel);
void playingFunc(NoteName n, uint8_t channel)
{
    switch (n)
    {
        case NoteName::C4:
            if (playing)
            {
                playStep = 0;
                return;
            }
            playing = true;
            if (playStep != 0) { return; }
            triggerTracks();
            playingTime = 0;
            return;
        case NoteName::_D4:
            playing = false;
            clearNotes(&channels[0]);
            clearNotes(&channels[1]);
            clearNotes(&channels[2]);
            clearNotes(&channels[3]);
            clearNotes(&channels[4]);
            gate = 0;
            setGate(0);
            return;
        case NoteName::E4:
            resetSequence();
            return;
            
        case NoteName::C5:
            tracks[0].playing = !tracks[0].playing;
            return;
        case NoteName::_D5:
            tracks[1].playing = !tracks[1].playing;
            return;
        case NoteName::E5:
            tracks[2].playing = !tracks[2].playing;
            return;
        case NoteName::F5:
            tracks[3].playing = !tracks[3].playing;
            return;
        case NoteName::G5:
            tracks[4].playing = !tracks[4].playing;
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
    }
    return 16;
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
    if (trackSet)
    {
        trackManager(n, vel);
        return;
    }
    
    // enter time - digit must start at 0
    if (setSeqTime)
    {
        uint8_t dv = getDigit(n);
        // valid digit
        if (dv <= 9)
        {
            // no more digits
            if (digit_S >= 4) { return; }
            seqDigits[digit_S] = dv;
            digit_S++;
            playNote((NoteName)(n + (NoteName::_A3 - NoteName::_A0)), MF_DURATION);
            return;
        }
        if (n != NoteName::C3) { return; }
    }
    if (trackSlotSelect)
    {
        uint8_t slot = getSaveSlot(n);
        trackSlotSelect = 0;
        // valid slot
        if (slot <= 15)
        {
            if (trackSlotSelect == 1)
            {
                // empty track
                if (!tracks[channel].notes)
                {
                    playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                    return;
                }
                saveTrack(&tracks[channel], slot);
            }
            else
            {
                deleteTrack(&tracks[channel]);
                loadTrack(&tracks[channel], slot, channel);
            }
            playNoteC((NoteName)(n + (NoteName::_A3 - NoteName::_A0)), channel, MF_DURATION);
            return;
        }
        playNoteC(NOTEFAIL_S, channel, MF_DURATION);
        return;
    }
    
    switch (n)
    {
        case NoteName::B3:
            exitSeq = true;
            return;
        case NoteName::C4:
            if (playing) { return; }
            playing = true;
            if (playStep != 0) { return; }
            triggerTracks();
            playingTime = 0;
            return;
        // case NoteName::_D4:
        //     playing = false;
        //     return;
        case NoteName::E4:
            resetSequence();
            return;
        case NoteName::F4:
        {
            resetSequence();
            Track* tac = &tracks[channel];
            deleteTrack(tac);
            createTrack(tac, channel);
            trackSet = tac;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
        case NoteName::G4:
            trackSet = &tracks[channel];
            trackSetState = 3;
            resetSequence();
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        
        case NoteName::C5:
            tracks[0].playing = !tracks[0].playing;
            triggerFeedbackC(tracks[0].playing, 0);
            return;
        case NoteName::_D5:
            tracks[1].playing = !tracks[1].playing;
            triggerFeedbackC(tracks[1].playing, 1);
            return;
        case NoteName::E5:
            tracks[2].playing = !tracks[2].playing;
            triggerFeedbackC(tracks[2].playing, 2);
            return;
        case NoteName::F5:
            tracks[3].playing = !tracks[3].playing;
            triggerFeedbackC(tracks[3].playing, 3);
            return;
        case NoteName::G5:
            tracks[4].playing = !tracks[4].playing;
            triggerFeedbackC(tracks[4].playing, 4);
            return;
        
        case NoteName::C3:
        {
            setSeqTime = !setSeqTime;
            playNote(NOTESELECT_S, MF_DURATION);
            // set arp time value
            if (!setSeqTime)
            {
                // digit is the number of digits entered
                if (digit_S == 0)
                {
                    tempoTime = 30000 / lv_S;
                    return;
                }
                uint32_t bpm = 0;
                uint8_t place = 0;
                // read in reverse order
                for (int8_t i = digit_S - 1; i >= 0; i--)
                {
                    bpm += seqDigits[i] * digitPlaces[place];
                    place++;
                }
                digit_S = 0;
                lv_S = bpm;
                tempoTime = 30000 / bpm;
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
            playStep &= 0xFFFE;
            return;
        case NoteName::E3:
            // save
            trackSlotSelect = 1;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::F3:
            // load
            trackSlotSelect = 2;
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
                if (playing && !seqClocked &&
                    tracks[channel].playing && tracks[channel].useMod)
                    { return true; }
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return true;
            case MidiCode::PitchWheel:
                onPitchBend(channel, MIDI.getCombinedData());
                return true;
            case MidiCode::TimingClock:
                if (playing && seqClocked) { triggerTracks(); }
                return true;
            case MidiCode::Start:
                if (playing) { return true; }
                playing = true;
                if (playStep != 0) { return true; }
                triggerTracks();
                playingTime = 0;
                return true;
            case MidiCode::Stop:
                resetSequence();
                return true;
        }
    }
    
    return true;
}

void triggerTracks()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        triggerTrack(&tracks[i], i, playStep);
    }
    
    playStep++;
    if (seqClocked) { playStep++; }
}
void modTracks(uint32_t pt)
{
    CubicInput ci = getInput(pt / (float)tempoTime);
    for (uint8_t i = 0; i < 5; i++)
    {
        modTrack(&tracks[i], i, ci);
    }
}

bool clockDivSet = false;
void trackManager(NoteName n, uint8_t vel)
{
    if (trackSetState == 0)
    {
        rangeBottom = n;
        trackSetState = 1;
        return;
    }
    if (trackSetState == 1)
    {
        trackSetState = 2;
        if (n < rangeBottom)
        {
            rangeTop = rangeBottom;
            rangeBottom = n;
            return;
        }
        rangeTop = n;
        return;
    }
    
    Notes q = (Notes)(n % 12);
    
    if (clockDivSet)
    {
        uint8_t dv = getDigit(n);
        // valid digit
        if (dv <= 9)
        {
            // no more digits
            if (digit_S >= 3) { return; }
            seqDigits[digit_S] = dv;
            digit_S++;
            playNote((NoteName)(n + (NoteName::_A3 - NoteName::_A0)), MF_DURATION);
            return;
        }
        if (q != Notes::A) { return; }
    }
    
    uint8_t channel = trackSet->channel;
    uint16_t mod = channels[channel].modulation;
    // in range
    if (trackSetState == 2 && rangeBottom <= n && rangeTop >= n)
    {
        // filter keys
        if (filterKeys && notInKey(n, filter)) { return; }
        
        addTrackValue(trackSet, { n, vel }, mod);
        playNoteC(n, channel, MF_DURATION);
        return;
    }
    
    switch (q)
    {
        case Notes::C:
            if (!finaliseTrack(trackSet))
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                return;
            }
            trackSet = nullptr;
            trackSetState = 0;
            return;
        case Notes::D:
            if (trackSetState == 3) { return; }
            addTrackValue(trackSet, NOTEOFF, mod);
            return;
        case Notes::E:
        {
            if (trackSetState == 3) { return; }
            uint8_t pos = trackSet->position;
            addTrackValue(trackSet, NOTEHOLD, mod);
            if (pos != 0)
            {
                playNoteC((NoteName)trackSet->notes[pos - 1].key, channel, MF_DURATION);
            }
            return;
        }
        case Notes::F:
            trackSet->useMod = !trackSet->useMod;
            triggerFeedbackC(trackSet->useMod, channel);
            return;
        case Notes::G:
            trackSet->halfTime = !trackSet->halfTime;
            triggerFeedbackC(trackSet->halfTime, channel);
            return;
        case Notes::A:
            clockDivSet = !clockDivSet;
            playNote(NOTESELECT_S, MF_DURATION);
            // set arp time value
            if (!clockDivSet)
            {
                // digit is the number of digits entered
                if (digit_S == 0) { return; }
                uint8_t value = 0;
                uint8_t place = 0;
                // read in reverse order
                for (int8_t i = digit_S - 1; i >= 0; i--)
                {
                    value += seqDigits[i] * digitPlaces[place];
                    place++;
                }
                digit_S = 0;
                trackSet->clockDivision = value;
            }
            return;
    }
}