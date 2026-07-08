use api::{Channel, Gate, MidiCode, Note, RA};

use crate::{Configuration, MenuFeedback, MenuWrapTrait, MenuWrapper, NoteCollection, NoteConfig, NoteOutput, OtherConfig, Panel, ProgramPortsMenu, RETRIG_TIME, SequencerConfig, SlotSelect, SpecialOpsMenu, VibratoMenu, VibratoOp, create_dynamic_menus, process_note};

create_dynamic_menus!(pub Menus:
    A => MenuWrapper<SpecialOpsMenu>,
    B => MenuWrapper<ProgramPortsMenu>,
    C => MenuWrapper<VibratoMenu>);

pub struct Program
{
    menu: Menus,
    pub panel: Panel,
    other_config: OtherConfig,
    note_config: NoteConfig,
    note_manager: RA<NoteCollection, 5>,
    sequen_config: SequencerConfig,
    vibrato: VibratoOp,
    active_channels: u8,
    
    is_mf: bool,
    mf_end_time: u32,
    mf_channel: Channel
}

impl Program
{
    // #[inline]
    // pub fn get_config<'a>(&'a mut self) -> Configuration<'a>
    // {
    //     return Configuration {
    //         other: &mut self.other_config,
    //         note: &mut self.note_config,
    //         sequen: &mut self.sequen_config,
    //         output: &mut self.panel.config,
    //         vibrato: &mut self.vibrato.config
    //     };
    // }
    
    pub fn on_midi_message(&mut self, message: MidiCode, time: u32)
    {
        if !self.menu.allow_message(message) { return; }
        
        match message
        {
            MidiCode::NoteOFF(channel, note) =>
            {
                if self.menu.is_none()
                {
                    self.menu.off_note(channel, note);
                }
                else
                {
                    self.remove_note(channel, note);
                }
            },
            MidiCode::NoteON(channel, note) =>
            {
                if self.menu.is_none()
                {
                    let mut config = Configuration {
                        other: &mut self.other_config,
                        note: &mut self.note_config,
                        sequen: &mut self.sequen_config,
                        output: &mut self.panel.config,
                        vibrato: &mut self.vibrato.config
                    };
                    let exit = self.menu.on_note(&mut config, time, channel, note);
                    if let Some(fb) = exit.1 { self.menu_feedback(fb, time); }
                    // exit menu
                    if exit.0 { self.menu = Menus::None; }
                }
                else
                {
                    self.push_note(channel, note);
                }
            },
            MidiCode::ControlChange(channel, cctype, _) => todo!(),
            MidiCode::PitchWheel(channel, _) => todo!(),
            _ => self.menu.on_message(message)
        }
    }
    pub fn on_loop(&mut self, time: u32)
    {
        // let config = self.get_config();
        
        // Menu feedback
        if self.is_mf && time > self.mf_end_time
        {
            self.panel.output_gate_off(SlotSelect::Channel(self.mf_channel));
            self.is_mf = false;
        }
        
        // Vibrato
        let mut pb_offsets = [0x0000; 16];
        self.vibrato.on_loop(time, &mut pb_offsets);
        self.panel.set_pf_offsets(&mut self.vibrato, &pb_offsets);
    }
    pub fn on_reset_switch(&mut self, time: u32)
    {
        self.vibrato.on_reset_switch();
        
        let exit = self.menu.on_reset_switch();
        if let Some(fb) = exit.1 { self.menu_feedback(fb, time); }
        if exit.0 { self.menu = Menus::None; }
    }
    
    fn menu_feedback(&mut self, fb: MenuFeedback, time: u32)
    {
        if !self.other_config.menu_feedback { return; }
        
        self.panel.output_note(SlotSelect::Channel(fb.channel), Note::new(fb.key, 0xFF));
        self.panel.output_gate_on_base(SlotSelect::Channel(fb.channel), Gate::zero());
        
        self.mf_end_time = time + fb.duration;
        self.mf_channel = fb.channel;
        self.is_mf = true;
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
                if self.other_config.always_delay
                {
                    self.panel.delay(RETRIG_TIME);
                }
                vi
            },
            NoteOutput::Retrig(vi, new) =>
            {
                if self.other_config.retrigger_new
                {
                    self.panel.output_gate_off(SlotSelect::ChannelVoice(channel, vi));
                    self.panel.delay(RETRIG_TIME);
                }
                else if self.other_config.always_delay
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
        match process_note(channel, note, &self.other_config)
        {
            Some(cn) => (channel, note) = cn,
            None => return
        }
        
        if self.other_config.all_channel_mode
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
        match process_note(channel, note, &self.other_config)
        {
            Some(cn) => (channel, note) = cn,
            None => return
        }
        
        if self.other_config.all_channel_mode
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
