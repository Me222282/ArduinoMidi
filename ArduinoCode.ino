#include <SPI.h>
// #include "Notes.h"
// #include "Coms.h"
#include "Midi.h"
// #include "Channels.h"
#include "Callbacks.h"

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
}

void readControls()
{
    int8_t oldOO = octaveOffset;
    octaveOffset = (int8_t)round(analogRead(OCTAVE) / 682.666666667f) - 3;
    
    uint8_t oldC = activeChannels;
    activeChannels = (uint8_t)round(analogRead(CHANNELS) / 1024.0f) + 1;
    uint8_t oldV = activeVoices;
    activeVoices = (uint8_t)round(analogRead(VOICES) / 1024.0f) + 1;
    
    float oldPBS = pitchBendSelect;
    pitchBendSelect = pbDiv[(uint8_t)round(analogRead(PITCHBEND) / 819.2f)];
    
    // Serial.println("READ1");
    
    bool oldM1 = mode1;
    mode1 = digitalRead(MODE1);
    uint8_t oldM2 = mode2;
    mode2 = (uint8_t)round(analogRead(MODE2) / 2048.0f);
    
    bool oldOp = option;
    option = digitalRead(OPTIONMODE);
    
    // Serial.println("READ2");
    
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
    
    // 5 - 5 for special options
    if (activeChannels == activeVoices && activeVoices == 5)
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
            case MidiCode::NoteOFF:
            {
                NoteName n = MIDI.getNote();
                uint8_t vel = MIDI.getData2();
                if (type == MidiCode::NoteOFF || vel == 0)
                {
                    // OFF
                    return;
                }
                
                switch (n)
                {
                    case NoteName::C4:
                        retriggerOld = !retriggerOld;
                        return;
                    case NoteName::D4:
                        retriggerNew = !retriggerNew;
                        return;
                }
                return;
            }
        }
    }
}