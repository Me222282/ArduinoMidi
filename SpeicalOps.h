#ifndef __spec
#define __spec

#include "Midi.h"
#include "Channels.h"
#include "Callbacks.h"

bool invokeArp = false;
bool channelArps[5] = { false, false, false, false, false };

void shouldInvokeArp()
{
    bool v = false;
    for (uint8_t i = 0; i < 5; i++)
    {
        v |= channelArps[i];
    }
    invokeArp = v;
}
bool choseFilter = false;
bool setArpTime = false;
uint8_t digit = 0;
uint32_t lv = 0;
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
                
                if (choseFilter)
                {
                    uint8_t fk = n - NoteName::C1;
                    if (fk < 12)
                    {
                        filter = (Notes)fk;
                        choseFilter = false;
                        return;
                    }
                }
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
                        resetValues();
                        return;
                    case NoteName::C4:
                        retriggerOld = !retriggerOld;
                        return;
                    case NoteName::Db4:
                        filterKeys = !filterKeys;
                        return;
                    case NoteName::_D4:
                        retriggerNew = !retriggerNew;
                        return;
                    case NoteName::Eb4:
                        choseFilter = !choseFilter;
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
                    case NoteName::_A4:
                        sortNotes = !sortNotes;
                        return;
                    case NoteName::B4:
                        saveSate();
                        return;
                    case NoteName::C5:
                        loadSate();
                        
                        clearArps();
                        clearNotes(&channels[0]);
                        clearNotes(&channels[1]);
                        clearNotes(&channels[2]);
                        clearNotes(&channels[3]);
                        clearNotes(&channels[4]);
                        
                        gate = 0;
                        setGate(0);
                        return;
                    case NoteName::C3:
                    {
                        setArpTime = !setArpTime;
                        // set arp time value
                        if (!setArpTime)
                        {
                            // digit is the number of digits entered
                            if (digit == 0)
                            {
                                setArpTimeOut(channel, 60000 / lv);
                                return;
                            }
                            uint32_t bpm = 0;
                            uint8_t place = 0;
                            // read in reverse order
                            for (int8_t i = digit - 1; i >= 0; i--)
                            {
                                bpm += arpDigits[i] * digitPlaces[place];
                                place++;
                            }
                            digit = 0;
                            lv = bpm;
                            setArpTimeOut(channel, 60000 / bpm);
                        }
                        return;
                    }
                    case NoteName::_D3:
                        setArpMode(channel, 0);
                        return;
                    case NoteName::E3:
                        setArpMode(channel, 1);
                        return;
                    case NoteName::F3:
                        setArpMode(channel, 2);
                        return;
                    case NoteName::G3:
                        setArpSort(channel, !arps[channel].sort);
                        return;
                    case NoteName::_A3:
                        setArpHT(channel, !arps[channel].halfTime);
                        return;
                }
                return;
            }
        }
    }
}

void resetValues()
{
    retriggerNew = true;
    retriggerOld = false;
    alwaysDelay = false;
    setNote = _setNoteNorm;
    setArpTime = false;
    sortNotes = false;
    digit = 0;
    initArps();
    channelArps[0] = false;
    channelArps[1] = false;
    channelArps[2] = false;
    channelArps[3] = false;
    channelArps[4] = false;
    invokeArp = false;
    
    filterKeys = false;
    filter = Notes::C;
}

void saveSate()
{
    eeWrite(35, retriggerOld);
    eeWrite(36, retriggerNew);
    eeWrite(37, alwaysDelay);
    eeWrite(38, setNote == _setSubNote);
    eeWrite(39, sortNotes);
    
    eeWrite(40, channelArps[0]);
    eeWrite(41, channelArps[1]);
    eeWrite(42, channelArps[2]);
    eeWrite(43, channelArps[3]);
    eeWrite(44, channelArps[4]);
    
    eeWrite(45, filterKeys);
    eeWrite(46, filter);
    EEPROM.commit();
}
void loadSate()
{
    retriggerOld = EEPROM.read(35);
    retriggerNew = EEPROM.read(36);
    alwaysDelay = EEPROM.read(37);
    bool microtone = EEPROM.read(38);
    sortNotes = EEPROM.read(39);
    
    setNote = microtone ? _setSubNote : _setNoteNorm;
    
    channelArps[0] = EEPROM.read(40);
    channelArps[1] = EEPROM.read(41);
    channelArps[2] = EEPROM.read(42);
    channelArps[3] = EEPROM.read(43);
    channelArps[4] = EEPROM.read(44);
    shouldInvokeArp();
    
    filterKeys = EEPROM.read(45);
    filter = EEPROM.read(46);
}

#endif