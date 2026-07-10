use api::{Channel, LinkedList, Note, RefNode};

use crate::{Arpeggio, ArpeggioConfig, ChannelOutput, Output, process_note};

struct ArpInstance
{
    notes: LinkedList<Note>,
    current: Option<RefNode<Note>>,
    current_time: u32,
    remove_current: bool,
    /// for alternating mode
    reversed: bool
}

pub(crate) struct Arpeggiator
{
    insts: [ArpInstance; 16],
    pub config: ArpeggioConfig,
    last_time: u32,
    clock_count: usize
}

impl ArpInstance
{
    fn add_note<E: api::Externals>(&mut self, config: &Arpeggio, mut output: ChannelOutput<E>, note: Note)
    {
        let rn = match config.sort_notes
        {
            true => self.notes.insert_ord(note),
            false => self.notes.append(note),
        };
        
        // if no current note (even if clocked because timing may not be perfect)
        if /* !clocked &&*/ self.current.is_none()
        {
            self.current = Some(rn);
            self.current_time = 0;
            output.push_note_post(note);
        }
    }
    
    fn remove_note(&mut self, key: u8)
    {
        // current is remove
        if self.current.as_ref().is_some_and(|cn| self.notes.get_ref(cn).key == key)
        {
            self.remove_current = true;
            return;
        }
        
        let remove = match self.notes.iter_forward_ref().find(|rn| self.notes.get_ref(rn).key == key)
        {
            Some(v) => v,
            // could not find note
            None => return
        };
        self.notes.remove(remove);
    }
    
    fn trigger_next<E: api::Externals>(&mut self, config: &Arpeggio, mut output: ChannelOutput<E>)
    {
        // no current means output first
        if self.current.is_none()
        {
            self.current = self.notes.first();
            match &self.current
            {
                Some(rn) => output.push_note_post(*self.notes.get_ref(rn)),
                None => {},
            }
            return;
        }
        // move out of self - so can be removed from linked list if needed
        let current = match core::mem::replace(&mut self.current, None)
        {
            Some(v) => v,
            None =>
            {
                self.current = self.notes.first();
                match &self.current
                {
                    Some(rn) => output.push_note_post(*self.notes.get_ref(rn)),
                    None => {},
                }
                return;
            },
        };
        
        let mut new_current = match config.mode
        {
            crate::ArpeggioMode::Ascending =>
            {
                match self.notes.get_next(&current)
                {
                    Some(v) => Some(v),
                    None => self.notes.first(),
                }
            },
            crate::ArpeggioMode::Decending =>
            {
                match self.notes.get_previous(&current)
                {
                    Some(v) => Some(v),
                    None => self.notes.last(),
                }
            },
            crate::ArpeggioMode::Alternating if self.reversed =>
            {
                match self.notes.get_previous(&current)
                {
                    Some(v) => Some(v),
                    // current is first
                    None =>
                    {
                        self.reversed = false;
                        self.notes.get_next(&current)
                        // incase there is only 1 note in the list
                            .or(self.notes.first())
                    },
                }
            },
            // don't need - self.reversed is false here
            crate::ArpeggioMode::Alternating /*if !self.reversed*/ =>
            {
                match self.notes.get_next(&current)
                {
                    Some(v) => Some(v),
                    // current is last
                    None =>
                    {
                        self.reversed = true;
                        self.notes.get_previous(&current)
                        // incase there is only 1 note in the list
                            .or(self.notes.last())
                    },
                }
            }
        };
        
        let old = *self.notes.get_ref(&current);
        if self.remove_current
        {
            self.notes.remove(current);
            self.remove_current = false;
        }
        
        // memory safty - about to remove the value new_current points to
        if self.notes.len() == 1 && self.remove_current
        {
            new_current = None;
        }
        self.current = new_current;
        
        // output.panel.output_trigger(crate::TriggerSource::Arpeggio(channel), true);
        
        // swap outputs
        match &self.current
        {
            Some(v) =>
            {
                output.push_note_post(*self.notes.get_ref(v));
            },
            None => {}
        };
        if !config.half_notes
        {
            output.remove_note_post(old);
        }
    }
}

impl Arpeggiator
{
    pub fn on_note<E: api::Externals>(&mut self, output: &mut Output<E>, mut channel: Channel, mut note: Note)
    {
        // process before arpeggio
        match process_note(channel, note, &output.config)
        {
            Some(cn) => (channel, note) = cn,
            None => return
        }
        
        let config = &self.config.arpeggios[channel as usize];
        if config.enabled
        {
            let arp = &mut self.insts[channel as usize];
            arp.add_note(config, ChannelOutput::new(output, channel), note);
            return;
        }
        
        output.push_note_post(channel, note);
    }
    
    pub fn off_note<E: api::Externals>(&mut self, output: &mut Output<E>, mut channel: Channel, mut note: Note)
    {
        // process before arpeggio
        match process_note(channel, note, &output.config)
        {
            Some(cn) => (channel, note) = cn,
            None => return
        }
        
        let config = &self.config.arpeggios[channel as usize];
        if config.enabled
        {
            let arp = &mut self.insts[channel as usize];
            arp.remove_note(note.key);
            return;
        }
        
        output.remove_note_post(channel, note);
    }
    
    pub fn on_loop<E: api::Externals>(&mut self, time: u32, output: &mut Output<E>)
    {
        if self.config.clocked_arpeggios { return; }
        
        let dt = time - self.last_time;
        self.last_time = time;
        
        for (i, (arp, config)) in self.insts.iter_mut().zip(&self.config.arpeggios).enumerate()
        {
            let cn = match &arp.current
            {
                Some(v) => v,
                None => continue
            };
            // this system of delta time ensures more accurate timing
            arp.current_time += dt;
            if arp.current_time >= config.time
            {
                arp.current_time -= config.time;
                arp.trigger_next(config, ChannelOutput::new(output, Channel::from_u8(i as u8)));
                continue;
            }
            // turn off at half time
            if config.half_notes && (arp.current_time + arp.current_time) > config.time
            {
                let note = *arp.notes.get_ref(cn);
                output.remove_note_post(Channel::from_u8(i as u8), note);
            }
        }
    }
    
    pub fn on_clock<E: api::Externals>(&mut self, output: &mut Output<E>)
    {
        let acc = self.clock_count;
        self.clock_count += 1;
        
        if acc % 6 == 0
        {
            for (i, (arp, config)) in self.insts.iter_mut().zip(&self.config.arpeggios).enumerate()
            {
                if arp.current.is_none() { continue; }
                arp.trigger_next(config, ChannelOutput::new(output, Channel::from_u8(i as u8)));
            }
            return;
        }
        // half time
        if acc % 3 == 0
        {
            for (i, (arp, config)) in self.insts.iter_mut().zip(&self.config.arpeggios).enumerate()
            {
                if !config.half_notes { continue; }
                let cn = match &arp.current
                {
                    Some(v) => v,
                    None => continue
                };
                let note = *arp.notes.get_ref(cn);
                output.remove_note_post(Channel::from_u8(i as u8), note);
            }
        }
    }
}