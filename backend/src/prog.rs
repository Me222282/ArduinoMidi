use api::{Channel, MidiCode, Note, RA};

use crate::{Configuration, MenuFeedback, MenuWrapper, NoteCollection, NoteConfig, NoteOutput, OtherConfig, Panel, ProgramPortsMenu, RETRIG_TIME, SequencerConfig, SlotSelect, SpecialOpsMenu, VibratoMenu, VibratoOp, create_dynamic_menus, process_note};

create_dynamic_menus!(pub Menus:
    A => MenuWrapper<SpecialOpsMenu>,
    B => MenuWrapper<ProgramPortsMenu>,
    C => MenuWrapper<VibratoMenu>);

pub struct Program
{
    menu: Menus,
    panel: Panel,
    other_config: OtherConfig,
    note_config: NoteConfig,
    note_manager: RA<NoteCollection, 5>,
    sequen_config: SequencerConfig,
    vibrato: VibratoOp,
    active_channels: u8
}

impl Program
{
    #[inline]
    pub fn get_config<'a>(&'a mut self) -> Configuration<'a>
    {
        return Configuration {
            other: &mut self.other_config,
            note: &mut self.note_config,
            sequen: &mut self.sequen_config,
            output: &mut self.panel.config,
            vibrato: &mut self.vibrato.config
        };
    }
    
    pub fn on_midi_message(&mut self, message: MidiCode, time: usize)
    {
        // if !listener.allow_message(message)
        // {
        //     return;
        // }
        
        // match message
        // {
        //     MidiCode::NoteOFF(channel, note) => listener.off_note(channel, note),
        //     MidiCode::NoteON(channel, note) =>
        //     {
        //         // exit
        //         if listener.on_note(panel, channel, note)
        //         {
        //             *listener = Men::None;
        //         }
        //     },
        //     MidiCode::ControlChange(channel, cctype, _) => todo!(),
        //     MidiCode::PitchWheel(channel, _) => todo!(),
        //     _ => listener.on_message(message)
        // }
    }
    pub fn on_loop(&mut self, time: usize)
    {
        // let config = self.get_config();
        
        // Vibrato
        let mut pb_offsets = [0x0000; 16];
        self.vibrato.on_loop(time, &mut pb_offsets);
        self.panel.set_pf_offsets(&pb_offsets);
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
