#![allow(non_camel_case_types)]

pub enum MidiCode
{
    NoteOFF(u8, Note),
    NoteON(u8, Note),
    PolyphonicAftertouch(u8, Note),
    ControlChange(u8, CCType, u8),
    ProgramChange(u8, u8),
    ChannelPressure(u8, u8),
    PitchWheel(u8, u16),
    
    // SystemExclusiveStart,
    // SystemExclusiveEnd,
    SongPointer(u16),
    SongSelect(u8),
    TuneRequest,
    QuarterFrame(QFData),
    TimingClock,
    MeasureEnd,
    Tick,
    Start,
    Continue,
    Stop,
    ActiveSensing,
    Reset
}

pub enum QFData
{
    FramesLSN(u8),
    FramesMSN(u8),
    SecondsLSN(u8),
    SecondsMSN(u8),
    MinutesLSN(u8),
    MinutesMSN(u8),
    HoursLSN(u8),
    HoursMSN(QF7, u8)
}
impl QFData
{
    pub const fn from_u8(data: u8) -> QFData
    {
        let code = data >> 4;
        let value = data & 0b00001111;
        return match code
        {
            0 => QFData::FramesLSN(value),
            1 => QFData::FramesMSN(value),
            2 => QFData::SecondsLSN(value),
            3 => QFData::SecondsMSN(value),
            4 => QFData::MinutesLSN(value),
            5 => QFData::MinutesMSN(value),
            6 => QFData::HoursLSN(value),
            7 =>
            {
                let fps = match (data & 0b110) >> 1
                {
                    0 => QF7::FPS24,
                    1 => QF7::FPS25,
                    2 => QF7::FPS30DropFrame,
                    3 => QF7::FPS30,
                    _ => panic!()
                };
                QFData::HoursMSN(fps, data & 1)
            },
            _ => panic!()
        };
    }
}

pub enum QF7
{
    FPS24,
    FPS25,
    FPS30DropFrame,
    FPS30
}

pub struct Note
{
    pub key: u8,
    pub velocity: u8
}
impl Note
{
    pub const fn new(key: u8, vel: u8) -> Note
    {
        return Note { key, velocity: vel };
    }
}

// pub struct NoteName(u8);
// impl NoteName
// {
//     C_1 = 0u8,
//     Db_1 = 1u8,
//     D_1 = 2u8,
//     Eb_1 = 3u8,
//     E_1 = 4u8,
//     F_1 = 5u8,
//     Gb_1 = 6u8,
//     G_1 = 7u8,
//     Ab_1 = 8u8,
//     A_1 = 9u8,
//     Bb_1 = 10u8,
//     B_1 = 11u8,
//     C0 = 12u8,
//     Db0 = 13u8,
//     _D0 = 14u8,
//     Eb0 = 15u8,
//     E0 = 16u8,
//     F0 = 17u8,
//     Gb0 = 18u8,
//     G0 = 19u8,
//     Ab0 = 20u8,
//     _A0 = 21u8,
//     Bb0 = 22u8,
//     _B0 = 23u8,
//     C1 = 24u8,
//     Db1 = 25u8,
//     _D1 = 26u8,
//     Eb1 = 27u8,
//     E1 = 28u8,
//     F1 = 29u8,
//     Gb1 = 30u8,
//     G1 = 31u8,
//     Ab1 = 32u8,
//     _A1 = 33u8,
//     Bb1 = 34u8,
//     _B1 = 35u8,
//     C2 = 36u8,
//     Db2 = 37u8,
//     _D2 = 38u8,
//     Eb2 = 39u8,
//     E2 = 40u8,
//     F2 = 41u8,
//     Gb2 = 42u8,
//     G2 = 43u8,
//     Ab2 = 44u8,
//     _A2 = 45u8,
//     Bb2 = 46u8,
//     B2 = 47u8,
//     C3 = 48u8,
//     Db3 = 49u8,
//     _D3 = 50u8,
//     Eb3 = 51u8,
//     E3 = 52u8,
//     F3 = 53u8,
//     Gb3 = 54u8,
//     G3 = 55u8,
//     Ab3 = 56u8,
//     _A3 = 57u8,
//     Bb3 = 58u8,
//     B3 = 59u8,
//     C4 = 60u8,
//     Db4 = 61u8,
//     _D4 = 62u8,
//     Eb4 = 63u8,
//     E4 = 64u8,
//     F4 = 65u8,
//     Gb4 = 66u8,
//     G4 = 67u8,
//     Ab4 = 68u8,
//     _A4 = 69u8,
//     Bb4 = 70u8,
//     B4 = 71u8,
//     C5 = 72u8,
//     Db5 = 73u8,
//     _D5 = 74u8,
//     Eb5 = 75u8,
//     E5 = 76u8,
//     F5 = 77u8,
//     Gb5 = 78u8,
//     G5 = 79u8,
//     Ab5 = 80u8,
//     _A5 = 81u8,
//     Bb5 = 82u8,
//     B5 = 83u8,
//     C6 = 84u8,
//     Db6 = 85u8,
//     _D6 = 86u8,
//     Eb6 = 87u8,
//     E6 = 88u8,
//     F6 = 89u8,
//     Gb6 = 90u8,
//     G6 = 91u8,
//     Ab6 = 92u8,
//     _A6 = 93u8,
//     Bb6 = 94u8,
//     B6 = 95u8,
//     C7 = 96u8,
//     Db7 = 97u8,
//     _D7 = 98u8,
//     Eb7 = 99u8,
//     E7 = 100u8,
//     F7 = 101u8,
//     Gb7 = 102u8,
//     G7 = 103u8,
//     Ab7 = 104u8,
//     _A7 = 105u8,
//     Bb7 = 106u8,
//     B7 = 107u8,
//     C8 = 108u8,
//     Db8 = 109u8,
//     _D8 = 110u8,
//     Eb8 = 111u8,
//     E8 = 112u8,
//     F8 = 113u8,
//     Gb8 = 114u8,
//     G8 = 115u8,
//     Ab8 = 116u8,
//     A8 = 117u8,
//     Bb8 = 118u8,
//     B8 = 119u8,
//     C9 = 120u8,
//     Db9 = 121u8,
//     _D9 = 122u8,
//     Eb9 = 123u8,
//     E9 = 124u8,
//     F9 = 125u8,
//     Gb9 = 126u8,
//     G9 = 127u8
// }

