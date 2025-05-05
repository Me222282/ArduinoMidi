#include <Arduino.h>
#include "CCMenu.h"

#include "../../Callbacks.h"
#include "SpeicalOps.h"
#include "../core/Coms.h"
#include "../core/Globals.h"
#include "../core/NVData.h"
#include "../../MemLocations.h"

bool _setCCSource;
NoteName _ccSetPos;

uint16_t _lv_cc = 0;

uint8_t getSetPosLocation(NoteName pos)
{
    switch (pos)
    {
        case NoteName::C3:
            return 0;
        case NoteName::_D3:
            return 1;
        case NoteName::E3:
            return 2;
        case NoteName::F3:
            return 3;
        case NoteName::G3:
            return 4;
    }
    
    return 255;
}
void setNumber(NoteName pos)
{
    if (_ccSetPos != 0 && _ccSetPos != pos) { return; }
    
    uint8_t index = getSetPosLocation(pos);
    
    _ccSetPos = pos;
    _setCCSource = !_setCCSource;
    // set value
    if (!_setCCSource)
    {
        _ccSetPos = (NoteName)0;
        uint16_t v = getEnteredValue(_lv_cc);
        if (v > 127)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
        _lv_cc = v;
        ccListeners[index] = v;
    }
    playNote(NOTESELECT_S, MF_DURATION);
}

void manageMenuNotes(NoteName n)
{
    if (_setCCSource)
    {
        // valid digit
        bool v = addDigit(n, 3);
        if (v) { return; }
        if (n != _ccSetPos)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }

    switch (n)
    {
        // set to defaults
        case NoteName::B3:
            resetCCMValues();
            triggerFeedback(true);
            return;
        case NoteName::C4:
            ccOutputs[0] = !ccOutputs[0];
            triggerFeedback(ccOutputs[0]);
            return;
        case NoteName::_D4:
            ccOutputs[1] = !ccOutputs[1];
            triggerFeedback(ccOutputs[1]);
            return;
        case NoteName::E4:
            ccOutputs[2] = !ccOutputs[2];
            triggerFeedback(ccOutputs[2]);
            return;
        case NoteName::F4:
            ccOutputs[3] = !ccOutputs[3];
            triggerFeedback(ccOutputs[3]);
            return;
        case NoteName::G4:
            ccOutputs[4] = !ccOutputs[4];
            triggerFeedback(ccOutputs[4]);
            return;
        case NoteName::_A4:
            ccChannelMode = !ccChannelMode;
            triggerFeedback(ccChannelMode);
            return;
        
        case NoteName::B4:
            saveCCMState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::Bb4:
            loadCCMState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::C3:
            setNumber(n);
            return;
        case NoteName::_D3:
            setNumber(n);
            return;
        case NoteName::E3:
            setNumber(n);
            return;
        case NoteName::F3:
            setNumber(n);
            return;
        case NoteName::G3:
            setNumber(n);
            return;
    }
}

void ccMenuFunction()
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
                manageMenuNotes(MIDI.getNote());
                return;
            }
            case MidiCode::CC:
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return;
        }
    }
}

void resetCCMValues()
{
    ccListeners[0] = 0;
    ccListeners[1] = 0;
    ccListeners[2] = 0;
    ccListeners[3] = 0;
    ccListeners[4] = 0;
    ccOutputs[0] = false;
    ccOutputs[1] = false;
    ccOutputs[2] = false;
    ccOutputs[3] = false;
    ccOutputs[4] = false;
    ccChannelMode = false;
}

void saveCCMState()
{
    openDataSpace(DataSpace::CCData, true);
    
    setSpaceByte(CC_OUT_NUMBER_1, ccListeners[0]);
    setSpaceByte(CC_OUT_NUMBER_2, ccListeners[1]);
    setSpaceByte(CC_OUT_NUMBER_3, ccListeners[2]);
    setSpaceByte(CC_OUT_NUMBER_4, ccListeners[3]);
    setSpaceByte(CC_OUT_NUMBER_5, ccListeners[4]);
    
    setSpaceByte(CC_USE_OUT_1, ccOutputs[0]);
    setSpaceByte(CC_USE_OUT_2, ccOutputs[1]);
    setSpaceByte(CC_USE_OUT_3, ccOutputs[2]);
    setSpaceByte(CC_USE_OUT_4, ccOutputs[3]);
    setSpaceByte(CC_USE_OUT_5, ccOutputs[4]);
    
    setSpaceByte(CC_MULTI_CHANNEL, ccChannelMode);
    
    commitSpace();
    closeDataSpace();
}
void loadCCMState()
{
    openDataSpace(DataSpace::CCData, false);
    
    ccListeners[0] = getSpaceByte(CC_OUT_NUMBER_1);
    ccListeners[1] = getSpaceByte(CC_OUT_NUMBER_2);
    ccListeners[2] = getSpaceByte(CC_OUT_NUMBER_3);
    ccListeners[3] = getSpaceByte(CC_OUT_NUMBER_4);
    ccListeners[4] = getSpaceByte(CC_OUT_NUMBER_5);
    
    ccOutputs[0] = getSpaceByte(CC_USE_OUT_1);
    ccOutputs[1] = getSpaceByte(CC_USE_OUT_2);
    ccOutputs[2] = getSpaceByte(CC_USE_OUT_3);
    ccOutputs[3] = getSpaceByte(CC_USE_OUT_4);
    ccOutputs[4] = getSpaceByte(CC_USE_OUT_5);
    
    ccChannelMode = getSpaceByte(CC_MULTI_CHANNEL);
    
    closeDataSpace();
}

Menu _cc_Menu;
void ccOpen()
{
    playNote(NOTESELECT_S, MF_DURATION);
}
void ccExit()
{
    _cc_Menu.active = false;
}

Menu* getCCMenu()
{
    _cc_Menu = { true, ccMenuFunction, ccExit, ccOpen };
    return &_cc_Menu;
}