#include <Arduino.h>
#include "Sequencer.h"

#include "../../Callbacks.h"
#include "../../MenuFeedback.h"
#include "../core/Midi.h"
#include "../core/Globals.h"
#include "../notes/Channels.h"
#include "Track.h"

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
bool exit = false;

bool tapTempo_S = false;
uint32_t tapTempoTime_S = 0;

bool setSeqTime = false;
uint8_t digit_S = 0;
uint32_t lv_S = 0;
uint8_t seqDigits[4] = { 0, 0, 0, 0 };

uint32_t playingTime = 0;
uint32_t elapsedTime = 0;

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
}

void trackManager(NoteName n, uint8_t vel);
void manageSeqNote(NoteName n, uint8_t vel, uint8_t channel)
{
    if (trackSet)
    {
        trackManager(n, vel);
        return;
    }
    
    switch (n)
    {
        case NoteName::B3:
            exit = true;
            return;
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
            return;
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
            return;
        }
        case NoteName::G4:
            trackSet = &tracks[channel];
            trackSetState = 3;
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
        
        case NoteName::C3:
        {
            setSeqTime = !setSeqTime;
            // set arp time value
            if (!setSeqTime)
            {
                // digit is the number of digits entered
                if (digit_S == 0)
                {
                    tempoTime = 30000 / lv_S;
                    return;
                }
                uint32_t bpm = 0;if (playing) { playStep = 0; }
                playing = true;
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
            if (tapTempo_S)
            {
                tempoTime = millis() - tapTempoTime_S;
                tapTempo_S = false;
                return;
            }
            tapTempo_S = true;
            tapTempoTime_S = millis();
            return;
        case NoteName::_D3:
            playMode = true;
            return;
        case NoteName::Eb3:
            seqClocked = !seqClocked;
            playStep &= 0xFFFE;
            return;
    }
}

void triggerTracks();
void modTracks(uint32_t pt);

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
    
    // mf only for track set
    if (trackSet) { invokeMF(); }
    
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
                    bool con = !exit;
                    exit = false;
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

NoteName rangeBottom;
NoteName rangeTop;
bool trackSetState = 0;
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
    
    uint8_t channel = trackSet->channel;
    uint16_t mod = channels[channel].modulation;
    // in range
    if (trackSetState == 2 && rangeBottom <= n && rangeTop >= n)
    {
        addTrackValue(trackSet, { n, vel }, mod);
        playNoteC(n, channel, 125);
        return;
    }
    
    Notes q = (Notes)(n % 12);
    
    switch (q)
    {
        case Notes::C:
            if (!finaliseTrack(trackSet))
            {
                playNoteC(NoteName::C2, channel, 125);
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
                playNoteC((NoteName)trackSet->notes[pos - 1].key, channel, 125);
            }
            return;
        }
        case Notes::F:
            trackSet->useMod = !trackSet->useMod;
            return;
        case Notes::G:
            trackSet->halfTime = !trackSet->halfTime;
            return;
    }
}