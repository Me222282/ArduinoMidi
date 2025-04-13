#include <Arduino.h>
#include <EEPROM.h>
#include "CCMenu.h"

#include "Callbacks.h"
#include "MenuFeedback.h"
#include "SpeicalOps.h"
#include "src/core/Coms.h"
#include "src/core/Globals.h"
#include "MemLocations.h"

bool setCCSource;
NoteName ccSetPos;

uint8_t digit_cc = 0;
uint32_t lv_cc = 0;
uint8_t ccDigits[3] = { 0, 0, 0 };

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
    if (ccSetPos != 0 && ccSetPos != pos) { return; }
    
    uint8_t index = getSetPosLocation(pos);
    
    ccSetPos = pos;
    setCCSource = !setCCSource;
    playNote(NOTESELECT_S, MF_DURATION);
    // set arp time value
    if (!setCCSource)
    {
        ccSetPos = (NoteName)0;
        // digit is the number of digits entered
        if (digit_cc == 0)
        {
            ccListeners[index] = lv_cc;
            return;
        }
        uint32_t listener = 0;
        uint8_t place = 0;
        // read in reverse order
        for (int8_t i = digit_cc - 1; i >= 0; i--)
        {
            listener += ccDigits[i] * digitPlaces[place];
            place++;
        }
        digit_cc = 0;
        lv_cc = listener;
        ccListeners[index] = listener;
    }
}

void manageMenuNotes(NoteName n)
{
    // enter time - digit must start at 0
    if (setCCSource)
    {
        uint8_t dv = getDigit(n);
        // valid digit
        if (dv <= 9)
        {
            // no more digits
            if (digit_cc >= 3) { return; }
            ccDigits[digit_cc] = dv;
            digit_cc++;
            playNumber(n);
            return;
        }
        if (n != ccSetPos) { return; }
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
    eeWrite(CC_OUT_NUMBER_1, ccListeners[0]);
    eeWrite(CC_OUT_NUMBER_2, ccListeners[1]);
    eeWrite(CC_OUT_NUMBER_3, ccListeners[2]);
    eeWrite(CC_OUT_NUMBER_4, ccListeners[3]);
    eeWrite(CC_OUT_NUMBER_5, ccListeners[4]);
    
    eeWrite(CC_USE_OUT_1, ccOutputs[0]);
    eeWrite(CC_USE_OUT_2, ccOutputs[1]);
    eeWrite(CC_USE_OUT_3, ccOutputs[2]);
    eeWrite(CC_USE_OUT_4, ccOutputs[3]);
    eeWrite(CC_USE_OUT_5, ccOutputs[4]);
    
    eeWrite(CC_MULTI_CHANNEL, ccChannelMode);
    EEPROM.commit();
}
void loadCCMState()
{
    ccListeners[0] = EEPROM.read(CC_OUT_NUMBER_1);
    ccListeners[1] = EEPROM.read(CC_OUT_NUMBER_2);
    ccListeners[2] = EEPROM.read(CC_OUT_NUMBER_3);
    ccListeners[3] = EEPROM.read(CC_OUT_NUMBER_4);
    ccListeners[4] = EEPROM.read(CC_OUT_NUMBER_5);
    
    ccOutputs[0] = EEPROM.read(CC_USE_OUT_1);
    ccOutputs[1] = EEPROM.read(CC_USE_OUT_2);
    ccOutputs[2] = EEPROM.read(CC_USE_OUT_3);
    ccOutputs[3] = EEPROM.read(CC_USE_OUT_4);
    ccOutputs[4] = EEPROM.read(CC_USE_OUT_5);
    
    ccChannelMode = EEPROM.read(CC_MULTI_CHANNEL);
}