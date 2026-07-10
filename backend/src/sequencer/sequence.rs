use alloc::boxed::Box;
use api::{Cubic, CubicInput, Externals, Note};

use crate::{ChannelOutput, sequencer::{NOTE_HOLD, NOTE_OFF, TrackBank, TrackRef}};

pub(super) struct Sequence
{
    /// interpret u8 as +1 (so cannot represent zero)
    tracks: Box<[(TrackRef, u8); 256]>,
    cubic: Cubic,
    
    track_index: u16,
    /// number of completed repeats
    current_count: u16,
    track_step: u16,
    playing: bool,
    paused: bool,
    
    current_clock_div: u8,
    current_note: Note,
    next_note: Note,
    half_time: bool,
    
    /// decremented every time step - next step when it hits zero
    time_steps: u16,
    
    skip: u8,
    one_shot: bool
}

impl Sequence
{
    pub fn play(&mut self, one_shot: bool, bank: &TrackBank)
    {
        if self.playing { return; }
        if !self.paused
        {
            self.track_index = 0;
            self.current_count = 0;
            self.track_step = 0;
            self.time_steps = 0;
            
            // setup first notes
            let current = &self.tracks[0];
            let track = current.0.get_ref(bank);
            self.half_time = track.half_time;
            self.current_clock_div = track.clock_div;
            self.current_note = NOTE_OFF;
            self.next_note = track.get_step(0).unwrap_or((NOTE_OFF, 0)).0;
            
        }
        self.one_shot = one_shot;
        self.playing = true;
        self.paused = false;
    }
    pub fn pause(&mut self)
    {
        self.playing = false;
        self.paused = true;
    }
    pub fn stop(&mut self)
    {
        self.playing = false;
        self.paused = false;
    }
    
    pub fn inc_skip(&mut self)
    {
        self.skip += 1;
    }
    pub fn set_one_shot(&mut self)
    {
        self.one_shot = true;
    }
    
    pub fn on_time_step<E: Externals>(&mut self, mut output: ChannelOutput<E>, bank: &TrackBank)
    {
        // at this point trackstep represents the index of self.next_note
        
        if !self.playing { return; }
        
        self.time_steps -= 1;
        
        // continue waiting
        if self.time_steps > 1 { return; }
        // perform half time stuff
        if self.time_steps == 1
        {
            // only turn off early if next note is different and we are currently not resting
            if self.half_time && self.current_note != NOTE_OFF && self.next_note != NOTE_HOLD
            {
                output.remove_note(self.current_note);
            }
            // otherwise do nothing on half time
            return;
        }
        // self.time_steps == 0 here
        
        // reset counter - encodes the clock div for this step
        // allows clock_div to change
        self.time_steps = (self.current_clock_div as u16) << 1;
        self.track_step += 1;
        
        let current = &self.tracks[self.track_index as usize];
        let track = current.0.get_ref(bank);
        // get next trackstep - already have current
        let next = track.get_step(self.track_step);
        
        let old = self.current_note;
        self.current_note = self.next_note;
        self.next_note = match next
        {
            // TODO: cubic
            Some((note, _)) => note,
            None =>
            {
                self.track_step = 0;
                self.current_count += 1;
                // move to next track
                if self.current_count > current.1 as u16
                {
                    self.next_track(bank).0
                }
                // repeat current track
                else
                {
                    track.get_step(0).map(|x| x.0).unwrap_or(NOTE_OFF)
                }
            },
        };
        
        // swap outputs - in this order so that retrig is performed
        // do nothing if we are to hold
        if self.current_note != NOTE_HOLD
        {
            if self.current_note != NOTE_OFF
            {
                output.push_note(self.current_note);
            }
            if !self.half_time && old != NOTE_OFF
            {
                output.remove_note(old);
            }
        }
    }
    
    pub fn next_track(&mut self, bank: &TrackBank) -> (Note, u16)
    {
        self.track_index += 1;
        self.current_count = 0;
        let current = &self.tracks[self.track_index as usize];
        let track = current.0.get_ref(bank);
        self.current_clock_div = track.clock_div;
        self.half_time = track.half_time;
        return track.get_step(0).unwrap_or((NOTE_OFF, 0));
    }
}