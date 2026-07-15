mod feedback;
pub use feedback::*;
mod menu_wrapper;
pub use menu_wrapper::*;
mod special_ops;
pub use special_ops::*;
mod vibrato;
pub use vibrato::*;
mod program_ports;
pub use program_ports::*;
mod manager;
pub use manager::*;

use core::ops::RangeBounds;
use api::{Channel, MidiCode, Note, NoteKey, NvsInterface};
use crate::{Configuration, Output};

macro_rules! menu_toggle
{
    ($value:expr) =>
    {{
        let nv = !$value;
        $value = nv;
        Some(crate::MenuFeedback::boolean(nv, Channel::All))
    }};
}
pub(crate) use menu_toggle;
macro_rules! menu_toggle_channel
{
    ($channel:ident, $value:expr) =>
    {{
        let nv = !$value;
        $value = nv;
        Some(crate::MenuFeedback::boolean(nv, $channel))
    }};
}
pub(crate) use menu_toggle_channel;
macro_rules! value_or_last
{
    ($op:expr, $last:expr) =>
    {{
        let v = $op.unwrap_or($last);
        $last = v;
        v
    }};
}
pub(crate) use value_or_last;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MenuState<T = ()>
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
    Custom(T),
    Exit
}
impl<T> MenuState<T>
{
    #[must_use]
    pub fn number<R: RangeBounds<usize>>(digits: u8, range: R, key: u8, channel: Channel) -> Self
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
    type State;
    
    #[inline]
    #[must_use]
    fn auto_close() -> bool { return true; }
    #[inline]
    #[must_use]
    fn menu_feedback(&self) -> bool { return true; }
    
    fn on_note(&mut self, config: &mut Configuration, channel: Channel, note: Note) -> (MenuState<Self::State>, Option<MenuFeedback>);
    fn on_number_input(&mut self, _config: &mut Configuration, _value: Option<usize>, _channel: Channel, _key: u8) { }
    fn on_tap_time(&mut self, _config: &mut Configuration, _value: u32, _channel: Channel, _key: u8) { }
    fn on_key_select(&mut self, _config: &mut Configuration, _value: NoteKey, _channel: Channel, _key: u8) { }
    fn on_custom_state(&mut self, _state: Self::State, _channel: Channel, _note: Note) -> (MenuState<Self::State>, Option<MenuFeedback>) { (MenuState::Listening, None) }
    
    fn off_note(&self, _channel: Channel, _note: Note) { }
    fn on_message<E: api::Externals>(&mut self, _output: &mut Output<E>, _message: MidiCode) { }
    #[must_use]
    fn allow_message(&self, _message: MidiCode) -> bool { true }
    
    fn reset_values(&mut self, config: &mut Configuration);
    fn save_values<T: NvsInterface>(&self, config: &Configuration, nvs: &mut T);
    fn load_values<T: NvsInterface>(&mut self, config: &mut Configuration, nvs: &mut T);
}
