#![no_std]
#![feature(core_intrinsics)]
#![feature(allocator_api)]

mod panel;
pub use crate::panel::*;

mod menus;
pub(crate) use crate::menus::*;

mod notes;
pub(crate) use crate::notes::*;

mod operators;
pub use crate::operators::*;

mod sequencer;
pub(crate) use crate::sequencer::*;

mod prog;
pub use crate::prog::*;

use api::{CCType, Channel, ChannelVoice, NoteKey, NoteOffset};

extern crate alloc;

// ==========PIPELINE==========
// Channel Filter
// CC Outputs
// Menu
// Sequencer
// Channel Redirect
// Channel Offset
// KeyNote Filter
// Arpeggio
// All Channel Mode
// Vibrato + Pitch Bend + Mod Wheel
// Note Voice Processor
// Slot Allocation
// Output

pub const RETRIG_TIME: u32 = 4;
pub const FACTORY_RESET_TIME: u32 = 750;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum TriggerSource
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

pub(crate) enum ArpeggioMode
{
    Ascending,
    Decending,
    Alternating
}
pub(crate) struct Arpeggio
{
    enabled: bool,
    time: u32,
    mode: ArpeggioMode,
    sort_notes: bool,
    half_notes: bool
}
#[derive(Debug, PartialEq, Eq)]
pub(crate) enum AttenuationSource
{
    None,
    Modulation,
    CC(CCType)
}
#[derive(Debug, Clone, Copy)]
pub(crate) enum FreqCorrection
{
    None,
    Half,
    Full
}
pub(crate) struct Vibrato
{
    enabled: bool,
    function: fn(f32) -> f32,
    angular_velocity: f32,
    scale: f32,
    attenuation: AttenuationSource,
    freq_correction: FreqCorrection
}
pub(crate) struct ChannelRedirect
{
    enabled: bool,
    start: u8,
    end: u8,
    new_channel: Channel
}
// channel enabled pre everything, channel redirects are only on note outputs
// check for no channels enabled
pub(crate) struct ChannelFilter
{
    enabled: bool,
    note_filter: NoteKey,
    filter_keys: bool,
    redirect1: ChannelRedirect,
    redirect2: ChannelRedirect
}

pub(crate) struct Configuration<'a>
{
    // pub other: &'a mut OtherConfig,
    pub note: &'a mut NoteConfig,
    // pub sequen: &'a mut SequencerConfig,
    pub panel: &'a mut PanelConfig,
    pub output: &'a mut OutputConfig,
    pub vibrato: &'a mut VibratoConfig,
    pub arpeggio: &'a mut ArpeggioConfig
}

// pub struct OtherConfig
// {
//     // retrigger_old: bool,
//     // retrigger_new: bool,
//     // filter_keys: bool,
//     // filter: NoteKey,
//     // always_delay: bool,
//     // micro_tone: bool,
//     // forget_notes: bool,
//     // duplicate_release: bool,
//     // sort_notes: bool,
//     // all_channel_mode: bool,
//     // all_channel_pb: bool,
//     // alternate_allocations: bool,
//     // menu_feedback: bool,
//     // clocked_arpeggios: bool,
//     // global_vibrato: bool,
//     // per_channel_cc: bool,
//     pulse_length: usize,
    
//     // use_custom_allocations: bool,
//     // custom_allocations: [(Channel, u8); 5],
    
//     // channel_filters: [ChannelFilter; 16],
//     // arpeggios: [Arpeggio; 16],
//     // vibratos: [Vibrato; 16],
//     // channel_offsets: [NoteOffset; 16]
// }

pub(crate) struct OutputConfig
{
    retrigger_old: bool,
    retrigger_new: bool,
    always_delay: bool,
    all_channel_mode: bool,
    all_channel_pb: bool,
    all_channel_cc: bool,
    menu_feedback: bool,
    
    channel_filters: [ChannelFilter; 16],
    channel_offsets: [NoteOffset; 16]
}

pub(crate) struct VibratoConfig
{
    global_vibrato: bool,
    vibratos: [Vibrato; 16]
}

pub(crate) struct ArpeggioConfig
{
    clocked_arpeggios: bool,
    arpeggios: [Arpeggio; 16],
}

pub(crate) struct NoteConfig
{
    forget_notes: bool,
    duplicate_release: bool,
    sort_notes: bool,
}

#[derive(Debug, Default)]
pub(crate) struct SequencerConfig
{
    bar_size: u8,
    on_bar_trigger: bool,
    /// Half the full time - for half time stuff
    sequencer_tempo_time: u32,
    clocked_sequencer: bool,
}

pub(crate) struct PanelConfig
{
    use_custom_allocations: bool,
    custom_allocations: [ChannelVoice; 5],
    alternate_allocations: bool,
    
    per_channel_cc: bool,
    cc_enabled: [bool; 5],
    trig_enabled: [bool; 5],
    micro_tone: bool,
    triggers: [TriggerSource; 5],
    cc_sources: [(CCType, Channel); 5]
}