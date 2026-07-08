use api::{Channel, Note};

use crate::{ArpeggioConfig, Output};

pub struct Arpeggiator
{
    pub config: ArpeggioConfig
}

impl Arpeggiator
{
    pub fn on_note(&mut self, output: &mut Output, channel: Channel, note: Note)
    {
        output.push_note(channel, note);
    }
    
    pub fn off_note(&mut self, output: &mut Output, channel: Channel, note: Note)
    {
        output.remove_note(channel, note);
    }
    
    pub fn on_loop(&mut self, time: u32, output: &mut Output)
    {
        
    }
}