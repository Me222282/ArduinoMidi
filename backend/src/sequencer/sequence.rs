use alloc::boxed::Box;
use api::{Cubic, CubicInput, Externals, Note};

use crate::{ChannelOutput, sequencer::{NOTE_HOLD, NOTE_OFF, TrackBank, TrackData, TrackRef}};

pub(super) struct Sequence
{
    /// interpret u8 as +1 (so cannot represent zero)
    tracks: Box<[(TrackRef, u8); 256]>,
    size: u16,
    cubic: Cubic,
    
    track_index: u16,
    /// number of completed repeats
    current_count: u16,
    track_step: u16,
    /// cannot be `true` if `self.size == 0`
    playing: bool,
    paused: bool,
    
    current_clock_div: u8,
    current_note: Note,
    last_note: Note,
    next_note: Note,
    half_time: bool,
    
    /// decremented every time step - next step when it hits zero
    time_steps: u16,
    
    skip: u8,
    one_shot: bool,
    end_soon: bool
}

impl Sequence
{
    pub fn play<E: Externals>(&mut self, mut output: ChannelOutput<E>, one_shot: bool, bank: &TrackBank)
    {
        if self.playing || self.size == 0 { return; }
        if !self.paused
        {
            self.end_soon = false;
            self.track_index = 0;
            self.current_count = 0;
            self.track_step = 0;
            self.time_steps = 0;
            self.last_note = NOTE_OFF;
            
            // setup first notes
            let current = &self.tracks[0];
            let track = current.0.get_ref(bank);
            self.half_time = track.half_time;
            self.current_clock_div = track.clock_div;
            self.current_note = NOTE_OFF;
            self.next_note = track.get_step(0).unwrap_or((NOTE_OFF, 0)).0;
        }
        else
        {
            // continue last note
            if self.playing && self.time_steps > 1
            {
                output.push_note(self.last_note);
            }
        }
        self.one_shot = one_shot;
        self.playing = true;
        self.paused = false;
    }
    pub fn pause<E: Externals>(&mut self, mut output: ChannelOutput<E>)
    {
        self.playing = false;
        self.paused = true;
        if self.playing && self.last_note != NOTE_OFF
        {
            output.remove_note(self.last_note);
        }
    }
    pub fn stop<E: Externals>(&mut self, mut output: ChannelOutput<E>)
    {
        self.playing = false;
        self.paused = false;
        if self.playing && self.last_note != NOTE_OFF
        {
            output.remove_note(self.last_note);
        }
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
                // this was the last time step if we turn the note off early
                if self.end_soon
                {
                    self.playing = false;
                }
            }
            // otherwise do nothing on half time
            return;
        }
        // self.time_steps == 0 here
        if self.end_soon
        {
            self.playing = false;
        }
        
        // reset counter - encodes the clock div for this step
        // allows clock_div to change
        self.time_steps = (self.current_clock_div as u16) << 1;
        self.track_step += 1;
        
        let old = self.last_note;
        self.current_note = self.next_note;
        if self.current_note != NOTE_HOLD
        {
            self.last_note = self.current_note;
        }
        
        // end soon means that this is the end
        // current_note is just NOTE_OFF, so we are just turning off all notes
        if !self.end_soon
        {
            let current = &self.tracks[self.track_index as usize];
            let track = current.0.get_ref(bank);
            // get next trackstep - already have current
            let next = track.get_step(self.track_step);
            
            self.next_note = match next
            {
                // TODO: cubic
                Some((note, _)) => note,
                None => self.inc_track(bank).0,
            };
        }
        
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
    
    fn inc_track(&mut self, bank: &TrackBank) -> (Note, u16)
    {
        let mut ti = self.track_index;
        let mut current = &self.tracks[ti as usize];
        
        self.track_step = 0;
        let mut cc = self.current_count + 1 + self.skip as u16;
        self.skip = 0;
        
        // iterate through track indicies until we reach the correct TrackRef
        while cc > current.1 as u16
        {
            cc -= current.1 as u16 + 1;
            ti += 1;
            if ti >= self.size
            {
                ti = 0;
                // overshot the end of the sequence
                // the rest of the data can be ignored as
                // the step will be the last (stop playing)
                if self.one_shot
                {
                    self.end_soon = true;
                    return (NOTE_OFF, 0);
                }
            }
            current = &self.tracks[ti as usize];
        }
        self.current_count = cc;
        self.track_index = ti;
        
        // now get first note
        let track = current.0.get_ref(bank);
        return track.get_step(0).unwrap_or((NOTE_OFF, 0));
    }
    fn skip(&mut self)
    {
        self.current_count += self.skip as u16;
        self.skip = 0;
        
        
    }
    fn next_track(&mut self, bank: &TrackBank) -> (Note, u16)
    {
        self.track_index += 1;
        self.current_count = 0;
        if self.track_index >= self.size
        {
            self.track_index = 0;
            if self.one_shot
            {
                self.end_soon = true;
                return (NOTE_OFF, 0);
            }
        }
        
        let current = &self.tracks[self.track_index as usize];
        let track = current.0.get_ref(bank);
        self.current_clock_div = track.clock_div;
        self.half_time = track.half_time;
        return track.get_step(0).unwrap_or((NOTE_OFF, 0));
    }
}