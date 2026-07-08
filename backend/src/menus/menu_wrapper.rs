use api::{Channel, MidiCode, Note};

use crate::{Configuration, Menu, MenuFeedback, MenuState};

pub trait MenuWrapTrait
{
    fn on_note(&mut self, config: &mut Configuration,  time: u32, channel: Channel, note: Note) -> (bool, Option<MenuFeedback>);
    fn off_note(&mut self, channel: Channel, note: Note) { }
    
    fn on_reset_switch(&mut self) -> (bool, Option<MenuFeedback>) { (true, None) }
    fn on_message(&mut self, message: MidiCode) { }
    fn allow_message(&self, message: MidiCode) -> bool { true }
}

#[macro_export]
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
            fn on_note(&mut self, config: &mut crate::Configuration, time: u32, channel: Channel, note: Note) -> (bool, Option<MenuFeedback>)
            {
                return match self
                {
                    Self::None => (false, None),
                    $(Self::$n(t) => t.on_note(config, time, channel, note)),+
                };
            }
            fn off_note(&mut self, channel: Channel, note: Note)
            {
                match self
                {
                    Self::None => {},
                    $(Self::$n(t) => t.off_note(channel, note)),+
                }
            }
            
            fn on_reset_switch(&mut self) -> (bool, Option<MenuFeedback>)
            {
                match self
                {
                    Self::None => (false, None),
                    $(Self::$n(t) => t.on_reset_switch()),+
                }
            }
            fn on_message(&mut self, message: MidiCode)
            {
                match self
                {
                    Self::None => {},
                    $(Self::$n(t) => t.on_message(message)),+
                }
            }
            fn allow_message(&self, message: MidiCode) -> bool
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

pub const MAX_DIGITS: usize = 5;

pub struct MenuWrapper<T: Menu>
{
    // pub panel: &'a mut Panel,
    state: MenuState,
    digits: [u8; MAX_DIGITS],
    d_count: u8,
    time: u32,
    pub menu: T
}
impl<T: Menu> MenuWrapper<T>
{
    fn set_state(&mut self, mut fb: Option<MenuFeedback>, time: u32, state: MenuState) -> (bool, Option<MenuFeedback>)
    {
        if state == MenuState::Exit
        {
            return (true, None);
        }
        
        match state
        {
            MenuState::TapTime { key: _, channel } =>
            {
                self.time = time;
                fb = Some(MenuFeedback::note_option_short(channel));
            },
            MenuState::Number { digits, min, max, key, channel } =>
            {
                self.d_count = 0;
                self.digits = [0; 5];
                fb = Some(MenuFeedback::note_select(channel));
            },
            MenuState::KeySelect { key, channel } =>
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
    fn on_note(&mut self, config: &mut Configuration, time: u32, channel: Channel, note: Note) -> (bool, Option<MenuFeedback>)
    {
        return match self.state
        {
            MenuState::Listening =>
            {
                if !T::rsl()
                {
                    let ns = self.menu.on_note(config, channel, note);
                    return self.set_state(ns.1, time, ns.0);
                }
                
                match note.key
                {
                    Note::B3 =>
                    {
                        self.menu.reset_values();
                        (false, Some(MenuFeedback::boolean(true, Channel::All)))
                    },
                    Note::Bb4 =>
                    {
                        self.menu.load_values();
                        (false, Some(MenuFeedback::note_option(Channel::All)))
                    },
                    Note::B4 =>
                    {
                        self.menu.save_values();
                        (false, Some(MenuFeedback::note_option(Channel::All)))
                    },
                    _ =>
                    {
                        let ns = self.menu.on_note(config, channel, note);
                        self.set_state(ns.1, time, ns.0)
                    }
                }
            },
            MenuState::Number { digits, min, max, key, channel: cf } =>
            {
                if cf != Channel::All && cf != channel
                {
                    return (false, Some(MenuFeedback::note_fail(cf)));
                }
                (false, None)
            },
            MenuState::TapTime { key, channel } =>
            {
                
                (false, None)
            },
            MenuState::KeySelect { key, channel } =>
            {
                
                (false, None)
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
            if T::auto_close()
            {
                return (true, None);
            }
            
            return (false, None);
        }
        
        self.state = MenuState::Listening;
        return (false, Some(MenuFeedback::note_fail(Channel::All)));
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