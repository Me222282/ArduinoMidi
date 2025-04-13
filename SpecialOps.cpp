#include <Arduino.h>
#include <EEPROM.h>
#include "SpeicalOps.h"

#include "Arpeggio.h"
#include "src/core/Globals.h"
#include "src/core/Coms.h"
#include "src/notes/Notes.h"
#include "Callbacks.h"
#include "MenuFeedback.h"
#include "MemLocations.h"

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
bool tapTempo = false;
uint32_t tapTempoTime = 0;
bool choseFilter = false;
bool setArpTime = false;
uint8_t digit = 0;
uint32_t lv = 0;
uint8_t arpDigits[4] = { 0, 0, 0, 0 };
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
void manageSpecie(uint8_t channel, NoteName n)
{
    if (tapTempo)
    {
        setArpTimeOut(channel, millis() - tapTempoTime);
        tapTempo = false;
        playNote(NOTEOPTION_S, MF_DURATION_SHORT);
        return;
    }
    if (choseFilter)
    {
        uint8_t fk = n - NoteName::C1;
        if (fk < 12)
        {
            filter = (Notes)fk;
            choseFilter = false;
            playNote((NoteName)(fk + NoteName::C4), MF_DURATION);
            return;
        }
        if (n != NoteName::Eb4) { return; }
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
            playNumber(n);
            return;
        }
        if (n != NoteName::C3) { return; }
    }
    
    switch (n)
    {
        // set to defaults
        case NoteName::B3:
            resetOpsValues();
            triggerFeedback(true);
            return;
        case NoteName::C4:
            retriggerOld = !retriggerOld;
            triggerFeedback(retriggerOld);
            return;
        case NoteName::Db4:
            filterKeys = !filterKeys;
            triggerFeedback(filterKeys);
            return;
        case NoteName::_D4:
            retriggerNew = !retriggerNew;
            triggerFeedback(retriggerNew);
            return;
        case NoteName::Eb4:
            choseFilter = !choseFilter;
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        case NoteName::E4:
            alwaysDelay = !alwaysDelay;
            triggerFeedback(alwaysDelay);
            return;
        case NoteName::F4:
        {
            if (setNote == _setNoteNorm)
            {
                setNote = _setSubNote;
                triggerFeedback(true);
                return;
            }
            setNote = _setNoteNorm;
            triggerFeedback(false);
            return;
        }
        case NoteName::Gb4:
            forgetNotes = !forgetNotes;
            triggerFeedback(forgetNotes);
            return;
        case NoteName::G4:
        {
            bool v = !channelArps[channel];
            channelArps[channel] = v;
            if (!v) { clearArp(channel); }
            else { clearNotes(&channels[channel]); }
            shouldInvokeArp();
            triggerFeedbackC(v, channel);
            return;
        }
        case NoteName::_A4:
            sortNotes = !sortNotes;
            triggerFeedback(sortNotes);
            return;
        case NoteName::B4:
            saveOpsState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::Bb4:
            loadOpsState();
            
            clearArps();
            clearNotes(&channels[0]);
            clearNotes(&channels[1]);
            clearNotes(&channels[2]);
            clearNotes(&channels[3]);
            clearNotes(&channels[4]);
            
            gate = 0;
            setGate(0);
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::C5:
            allChannelMode = !allChannelMode;
            triggerFeedback(allChannelMode);
            return;
        case NoteName::_D5:
            if (isMenuFeedback)
            {
                triggerFeedback(false);
                isMenuFeedback = false;
                return;
            }
            isMenuFeedback = true;
            triggerFeedback(true);
            return;
        case NoteName::C3:
        {
            setArpTime = !setArpTime;
            playNote(NOTESELECT_S, MF_DURATION);
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
        case NoteName::Db3:
            tapTempo = true;
            tapTempoTime = millis();
            playNote(NOTEOPTION_S, MF_DURATION_SHORT);
            return;
        case NoteName::_D3:
            setArpMode(channel, 0);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::Eb3:
            arpClocked = !arpClocked;
            triggerFeedback(arpClocked);
            return;
        case NoteName::E3:
            setArpMode(channel, 1);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::F3:
            setArpMode(channel, 2);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::G3:
            setArpSort(channel, !arps[channel].sort);
            triggerFeedbackC(arps[channel].sort, channel);
            return;
        case NoteName::_A3:
            setArpHT(channel, !arps[channel].halfTime);
            triggerFeedbackC(arps[channel].halfTime, channel);
            return;
    }
}
void specialOptions()
{
    invokeMF();
    
    if (MIDI.read())
    {
        MidiCode type = MIDI.getType();
        uint8_t channel = MIDI.getChannel();
        if (channel >= 5) { return; }
        switch (type)
        {
            case MidiCode::NoteON:
            {
                // OFF
                if (MIDI.getData2() == 0) { return; }
                manageSpecie(channel, MIDI.getNote());
                return;
            }
            case MidiCode::CC:
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return;
        }
    }
}

void resetOpsValues()
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
    arpClocked = false;
    forgetNotes = false;
    allChannelMode = false;
    isMenuFeedback = true;
}

void saveOpsState()
{
    eeWrite(RETRIG_OLD, retriggerOld);
    eeWrite(RETRIG_NEW, retriggerNew);
    eeWrite(ALWAYS_DELAY, alwaysDelay);
    eeWrite(MICROTONAL, setNote == _setSubNote);
    eeWrite(SORT_NOTES, sortNotes);
    
    eeWrite(ENABLE_ARP_1, channelArps[0]);
    eeWrite(ENABLE_ARP_2, channelArps[1]);
    eeWrite(ENABLE_ARP_3, channelArps[2]);
    eeWrite(ENABLE_ARP_4, channelArps[3]);
    eeWrite(ENABLE_ARP_5, channelArps[4]);
    
    eeWrite(FILTER_KEYS, filterKeys);
    eeWrite(KEY_FILTER, filter);
    eeWrite(MIDI_CLOCKED_ARP, arpClocked);
    eeWrite(FORGET_NOTES, forgetNotes);
    eeWrite(ALL_CHANNEL, allChannelMode);
    eeWrite(FEEDBACK_ENABLED, isMenuFeedback);
    EEPROM.commit();
}
void loadOpsState()
{
    retriggerOld = EEPROM.read(RETRIG_OLD);
    retriggerNew = EEPROM.read(RETRIG_NEW);
    alwaysDelay = EEPROM.read(ALWAYS_DELAY);
    bool microtone = EEPROM.read(MICROTONAL);
    sortNotes = EEPROM.read(SORT_NOTES);
    
    setNote = microtone ? _setSubNote : _setNoteNorm;
    
    channelArps[0] = EEPROM.read(ENABLE_ARP_1);
    channelArps[1] = EEPROM.read(ENABLE_ARP_2);
    channelArps[2] = EEPROM.read(ENABLE_ARP_3);
    channelArps[3] = EEPROM.read(ENABLE_ARP_4);
    channelArps[4] = EEPROM.read(ENABLE_ARP_5);
    shouldInvokeArp();
    
    filterKeys = EEPROM.read(FILTER_KEYS);
    filter = (Notes)EEPROM.read(KEY_FILTER);
    arpClocked = EEPROM.read(MIDI_CLOCKED_ARP);
    forgetNotes = EEPROM.read(FORGET_NOTES);
    allChannelMode = EEPROM.read(ALL_CHANNEL);
    isMenuFeedback = EEPROM.read(FEEDBACK_ENABLED);
}