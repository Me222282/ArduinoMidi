#include <Arduino.h>
#include <EEPROM.h>
#include "TMenu.h"

#include "../../Callbacks.h"
#include "SpeicalOps.h"
#include "../core/Coms.h"
#include "../core/Globals.h"
#include "../../MemLocations.h"
#include "../notes/Channels.h"

typedef struct
{
    float (*function)(float);
    float w;
    float scale;
    bool enabled;
} Tremolo;

typedef struct
{
    int8_t oct;
    int8_t st;
} Offset;

bool _runTrem = false;
bool _globalRate = false;
Tremolo _tremolos[5];
Offset _noteOffsets[5];

void setCP(Channel* c, int16_t value)
{
    uint8_t end = c->position + c->places;
    for (uint8_t i = c->position; i < end; i++)
    {
        pbOffsets[i] = value;
        updatePitchBend(i);
    }
}
void invokeTremelo()
{
    if (!_runTrem) { return; }
    float time = (float)millis();
    
    if (_globalRate)
    {
        float y = _tremolos[0].function(time * _tremolos[0].w);
        for (uint8_t i = 0; i < activeChannels; i++)
        {
            Tremolo t = _tremolos[i];
            if (!t.enabled) { continue; }
            float nv = y * channels[i].modulation * t.scale;
            setCP(&channels[i], (int16_t)nv);
        }
        return;
    }
    
    for (uint8_t i = 0; i < activeChannels; i++)
    {
        Tremolo t = _tremolos[i];
        if (!t.enabled) { continue; }
        float v = t.function(time * t.w);
        v *= channels[i].modulation * t.scale;
        setCP(&channels[i], (int16_t)v);
    }
}

const float grad1 = 2.0f / PI;
const float grad2 = -grad1;
float tri(float t)
{
    t = fmodf(t, TWO_PI);
    if (t >= PI)
    {
        return 3.0f + (t * grad2);
    }
    return (t * grad1) - 1.0f;
}

const float setW = PI * 0.002f;
const float setmW = PI * 0.000002f;
const float setScale = 1.0f / 16384.0f;
const float initW = 4.0f * setW;
const float initScale = 43.0f * setScale;
void resetTTMValues()
{
    _runTrem = false;
    _tremolos[0] = { sinf, initW, initScale, false };
    _tremolos[1] = { sinf, initW, initScale, false };
    _tremolos[2] = { sinf, initW, initScale, false };
    _tremolos[3] = { sinf, initW, initScale, false };
    _tremolos[4] = { sinf, initW, initScale, false };
    _noteOffsets[0] = { 0, 0 };
    _noteOffsets[1] = { 0, 0 };
    _noteOffsets[2] = { 0, 0 };
    _noteOffsets[3] = { 0, 0 };
    _noteOffsets[4] = { 0, 0 };
    noteOffsets[0] = 0;
    noteOffsets[1] = 0;
    noteOffsets[2] = 0;
    noteOffsets[3] = 0;
    noteOffsets[4] = 0;
}

uint32_t f_to_i(float v)
{
    uint32_t* ptr = (uint32_t*)&v;
    return *ptr;
}
float i_to_f(uint32_t v)
{
    float* ptr = (float*)&v;
    return *ptr;
}
void calcOffset(uint8_t channel)
{
    Offset o = _noteOffsets[channel];
    noteOffsets[channel] = o.oct * 12 + o.st;
}
void calcRunTrem()
{
    _runTrem = _tremolos[0].enabled | _tremolos[1].enabled |
        _tremolos[2].enabled | _tremolos[3].enabled |
        _tremolos[4].enabled;
}


void saveTTMState()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        Tremolo t = _tremolos[i];
        uint8_t si = i * 10;
        uint32_t rate = f_to_i(t.w);
        eeWrite(si, rate >> 24);
        eeWrite(si + T_RATE_B, (rate >> 16) & 0xFF);
        eeWrite(si + T_RATE_C, (rate >> 8) & 0xFF);
        eeWrite(si + T_RATE_D, rate & 0xFF);
        uint32_t scale = f_to_i(t.scale);
        eeWrite(si + T_SCALE_A, scale >> 24);
        eeWrite(si + T_SCALE_B, (scale >> 16) & 0xFF);
        eeWrite(si + T_SCALE_C, (scale >> 8) & 0xFF);
        eeWrite(si + T_SCALE_D, scale & 0xFF);
        eeWrite(si + T_MODE, t.function == sinf);
        eeWrite(si + T_ENABLED, t.enabled);
    }
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 2;
        Offset o = _noteOffsets[i];
        eeWrite(si + OFF_OCT, o.oct);
        eeWrite(si + OFF_ST, o.st);
    }
}
void loadTTMState()
{
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 10;
        uint32_t rateA = EEPROM.read(si) << 24;
        uint32_t rateB = EEPROM.read(si + T_RATE_B) << 16;
        uint32_t rateC = EEPROM.read(si + T_RATE_C) << 8;
        uint32_t rateD = EEPROM.read(si + T_RATE_D);
        uint32_t scaleA = EEPROM.read(si + T_SCALE_A) << 24;
        uint32_t scaleB = EEPROM.read(si + T_SCALE_B) << 16;
        uint32_t scaleC = EEPROM.read(si + T_SCALE_C) << 8;
        uint32_t scaleD = EEPROM.read(si + T_SCALE_D);
        bool mode = EEPROM.read(si + T_MODE);
        bool enabled = EEPROM.read(si + T_ENABLED);
        _tremolos[i] =
        {
            mode ? sinf : tri,
            i_to_f(rateA + rateB + rateC + rateD),
            i_to_f(scaleA + scaleB + scaleC + scaleD),
            enabled
        };
    }
    calcRunTrem();
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 2;
        Offset o = {
            EEPROM.read(si + OFF_OCT),
            EEPROM.read(si + OFF_ST)
        };
        _noteOffsets[i] = o;
        calcOffset(i);
    }
}

