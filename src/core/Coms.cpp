#include <Arduino.h>
#include <SPI.h>
#include "Coms.h"
#include "Globals.h"

void configureGate()
{
    SPI.beginTransaction(GATESPI);
    digitalWrite(GATEIC, LOW);
    SPI.transfer16(0x4000);
    // Set direction to output
    SPI.transfer(0x00);
    digitalWrite(GATEIC, HIGH);
    SPI.endTransaction();
}

uint8_t gateCurrent;
void setGate(uint8_t value)
{
    gateCurrent = value;
    SPI.beginTransaction(GATESPI);
    digitalWrite(GATEIC, LOW);
    SPI.transfer16(0x4009);
    SPI.transfer(value);
    digitalWrite(GATEIC, HIGH);
    SPI.endTransaction();
}

void _trans(uint8_t pin, uint16_t trans, SPISettings spi)
{
    SPI.beginTransaction(spi);
    digitalWrite(pin, LOW);
    SPI.transfer16(trans);
    digitalWrite(pin, HIGH);
    SPI.endTransaction();
}

void _dac8(uint8_t n, uint8_t value, uint16_t command)
{
    uint8_t pin = n + 2;
    uint16_t trans = (value << 4) + command;
    _trans(pin, trans, DAC8SPI);
}
void setVel(uint8_t n, uint8_t value)
{
    if (oldValues[n].vel == value) { return; }
    oldValues[n].vel = value;
    
    _dac8(n, value, 0xF000);
}

void _setNoteNorm(uint8_t n, uint8_t value)
{
    if (oldValues[n].key == value) { return; }
    oldValues[n].key = value;
    
    _dac8(n, value << 1, 0x3000);
}
void _setSubNote(uint8_t n, uint8_t value)
{
    if (oldValues[n].key == value) { return; }
    oldValues[n].key = value;
    
    _dac8(n, value + 60, 0x3000);
}
void (*setNote)(uint8_t, uint8_t) = _setNoteNorm;

void _ic(uint8_t pin, uint16_t value, uint16_t command)
{
    uint16_t trans = value | command;
    _trans(pin, trans, DAC12SPI);
}
void _setPitchBend_ext(uint8_t n, uint16_t value)
{
    uint16_t c = 0x7000;
    if (n % 2 == 1) { c = 0xF000; }
    
    _ic(PITCHDAC1 - (n / 2), value, c);
}
void setPitchBend(uint8_t n, uint16_t value)
{
    float v = ((int16_t)value - 2048);
    v *= pitchBendSelect;
    int16_t con = (int16_t)v + 2048;
    
    uint16_t act = (uint16_t)con;
    if (oldPBs[n] == act) { return; }
    oldPBs[n] = act;
    _setPitchBend_ext(n, act);
}
void setMod(uint16_t value)
{
    _ic(PITCHDAC3, value, 0xF000);
}

// void configurePot(uint8_t pin)
// {
//     // ic(pin, 0xE0, 0x5100);
//     _ic(pin, 0xFF, 0x4100);
// }

// void setPot(uint8_t pin, bool bPot, uint8_t value)
// {
//     uint16_t c = 0x1000;
//     if (bPot) { c = 0x2000; }
    
//     _ic(pin, value, c);
// }