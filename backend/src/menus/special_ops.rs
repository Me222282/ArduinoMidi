use api::{Channel, Note, NoteKey};

use crate::{ArpeggioMode, Panel, MF_DURATION, Menu, MenuState, NOTEOPTION, menu_toggle, menu_toggle_channel, value_or_last};

pub struct SpecialOpsMenu
{
    tempo_lv: usize
}

const SET_TEMPO_KEY: u8 = Note::C3;
const TAP_TEMPO_KEY: u8 = Note::Db3;
const FILTER_SELECT_KEY: u8 = Note::Eb4;

impl Menu for SpecialOpsMenu
{
    fn on_note(mut menu: super::MenuInst<Self>, channel: Channel, note: Note) -> MenuState
    {
        let config = &mut menu.panel.configuration;
        
        // TODO: factory reset key
        
        let mut state = MenuState::Listening;
        match note.key
        {
            SET_TEMPO_KEY => state = MenuState::number(4, 10.., note.key, channel),
            TAP_TEMPO_KEY => state = MenuState::TapTime { key: note.key, channel },
            Note::D3 =>
            {
                config.arpeggios[channel as usize].mode = ArpeggioMode::Ascending;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::Eb3 => menu_toggle!(menu, config.clocked_arpeggios),
            Note::E3 =>
            {
                config.arpeggios[channel as usize].mode = ArpeggioMode::Decending;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::F3 =>
            {
                config.arpeggios[channel as usize].mode = ArpeggioMode::Alternating;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::G3 => menu_toggle_channel!(menu, channel, config.arpeggios[channel as usize].sort_notes),
            Note::A3 => menu_toggle_channel!(menu, channel, config.arpeggios[channel as usize].half_notes),
            
            Note::C4 => menu_toggle!(menu, config.retrigger_old),
            Note::Db4 => menu_toggle_channel!(menu, channel, config.channel_filters[channel as usize].filter_keys),
            Note::D4 => menu_toggle!(menu, config.retrigger_new),
            FILTER_SELECT_KEY => state = MenuState::KeySelect { key: note.key, channel },
            Note::E4 => menu_toggle!(menu, config.always_delay),
            Note::F4 => menu_toggle!(menu, config.micro_tone),
            Note::Gb4 => menu_toggle!(menu, config.forget_notes),
            Note::G4 => menu_toggle_channel!(menu, channel, config.arpeggios[channel as usize].enabled),
            Note::Ab4 => menu_toggle!(menu, config.duplicate_release),
            Note::A4 => menu_toggle!(menu, config.sort_notes),
            Note::C5 => menu_toggle!(menu, config.all_channel_mode),
            Note::Db5 => menu_toggle!(menu, config.alternate_allocations),
            Note::D5 => menu_toggle!(menu, config.menu_feedback),
            _ => {}
        }
        
        return state;
    }
    
    fn on_number_input(&mut self, panel: &mut Panel, value: Option<usize>, channel: Channel, key: u8)
    {
        match key
        {
            SET_TEMPO_KEY => panel.configuration.arpeggios[channel as usize].time = value_or_last!(value, self.tempo_lv),
            _ => {}
        }
    }
    fn on_tap_time(&mut self, panel: &mut Panel, value: usize, channel: Channel, key: u8)
    {
        match key
        {
            TAP_TEMPO_KEY => panel.configuration.arpeggios[channel as usize].time = value,
            _ => {}
        }
    }
    fn on_key_select(&mut self, panel: &mut Panel, value: NoteKey, channel: Channel, key: u8)
    {
        match key
        {
            FILTER_SELECT_KEY => panel.configuration.channel_filters[channel as usize].note_filter = value,
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