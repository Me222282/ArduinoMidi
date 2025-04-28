#include <Arduino.h>
#include "MenuFeedback.h"
#include "src/core/Coms.h"
#include "src/core/Globals.h"
#include "src/notes/Channels.h"

uint32_t startTime_MF;
uint32_t timeOut_MF;
bool is_MF = false;

bool isMenuFeedback = true;

// consts
const uint16_t digitPlaces[4] = { 1, 10, 100, 1000 };
uint8_t digit = 0;
uint8_t digits[4] = { 0, 0, 0, 0 };

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
bool addDigit(NoteName n, uint8_t max)
{
    uint8_t dv = getDigit(n);
    // valid digit
    if (dv <= 9)
    {
        // no more digits
        if (digit >= 4) { return false; }
        digits[digit] = dv;
        digit++;
        playNumber(n);
        return true;
    }
    return false;
}
uint16_t getEnteredValue(uint16_t last)
{
    // digit is the number of digits entered
    if (digit == 0) { return last; }
    uint16_t value = 0;
    uint8_t place = 0;
    // read in reverse order
    for (int8_t i = digit - 1; i >= 0; i--)
    {
        value += digits[i] * digitPlaces[place];
        place++;
    }
    digit = 0;
    return value;
}

void velocityMF(uint8_t i)
{
    if (option)
    {
        if (i == 0)
        {
            setMod(0x3FFF);
            return;
        }
        return;
    }
    setVel(i, 0xFF);
}

void playNote(NoteName n, uint32_t duration)
{
    if (!isMenuFeedback) { return; }
    velocityMF(0);
    setNote(0, n);
    velocityMF(1);
    setNote(1, n);
    velocityMF(2);
    setNote(2, n);
    velocityMF(3);
    setNote(3, n);
    velocityMF(4);
    setNote(4, n);
    setGate(0b11111);
    
    is_MF = true;
    startTime_MF = millis();
    timeOut_MF = duration;
}

const uint8_t gatePlaces[5] =
{
    0b00001, 0b00010, 0b00100, 0b01000, 0b10000
};

void playNoteC(NoteName n, uint8_t channel, uint32_t duration)
{
    if (!isMenuFeedback) { return; }
    if (channel >= activeChannels)
    {
        if (!allChannelMode) { return; }
        channel = 0;
    }
    Channel* c = &channels[channel];
    uint8_t end = c->position + c->places;
    uint8_t gate = 0;
    for (uint8_t i = c->position; i < end; i++)
    {
        velocityMF(i);
        setNote(i, n);
        gate |= gatePlaces[i];
    }
    setGate(gateCurrent | gate);
    
    is_MF = true;
    startTime_MF = millis();
    timeOut_MF = duration;
}
void invokeMF()
{
    if (is_MF && (millis() - startTime_MF) > timeOut_MF)
    {
        setGate(0);
        timeOut_MF = 0;
        startTime_MF = 0;
        is_MF = false;
    }
}