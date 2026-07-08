use api::{Channel, Gate, Note, RA};

use crate::{MenuFeedback, NoteCollection, NoteConfig, NoteOutput, OutputConfig, Panel, RETRIG_TIME, SlotSelect, get_only_note, process_note};

pub struct Output
{
    pub panel: Panel,
    pub note_manager: RA<NoteCollection, 5>,
    pub config: OutputConfig,
    pub note_config: NoteConfig,
    
    is_mf: bool,
    mf_end_time: u32,
    mf_channel: Channel,
    active_channels: u8,
}

impl Output
{
    pub fn menu_feedback(&mut self, fb: MenuFeedback, time: u32)
    {
        if !self.config.menu_feedback { return; }
        
        self.panel.output_note(SlotSelect::Channel(fb.channel), Note::new(fb.key, 0xFF));
        self.panel.output_gate_on_base(SlotSelect::Channel(fb.channel), Gate::zero());
        
        self.mf_end_time = time + fb.duration;
        self.mf_channel = fb.channel;
        self.is_mf = true;
    }
    
    pub fn on_loop(&mut self, time: u32)
    {
        if self.is_mf && time > self.mf_end_time
        {
            self.panel.output_gate_off(SlotSelect::Channel(self.mf_channel));
            self.is_mf = false;
        }
    }
    
    #[inline]
    pub fn get_only_note(&self) -> Option<Note>
    {
        return get_only_note(&self.note_manager);
    }
    
    fn get_note_collection(note_manager: &mut RA<NoteCollection, 5>, channel: Channel) -> Option<&mut NoteCollection>
    {
        for nc in note_manager.iter_mut()
        {
            if nc.is_channel(channel)
            {
                return Some(nc);
            }
        }
        
        return None;
    }
    
    fn manage_note_output(&mut self, note_output: NoteOutput, channel: Channel, mut note: Note)
    {
        let vi = match note_output
        {
            NoteOutput::Off(vi) =>
            {
                self.panel.output_gate_off(SlotSelect::ChannelVoice(channel, vi));
                return;
            },
            NoteOutput::New(vi) =>
            {
                if self.config.always_delay
                {
                    self.panel.delay(RETRIG_TIME);
                }
                vi
            },
            NoteOutput::Retrig(vi, new) =>
            {
                if self.config.retrigger_new
                {
                    self.panel.output_gate_off(SlotSelect::ChannelVoice(channel, vi));
                    self.panel.delay(RETRIG_TIME);
                }
                else if self.config.always_delay
                {
                    self.panel.delay(RETRIG_TIME);
                }
                note = new;
                vi
            },
            NoteOutput::None => return
        };
        let slots = SlotSelect::ChannelVoice(channel, vi);
        
        self.panel.output_note(slots, note);
        self.panel.output_gate_on(slots);
    }
    
    pub fn push_note(&mut self, mut channel: Channel, mut note: Note)
    {
        match process_note(channel, note, &self.config)
        {
            Some(cn) => (channel, note) = cn,
            None => return
        }
        
        if self.config.all_channel_mode
        {
            channel = Channel::from_u8(channel as u8 % self.active_channels);
        }
        
        let nc = match Self::get_note_collection(&mut self.note_manager, channel)
        {
            Some(nc) => nc,
            None => return,
        };
        
        let note_output = nc.push_note(&self.note_config, &self.panel.state, note);
        self.manage_note_output(note_output, channel, note);
    }
    
    pub fn remove_note(&mut self, mut channel: Channel, mut note: Note)
    {
        match process_note(channel, note, &self.config)
        {
            Some(cn) => (channel, note) = cn,
            None => return
        }
        
        if self.config.all_channel_mode
        {
            channel = Channel::from_u8(channel as u8 % self.active_channels);
        }
        
        let nc = match Self::get_note_collection(&mut self.note_manager, channel)
        {
            Some(nc) => nc,
            None => return,
        };
        
        let note_output = nc.remove_note(&self.note_config, &self.panel.state, note.key);
        self.manage_note_output(note_output, channel, note);
    }
}