uint16_t _lv_hz = 4;
uint16_t _lv_mhz = 4000;
uint16_t _lv_scale = 43;
bool _setRateHz = false;
bool _setRatemHz = false;
bool _setScale = false;

uint16_t _lv_st = 2;
bool _setOct = false;
bool _setST = false;
void manageMenuNotes(NoteName n, uint8_t channel)
{
    // enter time - digit must start at 0
    if (_setRateHz)
    {
        // valid digit
        bool v = addDigit(n, 3);
        if (v) { return; }
        if (n != NoteName::F4)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    if (_setRatemHz)
    {
        // valid digit
        bool v = addDigit(n, 5);
        if (v) { return; }
        if (n != NoteName::G4)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    if (_setScale)
    {
        // valid digit
        bool v = addDigit(n, 4);
        if (v) { return; }
        if (n != NoteName::_A4)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    if (_setST)
    {
        // valid digit
        bool v = addDigit(n, 2);
        if (v) { return; }
        if (n != NoteName::G5 && n != NoteName::_A5)
        {
            playNote(NOTEFAIL_S, MF_DURATION);
            return;
        }
    }
    
    switch (n)
    {
        // set to defaults
        case NoteName::B3:
            resetTTMValues();
            triggerFeedback(true);
            return;
        case NoteName::C4:
            _tremolos[channel].enabled = !_tremolos[channel].enabled;
            triggerFeedback(_tremolos[channel].enabled);
            calcRunTrem();
            return;
        case NoteName::_D4:
            _tremolos[channel].function = sinf;
            triggerFeedback(true);
            return;
        case NoteName::E4:
            _tremolos[channel].function = tri;
            triggerFeedback(false);
            return;
        case NoteName::F4:
        {
            _setRateHz = !_setRateHz;
            if (!_setRateHz)
            {
                uint16_t v = getEnteredValue(_lv_hz);
                // min hz
                if (v == 0)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                _lv_hz = v;
                _tremolos[channel].w = setW * v;
            }
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
        case NoteName::G4:
        {
            _setRatemHz = !_setRatemHz;
            if (!_setRatemHz)
            {
                uint16_t v = getEnteredValue(_lv_mhz);
                // min hz
                if (v == 0)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                _lv_mhz = v;
                _tremolos[channel].w = setmW * v;
            }
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
        case NoteName::_A4:
        {
            _setRatemHz = !_setRatemHz;
            if (!_setRatemHz)
            {
                uint16_t v = getEnteredValue(_lv_scale);
                // max scale
                if (v > 2048)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                _lv_scale = v;
                _tremolos[channel].scale = setScale * v;
            }
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
        
        case NoteName::B4:
            saveTTMState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::Bb4:
            loadTTMState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
            
        case NoteName::G5:
        {
            _setST = !_setST;
            if (!_setST)
            {
                uint16_t v = getEnteredValue(_lv_st);
                // max offset
                if (v > 24)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                _lv_st = v;
                _noteOffsets[channel].st = -v;
                calcOffset(channel);
            }
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
        case NoteName::_A5:
        {
            _setST = !_setST;
            if (!_setST)
            {
                uint16_t v = getEnteredValue(_lv_st);
                // max offset
                if (v > 24)
                {
                    playNote(NOTEFAIL_S, MF_DURATION);
                    return;
                }
                _lv_st = v;
                _noteOffsets[channel].st = v;
                calcOffset(channel);
            }
            playNote(NOTESELECT_S, MF_DURATION);
            return;
        }
        case NoteName::B5:
            _noteOffsets[channel].oct = -3;
            calcOffset(channel);
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::C6:
            _noteOffsets[channel].oct = -2;
            calcOffset(channel);
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::Db6:
            _noteOffsets[channel].oct = -1;
            calcOffset(channel);
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::_D6:
            _noteOffsets[channel].oct = 0;
            calcOffset(channel);
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::Eb6:
            _noteOffsets[channel].oct = 1;
            calcOffset(channel);
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::E6:
            _noteOffsets[channel].oct = 2;
            calcOffset(channel);
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::F6:
            _noteOffsets[channel].oct = 3;
            calcOffset(channel);
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
    }
}

void ttMenuFunction()
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