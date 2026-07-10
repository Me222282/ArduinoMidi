use api::{Channel, MidiCode, Note, NoteKey, NvsInterface};

use crate::{Configuration, Menu, MenuFeedback, MenuState};

pub trait MenuWrapTrait
{
    fn on_note<T: NvsInterface>(&mut self, config: &mut Configuration, nvs: &mut T, time: u32, channel: Channel, note: Note) -> (bool, Option<MenuFeedback>);
    fn off_note(&mut self, _config: &mut Configuration, _channel: Channel, _note: Note) { }
    
    fn on_reset_switch(&mut self) -> (bool, Option<MenuFeedback>) { (true, None) }
    fn on_message(&mut self, _message: MidiCode) { }
    fn allow_message(&self, _message: MidiCode) -> bool { true }
}

macro_rules! create_dynamic_menus
{
    ($visability:vis $name:ident: $($n:ident => $t:ty),+) =>
    {
        $visability enum $name
        {
            None,
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
        
        impl $name
        {
            pub fn is_none(&self) -> bool
            {
                return match self
                {
                    Self::None => true,
                    _ => false
                };
            }
        }
        
        impl crate::MenuWrapTrait for $name
        {
            fn on_note<T: api::NvsInterface>(&mut self, config: &mut crate::Configuration, nvs: &mut T, time: u32, channel: api::Channel, note: api::Note) -> (bool, Option<crate::MenuFeedback>)
            {
                return match self
                {
                    Self::None => (false, None),
                    $(Self::$n(t) => t.on_note::<T>(config, nvs, time, channel, note)),+
                };
            }
            fn off_note(&mut self, config: &mut crate::Configuration, channel: api::Channel, note: api::Note)
            {
                match self
                {
                    Self::None => {},
                    $(Self::$n(t) => t.off_note(config, channel, note)),+
                }
            }
            
            fn on_reset_switch(&mut self) -> (bool, Option<crate::MenuFeedback>)
            {
                match self
                {
                    Self::None => (false, None),
                    $(Self::$n(t) => t.on_reset_switch()),+
                }
            }
            fn on_message(&mut self, message: api::MidiCode)
            {
                match self
                {
                    Self::None => {},
                    $(Self::$n(t) => t.on_message(message)),+
                }
            }
            fn allow_message(&self, message: api::MidiCode) -> bool
            {
                return match self
                {
                    Self::None => true,
                    $(Self::$n(t) => t.allow_message(message)),+
                };
            }
        }
    };
}
pub(in crate::menus) use create_dynamic_menus;

pub const MAX_DIGITS: usize = 5;

fn get_value(digits: &[u8; 5], start: u8) -> usize
{
    let start = start as usize;
    
    let mut value = 0;
    value += digits[start] as usize;
    if start <= 0 { return value; }
    
    value += digits[start - 1] as usize * 10;
    if start <= 1 { return value; }
    
    value += digits[start - 2] as usize * 100;
    if start <= 2 { return value; }
    
    value += digits[start - 3] as usize * 1000;
    if start <= 3 { return value; }
    
    value += digits[start - 4] as usize * 10000;
    
    return value;
}

pub(in crate::menus) struct MenuWrapper<T: Menu>
{
    state: MenuState,
    digits: [u8; MAX_DIGITS],
    d_count: u8,
    tap_duration: u32,
    first_tap_time: u32,
    tap_count: usize,
    key_select: NoteKey,
    pub menu: T
}
impl<T: Menu> MenuWrapper<T>
{
    pub fn new(menu: T) -> Self
    {
        return Self {
            state: MenuState::Listening,
            digits: [0; MAX_DIGITS],
            d_count: 0,
            tap_duration: 0,
            first_tap_time: 0,
            tap_count: 0,
            key_select: NoteKey::C,
            menu
        };
    }
    pub fn into_menu(self) -> T
    {
        return self.menu;
    }
    
    fn set_state(&mut self, mut fb: Option<MenuFeedback>, state: MenuState) -> (bool, Option<MenuFeedback>)
    {
        if state == MenuState::Exit
        {
            return (true, None);
        }
        
        match state
        {
            MenuState::TapTime { key: _, channel } =>
            {
                self.tap_count = 0;
                self.tap_duration = 0;
                fb = Some(MenuFeedback::note_select(channel));
            },
            MenuState::Number { digits: _, min: _min, max: _max, key: _key, channel } =>
            {
                self.d_count = 0;
                self.digits = [0; MAX_DIGITS];
                fb = Some(MenuFeedback::note_select(channel));
            },
            MenuState::KeySelect { key: _, channel } =>
            {
                fb = Some(MenuFeedback::note_select(channel));
            }
            _ => {}
        }
        self.state = state;
        return (false, fb);
    }
}

impl<T: Menu> MenuWrapTrait for MenuWrapper<T>
{
    fn on_note<N: NvsInterface>(&mut self, config: &mut Configuration, nvs: &mut N, time: u32, channel: Channel, note: Note) -> (bool, Option<MenuFeedback>)
    {
        return match self.state
        {
            MenuState::Listening =>
            {
                match note.key
                {
                    Note::B3 =>
                    {
                        self.menu.reset_values(config);
                        (false, Some(MenuFeedback::note_on(Channel::All)))
                    },
                    Note::Bb4 =>
                    {
                        self.menu.load_values(config, nvs);
                        (false, Some(MenuFeedback::note_option(Channel::All)))
                    },
                    Note::B4 =>
                    {
                        self.menu.save_values(config, nvs);
                        (false, Some(MenuFeedback::note_option(Channel::All)))
                    },
                    _ =>
                    {
                        let ns = self.menu.on_note(config, channel, note);
                        self.set_state(ns.1, ns.0)
                    }
                }
            },
            MenuState::Number { digits, min, max, key, channel: cf } =>
            {
                // number is entered
                if note.key == key
                {
                    self.state = MenuState::Listening;
                    
                    let value = match self.d_count == 0
                    {
                        true => None,
                        false =>
                        {
                            let value = get_value(&self.digits, self.d_count - 1);
                            
                            // out of bounds
                            if value < min || value > max
                            {
                                return (false, Some(MenuFeedback::note_fail(cf)));
                            }
                            
                            Some(value)
                        },
                    };
                    
                    self.menu.on_number_input(config, value, channel, key);
                    return (false, Some(MenuFeedback::note_select(cf)));
                }
                
                // add digit
                if (cf != Channel::All && cf != channel) || self.d_count >= digits
                {
                    return (false, Some(MenuFeedback::note_fail(cf)));
                }
                
                let digit = match note.key
                {
                    Note::A0 => 1,
                    Note::B0 => 2,
                    Note::C1 => 3,
                    Note::D1 => 4,
                    Note::E1 => 5,
                    Note::F1 => 6,
                    Note::G1 => 7,
                    Note::A1 => 8,
                    Note::B1 => 9,
                    Note::C2 => 0,
                    _ => return (false, Some(MenuFeedback::note_fail(cf)))
                };
                self.digits[self.d_count as usize] = digit;
                self.d_count += 1;
                (false, Some(MenuFeedback::number(note.key, cf)))
            },
            MenuState::TapTime { key: _, channel: cf } =>
            {
                if cf != Channel::All && cf != channel
                {
                    return (false, Some(MenuFeedback::note_fail(cf)));
                }
                
                if self.tap_count != 0
                {
                    self.tap_duration = time - self.first_tap_time;
                }
                else
                {
                    self.first_tap_time = time;
                }
                self.tap_count += 1;
                (false, Some(MenuFeedback::note_option_short(cf)))
            },
            MenuState::KeySelect { key, channel: cf } =>
            {
                if cf != Channel::All && cf != channel
                {
                    return (false, Some(MenuFeedback::note_fail(cf)));
                }
                
                // exit key select
                if note.key == key
                {
                    self.menu.on_key_select(config, self.key_select, channel, key);
                    self.state = MenuState::Listening;
                    return (false, Some(MenuFeedback::note_select(cf)));
                }
                
                let key_scale = note.key - Note::C1;
                if key_scale < 12
                {
                    // won't fail due to check that key is in range
                    self.key_select = unsafe { NoteKey::from_key(note.key) };
                    return (false, Some(MenuFeedback::number(note.key, cf)));
                }
                
                // error
                (false, Some(MenuFeedback::note_fail(cf)))
            },
            MenuState::Exit => (true, None)
        }
    }
    
    // #[inline]
    // fn on_loop(&mut self, panel: &mut Panel)
    // {
    //     if self.feedback
    //     {
    //         let t = panel.get_time();
    //         if t - self.feedback_start >= self.feedback_length
    //         {
    //             self.feedback = false;
    //             panel.output_gate(Gate::zero());
    //         }
    //     }
        
    //     self.menu.on_loop();
    // }
    
    fn on_reset_switch(&mut self) -> (bool, Option<MenuFeedback>)
    {
        if self.state == MenuState::Listening
        {
            return (T::auto_close(), None);
        }
        
        self.state = MenuState::Listening;
        return (false, Some(MenuFeedback::note_fail(Channel::All)));
    }
    
    #[inline]
    fn off_note(&mut self, config: &mut Configuration, channel: Channel, note: Note)
    {
        if let MenuState::TapTime { key, channel: cf } = self.state
        {
            if cf == Channel::All || cf == channel
            {
                if self.tap_count > 0
                {
                    self.menu.on_tap_time(config, self.tap_duration / (self.tap_count - 1) as u32, channel, key);
                }
                self.state = MenuState::Listening;
            }
        }
        
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