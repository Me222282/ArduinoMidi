#![no_std]
#![feature(allocator_api)]
#![allow(incomplete_features)]
#![feature(generic_const_exprs)]

mod midi;
pub use crate::midi::*;

mod data;
pub use crate::data::*;

mod linked_list;
pub use crate::linked_list::*;

mod stack_array;
pub use crate::stack_array::*;

mod queue;
pub use crate::queue::*;

mod cubic;
pub use crate::cubic::*;

extern crate alloc;

pub enum InputMode
{
    TakeFirst,
    TakeLast,
    Ignore
}

pub struct PanelState
{
    pub channels: u8,
    pub voices: u8,
    pub octave: i8,
    pub pitch_bend: u8,
    pub stack: bool,
    pub modulation: bool,
    pub input: InputMode
}

pub struct Externals
{
    pub set_gate: fn(Gate),
    pub set_note: fn(usize, u8),
    pub set_vel: fn(usize, u8),
    pub set_pitch_bend: fn(usize, u16),
    pub set_mod: fn(u16),
    
    pub delay: fn(u32),
    pub get_note: fn(usize) -> u8,
    pub log: fn(f32) -> f32
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Gate(pub(crate) u8);

impl Gate
{
    pub fn set(self, bit: u8, value: bool) -> Gate
    {
        return match value
        {
            true => self.on(bit),
            false => self.off(bit)
        };
    }
    pub fn on(self, bit: u8) -> Gate
    {
        return Gate(self.0 | 1 << bit);
    }
    pub fn off(self, bit: u8) -> Gate
    {
        return Gate(self.0 & !(1 << bit));
    }
    pub fn get(&self, bit: u8) -> bool
    {
        return (self.0 & 1 << bit) > 0;
    }
    
    pub const fn zero() -> Gate
    {
        return Gate(0);
    }
    pub const fn all_on() -> Gate
    {
        return Gate(0b00011111);
    }
    pub const fn new(value: u8) -> Gate
    {
        return Gate(value);
    }
    
    pub const fn invert(self) -> Gate
    {
        return Gate(!self.0);
    }
    pub const fn mask(self, mask: Gate) -> Gate
    {
        return Gate(self.0 & mask.0);
    }
}