#include <SPI.h>
#include "Notes.h"
#include "Coms.h"
#include "Midi.h"
#include "Channels.h"

Midi MIDI(Serial0);

void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
void onNoteOff(uint8_t channel, uint8_t note);
void onPitchBend(uint8_t channel, uint16_t bend);
void onControlChange(uint8_t channel, CCType number, uint8_t value);
void updateAllNotes();
void updateAllPBs();
void updateOtherPorts();

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
    // Serial.println("OK");
    
    readControls();
    
    // Serial.println("PRE_MIDI");
    
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
                    break;
                }
                
                onNoteOn(channel, n, vel);
                break;
            }
            case MidiCode::CC:
            {
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                break;
            }
            case MidiCode::PitchWheel:
            {
                onPitchBend(channel, MIDI.getCombinedData());
                break;
            }
        }
    }
    
    // // only change if multiple in a row e.g. 5
    // // or only read on a certain condition e.g. midi message
    // int r = analogRead(PITCHBEND);
    // pitchBendSelect = (uint8_t)round(r / 819.2f);
}

void updateSlot(uint8_t com, Note n)
{
    n.key += (octaveOffset * 12);
    n.vel <<= 1;
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        if (v != com) { continue; }
        
        Note old = oldValues[i];
        if (old.key != n.key)
        {
            setNote(i, n.key);
        }
        if (!option && old.vel != n.vel)
        {
            setVel(i, n.vel);
        }
        oldValues[i] = n;
    }
    
    // other velocity
    if (option && com == 0)
    {
        // 8bit to 12bit
        uint16_t vel = n.vel << 4;
        if (oldMod != vel)
        {
            oldMod = vel;
            setMod(vel);
        }
    }
}
void onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (channel >= activeChannels) { return; }
    Channel* c = &channels[channel];
    Note n = { note, velocity };
    int8_t vi = pushNote(c, channel, n);
    
    if (vi < 0) { return; }
    
    // set data
    if (reTrig)
    {
        // turn off retriggered notes
        setGate(gateCurrent & ~reTrig);
        reTrig = 0;
        delay(RETRIGTIME);
    }
    
    uint8_t com = (channel << 4) | (uint8_t)vi;

    updateSlot(com, n);
    
    // gate
    setGate(gate);
}
void onNoteOff(uint8_t channel, uint8_t note)
{
    if (channel >= activeChannels) { return; }
    Channel* c = &channels[channel];
    int8_t vi = removeNote(c, channel, note);
    
    if (vi >= 0)
    {
        uint8_t com = (channel << 4) | (uint8_t)vi;
        Note n = c->locations[vi]->value;
        updateSlot(com, n);
    }
    
    // gate
    setGate(gate);
}
void onPitchBend(uint8_t channel, uint16_t bend)
{
    if (channel >= activeChannels) { return; }
    Channel* c = &channels[channel];
    
    c->pitchBend = bend;
    
    // set data
    uint8_t end = c->position + c->places;
    for (uint8_t i = c->position; i < end; i++)
    {
        setPitchBend(i, bend >> 2);
    }
}
void updateMod(Channel* c, uint16_t mod)
{
    // set data
    // goes to vel
    if (option)
    {
        mod >>= 6;
        
        uint8_t end = c->position + c->places;
        for (uint8_t i = c->position; i < end; i++)
        {
            if (oldValues[i].vel == mod) { continue; }
            oldValues[i].vel = mod;
            setVel(i, mod);
        }
        return;
    }
    
    if (c->position != 0) { return; }
    // goes to mod
    mod >>= 2;
    if (oldMod == mod) { return; }
    oldMod = mod;
    setMod(mod);
}
void onControlChange(uint8_t channel, CCType number, uint8_t value)
{
    // cc 01 is mod wheel
    if (channel >= activeChannels) { return; }
    Channel* c = &channels[channel];
    
    uint16_t mod = c->modulation;
    if (number == CCType::ModulationWheel_MSB)
    {
        c->modulation = (mod & 0x007F) | (value << 7);
        updateMod(c, mod);
        return;
    }
    if (number != CCType::ModulationWheel_LSB) { return; }
    // fine mod control
    c->modulation = (mod & 0x3F80) | value;
    updateMod(c, mod);
}
void updateAllNotes()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        uint8_t chI = v >> 4;
        uint8_t vI = v & 0b00001111;
        
        Channel* c = &channels[chI];
        NoteList* nl = c->locations[vI];
        if (!nl) { continue; }
        uint8_t n = nl->value.key;
        n += (octaveOffset * 12);
        uint8_t old = oldValues[i].key;
        // changes to make
        if (old != n)
        {
            setNote(i, n);
        }
        oldValues[i].key = n;
    }
}
void updateAllPBs()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        uint8_t chI = v >> 4;
        uint8_t vI = v & 0b00001111;
        
        Channel* c = &channels[chI];
        
        setPitchBend(i, c->pitchBend >> 2);
    }
}
void updateOtherPorts()
{
    Channel* c = &channels[0];
    
    if (option)
    {
        for (uint8_t i = 0; i < activeChannels; i++)
        {
            Channel* c = &channels[i];
            updateMod(c, c->modulation);
        }
        
        NoteList* nl = c->locations[0];
        if (!nl) { return; }
        
        // 7bit to 12bit
        uint16_t vel = nl->value.vel << 4;
        if (oldMod != vel)
        {
            oldMod = vel;
            setMod(vel);
        }
        return;
    }
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t v = slotAllocation[i];
        uint8_t chI = v >> 4;
        uint8_t vI = v & 0b00001111;
        
        Channel* c = &channels[chI];
        NoteList* nl = c->locations[vI];
        if (!nl) { continue; }
        uint16_t vel = nl->value.vel << 1;
        if (oldValues[i].vel != vel)
        {
            setVel(i, vel);
        }
    }
    
    updateMod(c, c->modulation);
}