use api::{Channel, ChannelSelect, Note};

use crate::{Configuration, Menu, MenuFeedback, menu_toggle, sequencer::Sequencer, value_or_last};

type MenuState = crate::MenuState<SeqMenuState>;

// #[derive(Debug, Default)]
pub struct SequencerMenu
{
    pub sequencer: Sequencer,
    bar_size_lv: usize,
    seq_time_lv: usize,
    return_state: MenuState
}

const SET_BAR_SIZE: u8 = Note::B2;
const SET_SEQ_TIME: u8 = Note::C3;
const TAP_TEMPO: u8 = Note::Db3;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SeqMenuState
{
    
}

impl SequencerMenu
{
    /// menu options that can be used whilst playing
    fn on_playing_note(&mut self, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        let mut state = MenuState::Listening;
        let fb = match note.key
        {
            Note::G2 =>
            {
                self.sequencer.play_mode -= ChannelSelect::All;
                None
            }
            Note::A2 => menu_toggle!(self.sequencer.config.on_bar_trigger),
            SET_BAR_SIZE => {state = MenuState::number(3, 1..=127, note.key, Channel::All); None},
            SET_SEQ_TIME => {state = MenuState::number(3, 10.., note.key, Channel::All); None},
            TAP_TEMPO => {state = MenuState::TapTime { key: note.key, channel: Channel::All }; None},
            Note::D3 => {
                self.sequencer.play_mode += channel.into();
                None
            },
            Note::C4 =>
            {
                self.sequencer.play();
                None
            },
            Note::D4 =>
            {
                self.sequencer.r#continue();
                None
            },
            Note::E4 =>
            {
                self.sequencer.stop();
                None
            },
            _ => None
        };
        
        return (state, fb);
    }
}
impl Menu for SequencerMenu
{
    type State = SeqMenuState;
    
    fn on_reset_switch(&mut self, current: MenuState) -> MenuState
    {
        // exit any play modes
        self.sequencer.play_mode -= ChannelSelect::All;
        self.sequencer.stop();
        
        if current == self.return_state
        {
            self.return_state = MenuState::Listening;
            return MenuState::Listening;
        }
        
        return self.return_state;
    }
    #[inline]
    fn menu_feedback(&self) -> bool { return self.sequencer.is_playing() }
    #[inline]
    fn return_state(&self) -> MenuState { return self.return_state; }
    #[inline]
    fn note_filter(&self, channel: Channel, _note: Note) -> bool
    {
        return !self.sequencer.play_mode.has_channel(channel);
    }
    
    #[inline]
    fn on_note(&mut self, _config: &mut Configuration, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        // playing menu only
        if self.sequencer.is_playing()
        {
            return self.on_playing_note(channel, note);
        }
        
        let mut state = MenuState::Listening;
        let fb = match note.key
        {
            Note::Eb3 => menu_toggle!(self.sequencer.config.clocked_sequencer),
            Note::Bb3 => return (MenuState::Exit, None),
            // do the rest of the menu functions
            _ =>
            {
                let res;
                (state, res) = self.on_playing_note(channel, note);
                res
            }
        };
        
        return (state, fb);
    }
    fn on_custom_state(&mut self, state: SeqMenuState, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        return (MenuState::Custom(state), None);
    }
    fn on_message<E: api::Externals>(&mut self, output: &mut crate::Output<E>, message: api::MidiCode)
    {
        match message
        {
            api::MidiCode::TimingClock =>
            {
                self.sequencer.on_clock(output);
            },
            _ => {}
        }
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