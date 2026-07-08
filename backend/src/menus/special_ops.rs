use api::{Channel, Note, NoteKey};

use crate::{ArpeggioMode, Configuration, Menu, MenuFeedback, MenuState, menu_toggle, menu_toggle_channel, value_or_last};

#[derive(Debug, Default)]
pub struct SpecialOpsMenu
{
    tempo_lv: usize
}

const SET_TEMPO_KEY: u8 = Note::C3;
const TAP_TEMPO_KEY: u8 = Note::Db3;
const FILTER_SELECT_KEY: u8 = Note::Eb4;

impl Menu for SpecialOpsMenu
{
    fn on_note(&mut self, config: &mut Configuration, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        // TODO: factory reset key
        
        let mut state = MenuState::Listening;
        let fb = match note.key
        {
            SET_TEMPO_KEY => {state = MenuState::number(4, 10.., note.key, channel); None},
            TAP_TEMPO_KEY => {state = MenuState::TapTime { key: TAP_TEMPO_KEY, channel }; None},
            Note::D3 =>
            {
                config.arpeggio.arpeggios[channel as usize].mode = ArpeggioMode::Ascending;
                Some(MenuFeedback::note_option(channel))
            },
            Note::Eb3 => menu_toggle!(menu, config.arpeggio.clocked_arpeggios),
            Note::E3 =>
            {
                config.arpeggio.arpeggios[channel as usize].mode = ArpeggioMode::Decending;
                Some(MenuFeedback::note_option(channel))
            },
            Note::F3 =>
            {
                config.arpeggio.arpeggios[channel as usize].mode = ArpeggioMode::Alternating;
                Some(MenuFeedback::note_option(channel))
            },
            Note::G3 => menu_toggle_channel!(menu, channel, config.arpeggio.arpeggios[channel as usize].sort_notes),
            Note::A3 => menu_toggle_channel!(menu, channel, config.arpeggio.arpeggios[channel as usize].half_notes),
            
            Note::C4 => menu_toggle!(menu, config.output.retrigger_old),
            Note::Db4 => menu_toggle_channel!(menu, channel, config.output.channel_filters[channel as usize].filter_keys),
            Note::D4 => menu_toggle!(menu, config.output.retrigger_new),
            FILTER_SELECT_KEY => {state = MenuState::KeySelect { key: FILTER_SELECT_KEY, channel }; None},
            Note::E4 => menu_toggle!(menu, config.output.always_delay),
            Note::F4 => menu_toggle!(menu, config.panel.micro_tone),
            Note::Gb4 => menu_toggle!(menu, config.note.forget_notes),
            Note::G4 => menu_toggle_channel!(menu, channel, config.arpeggio.arpeggios[channel as usize].enabled),
            Note::Ab4 => menu_toggle!(menu, config.note.duplicate_release),
            Note::A4 => menu_toggle!(menu, config.note.sort_notes),
            Note::C5 => menu_toggle!(menu, config.output.all_channel_mode),
            Note::Db5 => menu_toggle!(menu, config.other.alternate_allocations),
            Note::D5 => menu_toggle!(menu, config.output.menu_feedback),
            _ => None
        };
        
        return (state, fb);
    }
    
    fn on_number_input(&mut self, config: &mut Configuration, value: Option<usize>, channel: Channel, key: u8)
    {
        match key
        {
            SET_TEMPO_KEY => config.arpeggio.arpeggios[channel as usize].time = value_or_last!(value, self.tempo_lv) as u32,
            _ => {}
        }
    }
    fn on_tap_time(&mut self, config: &mut Configuration, value: u32, channel: Channel, key: u8)
    {
        match key
        {
            TAP_TEMPO_KEY => config.arpeggio.arpeggios[channel as usize].time = value,
            _ => {}
        }
    }
    fn on_key_select(&mut self, config: &mut Configuration, value: NoteKey, channel: Channel, key: u8)
    {
        match key
        {
            FILTER_SELECT_KEY => config.output.channel_filters[channel as usize].note_filter = value,
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