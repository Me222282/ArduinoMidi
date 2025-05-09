#include <Arduino.h>
#include "VMenu.h"

#include "../../Callbacks.h"
#include "SpeicalOps.h"
#include "../core/Coms.h"
#include "../core/Globals.h"
#include "../core/NVData.h"
#include "../../MemLocations.h"
#include "../notes/Channels.h"

typedef struct
{
    float (*function)(float);
    float w;
    float scale;
    bool enabled;
} Vibrato;

typedef struct
{
    int8_t oct;
    int8_t st;
} Offset;

bool _runTrem = false;
bool _globalRate = false;
Vibrato _vibratos[5];
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
void invokeVibrato()
{
    if (!_runTrem) { return; }
    float time = (float)millis();
    
    if (_globalRate)
    {
        float y = _vibratos[0].function(time * _vibratos[0].w);
        for (uint8_t i = 0; i < activeChannels; i++)
        {
            Vibrato v = _vibratos[i];
            if (!v.enabled) { continue; }
            float nv = y * channels[i].modulation * v.scale;
            setCP(&channels[i], (int16_t)nv);
        }
        return;
    }
    
    for (uint8_t i = 0; i < activeChannels; i++)
    {
        Vibrato t = _vibratos[i];
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
void resetVVMValues()
{
    _runTrem = false;
    _globalRate = false;
    for (uint8_t i = 0; i < 5; i++)
    {
        _vibratos[i] = { sinf, initW, initScale, false };
        _noteOffsets[i] = { 0, 0 };
        noteOffsets[i] = 0;
        setCP(&channels[i], 0);
    }
}

int8_t u_to_i(uint8_t v)
{
    int8_t* ptr = (int8_t*)&v;
    return *ptr;
}
uint8_t i_to_u(int8_t v)
{
    uint8_t* ptr = (uint8_t*)&v;
    return *ptr;
}
void calcOffset(uint8_t channel)
{
    Offset o = _noteOffsets[channel];
    noteOffsets[channel] = o.oct * 12 + o.st;
}
void calcRunTrem()
{
    _runTrem = _vibratos[0].enabled | _vibratos[1].enabled |
        _vibratos[2].enabled | _vibratos[3].enabled |
        _vibratos[4].enabled;
}


void saveVVMState()
{
    openDataSpace(DataSpace::TTData, true);
    
    for (uint8_t i = 0; i < 5; i++)
    {
        Vibrato t = _vibratos[i];
        uint8_t si = i * 10;
        t.function = (float (*)(float))(t.function == sinf);
        setSpaceData(si, t);
    }
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 2;
        Offset o = _noteOffsets[i];
        setSpaceByte(si + OFF_OCT, i_to_u(o.oct));
        setSpaceByte(si + OFF_ST, i_to_u(o.st));
    }
    
    setSpaceByte(TREM_GLOB, _globalRate);
    commitSpace();
    closeDataSpace();
}
void loadVVMState()
{
    openDataSpace(DataSpace::TTData, false);
    
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 10;
        Vibrato v = getSpaceData<Vibrato>(si);
        v.function = v.function ? sinf : tri;
        _vibratos[i] = v;
    }
    calcRunTrem();
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t si = i * 2;
        Offset o = {
            u_to_i(getSpaceByte(si + OFF_OCT)),
            u_to_i(getSpaceByte(si + OFF_ST))
        };
        _noteOffsets[i] = o;
        calcOffset(i);
    }
    
    _globalRate = getSpaceByte(TREM_GLOB);
    closeDataSpace();
}

uint16_t _lv_hz = 4;
uint16_t _lv_mhz = 4000;
uint16_t _lv_scale = 43;
bool _setRateHz = false;
bool _setRatemHz = false;
bool _setScale = false;

