use alloc::boxed::Box;
use api::Cubic;

use crate::TrackRef;

pub struct Sequence
{
    /// interpret u8 as +1 (so cannot be zero)
    tracks: Box<[(TrackRef, u8); 256]>,
    cubic: Cubic,
    track_index: u16,
    current_count: u16,
    track_step: u16,
    playing: bool
}