pub struct CCType(u8);
impl CCType
{
    pub const fn u8(value: u8) -> CCType
    {
        return CCType(value);
    }
    
    pub const MODULATION_WHEEL_MSB: CCType = CCType::u8(0x01);
    pub const BREATH_CONTROLLER_MSB: CCType = CCType::u8(0x02);
    pub const FOOT_CONTROLLER_MSB: CCType = CCType::u8(0x04);
    pub const PORTAMENTO_TIME_MSB: CCType = CCType::u8(0x05);
    pub const DATA_ENTRY_SLIDER_MSB: CCType = CCType::u8(0x06);
    pub const MAIN_VOLUME_MSB: CCType = CCType::u8(0x07);
    pub const MODULATION_WHEEL_LSB: CCType = CCType::u8(0x21);
    pub const BREATH_CONTROLLER_LSB: CCType = CCType::u8(0x22);
    pub const FOOT_CONTROLLER_LSB: CCType = CCType::u8(0x24);
    pub const PORTAMENTO_TIME_LSB: CCType = CCType::u8(0x25);
    pub const DATA_ENTRY_SLIDER_LSB: CCType = CCType::u8(0x26);
    pub const MAIN_VOLUME_LSB: CCType = CCType::u8(0x27);
    pub const SUSTAIN_PEDAL: CCType = CCType::u8(0x40);
    pub const PORTAMENTO: CCType = CCType::u8(0x41);
    pub const SOSTENATO_PEDAL: CCType = CCType::u8(0x42);
    pub const SOFT_PEDAL: CCType = CCType::u8(0x43);
    pub const RESET_ALL_CONTROLLERS: CCType = CCType::u8(0x70);
    pub const LOCAL: CCType = CCType::u8(0x7A);
    pub const ALL_NOTES_OFF: CCType = CCType::u8(0x7B);
    pub const OMNI_OFF: CCType = CCType::u8(0x7C);
    pub const OMNI_ON: CCType = CCType::u8(0x7D);
    pub const MONO: CCType = CCType::u8(0x7E);
    pub const POLY: CCType = CCType::u8(0x7F);
}

pub struct MidiParser
{
    in_exclusive: bool,
    data1: bool,
    data2: bool,
    code: u8,
    channel: u8,
    value: u8
}

impl MidiParser
{
    pub fn parse_byte(&mut self, data: u8) -> Option<MidiCode>
    {
        if self.in_exclusive
        {
            if data == 0xF7
            {
                self.in_exclusive = false;
            }
            return None;
        }
        
        if (data & 0b10000000) > 0
        {
            let code = data >> 4;
            if code == 0xF
            {
                match data
                {
                    0xF0 => self.in_exclusive = true,
                    0xF1 | 0xF3 =>
                    {
                        self.data1 = true;
                        self.data2 = false;
                        self.code = data;
                    },
                    0xF2 =>
                    {
                        self.data1 = true;
                        self.data2 = true;
                        self.code = data;
                    }
                    0xF6 => return Some(MidiCode::TuneRequest),
                    0xF8 => return Some(MidiCode::TimingClock),
                    0xF9 => return Some(MidiCode::MeasureEnd),
                    0xFA => return Some(MidiCode::Start),
                    0xFB => return Some(MidiCode::Continue),
                    0xFC => return Some(MidiCode::Stop),
                    0xFE => return Some(MidiCode::ActiveSensing),
                    0xFF => return Some(MidiCode::Reset),
                    _ => return None
                }
            }
            else
            {
                self.data1 = true;
                match code
                {
                    0x8 | 0x9 | 0xA | 0xB | 0xE => self.data2 = true,
                    0xC | 0xD => self.data2 = false,
                    _ => return None
                }
                self.channel = data & 0b00001111;
                self.code = data & 0b11110000;
            }
            
            return None;
        }
        
        if self.data1
        {
            self.data1 = false;
            if self.data2
            {
                self.value = data;
                return None;
            }
            
            return match self.code
            {
                0xF1 => Some(MidiCode::QuarterFrame(QFData::from_u8(data))),
                0xF3 => Some(MidiCode::SongSelect(data)),
                0xC0 => Some(MidiCode::ProgramChange(self.channel, data)),
                0xD0 => Some(MidiCode::ChannelPressure(self.channel, data)),
                _ => None
            };
        }
        // cannot get here if self.data1 is true
        if self.data2
        {
            self.data2 = false;
            
            return match self.code
            {
                0x80 => Some(MidiCode::NoteOFF(self.channel, Note::new(self.value, data))),
                0x90 => Some(MidiCode::NoteON(self.channel, Note::new(self.value, data))),
                0xA0 => Some(MidiCode::PolyphonicAftertouch(self.channel, Note::new(self.value, data))),
                0xB0 => Some(MidiCode::ControlChange(self.channel, CCType(self.value), data)),
                0xE0 => Some(MidiCode::PitchWheel(self.channel, self.value as u16 + ((data as u16) << 7))),
                0xF2 => Some(MidiCode::SongPointer(self.value as u16 + ((data as u16) << 7))),
                _ => None
            };
        }
        
        return None;
    }
}
