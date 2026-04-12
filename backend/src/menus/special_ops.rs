use api::{Channel, Note};

use crate::{ArpeggioMode, Configuration, MF_DURATION, Menu, MenuState, NOTEOPTION, menu_toggle, menu_toggle_channel};

enum State
{
    SetTempo,
    TapTempo,
}
pub struct SpecialOpsMenu<'a>
{
    state: State,
    config: &'a mut Configuration
}

const SET_TEMPO_KEY: u8 = Note::C3;

impl<'a> Menu for SpecialOpsMenu<'a>
{
    fn on_note(menu: &mut super::MenuWrapper<Self>, channel: Channel, note: Note) -> bool
    {
        let config = &mut menu.panel.configuration;
        
        match note.key
        {
            SET_TEMPO_KEY =>
            {
                menu.set_state(MenuState::Number { digits: 4, min: 10, max: usize::max_value(), key: note.key, channel, use_last: true });
                menu.menu.state = State::SetTempo;
            },
            Note::Db3 =>
            {
                menu.set_state(MenuState::TapTime { key: note.key, channel });
                menu.menu.state = State::TapTempo;
            },
            Note::D3 =>
            {
                config.arpeggios[channel as usize].mode = ArpeggioMode::Ascending;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
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
            Note::D4 => menu_toggle!(menu, config.retrigger_new),
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
        
        return false;
    }
    
    fn on_number_input(&mut self, value: usize, channel: Channel, key: u8)
    {
        match key
        {
            SET_TEMPO_KEY =>
            {
                self.config.arpeggios[channel as usize].time = value;
            },
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