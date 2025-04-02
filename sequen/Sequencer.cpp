#include "Arduino.h"
#include "Sequencer.h"

#include "../Callbacks.h"
#include "../core/Midi.h"
#include "../core/Globals.h"
#include "../notes/Channels.h"
#include "Track.h"

bool playMode = false;
uint32_t tempoTime = 500;
bool seqClocked = false;
bool playing = false;
uint16_t playStep = 0;

Track* tracks[5];

void onParamChange() { playMode = false; }

Track* trackSet = nullptr;

bool tapTempo_S = false;
uint32_t tapTempoTime_S = 0;

bool setSeqTime = false;
uint8_t digit = 0;
uint32_t lv = 0;
uint8_t seqDigits[4] = { 0, 0, 0, 0 };

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
        case NoteName::C4:
            if (playing) { playStep = 0; }
            playing = true;
            return;
        case NoteName::_D4:
            playing = false;
            return;
        case NoteName::E4:
            playing = false;
            playStep = 0;
            return;
        case NoteName::F4:
            deleteTrack(tracks[channel]);
            trackSet = createTrack(channel);
            tracks[channel] = trackSet;
            return;
        
        case NoteName::C3:
        {
            setSeqTime = !setSeqTime;
            // set arp time value
            if (!setSeqTime)
            {
                // digit is the number of digits entered
                if (digit == 0)
                {
                    tempoTime = 60000 / lv;
                    return;
                }
                uint32_t bpm = 0;
                uint8_t place = 0;
                // read in reverse order
                for (int8_t i = digit - 1; i >= 0; i--)
                {
                    bpm += seqDigits[i] * digitPlaces[place];
                    place++;
                }
                digit = 0;
                lv = bpm;
                tempoTime = 60000 / bpm;
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

bool seqLoopInvoke()
{
    
    
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
                    return true;
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
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return true;
            case MidiCode::PitchWheel:
                onPitchBend(channel, MIDI.getCombinedData());
                return true;
            case MidiCode::TimingClock:
                
                return true;
        }
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
            finaliseTrack(trackSet);
            trackSet = nullptr;
            break;
        case Notes::D:
            addTrackValue(trackSet, { 0xFF, 0x00 }, mod);
            break;
        case Notes::E:
            addTrackValue(trackSet, { 0xFF, 0xff }, mod);
            break;
    }
}