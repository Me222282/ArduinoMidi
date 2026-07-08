use api::{Channel, InputMode, LinkedList, Note, PanelState, Queue, SA, RefNode};

use crate::NoteConfig;

pub enum NoteOutput
{
    None,
    Off(u8),
    New(u8),
    Retrig(u8, Note)
}

pub struct NoteCollection
{
    notes: LinkedList<(Note, i8)>,
    locations: SA<Option<RefNode<(Note, i8)>>, 5>,
    old_notes: SA<u8, 5>,
    history: Queue<usize, 5>,
    channel: Channel
}

impl NoteCollection
{
    pub fn is_channel(&self, channel: Channel) -> bool
    {
        return self.channel == channel;
    }
    
    fn add_note(&mut self, sort: bool, value: (Note, i8)) -> RefNode<(Note, i8)>
    {
        if sort
        {
            return self.notes.insert(value, |l, r| l.0.key < r.0.key);
        }
        
        return self.notes.append(value);
    }
    
    pub fn push_note(&mut self, config: &NoteConfig, panel: &PanelState, note: Note) -> NoteOutput
    {
        let hole = self.find_next_index(config, panel, note.key);
        if let Some(i) = hole
        {
            let rn = self.add_note(config.sort_notes, (note, i as i8));
            self.locations[i] = Some(rn);
            self.old_notes[i] = 0xFF;
            return NoteOutput::New(i as u8);
        }
        
        // take a used slot
        let take = self.get_losable_note(config, panel, note.key);
        // add to list after slot chosen
        match take
        {
            Some(mut t) =>
            {
                let take_ref = self.notes.get_mut(&mut t);
                let hole = take_ref.1;
                take_ref.1 = -1;
                
                let rn = self.add_note(config.sort_notes, (note, hole));
                self.locations[hole as usize] = Some(rn);
                // was already a note so no need for old_notes to change
                
                if config.forget_notes
                {
                    // remove taken notes to forget
                    self.notes.remove(t);
                }
                
                return NoteOutput::Retrig(hole as u8, note);
            },
            None =>
            {
                if !config.forget_notes
                {
                    self.add_note(config.sort_notes, (note, -1));
                }
                return NoteOutput::None;
            }
        }
    }
    
    pub fn remove_note(&mut self, _config: &NoteConfig, panel: &PanelState, key: u8) -> NoteOutput
    {
        // only remove 1 note - important for arpeggios
        let remove = match self.notes.iter_forward_ref().find(|rn| self.notes.get_ref(rn).0.key == key)
        {
            Some(v) => v,
            // could not find note
            None => return NoteOutput::None
        };
        
        let hole = self.notes.get_ref(&remove).1;
        self.notes.remove(remove);
        // not a note being displayed
        if hole < 0 { return NoteOutput::None; }
        
        // fill hole if can
        let replace = self.get_next_note(panel);
        match replace
        {
            Some(mut r) =>
            {
                let ni = self.notes.get_mut(&mut r);
                ni.1 = hole;
                let note = ni.0;
                self.locations[hole as usize] = Some(r);
                return NoteOutput::Retrig(hole as u8, note);
            },
            // no more
            None =>
            {
                self.locations[hole as usize] = None;
                self.old_notes[hole as usize] = key;
                if !panel.stack
                {
                    // loop mode slot history
                    self.history.push(hole as usize);
                }
                // no more note
                // gateChannelNote(chI, hole, false);
                return NoteOutput::Off(hole as u8);
            }
        }
    }
    
    fn get_next_note(&self, panel: &PanelState) -> Option<RefNode<(Note, i8)>>
    {
        // all notes in use
        if self.notes.len() < self.locations.len()
        {
            return None;
        }
        
        match panel.input
        {
            InputMode::TakeFirst =>
            {
                if let Some(e) = self.notes.last()
                {
                    if self.notes.get_ref(&e).1 < 0
                    {
                        return Some(e);
                    }
                }
            },
            InputMode::TakeLast =>
            {
                // for newest note priority
                for x in self.notes.iter_backward_ref()
                {
                    if self.notes.get_ref(&x).1 < 0
                    {
                        return Some(x);
                    }
                }
                
                return None;
            },
            _ => {}
        }
        
        // for ignore new
        for x in self.notes.iter_forward_ref()
        {
            if self.notes.get_ref(&x).1 < 0
            {
                return Some(x);
            }
        }
        
        return None;
    }
    
    fn find_next_index(&mut self, config: &NoteConfig, panel: &PanelState, key: u8) -> Option<usize>
    {
        if !config.duplicate_release
        {
            // find key if already one of the open slots
            for (i, x) in self.old_notes.as_ref().iter().enumerate()
            {
                if *x != key { continue; }
            
                return Some(i);
            }
        }
        
        if !panel.stack
        {
            return self.history.pull();
        }
        
        for (i, x) in self.locations.as_ref().iter().enumerate()
        {
            if let None = x
            {
                return Some(i);
            }
        }
        
        return None;
    }
    
