use core::marker::PhantomData;

use api::{Channel, MidiCode, Note, NvsInterface, Switch};

use crate::{Arpeggiator, Configuration, MenuFeedback, MenuStorage, MenuWrapTrait, MenuWrapper, Output, ProgramPortsMenu, SequencerMenu, SpecialOpsMenu, VibratoMenu, create_dynamic_menus};

create_dynamic_menus!(pub(crate) Menus:
    A => MenuWrapper<SpecialOpsMenu>,
    B => MenuWrapper<ProgramPortsMenu>,
    C => MenuWrapper<VibratoMenu>,
    D => MenuWrapper<SequencerMenu>);

pub struct Program<E: api::Externals, N: NvsInterface>
{
    menu: Menus,
    menu_storage: MenuStorage,
    
    // sequen_config: SequencerConfig,
    arpeggio: Arpeggiator,
    pub output: Output<E>,
    _phantom_n: PhantomData<N>
}

impl<E: api::Externals, N: NvsInterface> Program<E, N>
{
    fn set_menu(&mut self, menu: Menus)
    {
        if self.menu.is_none()
        {
            self.menu = menu;
            return;
        }
        
        let old = core::mem::replace(&mut self.menu, menu);
        match old
        {
            Menus::A(mw) => self.menu_storage.set_special_ops(mw.into_menu()),
            Menus::B(mw) => self.menu_storage.set_program_ports(mw.into_menu()),
            Menus::C(mw) => self.menu_storage.set_vibrato(mw.into_menu()),
            Menus::D(mw) => self.menu_storage.set_sequencer(mw.into_menu()),
            Menus::None => {}
        }
    }
    
    pub fn on_midi_message(&mut self, nvs: &mut N, message: MidiCode, time: u32)
    {
        // ignore all messages from disabled channels
        let channel = message.get_channel();
        if channel != Channel::All && !self.output.config.channel_filters[channel as usize].enabled { return; }
        
        if !self.menu.allow_message(message) { return; }
        
        match message
        {
            MidiCode::NoteON(channel, note) =>
            {
                if self.menu.is_none()
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
                    if let Some(fb) = exit.1 { self.output.menu_feedback(fb, time); }
                    // exit menu
                    if exit.0 { self.set_menu(Menus::None); }
                }
                else
                {
                    self.arpeggio.on_note(&mut self.output, channel, note);
                }
            },
            MidiCode::NoteOFF(channel, note) =>
            {
                if self.menu.is_none()
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
                    self.menu.off_note(&mut config, channel, note);
                }
                else
                {
                    self.arpeggio.off_note(&mut self.output, channel, note);
                }
            },
            MidiCode::ControlChange(channel, cctype, value) => self.output.on_cc(channel, cctype, value),
            MidiCode::PitchWheel(channel, value) => self.output.on_pitch_bend(channel, value),
            
            // everything else should go to menu
            MidiCode::TimingClock =>
            {
                self.arpeggio.on_clock(&mut self.output);
                self.menu.on_message(message);
            }
            _ => self.menu.on_message(message)
        }
    }
    
    pub fn on_loop(&mut self, time: u32)
    {
        self.output.on_loop(time);
        self.arpeggio.on_loop(time, &mut self.output);
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
                        Note::A0 => self.menu = Menus::A(MenuWrapper::new(self.menu_storage.get_special_ops())),
                        Note::B0 => self.menu = Menus::D(MenuWrapper::new(self.menu_storage.get_sequencer())),
                        Note::C1 => self.menu = Menus::B(MenuWrapper::new(self.menu_storage.get_program_ports())),
                        Note::D1 => self.menu = Menus::C(MenuWrapper::new(self.menu_storage.get_vibrato())),
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
                if exit.0 { self.set_menu(Menus::None); }
            }
        }
    }
}
