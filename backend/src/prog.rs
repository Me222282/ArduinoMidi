use core::marker::PhantomData;

use api::{Channel, MidiCode, Note, NvsInterface, Switch};

use crate::{Arpeggiator, Configuration, MenuFeedback, MenuManager, MenuWrapTrait, Output};

pub struct Program<E: api::Externals, N: NvsInterface>
{
    menu: MenuManager,
    
    // sequen_config: SequencerConfig,
    arpeggio: Arpeggiator,
    pub output: Output<E>,
    _phantom_n: PhantomData<N>,
    
    factory_reset_count: u8,
    factory_reset_time: u32
}

impl<E: api::Externals, N: NvsInterface> Program<E, N>
{
    pub fn on_midi_message(&mut self, nvs: &mut N, message: MidiCode, time: u32)
    {
        // ignore all messages from disabled channels
        let channel = message.get_channel();
        if channel != Channel::All && !self.output.config.channel_filters[channel as usize].enabled { return; }
        
        if !self.menu.allow_message(message) { return; }
        
        match message
        {
            // NoteONs with velocity 0 are NoteOFFs
            MidiCode::NoteON(channel, note @ Note { key: _, velocity: 0 }) |
            MidiCode::NoteOFF(channel, note) =>
            {
                if self.menu.is_some()
                {
                    let mut config = Configuration {
                        // other: &mut self.other_config,
                        note: &mut self.output.note_config,
                        // sequen: &mut self.sequen_config,
                        output: &mut self.output.config,
                        panel: &mut self.output.panel.config,
                        vibrato: &mut self.output.vibrato.config,
                        arpeggio: &mut self.arpeggio.config
                    };
                    // do normal note if not processed
                    if !self.menu.off_note(&mut config, channel, note)
                    {
                        self.arpeggio.off_note(&mut self.output, channel, note);
                    }
                }
                else
                {
                    self.arpeggio.off_note(&mut self.output, channel, note);
                }
            },
            MidiCode::NoteON(channel, note) =>
            {
                if self.menu.is_some()
                {
                    let mut config = Configuration {
                        // other: &mut self.other_config,
                        note: &mut self.output.note_config,
                        // sequen: &mut self.sequen_config,
                        output: &mut self.output.config,
                        panel: &mut self.output.panel.config,
                        vibrato: &mut self.output.vibrato.config,
                        arpeggio: &mut self.arpeggio.config
                    };
                    let exit = self.menu.on_note(&mut config, nvs, time, channel, note);
                    // was note processed
                    if let Some(exit) = exit
                    {
                        if let Some(fb) = exit.1 { self.output.menu_feedback(fb, time); }
                        // exit menu
                        if exit.0 { self.menu.exit(); }
                        // special ops menu - factory reset
                        else if self.menu.is_special_ops()
                        {
                            // repeated key in time
                            if note.key == Note::B3 &&
                                (self.factory_reset_count == 0 || time - self.factory_reset_time <= crate::FACTORY_RESET_TIME)
                            {
                                self.factory_reset_time = time;
                                self.factory_reset_count += 1;
                                if self.factory_reset_count >= 3
                                {
                                    self.on_factory_reset(nvs);
                                }
                            }
                            else
                            {
                                self.factory_reset_count = 0;
                            }
                        }
                    }
                    // otherwise do normal note
                    self.arpeggio.on_note(&mut self.output, channel, note);
                }
                else
                {
                    self.arpeggio.on_note(&mut self.output, channel, note);
                }
            },
            MidiCode::ControlChange(channel, cctype, value) => self.output.on_cc(channel, cctype, value),
            MidiCode::PitchWheel(channel, value) => self.output.on_pitch_bend(channel, value),
            
            // everything else should go to menu
            MidiCode::TimingClock =>
            {
                self.arpeggio.on_clock(&mut self.output);
                self.menu.on_message(&mut self.output, message);
            }
            _ => self.menu.on_message(&mut self.output, message)
        }
    }
    
    pub fn on_loop(&mut self, time: u32)
    {
        self.output.on_loop(time);
        self.arpeggio.on_loop(time, &mut self.output);
        
        if let Some(s) = self.menu.is_sequencer()
        {
            s.sequencer.on_loop(&mut self.output, time);
        }
    }
    
    pub fn on_switch(&mut self, switch: Switch, time: u32)
    {
        self.output.on_switch(switch);
        
        if switch.is_resetting()
        {
            if self.menu.is_none()
            {
                // only pressing 1 note
                if let Some(n) = self.output.get_only_note()
                {
                    // enter menus
                    match n.key
                    {
                        Note::A0 => self.menu.open_special_ops(),
                        Note::B0 => self.menu.open_sequencer(),
                        Note::C1 => self.menu.open_program_ports(),
                        Note::D1 => self.menu.open_vibrato(),
                        _ => {}
                    }
                    // entered menu
                    if !self.menu.is_none()
                    {
                        self.output.menu_feedback(MenuFeedback::note_select(Channel::All), time);
                    }
                }
            }
            // in menu
            else
            {
                let exit = self.menu.on_reset_switch();
                if let Some(fb) = exit.1 { self.output.menu_feedback(fb, time); }
                if exit.0 { self.menu.exit(); }
            }
        }
    }
    
    pub fn on_factory_reset(&mut self, nvs: &mut N)
    {
        let mut config = Configuration {
            // other: &mut self.other_config,
            note: &mut self.output.note_config,
            // sequen: &mut self.sequen_config,
            output: &mut self.output.config,
            panel: &mut self.output.panel.config,
            vibrato: &mut self.output.vibrato.config,
            arpeggio: &mut self.arpeggio.config
        };
        
        self.menu.factory_reset(&mut config, nvs);
    }
}
