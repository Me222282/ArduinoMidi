use api::{Channel, Note};

pub const NOTE_FAIL: u8 = Note::B1;
pub const NOTE_ON: u8 = Note::C4;
pub const NOTE_OFF: u8 = Note::G3;
pub const NOTE_SELECT: u8 = Note::C3;
pub const NOTE_OPTION: u8 = Note::G4;

pub const MF_DURATION: u32 = 125;
pub const MF_DURATION_SHORT: u32 = 75;

pub struct MenuFeedback
{
    // velocity ix fixed (value chosen in prog.rs)
    pub key: u8,
    pub duration: u32,
    pub channel: Channel
}

impl MenuFeedback
{
    #[inline]
    pub const fn new(key: u8, duration: u32, channel: Channel) -> Self
    {
        return Self { key, duration, channel };
    }
    
    #[inline]
    pub const fn note_fail(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_FAIL,
            duration: MF_DURATION,
            channel
        };
    }
    #[inline]
    pub const fn note_fail_short(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_FAIL,
            duration: MF_DURATION_SHORT,
            channel
        };
    }
    #[inline]
    pub const fn note_on(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_ON,
            duration: MF_DURATION,
            channel
        };
    }
    #[inline]
    pub const fn note_on_short(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_ON,
            duration: MF_DURATION_SHORT,
            channel
        };
    }
    #[inline]
    pub const fn note_off(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_OFF,
            duration: MF_DURATION,
            channel
        };
    }
    #[inline]
    pub const fn note_off_short(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_OFF,
            duration: MF_DURATION_SHORT,
            channel
        };
    }
    #[inline]
    pub const fn note_select(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_SELECT,
            duration: MF_DURATION,
            channel
        };
    }
    #[inline]
    pub const fn note_select_short(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_SELECT,
            duration: MF_DURATION_SHORT,
            channel
        };
    }
    #[inline]
    pub const fn note_option(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_OPTION,
            duration: MF_DURATION,
            channel
        };
    }
    #[inline]
    pub const fn note_option_short(channel: Channel) -> Self
    {
        return Self {
            key: NOTE_OPTION,
            duration: MF_DURATION_SHORT,
            channel
        };
    }
    #[inline]
    pub const fn boolean(value: bool, channel: Channel) -> Self
    {
        return Self {
            key: match value {
                true => NOTE_ON,
                false => NOTE_OFF
            },
            duration: MF_DURATION,
            channel
        };
    }
    pub const fn number(key: u8, channel: Channel) -> Self
    {
        return Self {
            key: Note::A3 - Note::A0 + key,
            duration: MF_DURATION,
            channel
        };
    }
    pub const fn slot(key: u8, channel: Channel) -> Self
    {
        return Self {
            key: Note::A2 - Note::A0 + key,
            duration: MF_DURATION,
            channel
        };
    }
}