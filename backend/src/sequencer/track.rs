use alloc::boxed::Box;
use api::Note;

pub const NOTE_OFF: Note = Note::new(0xFF, 0);
pub const NOTE_HOLD: Note = Note::new(0xFF, 0xFF);

#[derive(Debug)]
pub struct TrackBank
{
    tracks: Box<[TrackData; 32]>
}

pub enum TrackRef
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
pub struct TrackData
{
    steps: [(Note, u16); 256],
    /// interpret as +1 (so cannot be zero)
    size: u8,
    clock_div: u8,
    use_mod: bool,
    half_time: bool
}
impl TrackData
{
    pub fn on_time_step()
    {
        
    }
}
