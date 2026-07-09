use core::{f32::{self, consts::{PI, TAU}}, intrinsics};

use api::{Channel, Note};

use crate::{Configuration, FreqCorrection, Menu, MenuFeedback, MenuState, menu_toggle, menu_toggle_channel, value_or_last};

#[derive(Debug, Default)]
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

const GRAD1: f32 = 2.0 / PI;
const GRAD2: f32 = -GRAD1;
fn tri(t: f32) -> f32
{
    let t = t % TAU;
    if t >= PI
    {
        return 3.0 + (t * GRAD2);
    }
    return (t * GRAD1) - 1.0;
}
#[inline]
fn sin(t: f32) -> f32
{
    return intrinsics::sinf32(t);
}

impl Menu for VibratoMenu
{
    fn on_note(&mut self, config: &mut Configuration, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        let mut state = MenuState::Listening;
        let fb = match note.key
        {
            Note::C4 => menu_toggle_channel!(channel, config.vibrato.vibratos[channel as usize].enabled),
            Note::Db4 =>
            {
                config.vibrato.vibratos[channel as usize].freq_correction = FreqCorrection::None;
                Some(MenuFeedback::note_option(channel))
            },
            Note::D4 =>
            {
                config.vibrato.vibratos[channel as usize].function = sin;
                Some(MenuFeedback::note_option(channel))
            },
            Note::Eb4 =>
            {
                config.vibrato.vibratos[channel as usize].freq_correction = FreqCorrection::Half;
                Some(MenuFeedback::note_option(channel))
            },
            Note::E4 =>
            {
                config.vibrato.vibratos[channel as usize].function = tri;
                Some(MenuFeedback::note_option(channel))
            },
            SET_RATE_KEY => {state = MenuState::number(3, 1.., note.key, channel); None},
            Note::Gb4 =>
            {
                config.vibrato.vibratos[channel as usize].freq_correction = FreqCorrection::Full;
                Some(MenuFeedback::note_option(channel))
            },
            SET_MRATE_KEY => {state = MenuState::number(5, 1.., note.key, channel); None},
            SET_SCALE_KEY => {state = MenuState::number(4, ..=2048, note.key, channel); None},
            Note::C5 => menu_toggle!(config.vibrato.global_vibrato),
            MINUS_ST_KEY => {state = MenuState::number(2, ..=24, note.key, channel); None},
            PLUS_ST_KEY => {state = MenuState::number(2, ..=24, note.key, channel); None},
            Note::B5 =>
            {
                config.output.channel_offsets[channel as usize].octave = -3;
                Some(MenuFeedback::note_option(channel))
            },
            Note::C6 =>
            {
                config.output.channel_offsets[channel as usize].octave = -2;
                Some(MenuFeedback::note_option(channel))
            },
            Note::Db6 =>
            {
                config.output.channel_offsets[channel as usize].octave = -1;
                Some(MenuFeedback::note_option(channel))
            },
            Note::D6 =>
            {
                config.output.channel_offsets[channel as usize].octave = 0;
                Some(MenuFeedback::note_option(channel))
            },
            Note::Eb6 =>
            {
                config.output.channel_offsets[channel as usize].octave = 1;
                Some(MenuFeedback::note_option(channel))
            },
            Note::E6 =>
            {
                config.output.channel_offsets[channel as usize].octave = 2;
                Some(MenuFeedback::note_option(channel))
            },
            Note::F6 =>
            {
                config.output.channel_offsets[channel as usize].octave = 3;
                Some(MenuFeedback::note_option(channel))
            },
            _ => None
        };
        
        return (state, fb);
    }
    
    fn on_number_input(&mut self, config: &mut Configuration, value: Option<usize>, channel: Channel, key: u8)
    {
        let ci = match config.vibrato.global_vibrato
        {
            true => 0,
            false => channel as usize,
        };
        
        match key
        {
            SET_RATE_KEY => config.vibrato.vibratos[ci].angular_velocity = value_or_last!(value, self.rate_lv) as f32 * W_SET,
            SET_MRATE_KEY => config.vibrato.vibratos[ci].angular_velocity = value_or_last!(value, self.mrate_lv) as f32 * MW_SET,
            SET_SCALE_KEY => config.vibrato.vibratos[channel as usize].scale = value_or_last!(value, self.scale_lv) as f32 * SCALE_SET,
            MINUS_ST_KEY => config.output.channel_offsets[channel as usize].semi_tone = -(value_or_last!(value, self.st_lv) as i8),
            PLUS_ST_KEY => config.output.channel_offsets[channel as usize].semi_tone = value_or_last!(value, self.st_lv) as i8,
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