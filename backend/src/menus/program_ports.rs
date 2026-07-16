use api::{CCType, Channel, Note};

use crate::{Configuration, Menu, MenuFeedback, MenuState, TriggerSource, menu_toggle, menu_toggle_channel, value_or_last};

#[derive(Debug, Default)]
pub struct ProgramPortsMenu
{
    pulse_length_lv: usize,
    cc_source_lv: usize,
    trig_set_index: u8,
}

const PULSE_LENGTH_KEY: u8 = Note::A3;
const CC1_KEY: u8 = Note::C5;
const CC2_KEY: u8 = Note::D5;
const CC3_KEY: u8 = Note::E5;
const CC4_KEY: u8 = Note::F5;
const CC5_KEY: u8 = Note::G5;

const TRIG_SRC_ARP: u8 = Note::D3;
const TRIG_SRC_NOTE: u8 = Note::Eb3;
const TRIG_SRC_SEQ: u8 = Note::E3;
const TRIG_SRC_TRACK: u8 = Note::F3;

impl Menu for ProgramPortsMenu
{
    type State = ();
    
    fn on_note(&mut self, config: &mut Configuration, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        let mut state = MenuState::Listening;
        let fb = match note.key
        {
            Note::C3 if channel <= Channel::C5 => menu_toggle_channel!(channel, config.panel.trig_enabled[channel as usize]),
            Note::Db3 if channel <= Channel::C5 =>
            {
                config.panel.triggers[channel as usize] = TriggerSource::MidiClock;
                Some(MenuFeedback::note_option(channel))
            },
            TRIG_SRC_ARP if channel <= Channel::C5 =>
            {
                // config.panel.triggers[channel as usize].set_source(TriggerSource::Arpeggio(Channel::All));
                self.trig_set_index = channel as u8;
                state = MenuState::number(2, 0..=16, note.key, channel);
                None
            },
            TRIG_SRC_NOTE if channel <= Channel::C5 =>
            {
                // config.panel.triggers[channel as usize].set_source(TriggerSource::Note(Channel::All));
                self.trig_set_index = channel as u8;
                state = MenuState::number(2, 0..=16, note.key, channel);
                None
            },
            TRIG_SRC_SEQ if channel <= Channel::C5 =>
            {
                // config.panel.triggers[channel as usize].set_source(TriggerSource::Sequence(Channel::All));
                self.trig_set_index = channel as u8;
                state = MenuState::number(2, 0..=16, note.key, channel);
                None
            },
            TRIG_SRC_TRACK if channel <= Channel::C5 =>
            {
                // config.panel.triggers[channel as usize].set_source(TriggerSource::Track(Channel::All));
                self.trig_set_index = channel as u8;
                state = MenuState::number(2, 0..=16, note.key, channel);
                None
            },
            Note::G3 if channel <= Channel::C5 =>
            {
                config.panel.triggers[channel as usize] = TriggerSource::SequencerBeat;
                Some(MenuFeedback::note_option(channel))
            },
            PULSE_LENGTH_KEY => {state = MenuState::number(4, 1.., note.key, Channel::All); None},
            Note::C4 => menu_toggle!(config.panel.cc_enabled[0]),
            Note::D4 => menu_toggle!(config.panel.cc_enabled[1]),
            Note::E4 => menu_toggle!(config.panel.cc_enabled[2]),
            Note::F4 => menu_toggle!(config.panel.cc_enabled[3]),
            Note::G4 => menu_toggle!(config.panel.cc_enabled[4]),
            Note::A4 => menu_toggle!(config.panel.per_channel_cc),
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
            // PULSE_LENGTH_KEY => config.other.pulse_length = value_or_last!(value, self.pulse_length_lv),
            CC1_KEY => config.panel.cc_sources[0] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC2_KEY => config.panel.cc_sources[1] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC3_KEY => config.panel.cc_sources[2] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC4_KEY => config.panel.cc_sources[3] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            CC5_KEY => config.panel.cc_sources[4] = (CCType::u8(value_or_last!(value, self.cc_source_lv) as u8), channel),
            _ => {}
        }
    }
    fn on_slot_select(&mut self, config: &mut Configuration, value: u8, _channel: Channel, key: u8)
    {
        match key
        {
            TRIG_SRC_ARP => config.panel.triggers[self.trig_set_index as usize].set_source(TriggerSource::Arpeggio(Channel::from_u8(value.wrapping_sub(1)))),
            TRIG_SRC_NOTE => config.panel.triggers[self.trig_set_index as usize].set_source(TriggerSource::Note(Channel::from_u8(value.wrapping_sub(1)))),
            TRIG_SRC_SEQ => config.panel.triggers[self.trig_set_index as usize].set_source(TriggerSource::Sequence(Channel::from_u8(value.wrapping_sub(1)))),
            TRIG_SRC_TRACK => config.panel.triggers[self.trig_set_index as usize].set_source(TriggerSource::Track(Channel::from_u8(value.wrapping_sub(1)))),
            _ => {}
        }
    }
    
    fn reset_values(&mut self, config: &mut Configuration)
    {
        todo!()
    }
    
    fn save_values<T: api::NvsInterface>(&self, config: &Configuration, nvs: &mut T)
    {
        todo!()
    }
    
    fn load_values<T: api::NvsInterface>(&mut self, config: &mut Configuration, nvs: &mut T)
    {
        todo!()
    }
}