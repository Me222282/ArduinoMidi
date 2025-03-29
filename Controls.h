#include "Globals.h"

uint8_t pins[5] = {
    OCTAVE,
    CHANNELS,
    VOICES,
    PITCHBEND,
    MODE2
};
float divs[5] = {
    682.666666667f,
    1024.0f,
    1024.0f,
    819.2f,
    2048.0f
};
uint8_t cv[5] = { 0, 0, 0, 0, 0 };
uint8_t nv[5] = { 0, 0, 0, 0, 0 };
uint8_t nc[5] = { 0, 0, 0, 0, 0 };

uint8_t readControl(uint8_t index)
{
    uint8_t pin = pins[index];
    
    // pre read
    analogRead(pin);
    uint8_t value = (uint8_t)round(analogRead(pin) / divs[index]);
    // no change
    uint8_t current = cv[index];
    if (value == current)
    {
        nc[index] = 0;
        return value;
    }
    
    uint8_t count = nc[index] + 1;
    if (value != nv[index])
    {
        nv[index] = value;
        nc[index] = 0;
        return current;
    }
    
    // num in row before change
    if (count >= 10)
    {
        cv[index] = value;
        nc[index] = 0;
        return value;
    }
    
    nc[index] = count;
    return current;
}