#include <Arduino.h>
#include "CCMenu.h"

#include "../../Callbacks.h"
#include "SpeicalOps.h"
#include "../core/Coms.h"
#include "../core/Globals.h"
#include "../core/NVData.h"
#include "../../MemLocations.h"
#include "../notes/Channels.h"

bool _setCCSource;
NoteName _ccSetPos;

uint16_t _lv_cc = 0;

enum PulseSource : uint8_t
{
    Arpeggio,
    Sequencer,
    Track
};

typedef struct
{
    bool enabled;
    PulseSource source;
    uint8_t channel;
} Pulse;

bool _useCCNumber[5];
Pulse _pulseSlots[5];

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

uint32_t _pulseTimes[5];
uint32_t _timeOut_Pulse = 5;

void updateCCOuts()
{
    if (ccChannelMode)
    {
        for (uint8_t i = 0; i < activeChannels; i++)
        {
            if (!_useCCNumber[i]) { continue; }
            Channel* c = &channels[i];
            uint8_t end = c->position + c->places;
            for (uint8_t j = c->position; j < end; j++)
            {
                ccOutputs[j] = !_pulseSlots[j].enabled;
                _pulseTimes[j] = 0;
            }
        }
        return;
    }
    
    for (uint8_t i = 0; i < 5; i++)
    {
        ccOutputs[i] = _useCCNumber[i] && !_pulseSlots[i].enabled;
        _pulseTimes[i] = 0;
    }
}

void onSequence()
{
    uint32_t time = millis();
    for (uint8_t i = 0; i < 5; i++)
    {
        Pulse p = _pulseSlots[i];
        if (!p.enabled || p.source != PulseSource::Sequencer) { continue; }
        setVelUnchecked(i, 0xFF);
        _pulseTimes[i] = time;
    }
}
void onTrack(uint8_t channel)
{
    uint32_t time = millis();
    for (uint8_t i = 0; i < 5; i++)
    {
        Pulse p = _pulseSlots[i];
        if (!p.enabled || p.source != PulseSource::Track || p.channel != channel) { continue; }
        setVelUnchecked(i, 0xFF);
        _pulseTimes[i] = time;
    }
}
void onArp(uint8_t channel)
{
    uint32_t time = millis();
    for (uint8_t i = 0; i < 5; i++)
    {
        Pulse p = _pulseSlots[i];
        if (!p.enabled || p.source != PulseSource::Arpeggio || p.channel != channel) { continue; }
        setVelUnchecked(i, 0xFF);
        _pulseTimes[i] = time;
    }
}

void invokePulse()
{
    uint32_t time = millis();
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint32_t pt = _pulseTimes[i];
        if (!pt || time < (pt + _timeOut_Pulse)) { continue; }
        setVelUnchecked(i, 0);
        _pulseTimes[i] = 0;
    }
}

