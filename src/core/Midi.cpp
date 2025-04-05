#include <Arduino.h>
#include "Midi.h"

// bool noteEquals(NoteName n1, Notes n2)
// {
//     return (n1 % 12) == n2;
// }
bool notInKey(NoteName note, Notes key)
{
    uint8_t v = (note + 12 - key) % 12;
    return v == 1 || v == 3 || v == 6 || v == 8 || v == 10;
}

Midi MIDI(Serial0);

void Midi::begin()
{
    _serial.begin(31250);
}
bool Midi::read()
{
    if (_serial.available() >= 3)
    {
        uint8_t status = (uint8_t)_serial.read();
        _data1 = (uint8_t)_serial.read();
        _data2 = (uint8_t)_serial.read();
        
        _channel = status & 0b00001111;
        uint8_t type = (status & 0b11110000) >> 4;
        if (type == 0xF)
        {
            _type = (MidiCode)status;
            _channel = 0;
            return true;
        }
        
        _type = (MidiCode)type;
    }
    
    return false;
}
CCType Midi::getCC()
{
    return (CCType)_data1;
}
NoteName Midi::getNote()
{
    return (NoteName)_data1;
}
MidiCode Midi::getType()
{
    return _type;
}
uint8_t Midi::getData1()
{
    return _data1;
}
uint8_t Midi::getData2()
{
    return _data2;
}
uint8_t Midi::getChannel()
{
    return _channel;
}
uint16_t Midi::getCombinedData()
{
    return (_data2 << 7) | _data1;
}