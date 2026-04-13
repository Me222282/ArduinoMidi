use api::{CCType, Channel, Note};

use crate::{MF_DURATION, Menu, MenuState, NOTEOPTION, Panel, TriggerSource, menu_toggle, value_or_last};

pub struct ProgramPortsMenu<'a>
{
    panel: &'a mut Panel,
    channel_lv: usize,
    pulse_length_lv: usize,
    cc_source_lv: usize
}

const CHANNEL_KEY: u8 = Note::G3;
const PULSE_LENGTH_KEY: u8 = Note::A3;
const CC1_KEY: u8 = Note::C5;
const CC2_KEY: u8 = Note::D5;
const CC3_KEY: u8 = Note::E5;
const CC4_KEY: u8 = Note::F5;
const CC5_KEY: u8 = Note::G5;

impl<'a> Menu for ProgramPortsMenu<'a>
{
    fn on_note(menu: &mut super::MenuWrapper<Self>, channel: Channel, note: Note) -> bool
    {
        let config = &mut menu.panel.configuration;
        
        match note.key
        {
            Note::C3 if channel <= Channel::C5 =>
            {
                let enabled = menu.panel.toggle_trigger(channel as usize);
                menu.trigger_feedback(enabled, channel);
            },
            Note::D3 if channel <= Channel::C5 =>
            {
                menu.panel.set_trigger(channel as usize, TriggerSource::Arpeggio(Channel::All));
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::Eb3 if channel <= Channel::C5 =>
            {
                menu.panel.set_trigger(channel as usize, TriggerSource::Note(Channel::All));
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::E3 if channel <= Channel::C5 =>
            {
                menu.panel.set_trigger(channel as usize, TriggerSource::Sequence(Channel::All));
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::F3 if channel <= Channel::C5 =>
            {
                menu.panel.set_trigger(channel as usize, TriggerSource::Track(Channel::All));
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            CHANNEL_KEY if channel <= Channel::C5 => menu.set_state(MenuState::number(2, 1..16, note.key, channel)),
            PULSE_LENGTH_KEY => menu.set_state(MenuState::number(4, 1.., note.key, Channel::All)),
            Note::C4 =>
            {
                let enabled = menu.panel.toggle_cc(0);
                menu.trigger_feedback(enabled, Channel::All);
            },
            Note::D4 =>
            {
                let enabled = menu.panel.toggle_cc(1);
                menu.trigger_feedback(enabled, Channel::All);
            },
            Note::E4 =>
            {
                let enabled = menu.panel.toggle_cc(2);
                menu.trigger_feedback(enabled, Channel::All);
            },
            Note::F4 =>
            {
                let enabled = menu.panel.toggle_cc(3);
                menu.trigger_feedback(enabled, Channel::All);
            },
            Note::G4 =>
            {
                let enabled = menu.panel.toggle_cc(4);
                menu.trigger_feedback(enabled, Channel::All);
            },
            Note::A4 => menu_toggle!(menu, config.per_channel_cc),
            CC1_KEY if channel <= Channel::C5 => menu.set_state(MenuState::number(3, ..127, note.key, channel)),
            CC2_KEY if channel <= Channel::C5 => menu.set_state(MenuState::number(3, ..127, note.key, channel)),
            CC3_KEY if channel <= Channel::C5 => menu.set_state(MenuState::number(3, ..127, note.key, channel)),
            CC4_KEY if channel <= Channel::C5 => menu.set_state(MenuState::number(3, ..127, note.key, channel)),
            CC5_KEY if channel <= Channel::C5 => menu.set_state(MenuState::number(3, ..127, note.key, channel)),
            _ => {}
        }
        
        return false;
    }
    
    fn on_number_input(&mut self, value: Option<usize>, channel: Channel, key: u8)
    {
        match key
        {
            CHANNEL_KEY =>
            {
                let c = value_or_last!(value, self.channel_lv);
                self.panel.set_trigger_channel(channel as usize, Channel::from_u8(c as u8));
            },
            PULSE_LENGTH_KEY => self.panel.configuration.pulse_length = value_or_last!(value, self.pulse_length_lv),
            CC1_KEY => self.panel.set_cc(0, CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC2_KEY => self.panel.set_cc(1, CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC3_KEY => self.panel.set_cc(2, CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC4_KEY => self.panel.set_cc(3, CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC5_KEY => self.panel.set_cc(4, CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            _ => {}
        }
    }
    
    fn reset_values(&self)
    {
        todo!()
    }

    fn save_values(&self)
    {
        todo!()
    }

    fn load_values(&self)
    {
        todo!()
    }
}