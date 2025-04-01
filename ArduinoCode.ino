#include <SPI.h>
// #include "Notes.h"
// #include "Coms.h"
// #include "Midi.h"
// #include "Channels.h"
// #include "Callbacks.h"
#include "Arpeggio.h"
#include "Controls.h"
#include "SpeicalOps.h"

Midi MIDI(Serial0);

void resetValues();
void saveSate();
void loadSate();

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
    EEPROM.begin(45);
    
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
    loadArps();
    loadSate();
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
        clearArps();
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
        clearArps();
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
                CCType number = MIDI.getCC();
                if (number == CCType::ResetAllControllers)
                {
                    resetValues();
                    return;
                }
                
                onControlChange(channel, number, MIDI.getData2());
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