    fn losable_u_note(&self, panel: &PanelState) -> Option<RefNode<(Note, i8)>>
    {
        match panel.input
        {
            InputMode::TakeFirst =>
            {
                return self.notes.last();
            },
            // for newest note priority
            InputMode::TakeLast =>
            {
                // start from front of stack
                if self.notes.len() < (self.locations.len() * 2)
                {
                    for x in self.notes.iter_forward_ref()
                    {
                        if self.notes.get_ref(&x).1 >= 0
                        {
                            return Some(x);
                        }
                    }
                    
                    return None;
                }
                
                // otherwise from back
                let mut next = self.notes.last();
                for x in self.notes.iter_backward_ref().skip(1)
                {
                    if self.notes.get_ref(&x).1 < 0
                    {
                        return next;
                    }
                    next = Some(x);
                }
                
                return None;
            },
            _ => return None
        }
    }
    
    fn losable_o_note(&self, panel: &PanelState, key: u8) -> Option<RefNode<(Note, i8)>>
    {
        match panel.input
        {
            InputMode::TakeFirst =>
            {
                let end;
                match self.notes.last()
                {
                    Some(v) => end = v,
                    None => return None
                }
                if self.notes.get_ref(&end).0.key < key
                {
                    return self.notes.last();
                }
                
                // for lowest note priority modified
                // start from top of stack
                if self.notes.len() < (self.locations.len() * 2)
                {
                    for x in self.notes.iter_backward_ref().skip(1)
                    {
                        let nl = self.notes.get_ref(&x);
                        if nl.0.key < key { return None; }
                        if nl.1 >= 0 { return Some(x); }
                    }
                    
                    return None;
                }
                
                // start from bottom of stack
                let mut last;
                match self.notes.first()
                {
                    Some(v) => last = v,
                    None => return None
                }
                for x in self.notes.iter_forward_ref().skip(1)
                {
                    if self.notes.get_ref(&x).1 < 0
                    {
                        if self.notes.get_ref(&last).0.key < key { return None; }
                        return Some(last);
                    }
                    last = x;
                }
                
                return None;
            },
            // for highest note priority
            InputMode::TakeLast =>
            {
                // start from bottom of stack
                if self.notes.len() < (self.locations.len() * 2)
                {
                    for x in self.notes.iter_forward_ref()
                    {
                        let nl = self.notes.get_ref(&x);
                        if nl.0.key > key { return None; }
                        if nl.1 >= 0 { return Some(x); }
                    }
                    
                    return None;
                }
                
                // start from top of stack
                let mut next;
                match self.notes.last()
                {
                    Some(v) => next = v,
                    None => return None
                }
                for x in self.notes.iter_backward_ref().skip(1)
                {
                    if self.notes.get_ref(&x).1 < 0
                    {
                        if self.notes.get_ref(&next).0.key > key { return None; }
                        return Some(next);
                    }
                    next = x;
                }
                
                return None;
            },
            // for lowest note priority
            InputMode::Ignore =>
            {
                // start from top of stack
                if self.notes.len() < (self.locations.len() * 2)
                {
                    for x in self.notes.iter_backward_ref()
                    {
                        let nl = self.notes.get_ref(&x);
                        if nl.0.key < key { return None; }
                        if nl.1 >= 0 { return Some(x); }
                    }
                    
                    return None;
                }
                
                // start from bottom of stack
                let mut last;
                match self.notes.first()
                {
                    Some(v) => last = v,
                    None => return None
                }
                for x in self.notes.iter_forward_ref().skip(1)
                {
                    if self.notes.get_ref(&x).1 < 0
                    {
                        if self.notes.get_ref(&last).0.key < key { return None; }
                        return Some(last);
                    }
                    last = x;
                }
                
                return None;
            }
        }
    }
    
    fn get_losable_note(&self, config: &NoteConfig, panel: &PanelState, key: u8) -> Option<RefNode<(Note, i8)>>
    {
        if config.sort_notes
        {
            return self.losable_o_note(panel, key);
        }
        return self.losable_u_note(panel);
    }
}

pub(crate) fn get_only_note(manager: &SA<NoteCollection, 5>) -> Option<Note>
{
    let mut note = Note::new(0, 0);
    let mut set_note = false;
    
    for nc in manager.iter()
    {
        for ptrs in nc.locations.iter()
        {
            if let Some(rn) = ptrs
            {
                // fuond more than 1 note
                if set_note { return None; }
                note = nc.notes.get_ref(rn).0;
                set_note = true;
            }
        }
    }
    
    // didnt find any notes
    if !set_note { return None; }
    return Some(note);
}
