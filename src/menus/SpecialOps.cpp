#include <Arduino.h>
#include "SpeicalOps.h"

#include "../../Arpeggio.h"
#include "../core/Globals.h"
#include "../core/Coms.h"
#include "../core/NVData.h"
#include "../notes/Notes.h"
#include "../../Callbacks.h"
#include "../../MemLocations.h"
#include "CCMenu.h"
#include "VMenu.h"
#include "../sequen/Sequencer.h"

bool invokeArp = false;
bool channelArps[5] = { false, false, false, false, false };

void factoryReset()
{
    clearAllNV();
    
    // reset all values then save them to flash
    
    resetOpsValues();
    saveOpsState();
    
    resetCCMValues();
    saveCCMState();
    
    resetVVMValues();
    saveVVMState();
    
    resetSeqValues();
    saveSeqState();
    
    for (uint8_t i = 0; i < 32; i++)
    {
        deleteSave(i);
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
bool _tapTempo = false;
uint32_t _tapTempoTime = 0;
bool _chooseFilter = false;
bool _setArpTime = false;
uint16_t _lv = 120;
uint8_t _resetCount = 0;
void manageSpecie(uint8_t channel, NoteName n)
{
    if (_tapTempo)
    {
        // setArpTimeOut(channel, millis() - tapTempoTime);
        arps[channel].timeOut = millis() - _tapTempoTime;
        _tapTempo = false;
        playNote(NOTEOPTION_S, MF_DURATION_SHORT);
        return;
    }
    if (_chooseFilter)
    {
        uint8_t fk = n - NoteName::C1;
        if (fk < 12)
        {
            filter = (Notes)fk;
            _chooseFilter = false;
            playNumber(n);
            return;
        }
        if (n != NoteName::Eb4)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    // enter time - digit must start at 0
    if (_setArpTime)
    {
        // valid digit
        bool v = addDigit(n, 4);
        if (v) { return; }
        if (n != NoteName::C3)
        {
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        }
    }
    
    // hard memory reset
    if (n == NoteName::B2)
    {
        _resetCount++;
        if (_resetCount >= 3)
        {
            _resetCount = 0;
            factoryReset();
            triggerFeedback(true);
            return;
        }
        triggerFeedback(false);
        return;
    }
    _resetCount = 0;
    
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
            _chooseFilter = !_chooseFilter;
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
            // if (!v) { clearArp(channel); }
            // else { clearNotes(&channels[channel]); }
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
            // clearArps();
            // clearNotes(&channels[0]);
            // clearNotes(&channels[1]);
            // clearNotes(&channels[2]);
            // clearNotes(&channels[3]);
            // clearNotes(&channels[4]);
            
            // gate = 0;
            // setGate(0);
            loadOpsState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::C5:
            allChannelMode = !allChannelMode;
            triggerFeedback(allChannelMode);
            return;
        case NoteName::Db5:
            altAllocations = !altAllocations;
            triggerFeedback(altAllocations);
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
        // arps
        case NoteName::C3:
        {
            _setArpTime = !_setArpTime;
            // set arp time value
            if (!_setArpTime)
            {
                uint16_t v = getEnteredValue(_lv);
                // min tempo
                if (v < 10)
                {
                    playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                    return;
                }
                _lv = v;
                // setArpTimeOut(channel, 60000 / bpm);
                arps[channel].timeOut = 60000 / v;
            }
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        }
        case NoteName::Db3:
            _tapTempo = true;
            _tapTempoTime = millis();
            playNote(NOTEOPTION_S, MF_DURATION_SHORT);
            return;
        case NoteName::_D3:
            // setArpMode(channel, 0);
            arps[channel].mode = 0;
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::Eb3:
            arpClocked = !arpClocked;
            triggerFeedback(arpClocked);
            return;
        case NoteName::E3:
            // setArpMode(channel, 1);
            arps[channel].mode = 1;
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::F3:
            // setArpMode(channel, 2);
            arps[channel].mode = 2;
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::G3:
            // setArpSort(channel, !arps[channel].sort);
            arps[channel].sort = !arps[channel].sort;
            triggerFeedbackC(arps[channel].sort, channel);
            return;
        case NoteName::_A3:
            // setArpHT(channel, !arps[channel].halfTime);
            arps[channel].halfTime = !arps[channel].halfTime;
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
    _setArpTime = false;
    sortNotes = false;
    resetArps();
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
    saveArps();
    
    openDataSpace(DataSpace::Special, true);
    
    setSpaceByte(RETRIG_OLD, retriggerOld);
    setSpaceByte(RETRIG_NEW, retriggerNew);
    setSpaceByte(ALWAYS_DELAY, alwaysDelay);
    setSpaceByte(MICROTONAL, setNote == _setSubNote);
    setSpaceByte(SORT_NOTES, sortNotes);
    
    setSpaceByte(ENABLE_ARP_1, channelArps[0]);
    setSpaceByte(ENABLE_ARP_2, channelArps[1]);
    setSpaceByte(ENABLE_ARP_3, channelArps[2]);
    setSpaceByte(ENABLE_ARP_4, channelArps[3]);
    setSpaceByte(ENABLE_ARP_5, channelArps[4]);
    
    setSpaceByte(FILTER_KEYS, filterKeys);
    setSpaceByte(KEY_FILTER, filter);
    setSpaceByte(MIDI_CLOCKED_ARP, arpClocked);
    setSpaceByte(FORGET_NOTES, forgetNotes);
    setSpaceByte(ALL_CHANNEL, allChannelMode);
    setSpaceByte(FEEDBACK_ENABLED, isMenuFeedback);
    
    commitSpace();
    closeDataSpace();
}
void loadOpsState()
{
    loadArps();
    
    openDataSpace(DataSpace::Special, false);
    
    retriggerOld = getSpaceByte(RETRIG_OLD);
    retriggerNew = getSpaceByte(RETRIG_NEW);
    alwaysDelay = getSpaceByte(ALWAYS_DELAY);
    bool microtone = getSpaceByte(MICROTONAL);
    sortNotes = getSpaceByte(SORT_NOTES);
    
    setNote = microtone ? _setSubNote : _setNoteNorm;
    
    channelArps[0] = getSpaceByte(ENABLE_ARP_1);
    channelArps[1] = getSpaceByte(ENABLE_ARP_2);
    channelArps[2] = getSpaceByte(ENABLE_ARP_3);
    channelArps[3] = getSpaceByte(ENABLE_ARP_4);
    channelArps[4] = getSpaceByte(ENABLE_ARP_5);
    shouldInvokeArp();
    
    filterKeys = getSpaceByte(FILTER_KEYS);
    filter = (Notes)getSpaceByte(KEY_FILTER);
    arpClocked = getSpaceByte(MIDI_CLOCKED_ARP);
    forgetNotes = getSpaceByte(FORGET_NOTES);
    allChannelMode = getSpaceByte(ALL_CHANNEL);
    isMenuFeedback = getSpaceByte(FEEDBACK_ENABLED);
    
    closeDataSpace();
}

Menu _s_Menu;
void specOpen()
{
    playNote(NOTESELECT_S, MF_DURATION);
}
void specExit()
{
    _s_Menu.active = false;
    _resetCount = 0;
    _setArpTime = false;
    _chooseFilter = false;
    getEnteredValue(0);
}

Menu* getSpecialMenu()
{
    _s_Menu = { true, specialOptions, specExit, specOpen };
    return &_s_Menu;
}