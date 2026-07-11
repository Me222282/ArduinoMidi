use alloc::boxed::Box;
use api::{Cubic, CubicInput, Externals, Note};

use crate::{ChannelOutput, sequencer::{NOTE_HOLD, NOTE_OFF, TrackBank, TrackRef}};

pub(super) struct Sequence
{
    /// interpret u8 as +1 (so cannot represent zero)
    tracks: Box<[(TrackRef, u8); 256]>,
    /// size == 0 => no sequence
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
    current_step: (Note, u16),
    /// the last outputed step (anything but `NOTE_HOLD`)
    last_note: Note,
    next_step: (Note, u16),
    half_time: bool,
    use_mod: bool,
    
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
            self.use_mod = track.use_mod;
            
            self.next_step = track.get_step(0).unwrap_or((NOTE_OFF, 0));
            let mut cm = 0;
            if self.use_mod
            {
                cm = self.get_last_mod(bank);
                // when does this way, mod will smoothly interpolate across to the next outputed value
                // so will be different if we skip a section
                self.cubic = Cubic::generate(cm, cm, self.next_step.1, self.get_next_mod(bank));
            }
            
            self.current_step = (NOTE_OFF, cm);
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
    
    fn get_last_mod(&self, bank: &TrackBank) -> u16
    {
        let track = self.tracks[self.size as usize - 1].0.get_ref(bank);
        return track.get_last_step().1;
    }
    /// run after `inc_track` to get the correct data
    fn get_next_mod(&self, bank: &TrackBank) -> u16
    {
        let ts = self.track_step + 1;
        let mut ti = self.track_index;
        
        // same as in on_time_step
        let current = &self.tracks[ti as usize];
        let track = current.0.get_ref(bank);
        let next = track.get_step(ts);
        
        return match next
        {
            Some((_, m)) => m,
            None =>
            {
                // use next track - similar to inc_track
                let cc = self.current_count + 1;
                
                if cc > current.1 as u16
                {
                    ti += 1;
                    if ti > self.size { ti = 0; }
                    let current = &self.tracks[ti as usize];
                    let track = current.0.get_ref(bank);
                    return track.get_step(0).unwrap_or((NOTE_OFF, 0)).1;
                }
                
                return track.get_step(0).unwrap_or((NOTE_OFF, 0)).1;
            },
        };
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
            if self.half_time && self.current_step.0 != NOTE_OFF && self.next_step.0 != NOTE_HOLD
            {
                output.remove_note(self.current_step.0);
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
        let cs = self.next_step;
        let last_mod = self.current_step.1;
        self.current_step = cs;
        if cs.0 != NOTE_HOLD
        {
            self.last_note = cs.0;
        }
        
        // end soon means that this is the end
        // current_note is just NOTE_OFF, so we are just turning off all notes
        if !self.end_soon
        {
            let current = &self.tracks[self.track_index as usize];
            let track = current.0.get_ref(bank);
            // get next trackstep - already have current
            let next = track.get_step(self.track_step);
            
            self.next_step = next.unwrap_or_else(|| self.inc_track(bank));
            if self.use_mod
            {
                self.cubic = Cubic::generate(last_mod, cs.1, self.next_step.1, self.get_next_mod(bank));
            }
        }
        
        // swap outputs - in this order so that retrig is performed
        // do nothing if we are to hold
        if cs.0 != NOTE_HOLD
        {
            if cs.0 != NOTE_OFF
            {
                output.push_note(cs.0);
            }
            if !self.half_time && old != NOTE_OFF
            {
                output.remove_note(old);
            }
        }
    }
    pub fn on_sub_step<E: Externals>(&mut self, mut output: ChannelOutput<E>, time: CubicInput)
    {
        if !self.playing || !self.use_mod { return; }
        
        let m = self.cubic.compute(time);
        output.set_modulation((m as u16).clamp(0, 0x3FFF));
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
        
        // get data about current track
        let track = current.0.get_ref(bank);
        self.current_clock_div = track.clock_div;
        self.half_time = track.half_time;
        self.use_mod = track.use_mod;
        // now get first note
        return track.get_step(0).unwrap_or((NOTE_OFF, 0));
    }
}