#ifndef __mid
#define __mid

enum MidiCode: uint8_t
{
    NoteOFF = 0x8,
    NoteON = 0x9,
    PolyphonicAftertouch = 0xA,
    CC = 0xB,
    PC = 0xC,
    ChannelAftertouch = 0xD,
    PitchWheel = 0xE,
    
    SystemExclusiveStart = 0xF0,
    SystemExclusiveEnd = 0xF7,
    SongPointer = 0xF2,
    SongSelect = 0xF3,
    TuneRequest = 0xF6,
    QuarterFrame = 0xF1,
    TimingClock = 0xF8,
    MeasureEnd = 0xF9,
    Start = 0xFA,
    Continue = 0xFB,
    Stop = 0xFC,
    ActiveSensing = 0xFE,
    Reset = 0xFF,
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
    _D0 = 14U,
    Eb0 = 15U,
    E0 = 16U,
    F0 = 17U,
    Gb0 = 18U,
    G0 = 19U,
    Ab0 = 20U,
    _A0 = 21U,
    Bb0 = 22U,
    _B0 = 23U,
    C1 = 24U,
    Db1 = 25U,
    _D1 = 26U,
    Eb1 = 27U,
    E1 = 28U,
    F1 = 29U,
    Gb1 = 30U,
    G1 = 31U,
    Ab1 = 32U,
    _A1 = 33U,
    Bb1 = 34U,
    _B1 = 35U,
    C2 = 36U,
    Db2 = 37U,
    _D2 = 38U,
    Eb2 = 39U,
    E2 = 40U,
    F2 = 41U,
    Gb2 = 42U,
    G2 = 43U,
    Ab2 = 44U,
    _A2 = 45U,
    Bb2 = 46U,
    B2 = 47U,
    C3 = 48U,
    Db3 = 49U,
    _D3 = 50U,
    Eb3 = 51U,
    E3 = 52U,
    F3 = 53U,
    Gb3 = 54U,
    G3 = 55U,
    Ab3 = 56U,
    _A3 = 57U,
    Bb3 = 58U,
    B3 = 59U,
    C4 = 60U,
    Db4 = 61U,
    _D4 = 62U,
    Eb4 = 63U,
    E4 = 64U,
    F4 = 65U,
    Gb4 = 66U,
    G4 = 67U,
    Ab4 = 68U,
    _A4 = 69U,
    Bb4 = 70U,
    B4 = 71U,
    C5 = 72U,
    Db5 = 73U,
    _D5 = 74U,
    Eb5 = 75U,
    E5 = 76U,
    F5 = 77U,
    Gb5 = 78U,
    G5 = 79U,
    Ab5 = 80U,
    _A5 = 81U,
    Bb5 = 82U,
    B5 = 83U,
    C6 = 84U,
    Db6 = 85U,
    _D6 = 86U,
    Eb6 = 87U,
    E6 = 88U,
    F6 = 89U,
    Gb6 = 90U,
    G6 = 91U,
    Ab6 = 92U,
    _A6 = 93U,
    Bb6 = 94U,
    B6 = 95U,
    C7 = 96U,
    Db7 = 97U,
    _D7 = 98U,
    Eb7 = 99U,
    E7 = 100U,
    F7 = 101U,
    Gb7 = 102U,
    G7 = 103U,
    Ab7 = 104U,
    _A7 = 105U,
    Bb7 = 106U,
    B7 = 107U,
    C8 = 108U,
    Db8 = 109U,
    _D8 = 110U,
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
    _D9 = 122U,
    Eb9 = 123U,
    E9 = 124U,
    F9 = 125U,
    Gb9 = 126U,
    G9 = 127U
};

enum Notes: uint8_t
{
    C = NoteName::C_1,
    Db = NoteName::Db_1,
    D = NoteName::D_1,
    Eb = NoteName::Eb_1,
    E = NoteName::E_1,
    F = NoteName::F_1,
    Gb = NoteName::Gb_1,
    G = NoteName::G_1,
    Ab = NoteName::Ab_1,
    A = NoteName::A_1,
    Bb = NoteName::Bb_1,
    B = NoteName::B_1
};

bool noteEquals(NoteName n1, Notes n2);
bool notInKey(NoteName note, Notes key);

class Midi
{
public:
    inline Midi(HardwareSerial &serial) :
        _serial(serial)
    {
        
    }
    void begin();
    bool read();
    CCType getCC();
    NoteName getNote();
    MidiCode getType();
    uint8_t getData1();
    uint8_t getData2();
    uint8_t getChannel();
    uint16_t getCombinedData();
    
private:
    HardwareSerial &_serial;
    MidiCode _type;
    uint8_t _data1;
    uint8_t _data2;
    
    uint8_t _channel;
};

#endif