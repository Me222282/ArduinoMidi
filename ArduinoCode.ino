#include <SPI.h>
// #include "Notes.h"
// #include "Coms.h"
#include "Midi.h"
// #include "Channels.h"
// #include "Callbacks.h"
#include "Arpeggio.h"
#include "Controls.h"

Midi MIDI(Serial0);

void setup()
{
    pinMode(OCTAVE, INPUT);
    pinMode(MODE1, INPUT);
    pinMode(MODE2, INPUT);
    pinMode(VOICES, INPUT);
    pinMode(CHANNELS, INPUT);
    pinMode(PITCHBEND, INPUT);
    pinMode(NOTEDAC1, OUTPUT);
    pinMode(NOTEDAC2, OUTPUT);
    pinMode(NOTEDAC3, OUTPUT);
    pinMode(NOTEDAC4, OUTPUT);
    pinMode(NOTEDAC5, OUTPUT);
    pinMode(PITCHDAC1, OUTPUT);
    pinMode(PITCHDAC2, OUTPUT);
    pinMode(PITCHDAC3, OUTPUT);
    pinMode(GATEIC, OUTPUT);
    pinMode(OPTIONMODE, INPUT);
    
    digitalWrite(NOTEDAC1, HIGH);
    digitalWrite(NOTEDAC2, HIGH);
    digitalWrite(NOTEDAC3, HIGH);
    digitalWrite(NOTEDAC4, HIGH);
    digitalWrite(NOTEDAC5, HIGH);
    digitalWrite(PITCHDAC1, HIGH);
    digitalWrite(PITCHDAC2, HIGH);
    digitalWrite(PITCHDAC3, HIGH);
    digitalWrite(GATEIC, HIGH);
    
    MIDI.begin();
    // Serial.begin(38400);
    
    SPI.begin();
    
    // Setup GATEIC
    SPI.beginTransaction(GATESPI);
    digitalWrite(GATEIC, LOW);
    SPI.transfer16(0x4000);
    // Set direction to output
    SPI.transfer(0x00);
    digitalWrite(GATEIC, HIGH);
    SPI.endTransaction();
    
    setChannels(1, 1);
    initArps();
}

bool specOps = false;
bool a0Pressed = false;
void readControls()
{
    int8_t oldOO = octaveOffset;
    octaveOffset = readControl(0) - 3;
    
    uint8_t oldC = activeChannels;
    activeChannels = readControl(1) + 1;
    uint8_t oldV = activeVoices;
    activeVoices = readControl(2) + 1;
    
    float oldPBS = pitchBendSelect;
    uint8_t pbI = readControl(3);
    pitchBendSelect = pbDiv[pbI];
    
    bool oldM1 = mode1;
    mode1 = digitalRead(MODE1);
    uint8_t oldM2 = mode2;
    mode2 = readControl(4);
    
    bool oldOp = option;
    option = digitalRead(OPTIONMODE);
    
    if (oldM2 != mode2 || oldM1 != mode1)
    {
        clearNotes(&channels[0]);
        clearNotes(&channels[1]);
        clearNotes(&channels[2]);
        clearNotes(&channels[3]);
        clearNotes(&channels[4]);
        
        gate = 0;
        setGate(0);
        specOps = a0Pressed;
        a0Pressed = false;
    }
    if (activeChannels != oldC || activeVoices != oldV)
    {
        setChannels(activeChannels, activeVoices);
        gate = 0;
        setGate(gate);
        updateAllPBs();
        specOps = a0Pressed;
        a0Pressed = false;
    }
    else
    {
        if (oldOO != octaveOffset)
        {
            updateAllNotes();
        }
        if (oldPBS != pitchBendSelect)
        {
            updateAllPBs();
        }
        if (oldOp != option)
        {
            updateOtherPorts();
        }
    }
}

bool invokeArp = false;
bool channelArps[5] = { false, false, false, false, false };
void loop()
{
    readControls();
    
    if (specOps)
    {
        specialOptions();
        return;
    }
    
    if (invokeArp) { invokeArps(); }
    
    if (MIDI.read())
    {
        MidiCode type = MIDI.getType();
        uint8_t channel = MIDI.getChannel();
        switch (type)
        {
            case MidiCode::NoteON:
            case MidiCode::NoteOFF:
            {
                uint8_t n = MIDI.getData1();
                uint8_t vel = MIDI.getData2();
                if (type == MidiCode::NoteOFF || vel == 0)
                {
                    if (n == NoteName::_A0) { a0Pressed = false; }
                    if (channelArps[channel])
                    {
                        arpRemoveNote(channel, n);
                        return;
                    }
                    onNoteOff(channel, n);
                    return;
                }
                
                if (n == NoteName::_A0) { a0Pressed = true; }
                if (channelArps[channel])
                {
                    arpAddNote(channel, { n, vel });
                    return;
                }
                onNoteOn(channel, n, vel);
                return;
            }
            case MidiCode::CC:
            {
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return;
            }
            case MidiCode::PitchWheel:
            {
                onPitchBend(channel, MIDI.getCombinedData());
                return;
            }
        }
    }
}

