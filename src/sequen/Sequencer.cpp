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
#include "../core/Coms.h"

bool playMode = false;
// half the tempo time
uint32_t tempoTime = 250;
bool seqClocked = false;
uint32_t seqClockCount = 0;
bool playing = false;
uint16_t playStep = 0;

Track tracks[5];

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

void onParamChange()
{
    playMode = false;
    if (trackSet)
    {
        if (trackSetState == 2 && !finaliseTrack(trackSet))
        {
            deleteTrack(trackSet);
        }
        
        trackSet->lastNote = { 255, 0 };
        trackSet = nullptr;
        trackSetState = 0;
        triggerFeedback(false);
        return;
    }
}

void triggerTracks();
// must come after triggerTracks
void modTracks(uint32_t pt);

void stopSequence()
{
    clearNotes(&channels[0]);
    clearNotes(&channels[1]);
    clearNotes(&channels[2]);
    clearNotes(&channels[3]);
    clearNotes(&channels[4]);
    gate = 0;
    setGate(0);
}
void resetSequence()
{
    playStep = 0;
    seqClockCount = 0;
    playingTime = 0;
    tracks[0].position = 0;
    tracks[0].lastNote = { 255, 0 };
    tracks[1].position = 0;
    tracks[1].lastNote = { 255, 0 };
    tracks[2].position = 0;
    tracks[2].lastNote = { 255, 0 };
    tracks[3].position = 0;
    tracks[3].lastNote = { 255, 0 };
    tracks[4].position = 0;
    tracks[4].lastNote = { 255, 0 };
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
            stopSequence();
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
            playNumber(n);
            return;
        }
        if (n != NoteName::C3) { return; }
    }
    if (trackSlotSelect)
    {
        uint8_t slot = getSaveSlot(n);
        uint8_t action = trackSlotSelect;
        trackSlotSelect = 0;
        // valid slot
        if (slot <= 31)
        {
            if (action == 1)
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
            playNumberC(n, channel);
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
            resetSequence();
            playing = true;
            if (!seqClocked)
            {
                triggerTracks();
                modTracks(0);
            }
            return;
        case NoteName::_D4:
            playing = true;
            return;
        // case NoteName::E4:
        //     resetSequence();
        //     return;
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
            resetSequence();
            trackSet = &tracks[channel];
            trackSetState = 3;
            playNote(NOTESELECT_S, MF_DURATION);
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
            resetSequence();
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
                if (playing &&
                    tracks[channel].playing && tracks[channel].useMod)
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
                stopSequence();
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
            playNumber(n);
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
        trackSet->lastNote = { n, vel };
        playNoteC(n, channel, MF_DURATION);
        return;
    }
    
    switch (q)
    {
        case Notes::C:
            if (trackSetState != 3 && !finaliseTrack(trackSet))
            {
                playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                return;
            }
            trackSet->lastNote = { 255, 0 };
            trackSet = nullptr;
            trackSetState = 0;
            triggerFeedbackC(true, channel);
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
                playNoteC((NoteName)trackSet->lastNote.key, channel, MF_DURATION);
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

void resetSeqValues()
{
    tempoTime = 250;
}

void saveSeqState()
{
    eeWrite(SEQ_TIMEOUT_A, tempoTime >> 24);
    eeWrite(SEQ_TIMEOUT_B, (tempoTime >> 16) & 0xFF);
    eeWrite(SEQ_TIMEOUT_C, (tempoTime >> 8) & 0xFF);
    eeWrite(SEQ_TIMEOUT_D, tempoTime & 0xFF);
    EEPROM.commit();
}
void loadSeqState()
{
    uint32_t timeA = EEPROM.read(SEQ_TIMEOUT_A) << 24;
    uint32_t timeB = EEPROM.read(SEQ_TIMEOUT_B) << 16;
    uint32_t timeC = EEPROM.read(SEQ_TIMEOUT_C) << 8;
    uint32_t timeD = EEPROM.read(SEQ_TIMEOUT_D);
    tempoTime = timeA + timeB + timeC + timeD;
}