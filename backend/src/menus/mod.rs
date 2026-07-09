pub mod feedback;
pub use feedback::*;
pub mod menu_wrapper;
pub use menu_wrapper::*;
pub mod special_ops;
pub use special_ops::*;
pub mod vibrato;
pub use vibrato::*;
pub mod program_ports;
pub use program_ports::*;

use core::ops::RangeBounds;
use api::{Channel, MidiCode, Note, NoteKey};
use crate::{Configuration, SequencerMenu};

#[macro_export]
macro_rules! menu_toggle
{
    ($menu:ident, $value:expr) =>
    {{
        let nv = !$value;
        $value = nv;
        Some(crate::MenuFeedback::boolean(nv, Channel::All))
    }};
}
#[macro_export]
macro_rules! menu_toggle_channel
{
    ($menu:ident, $channel:ident, $value:expr) =>
    {{
        let nv = !$value;
        $value = nv;
        Some(crate::MenuFeedback::boolean(nv, $channel))
    }};
}
#[macro_export]
macro_rules! value_or_last
{
    ($op:expr, $last:expr) =>
    {{
        let v = $op.unwrap_or($last);
        $last = v;
        v
    }};
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MenuState
{
    Listening,
    Number{
        digits: u8,
        min: usize,
        max: usize,
        key: u8,
        channel: Channel
    },
    TapTime{
        key: u8,
        channel: Channel
    },
    KeySelect{
        key: u8,
        channel: Channel
    },
    Exit
}
impl MenuState
{
    pub fn number<R: RangeBounds<usize>>(digits: u8, range: R, key: u8, channel: Channel) -> MenuState
    {
        let min = match range.start_bound()
        {
            core::ops::Bound::Included(v) => *v,
            core::ops::Bound::Excluded(v) => *v + 1,
            core::ops::Bound::Unbounded => usize::MIN,
        };
        let max = match range.end_bound()
        {
            core::ops::Bound::Included(v) => *v,
            core::ops::Bound::Excluded(v) => *v - 1,
            core::ops::Bound::Unbounded => usize::MAX,
        };
        return MenuState::Number { digits, min, max, key, channel }
    }
}

pub trait Menu
    where Self: Sized
{
    fn auto_close() -> bool { return true; }
    
    fn on_note(&mut self, config: &mut Configuration, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>);
    fn on_number_input(&mut self, _config: &mut Configuration, _value: Option<usize>, _channel: Channel, _key: u8) { }
    fn on_tap_time(&mut self, _config: &mut Configuration, _value: u32, _channel: Channel, _key: u8) { }
    fn on_key_select(&mut self, _config: &mut Configuration, _value: NoteKey, _channel: Channel, _key: u8) { }
    
    fn off_note(&self, _channel: Channel, _note: Note) { }
    fn on_message(&self, _message: MidiCode) { }
    fn allow_message(&self, _message: MidiCode) -> bool { true }
    fn on_loop(&self) {}
    
    fn reset_values(&self);
    fn save_values(&self);
    fn load_values(&self);
}

#[derive(Debug, Default)]
pub struct MenuStorage
{
    vibrato: VibratoMenu,
    special_ops: SpecialOpsMenu,
    program_ports: ProgramPortsMenu,
    sequencer: SequencerMenu
}

impl MenuStorage
{
    pub fn get_vibrato(&mut self) -> VibratoMenu
    {
        return core::mem::replace(&mut self.vibrato, VibratoMenu::default());
    }
    pub fn get_special_ops(&mut self) -> SpecialOpsMenu
    {
        return core::mem::replace(&mut self.special_ops, SpecialOpsMenu::default());
    }
    pub fn get_program_ports(&mut self) -> ProgramPortsMenu
    {
        return core::mem::replace(&mut self.program_ports, ProgramPortsMenu::default());
    }
    pub fn get_sequencer(&mut self) -> SequencerMenu
    {
        return core::mem::replace(&mut self.sequencer, SequencerMenu::default());
    }
    
    pub fn set_vibrato(&mut self, menu: VibratoMenu)
    {
        self.vibrato = menu;
    }
    pub fn set_special_ops(&mut self, menu: SpecialOpsMenu)
    {
        self.special_ops = menu;
    }
    pub fn set_program_ports(&mut self, menu: ProgramPortsMenu)
    {
        self.program_ports = menu;
    }
    pub fn set_sequencer(&mut self, menu: SequencerMenu)
    {
        self.sequencer = menu;
    }
}