void shouldInvokeArp()
{
    bool v = false;
    for (uint8_t i = 0; i < 5; i++)
    {
        v |= channelArps[i];
    }
    invokeArp = v;
}
bool setArpTime = false;
uint8_t digit = 0;
uint8_t arpDigits[4] = { 0, 0, 0, 0 };
// consts
const uint16_t digitPlaces[5] = { 1, 10, 100, 1000, 10000 };
uint8_t getDigit(NoteName n)
{
    switch (n)
    {
        case NoteName::_A0:
            return 1;
        case NoteName::_B0:
            return 2;
        case NoteName::C1:
            return 3;
        case NoteName::_D1:
            return 4;
        case NoteName::E1:
            return 5;
        case NoteName::F1:
            return 6;
        case NoteName::G1:
            return 7;
        case NoteName::_A1:
            return 8;
        case NoteName::_B1:
            return 9;
        case NoteName::C2:
            return 0;
    }
    return 11;
}
void specialOptions()
{
    if (MIDI.read())
    {
        MidiCode type = MIDI.getType();
        uint8_t channel = MIDI.getChannel();
        switch (type)
        {
            case MidiCode::NoteON:
            {
                // OFF
                if (MIDI.getData2() == 0) { return; }
                NoteName n = MIDI.getNote();
                
                // enter time - digit must start at 0
                if (setArpTime)
                {
                    uint8_t dv = getDigit(n);
                    // valid digit
                    if (dv <= 9)
                    {
                        // no more digits
                        if (digit >= 4) { return; }
                        arpDigits[digit] = dv;
                        digit++;
                        return;
                    }
                }
                
                switch (n)
                {
                    // set to defaults
                    case NoteName::B3:
                    {
                        retriggerNew = true;
                        retriggerOld = false;
                        alwaysDelay = false;
                        setNote = _setNoteNorm;
                        setArpTime = false;
                        digit = 0;
                        clearArp(0);
                        clearArp(1);
                        clearArp(2);
                        clearArp(3);
                        clearArp(4);
                        initArps();
                        channelArps[0] = false;
                        channelArps[1] = false;
                        channelArps[2] = false;
                        channelArps[3] = false;
                        channelArps[4] = false;
                        invokeArp = false;
                        return;
                    }
                    case NoteName::C4:
                        retriggerOld = !retriggerOld;
                        return;
                    case NoteName::_D4:
                        retriggerNew = !retriggerNew;
                        return;
                    case NoteName::E4:
                        alwaysDelay = !alwaysDelay;
                        return;
                    case NoteName::F4:
                    {
                        if (setNote == _setNoteNorm)
                        {
                            setNote = _setSubNote;
                            return;
                        }
                        setNote = _setNoteNorm;
                        return;
                    }
                    case NoteName::G4:
                    {
                        bool v = !channelArps[channel];
                        channelArps[channel] = v;
                        if (!v) { clearArp(channel); }
                        else { clearNotes(&channels[channel]); }
                        shouldInvokeArp();
                        return;
                    }
                    case NoteName::C3:
                    {
                        setArpTime = !setArpTime;
                        // set arp time value
                        if (!setArpTime)
                        {
                            uint32_t bpm = 0;
                            // digit is the number of digits entered
                            if (digit == 0) { digit = 4; }
                            uint8_t place = 0;
                            // read in reverse order
                            for (int8_t i = digit - 1; i >= 0; i--)
                            {
                                bpm += arpDigits[i] * digitPlaces[place];
                                place++;
                            }
                            digit = 0;
                            arps[channel].timeOut = 60000 / bpm;
                        }
                        return;
                    }
                    case NoteName::_D3:
                        arps[channel].mode = 0;
                        return;
                    case NoteName::E3:
                        arps[channel].mode = 1;
                        return;
                    case NoteName::F3:
                        arps[channel].mode = 2;
                        return;
                    case NoteName::G3:
                        arps[channel].sort = !arps[channel].sort;
                        return;
                    case NoteName::_A3:
                        arps[channel].halfTime = !arps[channel].halfTime;
                        return;
                }
                return;
            }
        }
    }
}