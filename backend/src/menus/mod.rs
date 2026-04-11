use api::{Gate, MidiCode, Note};

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

#[derive(Debug, Clone, Copy)]
pub enum MenuState
{
    Listening,
    Number{
        digits: u8,
        min: usize,
        max: usize,
        key: u8
    },
    TapTime{
        key: u8
    },
    KeySelect{
        key: u8
    }
}

pub trait Menu
    where Self: Sized
{
    fn auto_close() -> bool { return true; }
    fn rsl() -> bool { return true; }
    
    fn on_note(menu: &mut MenuWrapper<Self>, channel: u8, note: Note) -> bool;
    fn on_number_input(&mut self, value: usize, key: u8) { }
    fn on_tap_time(&mut self, value: usize, key: u8) { }
    fn on_key_select(&mut self, value: usize, key: u8) { }
    
    fn on_message(&mut self, message: MidiCode) {}
    fn allow_message(&self, message: MidiCode) -> bool { true }
    fn on_loop(&mut self) {}
    
    fn reset_values(&mut self);
    fn save_values(&mut self);
    fn load_values(&mut self);
}

pub const MAX_DIGITS: usize = 5;
pub const NOTEFAIL: u8 = Note::B1;
pub const NOTEON: u8 = Note::C4;
pub const NOTEOFF: u8 = Note::G3;
pub const NOTESELECT: u8 = Note::C3;
pub const NOTEOPTION: u8 = Note::G4;

pub const MF_DURATION: usize = 125;
pub const MF_DURATION_SHORT: usize = 75;

pub struct MenuWrapper<'a, T: Menu>
{
    pub panel: &'a mut Panel,
    state: MenuState,
    digits: [u8; MAX_DIGITS],
    d_count: u8,
    time: usize,
    pub menu: T,
    
    feedback: bool,
    feedback_start: usize,
    feedback_length: usize
}
impl<'a, T: Menu> MenuWrapper<'a, T>
{
    pub fn set_state(&mut self, state: MenuState)
    {
        match state
        {
            MenuState::TapTime { key: _ } => self.time = self.panel.get_time(),
            MenuState::Number { digits, min, max, key } =>
            {
                self.d_count = 0;
                self.digits = [0; 5];
            },
            _ => {}
        }
        self.state = state;
    }
    
    #[inline]
    pub fn trigger_feedback(&mut self, value: bool)
    {
        let k = match value
        {
            true => NOTEON,
            false => NOTEOFF
        };
        self.play_note(k, MF_DURATION);
    }
    #[inline]
    pub fn trigger_feedback_channel(&mut self, value: bool, channel: u8)
    {
        let k = match value
        {
            true => NOTEON,
            false => NOTEOFF
        };
        self.play_note_channel(k, MF_DURATION, channel);
    }
    pub fn play_note(&mut self, key: u8, duration: usize)
    {
        let gate = self.panel.output_note(crate::SlotSelect::All, Note::new(key, 100));
        self.feedback = true;
        self.feedback_length = duration;
        self.panel.output_gate(gate);
        self.feedback_start = self.panel.get_time();
    }
    pub fn play_note_channel(&mut self, key: u8, duration: usize, channel: u8)
    {
        let gate = self.panel.output_note(crate::SlotSelect::Channel(channel), Note::new(key, 100));
        self.feedback = true;
        self.feedback_length = duration;
        self.panel.output_gate(gate);
        self.feedback_start = self.panel.get_time();
    }
}

impl<'a, T: Menu> InputListener for MenuWrapper<'a, T>
{
    fn on_note(&mut self, channel: u8, note: Note) -> bool
    {
        return match self.state
        {
            MenuState::Listening =>
            {
                if !T::rsl()
                {
                    return T::on_note(self, channel, note);
                }
                
                match note.key
                {
                    Note::B3 =>
                    {
                        self.menu.reset_values();
                        self.trigger_feedback(true);
                        false
                    },
                    Note::Bb4 =>
                    {
                        self.menu.load_values();
                        self.play_note(NOTEOPTION, MF_DURATION);
                        false
                    },
                    Note::B4 =>
                    {
                        self.menu.save_values();
                        self.play_note(NOTEOPTION, MF_DURATION);
                        false
                    },
                    _ => T::on_note(self, channel, note)
                }
            },
            MenuState::Number { digits, min, max, key } =>
            {
                
                false
            },
            MenuState::TapTime { key } =>
            {
                
                false
            },
            MenuState::KeySelect { key } =>
            {
                
                false
            },
        }
    }
    
    #[inline]
    fn on_loop(&mut self)
    {
        if self.feedback
        {
            let t = self.panel.get_time();
            if t - self.feedback_start >= self.feedback_length
            {
                self.feedback = false;
                self.panel.output_gate(Gate::zero());
            }
        }
        
        self.menu.on_loop();
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