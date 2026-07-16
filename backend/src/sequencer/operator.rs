use alloc::boxed::Box;
use api::{Channel, ChannelSelect};

use crate::{Output, SequencerConfig, sequencer::{Sequence, TrackBank}};

// #[derive(Debug, Default)]
pub struct Sequencer
{
    pub config: SequencerConfig,
    playing: bool,
    
    pub(super) play_mode: ChannelSelect,
    
    on_stop: bool,
    on_continue: bool,
    playing_time: u32,
    last_time: u32,
    
    seq_play: [bool; 5],
    pub(super) sequences: [Box<Sequence>; 5],
    pub(super) bank: TrackBank
}

impl Sequencer
{
    pub(super) fn set_time(&mut self, value: u32)
    {
        // half so that half time notes are triggered
        self.config.sequencer_tempo_time = value >> 1;
    }
    
    pub fn on_loop<E: api::Externals>(&mut self, output: &mut Output<E>, time: u32)
    {
        self.on_update(output);
        
        // doesnt matter about last_time not being updated if not in the sequencer menu
        // as when first entered - no sequences will be playing straight the next on_loop
        let dt = time - self.last_time;
        self.last_time = time;
        
        if !self.playing || self.config.clocked_sequencer { return; }
        
        self.playing_time += dt;
        if self.playing_time >= self.config.sequencer_tempo_time
        {
            self.playing_time -= dt;
            self.on_time_step(output);
        }
        
        let sub = self.playing_time as f32 / self.config.sequencer_tempo_time as f32;
        self.on_sub_step(output, sub);
    }
    pub(super) fn on_clock<E: api::Externals>(&mut self, output: &mut Output<E>)
    {
        // always counting
        let acc = self.playing_time;
        // use playing time as step counter
        self.playing_time += 1;
        
        if !self.playing || !self.config.clocked_sequencer { return; }
        
        if acc % 3 == 0
        {
            self.on_time_step(output);
        }
        
        let sub = (acc % 3) as f32 / 3.0;
        self.on_sub_step(output, sub);
    }
    /// every loop - perform user inputs that need access to `output`
    fn on_update<E: api::Externals>(&mut self, output: &mut Output<E>)
    {
        if self.on_stop
        {
            self.on_stop = false;
            self.playing = false;
            
            // stop sequencers
            for (s, channel) in self.sequences.iter_mut().zip(self.config.sequence_channels)
            {
                s.stop(output.get_channel_only(channel));
            }
        }
        if self.on_continue
        {
            self.on_continue = false;
            self.playing = true;
            
            // continue sequencers
            for ((s, channel), enabled) in self.sequences.iter_mut().zip(self.config.sequence_channels).zip(self.seq_play)
            {
                if !enabled { continue; }
                s.r#continue(output.get_channel_only(channel), &self.bank);
            }
        }
        
        // stop disabled sequences
        for ((s, channel), enabled) in self.sequences.iter_mut().zip(self.config.sequence_channels).zip(self.seq_play)
        {
            if !enabled
            {
                s.stop(output.get_channel_only(channel));
            }
        }
    }
    fn on_time_step<E: api::Externals>(&mut self, output: &mut Output<E>)
    {
        for ((s, channel), enabled) in self.sequences.iter_mut().zip(self.config.sequence_channels).zip(self.seq_play)
        {
            if !enabled { continue; }
            s.on_time_step(output.get_channel_only(channel), &self.bank);
        }
    }
    /// `sub` is 0.0 - 1.0 between every `on_time_step`
    fn on_sub_step<E: api::Externals>(&mut self, output: &mut Output<E>, sub: f32)
    {
        for ((s, channel), enabled) in self.sequences.iter_mut().zip(self.config.sequence_channels).zip(self.seq_play)
        {
            if !enabled { continue; }
            s.on_sub_step(output.get_channel_only(channel), sub);
        }
    }
    
    #[inline]
    #[must_use]
    pub(super) fn is_playing(&self) -> bool
    {
        return self.playing | self.on_stop;
    }
    pub(super) fn play(&mut self)
    {
        self.playing = true;
        self.playing_time = 0;
        
        // start sequencers
        for (s, enabled) in self.sequences.iter_mut().zip(self.seq_play)
        {
            if !enabled { continue; }
            s.play(&self.bank);
        }
    }
    pub(super) fn stop(&mut self)
    {
        self.on_stop = true;
    }
    pub(super) fn r#continue(&mut self)
    {
        self.on_stop = true;
    }
    
    pub(super) fn play_stop_seq(&mut self, index: usize) -> bool
    {
        let seq = self.sequences[index].as_mut();
        seq.set_one_shot(false);
        
        if self.seq_play[index]
        {
            self.seq_play[index] = false;
            return false;
        }
        // self.seq_play[index] is false
        self.seq_play[index] = true;
        if self.playing
        {
            seq.play(&self.bank);
        }
        return true;
    }
    pub(super) fn one_shot_seq(&mut self, index: usize)
    {
        let seq = self.sequences[index].as_mut();
        seq.set_one_shot(true);
        
        self.seq_play[index] = true;
        if self.playing
        {
            // will be ignored if already playing
            seq.play(&self.bank);
        }
    }
    pub(super) fn reset_seq(&mut self, index: usize)
    {
        let seq = self.sequences[index].as_mut();
        seq.reset_skip();
        seq.reset(&self.bank);
    }
    #[must_use]
    pub(super) fn get_channel(&self, index: usize) -> Channel
    {
        return self.config.sequence_channels[index];
    }
    #[must_use]
    pub(super) fn get_sequence<'a>(&'a self, channel: Channel) -> Option<&'a Sequence>
    {
        for (s, c) in self.sequences.iter().zip(self.config.sequence_channels)
        {
            if c == channel
            {
                return Some(s);
            }
        }
        
        return None;
    }
}