#include <Arduino.h>
#include "Controls.h"
#include "src/core/Globals.h"

uint8_t pins[5] = {
    OCTAVE,
    CHANNELS,
    VOICES,
    PITCHBEND,
    MODE2
};
float _divs[5] = {
    682.666666667f,
    1024.0f,
    1024.0f,
    819.2f,
    2048.0f
};
uint8_t _cv[5] = { 0, 0, 0, 0, 0 };
uint8_t _nv[5] = { 0, 0, 0, 0, 0 };
uint8_t _nc[5] = { 0, 0, 0, 0, 0 };

uint8_t readControl(uint8_t index)
{
    uint8_t pin = pins[index];
    
    // pre read
    analogRead(pin);
    uint8_t value = (uint8_t)round(analogRead(pin) / _divs[index]);
    // no change
    uint8_t current = _cv[index];
    if (value == current)
    {
        _nc[index] = 0;
        return value;
    }
    
    uint8_t count = _nc[index] + 1;
    if (value != _nv[index])
    {
        _nv[index] = value;
        _nc[index] = 0;
        return current;
    }
    
    // num in row before change
    if (count >= 10)
    {
        _cv[index] = value;
        _nc[index] = 0;
        return value;
    }
    
    _nc[index] = count;
    return current;
}