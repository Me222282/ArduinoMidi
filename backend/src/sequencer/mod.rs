pub mod menu;
pub use self::menu::*;

use crate::SequencerConfig;

#[derive(Debug, Default)]
pub(self) struct Sequencer
{
    config: SequencerConfig
}