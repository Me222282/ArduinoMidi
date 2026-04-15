pub mod special_ops;
pub use special_ops::*;
pub mod vibrato;
pub use vibrato::*;
pub mod program_ports;
pub use program_ports::*;

use core::ops::RangeBounds;
use api::{Channel, Gate, MidiCode, Note, NoteKey};
use crate::{InputListener, Panel};

// macro_rules! create_menu
// {
//     ($channel:expr, $note:expr, $mf:expr; $( $match:expr => $func:expr ),*) =>
//     {
//         match $note.key
//         {
//             $(
//                 $match => $func.invoke($mf, $channel, $note),
//             )*
//             _ => {}
//         }
//     };
// }

// pub struct ToggleValue<'a, T: Menu>
// {
//     value: &'a mut bool,
//     phaton: PhantomData<T>
// }
// impl<'a, T: Menu> ToggleValue<'a, T>
// {
//     pub fn new(value: &'a mut bool) -> Self
//     {
//         return ToggleValue { value, phaton: PhantomData };
//     }
//     pub fn invoke(self, menu: &mut MenuWrapper<T>, _channel: u8, _note: Note)
//     {
//         let nv = !*self.value;
//         *self.value = nv;
//         menu.trigger_feedback(nv);
//     }
// }
// pub struct NumberValue<'a, T: Menu>
// {
//     value: &'a mut usize,
//     phaton: PhantomData<T>
// }
// impl<'a, T: Menu> NumberValue<'a, T>
// {
//     pub fn new(value: &'a mut bool) -> Self
//     {
//         return ToggleValue { value, phaton: PhantomData };
//     }
//     pub fn invoke(self, menu: &mut MenuWrapper<T>, _channel: u8, _note: Note)
//     {
//         let nv = !*self.value;
//         *self.value = nv;
//         menu.trigger_feedback(nv);
//     }
// }

// pub struct Menu
// {
//     op1: bool,
//     op2: bool,
//     op3: bool,
//     op4: bool,
//     op5: bool,
//     op6: bool,
// }
// impl Menu
// {
//     fn on_note(&mut self, channel: u8, note: Note)
//     {
//         let mut mf = MenuFeedback {};
        
//         create_menu!(channel, note, &mut mf;
//             4 => ToggleValue::new(&mut self.op1),
//             5 => ToggleValue::new(&mut self.op2),
//             7 => ToggleValue::new(&mut self.op3)
//         );
//     }
// }

