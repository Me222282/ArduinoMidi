mod vibrato;
pub(crate) use vibrato::*;
mod arpeggio;
pub(crate) use arpeggio::*;
mod output;
pub use output::*;

use api::{Channel, Note};

use crate::{ChannelRedirect, OutputConfig};

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