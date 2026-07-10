use alloc::boxed::Box;
use api::Note;

pub(super) const NOTE_OFF: Note = Note::new(0xFF, 0);
pub(super) const NOTE_HOLD: Note = Note::new(0xFF, 0xFF);

#[derive(Debug)]
pub(super) struct TrackBank
{
    tracks: [Box<TrackData>; 32]
}

pub(super) enum TrackRef
{
    Bank(u8),
    Owned(Box<TrackData>)
}
impl TrackRef
{
    pub fn get_ref<'a>(&'a self, bank: &'a TrackBank) -> &'a TrackData
    {
        return match self
        {
            TrackRef::Bank(i) => &bank.tracks[*i as usize],
            TrackRef::Owned(track_data) => &track_data,
        };
    }
    pub fn get_mut<'a>(&'a mut self, bank: &'a mut TrackBank) -> &'a mut TrackData
    {
        return match self
        {
            TrackRef::Bank(i) => &mut bank.tracks[*i as usize],
            TrackRef::Owned(track_data) => track_data.as_mut(),
        };
    }
}
impl Default for TrackRef
{
    fn default() -> Self
    {
        return Self::Bank(0);
    }
}

/// Do not stack alloc
#[derive(Debug)]
pub(super) struct TrackData
{
    steps: [(Note, u16); 256],
    /// interpret as +1 (so cannot represent zero)
    size: u8,
    /// clocl_div == 0 => empty track
    pub clock_div: u8,
    pub use_mod: bool,
    pub half_time: bool
}
impl TrackData
{
    pub fn empty() -> Box<Self>
    {
        // all values can safely be zeros
        return unsafe { Box::new_zeroed().assume_init() };
    }
    
    /// bool determines whether note is the last one
    pub fn get_step(&self, step: u16) -> Option<(Note, u16)>
    {
        if step > self.size as u16
        {
            return None;
        }
        let value = self.steps[step as usize];
        return Some((value.0, value.1));
    }
    pub fn is_empty(&self) -> bool
    {
        return self.clock_div == 0;
    }
}
