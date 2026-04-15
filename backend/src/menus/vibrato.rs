use core::f32::consts::PI;

use api::{Channel, Note};

use crate::{Panel, MF_DURATION, Menu, MenuState, NOTEOPTION, menu_toggle, menu_toggle_channel, value_or_last};

pub struct VibratoMenu
{
    rate_lv: usize,
    mrate_lv: usize,
    scale_lv: usize,
    st_lv: usize,
}

const SET_RATE_KEY: u8 = Note::F4;
const SET_MRATE_KEY: u8 = Note::G4;
const SET_SCALE_KEY: u8 = Note::A4;
const MINUS_ST_KEY: u8 = Note::G5;
const PLUS_ST_KEY: u8 = Note::A5;

const W_SET: f32 = PI * 0.002;
const MW_SET: f32 = PI * 0.000002;
const SCALE_SET: f32 = 1.0 / 16384.0;

impl Menu for VibratoMenu
{
    fn on_note(mut menu: super::MenuInst<Self>, channel: Channel, note: Note) -> MenuState
    {
        let config = &mut menu.panel.configuration;
        
        let mut state = MenuState::Listening;
        match note.key
        {
            Note::C4 => menu_toggle_channel!(menu, channel, config.vibratos[channel as usize].enabled),
            Note::D4 =>
            {
                // config.vibratos[channel as usize].function = ;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::E4 =>
            {
                // config.vibratos[channel as usize].function = ;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            SET_RATE_KEY => state = MenuState::number(3, 1.., note.key, channel),
            SET_MRATE_KEY => state = MenuState::number(5, 1.., note.key, channel),
            SET_SCALE_KEY => state = MenuState::number(4, ..=2048, note.key, channel),
            Note::C5 => menu_toggle!(menu, config.global_vibrato),
            MINUS_ST_KEY => state = MenuState::number(2, ..=24, note.key, channel),
            PLUS_ST_KEY => state = MenuState::number(2, ..=24, note.key, channel),
            Note::B5 =>
            {
                config.channel_offsets[channel as usize].octave = -3;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::C6 =>
            {
                config.channel_offsets[channel as usize].octave = -2;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::Db6 =>
            {
                config.channel_offsets[channel as usize].octave = -1;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::D6 =>
            {
                config.channel_offsets[channel as usize].octave = 0;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::Eb6 =>
            {
                config.channel_offsets[channel as usize].octave = 1;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::E6 =>
            {
                config.channel_offsets[channel as usize].octave = 2;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            Note::F6 =>
            {
                config.channel_offsets[channel as usize].octave = 3;
                menu.play_note(NOTEOPTION, MF_DURATION, channel);
            },
            _ => {}
        }
        
        return state;
    }
    
    fn on_number_input(&mut self, panel: &mut Panel, value: Option<usize>, channel: Channel, key: u8)
    {
        let ci = match panel.configuration.global_vibrato
        {
            true => 0,
            false => channel as usize,
        };
        
        match key
        {
            SET_RATE_KEY => panel.configuration.vibratos[ci].angular_velocity = value_or_last!(value, self.rate_lv) as f32 * W_SET,
            SET_MRATE_KEY => panel.configuration.vibratos[ci].angular_velocity = value_or_last!(value, self.mrate_lv) as f32 * MW_SET,
            SET_SCALE_KEY => panel.configuration.vibratos[channel as usize].scale = value_or_last!(value, self.scale_lv) as f32 * SCALE_SET,
            MINUS_ST_KEY => panel.configuration.channel_offsets[channel as usize].semi_tone = -(value_or_last!(value, self.st_lv) as i8),
            PLUS_ST_KEY => panel.configuration.channel_offsets[channel as usize].semi_tone = value_or_last!(value, self.st_lv) as i8,
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