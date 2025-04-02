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

bool tapTempo_S = false;
uint32_t tapTempoTime_S = 0;

bool setSeqTime = false;
uint8_t digit_S = 0;
uint32_t lv_S = 0;
uint8_t seqDigits[4] = { 0, 0, 0, 0 };
void manageSeqNote(NoteName n, uint8_t channel)
{
    switch (n)
    {
        case NoteName::C4:
            if (playing) { playStep = 0; }
            playing = true;
            return;
        case NoteName::D4:
            playing = false;
            return;
        case NoteName::E4:
            playing = false;
            playStep = 0;
            return;
        case NoteName::F4:
            deleteTrack(tracks[0]);
            tracks[0] = createTrack();
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
                    tempoTime = 60000 / lv_S;
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
                digit_S = 0;
                lv_S = bpm;
                tempoTime = 60000 / bpm;
            }
            return;
        }
        case NoteName::_Db3:
            if (tapTempo_S)
            {
                tempoTime = millis() - tapTempoTime_S;
                tapTempo_S = false;
                return;
            }
            tapTempo_S = true;
            tapTempoTime_S = millis();
            return;
        case NoteName::D3:
            playMode = true;
            return;
        case NoteName::_Eb3:
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
                        manageSeqNote(MIDI.getNote(), channel);
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
                onControlChange(channel, number, MIDI.getData2());
                return true;
            case MidiCode::PitchWheel:
                onPitchBend(channel, MIDI.getCombinedData());
                return true;
            case MidiCode::TimingClock:
                if (invokeArp && arpClocked) { clockedArp(); }
                return true;
        }
    }
}