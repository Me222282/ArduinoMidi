#![no_std]

mod panel;
pub use crate::panel::*;

mod menus;
pub use crate::menus::*;

mod notes;
pub use crate::notes::*;

mod operators;
pub use crate::operators::*;

pub mod prog;
// pub use crate::prog::*;

use api::CCType;
use api::Channel;
use api::NoteKey;
use api::NoteOffset;

extern crate alloc;

// ==========PIPELINE==========
// Channel Filter
// CC Outputs
// Menu
// Sequencer
// Arpeggio
// Channel Redirect
// Channel Offset
// KeyNote Filter
// All Channel Mode
// Vibrato + Pitch Bend + Mod Wheel
// Note Voice Processor
// Slot Allocation
// Output

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TriggerSource
{
    Arpeggio(Channel),
    Note(Channel),
    Sequence(Channel),
    Track(Channel)
}
impl TriggerSource
{
    pub fn set_channel(&mut self, channel: Channel)
    {
        match self
        {
            TriggerSource::Arpeggio(cv) => *cv = channel,
            TriggerSource::Note(cv) => *cv = channel,
            TriggerSource::Sequence(cv) => *cv = channel,
            TriggerSource::Track(cv) => *cv = channel
        }
    }
    pub fn get_channel(self) -> Channel
    {
        return match self
        {
            TriggerSource::Arpeggio(channel) => channel,
            TriggerSource::Note(channel) => channel,
            TriggerSource::Sequence(channel) => channel,
            TriggerSource::Track(channel) => channel
        };
    }
    pub fn set_source(&mut self, mut source: TriggerSource)
    {
        source.set_channel(self.get_channel());
        *self = source;
    }
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
#[derive(Debug, PartialEq, Eq)]
pub enum AttenuationSource
{
    None,
    Modulation,
    CC(CCType)
}
pub struct Vibrato
{
    enabled: bool,
    function: fn(f32) -> f32,
    angular_velocity: f32,
    scale: f32,
    attenuation: AttenuationSource
}
pub struct ChannelRedirect
{
    enabled: bool,
    start: u8,
    end: u8,
    new_channel: Channel
}
// channel enabled pre everything, channel redirects are only on note outputs
// check for no channels enabled
pub struct ChannelFilter
{
    enabled: bool,
    note_filter: NoteKey,
    filter_keys: bool,
    redirect1: ChannelRedirect,
    redirect2: ChannelRedirect
}

pub struct Configuration<'a>
{
    pub other: &'a mut OtherConfig,
    pub note: &'a mut NoteConfig,
    pub sequen: &'a mut SequencerConfig,
    pub output: &'a mut OutputConfig,
    pub vibrato: &'a mut VibratoConfig
}

pub struct OtherConfig
{
    retrigger_old: bool,
    retrigger_new: bool,
    // filter_keys: bool,
    // filter: NoteKey,
    always_delay: bool,
    // micro_tone: bool,
    // forget_notes: bool,
    // duplicate_release: bool,
    // sort_notes: bool,
    all_channel_mode: bool,
    all_channel_pb: bool,
    alternate_allocations: bool,
    menu_feedback: bool,
    clocked_arpeggios: bool,
    // global_vibrato: bool,
    per_channel_cc: bool,
    pulse_length: usize,
    
    // use_custom_allocations: bool,
    // custom_allocations: [(Channel, u8); 5],
    
    channel_filters: [ChannelFilter; 16],
    arpeggios: [ArpeggioConfig; 16],
    // vibratos: [Vibrato; 16],
    channel_offsets: [NoteOffset; 16]
}

pub struct VibratoConfig
{
    global_vibrato: bool,
    vibratos: [Vibrato; 16]
}

pub struct NoteConfig
{
    forget_notes: bool,
    duplicate_release: bool,
    sort_notes: bool,
}

pub struct SequencerConfig
{
    bar_size: usize,
    on_bar_trigger: bool,
    sequencer_tempo_time: usize,
    clocked_sequencer: bool,
}

pub struct OutputConfig
{
    use_custom_allocations: bool,
    custom_allocations: [(Channel, u8); 5],
    
    cc_enabled: [bool; 5],
    trig_enabled: [bool; 5],
    micro_tone: bool,
    triggers: [TriggerSource; 5],
    cc_sources: [(CCType, Channel); 5]
}