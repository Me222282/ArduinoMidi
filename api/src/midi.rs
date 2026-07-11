#![allow(non_camel_case_types)]

use crate::{Channel, Note};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MidiCode
{
    NoteOFF(Channel, Note),
    NoteON(Channel, Note),
    PolyphonicAftertouch(Channel, Note),
    ControlChange(Channel, CCType, u8),
    ProgramChange(Channel, u8),
    ChannelPressure(Channel, u8),
    PitchWheel(Channel, u16),
    
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

impl MidiCode
{
    #[must_use]
    pub fn get_channel(self) -> Channel
    {
        return match self
        {
            MidiCode::NoteOFF(channel, _) => channel,
            MidiCode::NoteON(channel, _) => channel,
            MidiCode::PolyphonicAftertouch(channel, _) => channel,
            MidiCode::ControlChange(channel, _cctype, _) => channel,
            MidiCode::ProgramChange(channel, _) => channel,
            MidiCode::ChannelPressure(channel, _) => channel,
            MidiCode::PitchWheel(channel, _) => channel,
            _ => Channel::All
        };
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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
    #[must_use]
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

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum QF7
{
    FPS24,
    FPS25,
    FPS30DropFrame,
    FPS30
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CCType(u8);
impl CCType
{
    #[must_use]
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
    channel: Channel,
    value: u8
}

impl MidiParser
{
    #[must_use]
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
                self.channel = Channel::from_u8(data & 0b00001111);
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
