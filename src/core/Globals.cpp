#include <Arduino.h>
#include "Globals.h"
#include <EEPROM.h>
#include <Arduino.h>

// divisions to make pb smaller
const float pbDiv[6] = { 1.0f / 24.0f, 1.0f / 12.0f, 1.0f / 6.0f, 5.0f / 12.0f, 7.0f / 12.0f, 1.0f };

uint8_t activeChannels = 1;
uint8_t activeVoices = 1;
float pitchBendSelect = pbDiv[1];
// 0 - ignore, 1 - take last, 2 - take first
uint8_t mode2 = 0;
// OFF for loop
bool mode1 = false;

int8_t octaveOffset = 0;
// false for normal
bool option = false;

Note oldValues[5];
uint16_t oldPBs[5];
uint16_t oldMod = 0;

uint8_t gate = 0;
uint8_t reTrig = 0;

void setGateNote(uint8_t n, bool value)
{
    uint8_t g = 1 << n;
    if (value)
    {
        gate |= g;
        return;
    }

    gate &= ~g;
}
void reTrigNote(uint8_t n)
{
    uint8_t g = 1 << n;
    reTrig |= g;
}

void eeWrite(int address, uint8_t v)
{
    if (EEPROM.read(address) == v) { return; }
    EEPROM.write(address, v);
}