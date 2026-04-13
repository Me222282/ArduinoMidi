use core::f32::consts::PI;

use api::{Channel, Note};

use crate::{Configuration, MF_DURATION, Menu, MenuState, NOTEOPTION, menu_toggle, menu_toggle_channel};

pub struct VibratoMenu<'a>
{
    config: &'a mut Configuration
}

const SET_RATE_KEY: u8 = Note::F4;
const SET_MRATE_KEY: u8 = Note::G4;
const SET_SCALE_KEY: u8 = Note::A4;
const MINUS_ST_KEY: u8 = Note::G5;
const PLUS_ST_KEY: u8 = Note::A5;

const W_SET: f32 = PI * 0.002;
const MW_SET: f32 = PI * 0.000002;
const SCALE_SET: f32 = 1.0 / 16384.0;

impl<'a> Menu for VibratoMenu<'a>
{
    fn on_note(menu: &mut super::MenuWrapper<Self>, channel: Channel, note: Note) -> bool
    {
        let config = &mut menu.panel.configuration;
        
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
            SET_RATE_KEY => menu.set_state(MenuState::number(3, 1.., note.key, channel, true)),
            SET_MRATE_KEY => menu.set_state(MenuState::number(5, 1.., note.key, channel, true)),
            SET_SCALE_KEY => menu.set_state(MenuState::number(4, ..=2048, note.key, channel, true)),
            Note::C5 => menu_toggle!(menu, config.global_vibrato),
            MINUS_ST_KEY => menu.set_state(MenuState::number(2, ..=24, note.key, channel, true)),
            PLUS_ST_KEY => menu.set_state(MenuState::number(2, ..=24, note.key, channel, true)),
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
        
        return false;
    }
    
    fn on_number_input(&mut self, value: usize, channel: Channel, key: u8)
    {
        let ci = match self.config.global_vibrato
        {
            true => 0,
            false => channel as usize,
        };
        
        match key
        {
            SET_RATE_KEY => self.config.vibratos[ci].angular_velocity = value as f32 * W_SET,
            SET_MRATE_KEY => self.config.vibratos[ci].angular_velocity = value as f32 * MW_SET,
            SET_SCALE_KEY => self.config.vibratos[channel as usize].scale = value as f32 * SCALE_SET,
            MINUS_ST_KEY => self.config.channel_offsets[channel as usize].semi_tone = -(value as i8),
            PLUS_ST_KEY => self.config.channel_offsets[channel as usize].semi_tone = value as i8,
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