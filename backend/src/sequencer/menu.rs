use api::{Channel, Note};

use crate::{Configuration, Menu, MenuFeedback, MenuState, menu_toggle, sequencer::Sequencer, value_or_last};

#[derive(Debug, Default)]
pub(crate) struct SequencerMenu
{
    pub sequencer: Sequencer,
    bar_size_lv: usize,
    seq_time_lv: usize,
}

const SET_BAR_SIZE: u8 = Note::B2;
const SET_SEQ_TIME: u8 = Note::C3;
const TAP_TEMPO: u8 = Note::Db3;

impl Menu for SequencerMenu
{
    fn on_note(&mut self, _config: &mut Configuration, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        let mut state = MenuState::Listening;
        let fb = match note.key
        {
            Note::A2 => menu_toggle!(self.sequencer.config.on_bar_trigger),
            SET_BAR_SIZE => {state = MenuState::number(3, 1..=127, note.key, Channel::All); None},
            SET_SEQ_TIME => {state = MenuState::number(3, 10.., note.key, Channel::All); None},
            TAP_TEMPO => {state = MenuState::TapTime { key: note.key, channel: Channel::All }; None},
            Note::Eb3 => menu_toggle!(self.sequencer.config.clocked_sequencer),
            // Note::C4 => menu_toggle_channel!(menu, channel, config.vibrato.vibratos[channel as usize].enabled),
            Note::Bb3 => return (MenuState::Exit, None),
            _ => None
        };
        
        return (state, fb);
    }
    
    fn on_number_input(&mut self, _config: &mut Configuration, value: Option<usize>, _channel: Channel, key: u8)
    {
        match key
        {
            SET_BAR_SIZE => self.sequencer.config.bar_size = (value_or_last!(value, self.bar_size_lv) << 1) as u8,
            SET_SEQ_TIME => self.sequencer.set_time(60000 / value_or_last!(value, self.seq_time_lv) as u32),
            _ => {}
        }
    }
    
    fn on_tap_time(&mut self, _config: &mut Configuration, value: u32, _channel: Channel, key: u8)
    {
        match key
        {
            TAP_TEMPO => self.sequencer.set_time(value),
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