mod vibrato;
pub(crate) use vibrato::*;
mod arpeggio;
pub(crate) use arpeggio::*;
mod output;
pub use output::*;

use api::{Channel, Externals, Note};

use crate::{ChannelRedirect, OutputConfig, SlotSelect};

pub(crate) fn process_note(mut channel: Channel, mut note: Note, config: &OutputConfig) -> Option<(Channel, Note)>
{
    // Channel Redirect
    let filter = &config.channel_filters[channel as usize];
    channel = apply_redirect(&filter.redirect1, channel, note);
    channel = apply_redirect(&filter.redirect2, channel, note);
    
    // Channel Offset
    let offset = &config.channel_offsets[channel as usize];
    note.key = (note.key as i8 + offset.semi_tone + (offset.octave * 12)) as u8;
    
    // KeyNote Filter
    let filter = &config.channel_filters[channel as usize];
    if filter.filter_keys && !filter.note_filter.contains_note(note.key)
    {
        return None;
    }
    return Some((channel, note));
}
fn apply_redirect(cr: &ChannelRedirect, channel: Channel, note: Note) -> Channel
{
    if !cr.enabled ||
        note.key < cr.start || note.key > cr.end
    {
        return channel;
    }
    
    return cr.new_channel;
}

pub(crate) struct ChannelOutput<'a, E: Externals>
{
    output: &'a mut Output<E>,
    channel: Channel
}
impl<'a, E: Externals> ChannelOutput<'a, E>
{
    pub fn new(output: &'a mut Output<E>, channel: Channel) -> Self
    {
        return Self { output, channel };
    }
    
    #[inline]
    pub fn set_modulation(&mut self, value: u16)
    {
        self.output.set_modulation(self.channel, value);
    }
    #[inline]
    pub fn push_note(&mut self, note: Note)
    {
        self.output.push_note(self.channel, note);
    }
    #[inline]
    pub(in crate::operators) fn push_note_post(&mut self, note: Note)
    {
        self.output.push_note_post(self.channel, note);
    }
    #[inline]
    pub fn remove_note(&mut self, note: Note)
    {
        self.output.remove_note(self.channel, note);
    }
    #[inline]
    pub(in crate::operators) fn remove_note_post(&mut self, note: Note)
    {
        self.output.remove_note_post(self.channel, note);
    }
}