#[macro_export]
macro_rules! menu_toggle
{
    ($menu:ident, $value:expr) =>
    {{
        let nv = !$value;
        $value = nv;
        $menu.trigger_feedback(nv, Channel::All);
    }};
}
#[macro_export]
macro_rules! menu_toggle_channel
{
    ($menu:ident, $channel:ident, $value:expr) =>
    {{
        let nv = !$value;
        $value = nv;
        $menu.trigger_feedback(nv, $channel);
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

pub struct MenuInst<'a, T: Menu>
{
    pub panel: &'a mut Panel,
    mw: &'a mut MenuWrapper<T>
}
impl<'a, T: Menu> MenuInst<'a, T>
{
    fn new(panel: &'a mut Panel, mw: &'a mut MenuWrapper<T>) -> Self
    {
        return MenuInst { panel, mw };
    }
    
    #[inline]
    pub fn trigger_feedback(&mut self, value: bool, channel: Channel)
    {
        self.mw.trigger_feedback(self.panel, value, channel);
    }
    #[inline]
    pub fn play_note(&mut self, key: u8, duration: usize, channel: Channel)
    {
        self.mw.play_note(self.panel, key, duration, channel);
    }
}

pub trait Menu
    where Self: Sized
{
    fn auto_close() -> bool { return true; }
    fn rsl() -> bool { return true; }
    
    fn on_note(menu: MenuInst<Self>, channel: Channel, note: Note) -> MenuState;
    fn on_number_input(&mut self, panel: &mut Panel, value: Option<usize>, channel: Channel, key: u8) { }
    fn on_tap_time(&mut self, panel: &mut Panel, value: usize, channel: Channel, key: u8) { }
    fn on_key_select(&mut self, panel: &mut Panel, value: NoteKey, channel: Channel, key: u8) { }
    
    fn off_note(&self, channel: Channel, note: Note) { }
    fn on_message(&self, message: MidiCode) { }
    fn allow_message(&self, message: MidiCode) -> bool { true }
    fn on_loop(&self) {}
    
    fn reset_values(&self);
    fn save_values(&self);
    fn load_values(&self);
}

pub const MAX_DIGITS: usize = 5;
pub const NOTEFAIL: u8 = Note::B1;
pub const NOTEON: u8 = Note::C4;
pub const NOTEOFF: u8 = Note::G3;
pub const NOTESELECT: u8 = Note::C3;
pub const NOTEOPTION: u8 = Note::G4;

pub const MF_DURATION: usize = 125;
pub const MF_DURATION_SHORT: usize = 75;

pub struct MenuWrapper<T: Menu>
{
    // pub panel: &'a mut Panel,
    state: MenuState,
    digits: [u8; MAX_DIGITS],
    d_count: u8,
    time: usize,
    pub menu: T,
    
    feedback: bool,
    feedback_start: usize,
    feedback_length: usize
}
impl<T: Menu> MenuWrapper<T>
{
    pub fn on_reset_switch(&mut self, panel: &Panel) -> bool
    {
        if self.state == MenuState::Listening
        {
            if T::auto_close()
            {
                return true;
            }
            
            return false;
        }
        
        self.state = MenuState::Listening;
        self.play_note(panel, NOTEFAIL, MF_DURATION, Channel::All);
        return false;
    }
    
    fn set_state(&mut self, panel: &Panel, state: MenuState) -> bool
    {
        if state == MenuState::Exit
        {
            return true;
        }
        
        match state
        {
            MenuState::TapTime { key: _, channel } =>
            {
                self.time = panel.get_time();
                self.play_note(panel, NOTEOPTION, MF_DURATION_SHORT, channel);
            },
            MenuState::Number { digits, min, max, key, channel } =>
            {
                self.d_count = 0;
                self.digits = [0; 5];
                self.play_note(panel, NOTESELECT, MF_DURATION, channel);
            },
            MenuState::KeySelect { key, channel } =>
            {
                self.play_note(panel, NOTESELECT, MF_DURATION, channel);
            }
            _ => {}
        }
        self.state = state;
        return false;
    }
    
    #[inline]
    pub fn trigger_feedback(&mut self, panel: &Panel, value: bool, channel: Channel)
    {
        let k = match value
        {
            true => NOTEON,
            false => NOTEOFF
        };
        self.play_note(panel, k, MF_DURATION, channel);
    }
    pub fn play_note(&mut self, panel: &Panel, key: u8, duration: usize, channel: Channel)
    {
        // output_note accepts Channel::All
        let gate = panel.output_note(crate::SlotSelect::Channel(channel), Note::new(key, 100));
        self.feedback = true;
        self.feedback_length = duration;
        panel.output_gate(gate);
        self.feedback_start = panel.get_time();
    }
}

impl<T: Menu> InputListener for MenuWrapper<T>
{
    fn on_note(&mut self, panel: &mut Panel, channel: Channel, note: Note) -> bool
    {
        return match self.state
        {
            MenuState::Listening =>
            {
                if !T::rsl()
                {
                    let ns = T::on_note(MenuInst::new(panel, self), channel, note);
                    return self.set_state(panel, ns);
                }
                
                match note.key
                {
                    Note::B3 =>
                    {
                        self.menu.reset_values();
                        self.trigger_feedback(panel, true, Channel::All);
                        false
                    },
                    Note::Bb4 =>
                    {
                        self.menu.load_values();
                        self.play_note(panel, NOTEOPTION, MF_DURATION, Channel::All);
                        false
                    },
                    Note::B4 =>
                    {
                        self.menu.save_values();
                        self.play_note(panel, NOTEOPTION, MF_DURATION, Channel::All);
                        false
                    },
                    _ =>
                    {
                        let ns = T::on_note(MenuInst::new(panel, self), channel, note);
                        self.set_state(panel, ns)
                    }
                }
            },
            MenuState::Number { digits, min, max, key, channel: cf } =>
            {
                if cf != Channel::All && cf != channel
                {
                    self.play_note(panel, NOTEFAIL, MF_DURATION, cf);
                    return false;
                }
                false
            },
            MenuState::TapTime { key, channel } =>
            {
                
                false
            },
            MenuState::KeySelect { key, channel } =>
            {
                
                false
            },
            MenuState::Exit => true
        }
    }
    
    #[inline]
    fn on_loop(&mut self, panel: &mut Panel)
    {
        if self.feedback
        {
            let t = panel.get_time();
            if t - self.feedback_start >= self.feedback_length
            {
                self.feedback = false;
                panel.output_gate(Gate::zero());
            }
        }
        
        self.menu.on_loop();
    }
    
    #[inline]
    fn off_note(&mut self, channel: Channel, note: Note)
    {
        self.menu.off_note(channel, note);
    }
    #[inline]
    fn on_message(&mut self, message: MidiCode)
    {
        self.menu.on_message(message);
    }
    #[inline]
    fn allow_message(&self, message: MidiCode) -> bool
    {
        return self.menu.allow_message(message);
    }
}