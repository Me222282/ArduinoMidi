#![allow(non_upper_case_globals)]

#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NoteKey
{
    C = Note::C_1,
    Db = Note::Db_1,
    D = Note::D_1,
    Eb = Note::Eb_1,
    E = Note::E_1,
    F = Note::F_1,
    Gb = Note::Gb_1,
    G = Note::G_1,
    Ab = Note::Ab_1,
    A = Note::A_1,
    Bb = Note::Bb_1,
    B = Note::B_1
}

impl NoteKey
{
    pub const fn contains_note(self, key: u8) -> bool
    {
        let v = (self as u8 + 12 - key) % 12;
        return !(v == 1 || v == 3 || v == 6 || v == 8 || v == 10);
    }
    
    pub const unsafe fn from_key(key: u8) -> NoteKey
    {
        return unsafe { core::mem::transmute(key) };
    }
}
impl PartialEq<u8> for NoteKey
{
    fn eq(&self, other: &u8) -> bool
    {
        return other % 12 == *self as u8;
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct NoteOffset
{
    pub octave: i8,
    pub semi_tone: i8
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
#[repr(u8)]
pub enum Channel
{
    C1 = 0,
    C2 = 1,
    C3 = 2,
    C4 = 3,
    C5 = 4,
    C6 = 5,
    C7 = 6,
    C8 = 7,
    C9 = 8,
    C10 = 9,
    C11 = 10,
    C12 = 11,
    C13 = 12,
    C14 = 13,
    C15 = 14,
    C16 = 15,
    All = 0xFF
}
impl Channel
{
    pub fn from_u8(value: u8) -> Channel
    {
        return match value
        {
            0 => Channel::C1,
            1 => Channel::C2,
            2 => Channel::C3,
            3 => Channel::C4,
            4 => Channel::C5,
            5 => Channel::C6,
            6 => Channel::C7,
            7 => Channel::C8,
            8 => Channel::C9,
            9 => Channel::C10,
            10 => Channel::C11,
            11 => Channel::C12,
            12 => Channel::C13,
            13 => Channel::C14,
            14 => Channel::C15,
            15 => Channel::C16,
            _ => Channel::All
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
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
    
    pub const C_1: u8 = 0u8;
    pub const Db_1: u8 = 1u8;
    pub const D_1: u8 = 2u8;
    pub const Eb_1: u8 = 3u8;
    pub const E_1: u8 = 4u8;
    pub const F_1: u8 = 5u8;
    pub const Gb_1: u8 = 6u8;
    pub const G_1: u8 = 7u8;
    pub const Ab_1: u8 = 8u8;
    pub const A_1: u8 = 9u8;
    pub const Bb_1: u8 = 10u8;
    pub const B_1: u8 = 11u8;
    pub const C0: u8 = 12u8;
    pub const Db0: u8 = 13u8;
    pub const D0: u8 = 14u8;
    pub const Eb0: u8 = 15u8;
    pub const E0: u8 = 16u8;
    pub const F0: u8 = 17u8;
    pub const Gb0: u8 = 18u8;
    pub const G0: u8 = 19u8;
    pub const Ab0: u8 = 20u8;
    pub const A0: u8 = 21u8;
    pub const Bb0: u8 = 22u8;
    pub const B0: u8 = 23u8;
    pub const C1: u8 = 24u8;
    pub const Db1: u8 = 25u8;
    pub const D1: u8 = 26u8;
    pub const Eb1: u8 = 27u8;
    pub const E1: u8 = 28u8;
    pub const F1: u8 = 29u8;
    pub const Gb1: u8 = 30u8;
    pub const G1: u8 = 31u8;
    pub const Ab1: u8 = 32u8;
    pub const A1: u8 = 33u8;
    pub const Bb1: u8 = 34u8;
    pub const B1: u8 = 35u8;
    pub const C2: u8 = 36u8;
    pub const Db2: u8 = 37u8;
    pub const D2: u8 = 38u8;
    pub const Eb2: u8 = 39u8;
    pub const E2: u8 = 40u8;
    pub const F2: u8 = 41u8;
    pub const Gb2: u8 = 42u8;
    pub const G2: u8 = 43u8;
    pub const Ab2: u8 = 44u8;
    pub const A2: u8 = 45u8;
    pub const Bb2: u8 = 46u8;
    pub const B2: u8 = 47u8;
    pub const C3: u8 = 48u8;
    pub const Db3: u8 = 49u8;
    pub const D3: u8 = 50u8;
    pub const Eb3: u8 = 51u8;
    pub const E3: u8 = 52u8;
    pub const F3: u8 = 53u8;
    pub const Gb3: u8 = 54u8;
    pub const G3: u8 = 55u8;
    pub const Ab3: u8 = 56u8;
    pub const A3: u8 = 57u8;
    pub const Bb3: u8 = 58u8;
    pub const B3: u8 = 59u8;
    pub const C4: u8 = 60u8;
    pub const Db4: u8 = 61u8;
    pub const D4: u8 = 62u8;
    pub const Eb4: u8 = 63u8;
    pub const E4: u8 = 64u8;
    pub const F4: u8 = 65u8;
    pub const Gb4: u8 = 66u8;
    pub const G4: u8 = 67u8;
    pub const Ab4: u8 = 68u8;
    pub const A4: u8 = 69u8;
    pub const Bb4: u8 = 70u8;
    pub const B4: u8 = 71u8;
    pub const C5: u8 = 72u8;
    pub const Db5: u8 = 73u8;
    pub const D5: u8 = 74u8;
    pub const Eb5: u8 = 75u8;
    pub const E5: u8 = 76u8;
    pub const F5: u8 = 77u8;
    pub const Gb5: u8 = 78u8;
    pub const G5: u8 = 79u8;
    pub const Ab5: u8 = 80u8;
    pub const A5: u8 = 81u8;
    pub const Bb5: u8 = 82u8;
    pub const B5: u8 = 83u8;
    pub const C6: u8 = 84u8;
    pub const Db6: u8 = 85u8;
    pub const D6: u8 = 86u8;
    pub const Eb6: u8 = 87u8;
    pub const E6: u8 = 88u8;
    pub const F6: u8 = 89u8;
    pub const Gb6: u8 = 90u8;
    pub const G6: u8 = 91u8;
    pub const Ab6: u8 = 92u8;
    pub const A6: u8 = 93u8;
    pub const Bb6: u8 = 94u8;
    pub const B6: u8 = 95u8;
    pub const C7: u8 = 96u8;
    pub const Db7: u8 = 97u8;
    pub const D7: u8 = 98u8;
    pub const Eb7: u8 = 99u8;
    pub const E7: u8 = 100u8;
    pub const F7: u8 = 101u8;
    pub const Gb7: u8 = 102u8;
    pub const G7: u8 = 103u8;
    pub const Ab7: u8 = 104u8;
    pub const A7: u8 = 105u8;
    pub const Bb7: u8 = 106u8;
    pub const B7: u8 = 107u8;
    pub const C8: u8 = 108u8;
    pub const Db8: u8 = 109u8;
    pub const D8: u8 = 110u8;
    pub const Eb8: u8 = 111u8;
    pub const E8: u8 = 112u8;
    pub const F8: u8 = 113u8;
    pub const Gb8: u8 = 114u8;
    pub const G8: u8 = 115u8;
    pub const Ab8: u8 = 116u8;
    pub const A8: u8 = 117u8;
    pub const Bb8: u8 = 118u8;
    pub const B8: u8 = 119u8;
    pub const C9: u8 = 120u8;
    pub const Db9: u8 = 121u8;
    pub const D9: u8 = 122u8;
    pub const Eb9: u8 = 123u8;
    pub const E9: u8 = 124u8;
    pub const F9: u8 = 125u8;
    pub const Gb9: u8 = 126u8;
    pub const G9: u8 = 127u8;
}