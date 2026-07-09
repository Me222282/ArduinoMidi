use api::{Channel, Note};

use crate::{Configuration, Menu, MenuFeedback, MenuState, Sequencer};

#[derive(Debug, Default)]
pub struct SequencerMenu
{
    sequencer: Sequencer
}

impl Menu for SequencerMenu
{
    fn on_note(&mut self, config: &mut Configuration, channel: Channel, note: Note) -> (MenuState, Option<MenuFeedback>)
    {
        todo!()
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