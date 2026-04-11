#![no_std]

mod panel;
pub use crate::panel::*;

mod menus;
pub use crate::menus::*;

use api::NoteKey;
use api::NoteOffset;
use api::{MidiCode, Note};

pub trait InputListener
{
    fn on_loop(&mut self);
    fn on_note(&mut self, channel: u8, note: Note) -> bool;
    fn off_note(&mut self, channel: u8, note: Note) { }
    
    fn on_message(&mut self, message: MidiCode) { }
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
            fn off_note(&mut self, channel: u8, note: Note)
            {
                match self
                {
                    $(Self::$n(t) => t.off_note(channel, note)),+
                }
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
    Arpeggio(u8),
    Note(u8),
    Sequence(u8),
    Track(u8)
}

pub enum ArpeggioMode
{
    Ascending,
    Decending,
    Alternating
}
pub struct ArpeggioConfig
{
    enabled: bool,
    time: usize,
    mode: ArpeggioMode,
    sort_notes: bool,
    half_notes: bool
}
pub struct Vibrato
{
    enabled: bool,
    function: fn(f32) -> f32,
    angular_velocity: f32,
    scale: f32
}
pub struct ChannelRedirect
{
    enabled: bool,
    start: u8,
    end: u8,
    new_channel: u8
}
// channel enabled pre everything, channel redirects are only on note outputs
pub struct ChannelFilter
{
    enabled: bool,
    note_filter: NoteKey,
    filter_keys: bool,
    redirect1: ChannelRedirect,
    redirect2: ChannelRedirect
}

pub struct Configuration
{
    retrigger_old: bool,
    retrigger_new: bool,
    // filter_keys: bool,
    // filter: NoteKey,
    always_delay: bool,
    micro_tone: bool,
    forget_notes: bool,
    duplicate_release: bool,
    sort_notes: bool,
    all_channel_mode: bool,
    alternate_allocations: bool,
    menu_feedback: bool,
    clocked_arpeggios: bool,
    global_vibrato: bool,
    per_channel_cc: bool,
    
    use_custom_allocations: bool,
    custom_allocations: [(u8, u8); 5],
    
    bar_size: usize,
    on_bar_trigger: bool,
    sequencer_tempo_time: usize,
    clocked_sequencer: bool,
    
    channel_filters: [ChannelFilter; 16],
    arpeggios: [ArpeggioConfig; 16],
    vibratos: [Vibrato; 16],
    channel_offsets: [NoteOffset; 16],
    
    triggers: [TriggerSource; 5],
    cc_sources: [u8; 5]
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