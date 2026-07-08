use api::{CCType, Channel, Note};

use crate::{Configuration, Menu, MenuFeedback, MenuState, TriggerSource, menu_toggle, menu_toggle_channel, value_or_last};

#[derive(Debug, Default)]
pub struct ProgramPortsMenu
{
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

impl Menu for ProgramPortsMenu
{
    fn on_note(&mut self, config: &mut Configuration, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        let mut state = MenuState::Listening;
        let fb = match note.key
        {
            Note::C3 if channel <= Channel::C5 => menu_toggle_channel!(menu, channel, config.output.trig_enabled[channel as usize]),
            Note::D3 if channel <= Channel::C5 =>
            {
                config.output.triggers[channel as usize].set_source(TriggerSource::Arpeggio(Channel::All));
                Some(MenuFeedback::note_option(channel))
            },
            Note::Eb3 if channel <= Channel::C5 =>
            {
                config.output.triggers[channel as usize].set_source(TriggerSource::Note(Channel::All));
                Some(MenuFeedback::note_option(channel))
            },
            Note::E3 if channel <= Channel::C5 =>
            {
                config.output.triggers[channel as usize].set_source(TriggerSource::Sequence(Channel::All));
                Some(MenuFeedback::note_option(channel))
            },
            Note::F3 if channel <= Channel::C5 =>
            {
                config.output.triggers[channel as usize].set_source(TriggerSource::Track(Channel::All));
                Some(MenuFeedback::note_option(channel))
            },
            CHANNEL_KEY if channel <= Channel::C5 => {state = MenuState::number(2, 1..16, note.key, channel); None},
            PULSE_LENGTH_KEY => {state = MenuState::number(4, 1.., note.key, Channel::All); None},
            Note::C4 => menu_toggle!(menu, config.output.cc_enabled[0]),
            Note::D4 => menu_toggle!(menu, config.output.cc_enabled[1]),
            Note::E4 => menu_toggle!(menu, config.output.cc_enabled[2]),
            Note::F4 => menu_toggle!(menu, config.output.cc_enabled[3]),
            Note::G4 => menu_toggle!(menu, config.output.cc_enabled[4]),
            Note::A4 => menu_toggle!(menu, config.other.per_channel_cc),
            CC1_KEY if channel <= Channel::C5 => {state = MenuState::number(3, ..127, note.key, channel); None},
            CC2_KEY if channel <= Channel::C5 => {state = MenuState::number(3, ..127, note.key, channel); None},
            CC3_KEY if channel <= Channel::C5 => {state = MenuState::number(3, ..127, note.key, channel); None},
            CC4_KEY if channel <= Channel::C5 => {state = MenuState::number(3, ..127, note.key, channel); None},
            CC5_KEY if channel <= Channel::C5 => {state = MenuState::number(3, ..127, note.key, channel); None},
            _ => None
        };
        
        return (state, fb);
    }
    
    fn on_number_input(&mut self, config: &mut Configuration, value: Option<usize>, channel: Channel, key: u8)
    {
        match key
        {
            CHANNEL_KEY =>
            {
                let c = value_or_last!(value, self.channel_lv);
                config.output.triggers[channel as usize].set_channel(Channel::from_u8(c as u8));
            },
            PULSE_LENGTH_KEY => config.other.pulse_length = value_or_last!(value, self.pulse_length_lv),
            CC1_KEY => config.output.cc_sources[0] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC2_KEY => config.output.cc_sources[1] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC3_KEY => config.output.cc_sources[2] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC4_KEY => config.output.cc_sources[3] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC5_KEY => config.output.cc_sources[4] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
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