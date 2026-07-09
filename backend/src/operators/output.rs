use api::{CCType, Channel, Gate, Note, SA, Switch};

use crate::{MenuFeedback, NoteCollection, NoteConfig, NoteOutput, OutputConfig, Panel, RETRIG_TIME, SlotSelect, VibratoOp, get_only_note, process_note};

pub struct Output
{
    pub panel: Panel,
    pub note_manager: SA<NoteCollection, 5>,
    pub config: OutputConfig,
    pub note_config: NoteConfig,
    pub vibrato: VibratoOp,
    
    is_mf: bool,
    mf_end_time: u32,
    mf_channel: Channel,
    
    channel_voices: [(Channel, u8); 5],
    cv_count: u8,
    
    mod_values: [u16; 16],
    pb_values: [u16; 16]
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
        
        self.vibrato.on_loop(time, &mut self.panel);
    }
    
    #[inline]
    pub fn get_only_note(&self) -> Option<Note>
    {
        return get_only_note(&self.note_manager);
    }
    
    fn get_note_collection(note_manager: &mut SA<NoteCollection, 5>, channel: Channel) -> Option<&mut NoteCollection>
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
    
    pub fn push_note(&mut self, channel: Channel, note: Note)
    {
        match process_note(channel, note, &self.config)
        {
            Some((channel, note)) => self.push_note_post(channel, note),
            None => {}
        }
    }
    pub(in crate::operators) fn push_note_post(&mut self, mut channel: Channel, note: Note)
    {
        if self.config.all_channel_mode
        {
            channel = self.all_channel_modulo(channel);
        }
        
        let nc = match Self::get_note_collection(&mut self.note_manager, channel)
        {
            Some(nc) => nc,
            None => return,
        };
        
        let note_output = nc.push_note(&self.note_config, &self.panel.state, note);
        self.manage_note_output(note_output, channel, note);
    }
    
    pub fn remove_note(&mut self, channel: Channel, note: Note)
    {
        match process_note(channel, note, &self.config)
        {
            Some((channel, note)) => self.remove_note_post(channel, note),
            None => {}
        }
    }
    pub(in crate::operators) fn remove_note_post(&mut self, mut channel: Channel, note: Note)
    {
        if self.config.all_channel_mode
        {
            channel = self.all_channel_modulo(channel);
        }
        
        let nc = match Self::get_note_collection(&mut self.note_manager, channel)
        {
            Some(nc) => nc,
            None => return,
        };
        
        let note_output = nc.remove_note(&self.note_config, &self.panel.state, note.key);
        self.manage_note_output(note_output, channel, note);
    }
    
    pub fn set_modulation(&mut self, channel: Channel, value: u16)
    {
        if channel == Channel::All
        {
            for i in 0u8..16u8
            {
                self.set_modulation(Channel::from_u8(i), value);
            }
            return;
        }
        
        self.mod_values[channel as usize] = value;
        self.panel.output_modulation(channel, value);
        
        self.vibrato.on_modulation(channel, value);
    }
    
    pub fn on_cc(&mut self, mut channel: Channel, cc: CCType, value: u8)
    {
        if self.config.all_channel_cc
        {
            channel = self.all_channel_modulo(channel);
        }
        
        self.panel.output_control_change(cc, channel, value);
        
        match cc
        {
            CCType::ALL_NOTES_OFF =>
            {
                // TODO
            },
            CCType::MODULATION_WHEEL_MSB =>
            {
                let m = self.mod_values[channel as usize];
                self.set_modulation(channel, (m & 0x007F) | ((value as u16) << 7));
            },
            CCType::MODULATION_WHEEL_LSB =>
            {
                let m = self.mod_values[channel as usize];
                self.set_modulation(channel, (m & 0x3F80) | value as u16);
            },
            _ => {}
        }
    }
    
    pub fn on_pitch_bend(&mut self, mut channel: Channel, value: u16)
    {
        if self.config.all_channel_pb
        {
            channel = self.all_channel_modulo(channel);
        }
        
        self.pb_values[channel as usize] = value;
        self.panel.set_pitch_bend(&mut self.vibrato, channel, value);
    }
    
    fn all_channel_modulo(&mut self, channel: Channel) -> Channel
    {
        for cv in &self.channel_voices[0..(self.cv_count as usize)]
        {
            if cv.0 == channel
            {
                return channel;
            }
        }
        
        let index = channel as u8 % self.cv_count;
        return self.channel_voices[index as usize].0;
    }
    pub fn on_switch(&mut self, switch: Switch)
    {
        if switch.is_new_channels()
        {
            // IMPORTANT: channel voices is ordered by channel
            self.cv_count = self.panel.update_slot_allocations(&mut self.channel_voices);
            
            // reallocate note collections
            let iter = self.channel_voices[0..(self.cv_count as usize)].iter()
                .map(|cv| NoteCollection::new(cv.0, cv.1));
            self.note_manager = SA::from_iter(iter);
        }
        else if switch.is_resetting()
        {
            // reset note collections
            for nc in self.note_manager.iter_mut()
            {
                nc.clear();
            }
        }
        if switch.is_resetting()
        {
            self.vibrato.on_reset_switch();
            
            // reset outputs
            self.panel.output_gate_off(SlotSelect::All);
            self.panel.output_note(SlotSelect::All, Note::new(0, 0));
        }
        
        // non-resetting
        match switch
        {
            Switch::OCTAVE => self.panel.update_notes(),
            Switch::PITCH_BEND =>
            {
                // reoutput pitch bend values
                for &(channel, _) in &self.channel_voices[0..(self.cv_count as usize)]
                {
                    self.panel.set_pitch_bend(&mut self.vibrato, channel, self.pb_values[channel as usize]);
                }
            },
            Switch::MOD_OPTION =>
            {
                // reoutput velocities
                self.panel.update_vels();
                // reoutput mod values
                for &(channel, _) in &self.channel_voices[0..(self.cv_count as usize)]
                {
                    self.panel.output_modulation(channel, self.mod_values[channel as usize]);
                }
            },
            _ => {}
        }
    }
}