#include <Arduino.h>
#include "MenuFeedback.h"
#include "src/core/Coms.h"
#include "src/core/Globals.h"
#include "src/notes/Channels.h"

uint32_t startTime_MF;
uint32_t timeOut_MF;
bool is_MF = false;

bool isMenuFeedback = true;

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