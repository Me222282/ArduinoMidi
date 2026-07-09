pub mod menu;
pub(crate) use self::menu::*;

use crate::{Output, SequencerConfig};

#[derive(Debug, Default)]
pub(crate) struct Sequencer
{
    config: SequencerConfig,
    playing: bool
}

impl Sequencer
{
    pub fn set_time(&mut self, value: u32)
    {
        self.config.sequencer_tempo_time = value >> 1;
    }
    
    pub fn on_note<E: api::Externals>(&self, output: &mut Output<E>)
    {
        
    }
    pub fn on_loop<E: api::Externals>(&mut self, output: &mut Output<E>, time: u32)
    {
        
    }
}