uint8_t _lv_st = 2;
bool _setST = false;
void manageVMenuNotes(NoteName n, uint8_t channel)
{
    // enter time - digit must start at 0
    if (_setRateHz)
    {
        // valid digit
        bool v = addDigit(n, 3);
        if (v) { return; }
        if (n != NoteName::F4)
        {
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
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
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
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
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
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
            playNoteC(NOTEFAIL_S, channel, MF_DURATION);
            return;
        }
    }
    
    switch (n)
    {
        // set to defaults
        case NoteName::B3:
            resetVVMValues();
            triggerFeedback(true);
            return;
        case NoteName::C4:
        {
            bool v = !_vibratos[channel].enabled;
            _vibratos[channel].enabled = v;
            triggerFeedbackC(v, channel);
            calcRunTrem();
            if (!v)
            {
                setCP(&channels[channel], 0);
            }
            return;
        }
        case NoteName::_D4:
            _vibratos[channel].function = sinf;
            triggerFeedbackC(true, channel);
            return;
        case NoteName::E4:
            _vibratos[channel].function = tri;
            triggerFeedbackC(false, channel);
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
                    playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                    return;
                }
                _lv_hz = v;
                uint8_t i = _globalRate ? 0 : channel;
                _vibratos[i].w = setW * v;
            }
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
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
                    playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                    return;
                }
                _lv_mhz = v;
                uint8_t i = _globalRate ? 0 : channel;
                _vibratos[i].w = setmW * v;
            }
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        }
        case NoteName::_A4:
        {
            _setScale = !_setScale;
            if (!_setScale)
            {
                uint16_t v = getEnteredValue(_lv_scale);
                // max scale
                if (v > 2048)
                {
                    playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                    return;
                }
                _lv_scale = v;
                _vibratos[channel].scale = setScale * v;
            }
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        }
        
        case NoteName::B4:
            saveVVMState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::Bb4:
            loadVVMState();
            playNote(NOTEOPTION_S, MF_DURATION);
            return;
        case NoteName::C5:
            _globalRate = !_globalRate;
            triggerFeedback(_globalRate);
            return;
            
        case NoteName::G5:
        {
            _setST = !_setST;
            if (!_setST)
            {
                uint8_t v = getEnteredValue(_lv_st);
                // max offset
                if (v > 24)
                {
                    playNoteC(NOTEFAIL_S, channel, MF_DURATION);
                    return;
                }
                _lv_st = v;
                _noteOffsets[channel].st = -v;
                calcOffset(channel);
            }
            playNoteC(NOTESELECT_S, channel, MF_DURATION);
            return;
        }
        case NoteName::_A5:
        {
            _setST = !_setST;
            if (!_setST)
            {
                uint8_t v = getEnteredValue(_lv_st);
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
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::C6:
            _noteOffsets[channel].oct = -2;
            calcOffset(channel);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::Db6:
            _noteOffsets[channel].oct = -1;
            calcOffset(channel);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::_D6:
            _noteOffsets[channel].oct = 0;
            calcOffset(channel);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::Eb6:
            _noteOffsets[channel].oct = 1;
            calcOffset(channel);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::E6:
            _noteOffsets[channel].oct = 2;
            calcOffset(channel);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
            return;
        case NoteName::F6:
            _noteOffsets[channel].oct = 3;
            calcOffset(channel);
            playNoteC(NOTEOPTION_S, channel, MF_DURATION);
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
                manageVMenuNotes(MIDI.getNote(), channel);
                return;
            }
            case MidiCode::CC:
                onControlChange(channel, MIDI.getCC(), MIDI.getData2());
                return;
        }
    }
}

Menu _v_Menu;
void vtOpen()
{
    playNote(NOTESELECT_S, MF_DURATION);
}
void vExit()
{
    _v_Menu.active = false;
    _setRateHz = false;
    _setRatemHz = false;
    _setScale = false;
    _setST = false;
    getEnteredValue(0);
}

Menu* getVMenu()
{
    _v_Menu = { true, ttMenuFunction, vExit, vtOpen };
    return &_v_Menu;
}