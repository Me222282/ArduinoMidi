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
    int d1 = _serial.read();
    if (d1 < 0) { return false; }
    uint8_t data = (uint8_t)d1;
    if (_inExclusive)
    {
        if (data == 0xF7) { _inExclusive = false; }
        return false;
    }
    
    // status message
    if (data & 0b10000000)
    {
        uint8_t status = data;
        _channel = status & 0b00001111;
        uint8_t type = (status & 0b11110000) >> 4;
        if (type == 0xF)
        {
            if (status == 0xF0)
            {
                _inExclusive = true;
                return false;
            }
            
            _type = (MidiCode)status;
            _channel = 0;
            // 0 byte messages
            if (type != MidiCode::QuarterFrame && type != MidiCode::SongSelect &&
                type != MidiCode::SongPointer)
            {
                _data1 = 0;
                _data2 = 0;
                return true;
            }
            return false;
        }
        _type = (MidiCode)type;
        return false;
    }
    // data message
    if (_secondByte)
    {
        _data2 = data;
        _secondByte = false;
        return true;
    }
    // 1 byte messages
    if (_type == MidiCode::PC || _type == MidiCode::ChannelPressure ||
        _type == MidiCode::QuarterFrame || _type == MidiCode::SongSelect)
    {
        _data1 = data;
        _data2 = 0;
        return true;
    }
    
    // needs second byte
    _data1 = data;
    if (_serial.available() < 1)
    {
        _secondByte = true;
        return false;
    }
    _data2 = (uint8_t)_serial.read();
    return true;
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