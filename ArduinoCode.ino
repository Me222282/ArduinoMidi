#include <SPI.h>
// #include "Notes.h"
// #include "Coms.h"
#include "Midi.h"
// #include "Channels.h"
#include "Callbacks.h"
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
}

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
    }
    if (activeChannels != oldC || activeVoices != oldV)
    {
        setChannels(activeChannels, activeVoices);
        gate = 0;
        setGate(gate);
        updateAllPBs();
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

void loop()
{
    readControls();
    
    // 5, 5 for special options
    if (activeChannels == 5 && activeVoices == 5)
    {
        specialOptions();
        return;
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
                uint8_t n = MIDI.getData1();
                uint8_t vel = MIDI.getData2();
                if (type == MidiCode::NoteOFF || vel == 0)
                {
                    onNoteOff(channel, n);
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

void specialOptions()
{
    if (MIDI.read())
    {
        MidiCode type = MIDI.getType();
        switch (type)
        {
            case MidiCode::NoteON:
            {
                NoteName n = MIDI.getNote();
                // OFF
                if (MIDI.getData2() == 0) { return; }
                
                switch (n)
                {
                    case NoteName::B3:
                        retriggerNew = true;
                        retriggerOld = false;
                        alwaysDelay = false;
                        return;
                    case NoteName::C4:
                        retriggerOld = !retriggerOld;
                        return;
                    case NoteName::_D4:
                        retriggerNew = !retriggerNew;
                        return;
                    case NoteName::E4:
                        alwaysDelay = !alwaysDelay;
                        return;
                }
                return;
            }
        }
    }
}