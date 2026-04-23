use api::{Channel, InputMode, LinkedList, Note, Queue, RA, RefNode};

use crate::{InputListener, Panel};

pub struct NoteCollection
{
    notes: LinkedList<(Note, i8)>,
    locations: RA<Option<RefNode<(Note, i8)>>, 5>,
    old_notes: RA<u8, 5>,
    history: Queue<usize, 5>,
    channel: Channel
}

impl NoteCollection
{
    pub fn push_note(&mut self, panel: &Panel, note: Note)
    {
        let hole = self.find_next_index(panel, note.key);
        if let Some(i) = hole
        {
            let rn = self.notes.append((note, i as i8));
            self.locations[i] = Some(rn);
            self.old_notes[i] = 0xFF;
            return;
        }
        
        // take a used slot
        let take = self.get_losable_note(panel, note.key);
        // add to list after slot chosen
        match take
        {
            Some(mut t) =>
            {
                let take_ref = self.notes.get_mut(&mut t);
                let hole = 0;
                take_ref.1 = -1;
                
                let rn = self.notes.append((note, hole));
                self.locations[hole as usize] = Some(rn);
                // was already a note so no need for old_notes to change
                
                if panel.configuration.forget_notes
                {
                    // remove taken notes to forget
                    self.notes.remove(t);
                }
            },
            None =>
            {
                if !panel.configuration.forget_notes
                {
                    self.notes.append((note, -1));
                }
                return;
            }
        }
    }
    
    pub fn remove_note(&mut self, panel: &Panel, key: u8)
    {
        // only remove 1 note - important for arpeggios
        let remove_op = self.notes.iter_forward_ref().find(|rn| self.notes.get_ref(rn).0.key == key);
        let remove;
        match remove_op
        {
            Some(v) => remove = v,
            // could not find note
            None => return
        }
        
        let hole = self.notes.get_ref(&remove).1;
        self.notes.remove(remove);
        // not a note being displayed
        if hole < 0 { return; }
        
        // fill hole if can
        let replace = self.get_next_note(panel);
        match replace
        {
            Some(mut r) =>
            {
                self.notes.get_mut(&mut r).1 = hole;
                self.locations[hole as usize] = Some(r);
            },
            // no more
            None =>
            {
                self.locations[hole as usize] = None;
                self.old_notes[hole as usize] = key;
                if !panel.state.stack
                {
                    // loop mode slot history
                    self.history.push(hole as usize);
                }
                // no more note
                // gateChannelNote(chI, hole, false);
                return;
            }
        }
    }
    
    fn get_next_note(&self, panel: &Panel) -> Option<RefNode<(Note, i8)>>
    {
        // all notes in use
        if self.notes.len() < self.locations.len()
        {
            return None;
        }
        
        match panel.state.input
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
    
    fn find_next_index(&mut self, panel: &Panel, key: u8) -> Option<usize>
    {
        if !panel.configuration.duplicate_release
        {
            // find key if already one of the open slots
            for (i, x) in self.old_notes.as_ref().iter().enumerate()
            {
                if *x != key { continue; }
            
                return Some(i);
            }
        }
        
        if !panel.state.stack
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
    
    fn losable_u_note(&self, panel: &Panel) -> Option<RefNode<(Note, i8)>>
    {
        match panel.state.input
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
    
    fn losable_o_note(&self, panel: &Panel, key: u8) -> Option<RefNode<(Note, i8)>>
    {
        match panel.state.input
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
    
    fn get_losable_note(&self, panel: &Panel, key: u8) -> Option<RefNode<(Note, i8)>>
    {
        if panel.configuration.sort_notes
        {
            return self.losable_o_note(panel, key);
        }
        return self.losable_u_note(panel);
    }
}

pub struct NoteManager
{
    channels: RA<NoteCollection, 5>
}

impl InputListener for NoteManager
{
    fn on_loop(&mut self, panel: &mut Panel)
    {
        
    }

    fn on_note(&mut self, panel: &mut Panel, channel: Channel, note: Note) -> bool
    {
        // let 
        
        return false;
    }
}