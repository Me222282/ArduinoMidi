use alloc::boxed::Box;
use api::{Channel, ChannelSelect, Note};

use crate::{Output, SequencerConfig, sequencer::{Sequence, TrackBank}};

// #[derive(Debug, Default)]
pub struct Sequencer
{
    pub(super) config: SequencerConfig,
    playing: bool,
    
    pub(super) play_mode: ChannelSelect,
    
    on_stop: bool,
    on_continue: bool,
    
    seq_play: [bool; 5],
    pub(super) sequences: [Box<Sequence>; 5],
    pub(super) bank: TrackBank
}

impl Sequencer
{
    pub fn set_time(&mut self, value: u32)
    {
        self.config.sequencer_tempo_time = value >> 1;
    }
    
    pub fn on_loop<E: api::Externals>(&mut self, output: &mut Output<E>, time: u32)
    {
        if self.on_stop
        {
            self.on_stop = false;
            self.playing = false;
            
            // stop sequencers
            for s in &mut self.sequences
            {
                s.stop(output.get_channel_only(s.channel));
            }
        }
        if self.on_continue
        {
            self.on_continue = false;
            self.playing = true;
            
            // continue sequencers
            for (s, enabled) in self.sequences.iter_mut().zip(self.seq_play)
            {
                if !enabled { continue; }
                s.r#continue(output.get_channel_only(s.channel), &self.bank);
            }
        }
        
        // stop disabled sequences
        for (s, enabled) in self.sequences.iter_mut().zip(self.seq_play)
        {
            if !enabled
            {
                s.stop(output.get_channel_only(s.channel));
            }
        }
        
        if !self.playing || self.config.clocked_sequencer { return; }
    }
    pub fn on_clock<E: api::Externals>(&mut self, output: &mut Output<E>)
    {
        if !self.playing || !self.config.clocked_sequencer { return; }
        
        
    }
    
    #[inline]
    pub fn is_playing(&self) -> bool
    {
        return self.playing | self.on_stop;
    }
    pub fn play(&mut self)
    {
        self.playing = true;
        
        // start sequencers
        for (s, enabled) in self.sequences.iter_mut().zip(self.seq_play)
        {
            if !enabled { continue; }
            s.play(&self.bank);
        }
    }
    pub fn stop(&mut self)
    {
        self.on_stop = true;
    }
    pub fn r#continue(&mut self)
    {
        self.on_stop = true;
    }
    
    pub fn play_stop_seq(&mut self, index: usize) -> bool
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
    pub fn one_shot_seq(&mut self, index: usize)
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
    pub fn reset_seq(&mut self, index: usize)
    {
        let seq = self.sequences[index].as_mut();
        seq.reset_skip();
        seq.reset(&self.bank);
    }
    pub fn get_channel(&self, index: usize) -> Channel
    {
        return self.sequences[index].channel;
    }
}