bool _setPulseChan = false;
bool _setPulseLength = false;
uint8_t _lv_chan = 0;
uint8_t _lv_pl = 5;
void manageMenuNotes(NoteName n, uint8_t channel)
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
    
    if (_setPulseChan)
    {
        // valid digit
        bool v = addDigit(n, 2);
        if (v) { return; }
        if (n != NoteName::G3)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    if (_setPulseLength)
    {
        // valid digit
        bool v = addDigit(n, 4);
        if (v) { return; }
        if (n != NoteName::_A3)
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
            _useCCNumber[0] = !_useCCNumber[0];
            updateCCOuts();
            triggerFeedback(_useCCNumber[0]);
            return;
        case NoteName::_D4:
            _useCCNumber[1] = !_useCCNumber[1];
            updateCCOuts();
            triggerFeedback(_useCCNumber[1]);
            return;
        case NoteName::E4:
            _useCCNumber[2] = !_useCCNumber[2];
            updateCCOuts();
            triggerFeedback(_useCCNumber[2]);
            return;
        case NoteName::F4:
            _useCCNumber[3] = !_useCCNumber[3];
            updateCCOuts();
            triggerFeedback(_useCCNumber[3]);
            return;
        case NoteName::G4:
            _useCCNumber[4] = !_useCCNumber[4];
            updateCCOuts();
            triggerFeedback(_useCCNumber[4]);
            return;
        case NoteName::_A4:
            ccChannelMode = !ccChannelMode;
            updateCCOuts();
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
        case NoteName::C5:
            setNumber(n);
            return;
        case NoteName::_D5:
            setNumber(n);
            return;
        case NoteName::E5:
            setNumber(n);
            return;
        case NoteName::F5:
            setNumber(n);
            return;
        case NoteName::G5:
            setNumber(n);
            return;
        
        // PULSE
        case NoteName::C3:
            _pulseSlots[channel].enabled = !_pulseSlots[channel].enabled;
            updateCCOuts();
            triggerFeedback(_pulseSlots[channel].enabled);
            return;
        case NoteName::_D3:
            _pulseSlots[channel].source = PulseSource::Arpeggio;
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::E3:
            _pulseSlots[channel].source = PulseSource::Sequencer;
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::F3:
            _pulseSlots[channel].source = PulseSource::Track;
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::G3:
        {
            _setPulseChan = !_setPulseChan;
            // set arp time value
            if (!_setPulseChan)
            {
                uint16_t v = getEnteredValue(_lv_chan);
                // min tempo
                if (v > 16 || v < 1)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                _lv_chan = v;
                _pulseSlots[channel].channel = v;
            }
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
        case NoteName::_A3:
        {
            _setPulseLength = !_setPulseLength;
            // set arp time value
            if (!_setPulseLength)
            {
                uint16_t v = getEnteredValue(_lv_pl);
                // min tempo
                if (v == 0)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                _lv_pl = v;
                _timeOut_Pulse = v;
            }
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
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
                manageMenuNotes(MIDI.getNote(), channel);
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
    _useCCNumber[0] = false;
    _useCCNumber[1] = false;
    _useCCNumber[2] = false;
    _useCCNumber[3] = false;
    _useCCNumber[4] = false;
    ccChannelMode = false;
    
    for (uint8_t i = 0; i < 5; i++)
    {
        _pulseSlots[i] = { false, PulseSource::Sequencer, 0 };
    }
    _timeOut_Pulse = 5;
}

void saveCCMState()
{
    openDataSpace(DataSpace::CCData, true);
    
    setSpaceByte(CC_OUT_NUMBER_1, ccListeners[0]);
    setSpaceByte(CC_OUT_NUMBER_2, ccListeners[1]);
    setSpaceByte(CC_OUT_NUMBER_3, ccListeners[2]);
    setSpaceByte(CC_OUT_NUMBER_4, ccListeners[3]);
    setSpaceByte(CC_OUT_NUMBER_5, ccListeners[4]);
    
    setSpaceByte(CC_USE_OUT_1, _useCCNumber[0]);
    setSpaceByte(CC_USE_OUT_2, _useCCNumber[1]);
    setSpaceByte(CC_USE_OUT_3, _useCCNumber[2]);
    setSpaceByte(CC_USE_OUT_4, _useCCNumber[3]);
    setSpaceByte(CC_USE_OUT_5, _useCCNumber[4]);
    
    setSpaceByte(CC_MULTI_CHANNEL, ccChannelMode);
    
    for (uint8_t i = 0; i < 5; i++)
    {
        Pulse p = _pulseSlots[i];
        setSpaceData(i, p);
    }
    setSpaceInt(77, _timeOut_Pulse);
    
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
    
    _useCCNumber[0] = getSpaceByte(CC_USE_OUT_1);
    _useCCNumber[1] = getSpaceByte(CC_USE_OUT_2);
    _useCCNumber[2] = getSpaceByte(CC_USE_OUT_3);
    _useCCNumber[3] = getSpaceByte(CC_USE_OUT_4);
    _useCCNumber[4] = getSpaceByte(CC_USE_OUT_5);
    
    ccChannelMode = getSpaceByte(CC_MULTI_CHANNEL);
    
    for (uint8_t i = 0; i < 5; i++)
    {
        _pulseSlots[i] = getSpaceData<Pulse>(i);
    }
    _timeOut_Pulse = getSpaceInt(77);
    
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
    _setCCSource = false;
    _setPulseChan = false;
    _setPulseLength = false;
    _ccSetPos = (NoteName)0;
    getEnteredValue(0);
}

Menu* getCCMenu()
{
    _cc_Menu = { true, ccMenuFunction, ccExit, ccOpen };
    return &_cc_Menu;
}