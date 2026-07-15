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
    
    pub(super) sequences: [Box<Sequence>; 16],
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
            
            // continue sequencers
            for (i, s) in self.sequences.iter_mut().enumerate()
            {
                s.stop(output.get_channel_index_only(i));
            }
        }
        if self.on_continue
        {
            self.on_continue = false;
            self.playing = true;
            
            // continue sequencers
            for (i, s) in self.sequences.iter_mut().enumerate()
            {
                s.r#continue(output.get_channel_index_only(i), &self.bank);
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
        for s in &mut self.sequences
        {
            s.play(false, &self.bank);
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
}