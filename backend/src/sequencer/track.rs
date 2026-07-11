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
    #[must_use]
    pub fn get_ref<'a>(&'a self, bank: &'a TrackBank) -> &'a TrackData
    {
        return match self
        {
            TrackRef::Bank(i) => &bank.tracks[*i as usize],
            TrackRef::Owned(track_data) => &track_data,
        };
    }
    #[must_use]
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

pub enum AddStepResult
{
    Ok,
    End,
    Error
}

/// Do not stack alloc
#[derive(Debug)]
pub(super) struct TrackData
{
    steps: [(Note, u16); 256],
    /// interpret as +1 (so cannot represent zero)
    size: u8,
    /// clocl_div == 0 => empty track
    clock_div: u8,
    pub use_mod: bool,
    pub half_time: bool
}
impl TrackData
{
    #[inline]
    #[must_use]
    pub fn empty() -> Box<Self>
    {
        // all values can safely be zeros
        return unsafe { Box::new_zeroed().assume_init() };
    }
    
    /// bool determines whether note is the last one
    #[must_use]
    pub fn get_step(&self, step: u16) -> Option<(Note, u16)>
    {
        if step > self.size as u16
        {
            return None;
        }
        let value = self.steps[step as usize];
        return Some((value.0, value.1));
    }
    #[inline]
    #[must_use]
    pub fn get_last_step(&self) -> (Note, u16)
    {
        return self.steps[self.size as usize];
    }
    #[inline]
    #[must_use]
    pub fn is_empty(&self) -> bool
    {
        return self.clock_div == 0;
    }
    #[inline]
    #[must_use]
    pub fn get_clock_div(&self) -> u8
    {
        return self.clock_div;
    }
    
    pub fn add_step(&mut self, value: (Note, u16)) -> bool
    {
        if !self.is_empty() { return false; }
        
        self.steps[self.size as usize] = value;
        // reached maximum size
        if self.size == 0xFF
        {
            self.clock_div = 1;
        }
        self.size = self.size.wrapping_add(1);
        return true;
    }
    pub fn finalise(&mut self, clock_div: u8) -> bool
    {
        // no data was added
        if self.size == 0 && self.clock_div == 0 { return false; }
        
        self.clock_div = clock_div;
        self.size = self.size.wrapping_sub(1);
        return true;
    }
}
