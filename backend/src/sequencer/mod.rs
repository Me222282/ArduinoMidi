pub mod menu;
pub(crate) use self::menu::*;

use crate::SequencerConfig;

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
}