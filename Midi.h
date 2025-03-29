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
    NoteName getNote()
    {
        return (NoteName)_data1;
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

enum NoteName: uint8_t
{
    C_1 = 0U,
    Db_1 = 1U,
    D_1 = 2U,
    Eb_1 = 3U,
    E_1 = 4U,
    F_1 = 5U,
    Gb_1 = 6U,
    G_1 = 7U,
    Ab_1 = 8U,
    A_1 = 9U,
    Bb_1 = 10U,
    B_1 = 11U,
    C0 = 12U,
    Db0 = 13U,
    D0 = 14U,
    Eb0 = 15U,
    E0 = 16U,
    F0 = 17U,
    Gb0 = 18U,
    G0 = 19U,
    Ab0 = 20U,
    A0 = 21U,
    Bb0 = 22U,
    B0 = 23U,
    C1 = 24U,
    Db1 = 25U,
    D1 = 26U,
    Eb1 = 27U,
    E1 = 28U,
    F1 = 29U,
    Gb1 = 30U,
    G1 = 31U,
    Ab1 = 32U,
    A1 = 33U,
    Bb1 = 34U,
    B1 = 35U,
    C2 = 36U,
    Db2 = 37U,
    D2 = 38U,
    Eb2 = 39U,
    E2 = 40U,
    F2 = 41U,
    Gb2 = 42U,
    G2 = 43U,
    Ab2 = 44U,
    A2 = 45U,
    Bb2 = 46U,
    B2 = 47U,
    C3 = 48U,
    Db3 = 49U,
    D3 = 50U,
    Eb3 = 51U,
    E3 = 52U,
    F3 = 53U,
    Gb3 = 54U,
    G3 = 55U,
    Ab3 = 56U,
    A3 = 57U,
    Bb3 = 58U,
    B3 = 59U,
    C4 = 60U,
    Db4 = 61U,
    D4 = 62U,
    Eb4 = 63U,
    E4 = 64U,
    F4 = 65U,
    Gb4 = 66U,
    G4 = 67U,
    Ab4 = 68U,
    A4 = 69U,
    Bb4 = 70U,
    B4 = 71U,
    C5 = 72U,
    Db5 = 73U,
    D5 = 74U,
    Eb5 = 75U,
    E5 = 76U,
    F5 = 77U,
    Gb5 = 78U,
    G5 = 79U,
    Ab5 = 80U,
    A5 = 81U,
    Bb5 = 82U,
    B5 = 83U,
    C6 = 84U,
    Db6 = 85U,
    D6 = 86U,
    Eb6 = 87U,
    E6 = 88U,
    F6 = 89U,
    Gb6 = 90U,
    G6 = 91U,
    Ab6 = 92U,
    A6 = 93U,
    Bb6 = 94U,
    B6 = 95U,
    C7 = 96U,
    Db7 = 97U,
    D7 = 98U,
    Eb7 = 99U,
    E7 = 100U,
    F7 = 101U,
    Gb7 = 102U,
    G7 = 103U,
    Ab7 = 104U,
    A7 = 105U,
    Bb7 = 106U,
    B7 = 107U,
    C8 = 108U,
    Db8 = 109U,
    D8 = 110U,
    Eb8 = 111U,
    E8 = 112U,
    F8 = 113U,
    Gb8 = 114U,
    G8 = 115U,
    Ab8 = 116U,
    A8 = 117U,
    Bb8 = 118U,
    B8 = 119U,
    C9 = 120U,
    Db9 = 121U,
    D9 = 122U,
    Eb9 = 123U,
    E9 = 124U,
    F9 = 125U,
    Gb9 = 126U,
    G9 = 127U
};