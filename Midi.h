enum MidiCode: uint8_t
{
    NoteOFF = 0x8,
    NoteON = 0x9,
    PolyphonicAftertouch = 0xA,
    CC = 0xB,
    PC = 0xC,
    ChannelAftertouch = 0xD,
    PitchWheel = 0xE
};

enum CCType: uint8_t
{
    ModulationWheel_MSB = 0x01,
    BreathController_MSB = 0x02,
    FootController_MSB = 0x04,
    PortamentoTime_MSB = 0x05,
    DataEntrySlider_MSB = 0x06,
    MainVolume_MSB = 0x07,
    ModulationWheel_LSB = 0x21,
    BreathController_LSB = 0x22,
    FootController_LSB = 0x24,
    PortamentoTime_LSB = 0x25,
    DataEntrySlider_LSB = 0x26,
    MainVolume_LSB = 0x27,
    SustainPedal = 0x40,
    Portamento = 0x41,
    SostenatoPedal = 0x42,
    SoftPedal = 0x43,
    ResetAllControllers = 0x70,
    Local = 0x7A,
    AllNotesOff = 0x7B,
    OmniOff = 0x7C,
    OmniOn = 0x7D,
    Mono = 0x7E,
    Poly = 0x7F,
};

class Midi
{
public:
    Midi(HardwareSerial &serial) :
        _serial(serial)
    {
        
    }
    void begin()
    {
        _serial.begin(31250);
    }
    bool read()
    {
        if (_serial.available() >= 3)
        {
            uint8_t status = (uint8_t)_serial.read();
            _data1 = (uint8_t)_serial.read();
            _data2 = (uint8_t)_serial.read();
            
            _channel = status & 0b00001111;
            _type = (MidiCode)((status & 0b11110000) >> 4);
            return true;
        }
        
        return false;
    }
    CCType getCC()
    {
        return (CCType)_data1;
    }
    MidiCode getType()
    {
        return _type;
    }
    uint8_t getData1()
    {
        return _data1;
    }
    uint8_t getData2()
    {
        return _data2;
    }
    uint8_t getChannel()
    {
        return _channel;
    }
    uint16_t getCombinedData()
    {
        return (_data2 << 7) | _data1;
    }
    
private:
    HardwareSerial &_serial;
    MidiCode _type;
    uint8_t _data1;
    uint8_t _data2;
    
    uint8_t _channel;
};