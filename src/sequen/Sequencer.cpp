#include <Arduino.h>
#include "Sequencer.h"

#include "../../Callbacks.h"
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
            triggerTracks();
            playingTime = 0;
            return;
        case NoteName::_D4:
            playing = false;
            return;
        case NoteName::E4:
            playing = false;
            playStep = 0;
            return;
        case NoteName::F4:
        {
            Track* tac = &tracks[channel];
            deleteTrack(tac);
            createTrack(tac, channel);
            trackSet = tac;
            return;
        }
        
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
            return;
    }
}

void triggerTracks();
void modTracks(uint32_t pt);

bool seqLoopInvoke()
{
    if (playing && !seqClocked)
    {
        uint32_t time = millis();
        playingTime += time - elapsedTime;
        elapsedTime = time;
        
        if (playingTime >= tempoTime)
        {
            playingTime -= tempoTime;
            triggerTracks();
        }
        
        modTracks(playingTime);
    }
    
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
    
    uint16_t mod = channels[trackSet->channel].modulation;
    // in range
    if (rangeBottom <= n && rangeTop >= n)
    {
        addTrackValue(trackSet, { n, vel }, mod);
        return;
    }
    
    Notes q = (Notes)(n % 12);
    
    switch (q)
    {
        case Notes::C:
            if (!finaliseTrack(trackSet))
            {
                // play some sound
                return;
            }
            trackSet = nullptr;
            trackSetState = 0;
            return;
        case Notes::D:
            addTrackValue(trackSet, { 0xFF, 0x00 }, mod);
            return;
        case Notes::E:
            addTrackValue(trackSet, { 0xFF, 0xff }, mod);
            return;
        case Notes::F:
            trackSet->useMod = !trackSet->useMod;
            return;
        case Notes::G:
            trackSet->halfTime = !trackSet->halfTime;
            return;
    }
}