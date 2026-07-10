mod menu;
pub use menu::*;
mod track;
pub use track::*;
mod sequence;
pub use sequence::*;

use crate::{Output, SequencerConfig};

/// every half beat
#[derive(Debug, PartialEq, Eq, Clone, Copy)]
struct TimeStep(usize);
impl TimeStep
{
    pub fn inc(&mut self)
    {
        self.0 += 1;
    }
    pub fn reset(&mut self)
    {
        self.0 = 0;
    }
    pub fn is_full(self) -> bool
    {
        // equivalent to % 2 == 0
        return self.0 & 1 == 0;
    }
    pub fn is_half_time(self) -> bool
    {
        // equivalent to % 2 == 1
        return self.0 & 1 == 1;
    }
    pub fn is_next_note(self, offset: TimeStep, clock_div: u8) -> bool
    {
        // plus 1 allows ht to occur the step before the next clockdiv note trigger
        // so will be true for both the ht before, and the actual next step
        return ((self.0 - offset.0 + 1) >> 1) % clock_div as usize == 0;
    }
}

#[derive(Debug, Default)]
pub struct Sequencer
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