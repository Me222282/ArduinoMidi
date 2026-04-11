#![no_std]

mod panel;
pub use crate::panel::*;

mod menus;
pub use crate::menus::*;

use api::{MidiCode, Note};

pub trait InputListener
{
    fn on_loop(&mut self);
    fn on_note(&mut self, channel: u8, note: Note) -> bool;
    
    fn on_message(&mut self, message: MidiCode) {}
    fn allow_message(&self, message: MidiCode) -> bool { true }
}

macro_rules! create_dynamic_input_listener
{
    ($visability:vis $name:ident: $($n:ident => $t:ty),+) =>
    {
        $visability enum $name
        {
            $($n($t)),+
        }
        
        $(
            impl From<$t> for $name
            {
                #[inline]
                fn from(value: $t) -> Self
                {
                    return Self::$n(value);
                }
            }
        )+
        
        impl InputListener for $name
        {
            fn on_loop(&mut self)
            {
                match self
                {
                    $(Self::$n(t) => t.on_loop()),+
                }
            }
            fn on_note(&mut self, channel: u8, note: Note) -> bool
            {
                return match self
                {
                    $(Self::$n(t) => t.on_note(channel, note)),+
                };
            }
            
            fn on_message(&mut self, message: MidiCode)
            {
                match self
                {
                    $(Self::$n(t) => t.on_message(message)),+
                }
            }
            fn allow_message(&self, message: MidiCode) -> bool
            {
                return match self
                {
                    $(Self::$n(t) => t.allow_message(message)),+
                };
            }
        }
    };
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TriggerSource
{
    Arpeggio,
    Note,
    Sequence,
    Track
}

pub struct Configuration
{
    micro_tone: bool
}

// struct A
// {
    
// }
// impl InputListener for A
// {
//     fn on_loop(&mut self)
//     {
//         todo!()
//     }

//     fn on_note(&mut self, channel: u8, note: Note) -> bool
//     {
//         todo!()
//     }
// }
// struct B
// {
    
// }
// impl InputListener for B
// {
//     fn on_loop(&mut self)
//     {
//         todo!()
//     }

//     fn on_note(&mut self, channel: u8, note: Note) -> bool
//     {
//         todo!()
//     }
// }

// create_dynamic_input_listener!(pub DIL: A => A, B => B);