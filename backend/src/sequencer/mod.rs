pub mod menu;
pub use crate::menu::*;

use crate::SequencerConfig;

#[derive(Debug, Default)]
pub(crate) struct Sequencer
{
    config: SequencerConfig
}