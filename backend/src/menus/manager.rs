use api::NvsInterface;

use crate::{Configuration, Menu, MenuWrapTrait, ProgramPortsMenu, SequencerMenu, SpecialOpsMenu, VibratoMenu, menus::{MenuWrapper, create_dynamic_menus}};

create_dynamic_menus!(Menus:
    Spec => MenuWrapper<SpecialOpsMenu>,
    PP => MenuWrapper<ProgramPortsMenu>,
    Vib => MenuWrapper<VibratoMenu>,
    Seq => MenuWrapper<SequencerMenu>);

pub struct MenuManager
{
    menu: Menus,
    
    vibrato: Option<VibratoMenu>,
    special_ops: Option<SpecialOpsMenu>,
    program_ports: Option<ProgramPortsMenu>,
    sequencer: Option<SequencerMenu>
}

impl MenuManager
{
    pub fn open_vibrato(&mut self)
    {
        match core::mem::replace(&mut self.menu, Menus::None)
        {
            Menus::Spec(menu_wrapper) => self.special_ops = Some(menu_wrapper.into_menu()),
            Menus::PP(menu_wrapper) => self.program_ports = Some(menu_wrapper.into_menu()),
            Menus::Vib(_) => return,
            Menus::Seq(menu_wrapper) => self.sequencer = Some(menu_wrapper.into_menu()),
            Menus::None => {},
        }
        
        self.menu = self.get_vibrato();
    }
    pub fn open_program_ports(&mut self)
    {
        match core::mem::replace(&mut self.menu, Menus::None)
        {
            Menus::Spec(menu_wrapper) => self.special_ops = Some(menu_wrapper.into_menu()),
            Menus::PP(_) => return,
            Menus::Vib(menu_wrapper) => self.vibrato = Some(menu_wrapper.into_menu()),
            Menus::Seq(menu_wrapper) => self.sequencer = Some(menu_wrapper.into_menu()),
            Menus::None => {},
        }
        
        self.menu = self.get_program_ports();
    }
    pub fn open_special_ops(&mut self)
    {
        match core::mem::replace(&mut self.menu, Menus::None)
        {
            Menus::Spec(_) => return,
            Menus::PP(menu_wrapper) => self.program_ports = Some(menu_wrapper.into_menu()),
            Menus::Vib(menu_wrapper) => self.vibrato = Some(menu_wrapper.into_menu()),
            Menus::Seq(menu_wrapper) => self.sequencer = Some(menu_wrapper.into_menu()),
            Menus::None => {},
        }
        
        self.menu = self.get_special_ops();
    }
    pub fn open_sequencer(&mut self)
    {
        match core::mem::replace(&mut self.menu, Menus::None)
        {
            Menus::Spec(menu_wrapper) => self.special_ops = Some(menu_wrapper.into_menu()),
            Menus::PP(menu_wrapper) => self.program_ports = Some(menu_wrapper.into_menu()),
            Menus::Vib(menu_wrapper) => self.vibrato = Some(menu_wrapper.into_menu()),
            Menus::Seq(_) => return,
            Menus::None => {},
        }
        
        self.menu = self.get_sequencer();
    }
    pub fn exit(&mut self)
    {
        match core::mem::replace(&mut self.menu, Menus::None)
        {
            Menus::Spec(menu_wrapper) => self.special_ops = Some(menu_wrapper.into_menu()),
            Menus::PP(menu_wrapper) => self.program_ports = Some(menu_wrapper.into_menu()),
            Menus::Vib(menu_wrapper) => self.vibrato = Some(menu_wrapper.into_menu()),
            Menus::Seq(menu_wrapper) => self.sequencer = Some(menu_wrapper.into_menu()),
            Menus::None => {},
        }
    }
    
    #[inline]
    #[must_use]
    pub fn is_special_ops(&self) -> bool
    {
        if let Menus::Spec(_) = self.menu
        {
            return true;
        }
        return false;
    }
    #[inline]
    #[must_use]
    pub fn is_sequencer(&mut self) -> Option<&mut SequencerMenu>
    {
        if let Menus::Seq(s) = &mut self.menu
        {
            return Some(&mut s.menu);
        }
        return None;
    }
    #[inline]
    #[must_use]
    pub fn is_some(&self) -> bool
    {
        return !self.menu.is_none();
    }
    #[inline]
    #[must_use]
    pub fn is_none(&self) -> bool
    {
        return self.menu.is_none();
    }
    
    pub fn factory_reset<N: NvsInterface>(&mut self, config: &mut Configuration, nvs: &mut N)
    {
        self.exit();
        
        let menu = self.program_ports.as_mut().unwrap();
        menu.reset_values(config);
        menu.save_values(config, nvs);
        
        let menu = self.sequencer.as_mut().unwrap();
        menu.reset_values(config);
        menu.save_values(config, nvs);
        
        let menu = self.special_ops.as_mut().unwrap();
        menu.reset_values(config);
        menu.save_values(config, nvs);
        
        let menu = self.vibrato.as_mut().unwrap();
        menu.reset_values(config);
        menu.save_values(config, nvs);
    }
    
    fn get_vibrato(&mut self) -> Menus
    {
        return Menus::Vib(MenuWrapper::new(core::mem::replace(&mut self.vibrato, None).unwrap()));
    }
    fn get_special_ops(&mut self) -> Menus
    {
        return Menus::Spec(MenuWrapper::new(core::mem::replace(&mut self.special_ops, None).unwrap()));
    }
    fn get_program_ports(&mut self) -> Menus
    {
        return Menus::PP(MenuWrapper::new(core::mem::replace(&mut self.program_ports, None).unwrap()));
    }
    fn get_sequencer(&mut self) -> Menus
    {
        return Menus::Seq(MenuWrapper::new(core::mem::replace(&mut self.sequencer, None).unwrap()));
    }
}

impl MenuWrapTrait for MenuManager
{
    #[inline]
    fn on_note<T: api::NvsInterface>(&mut self, config: &mut crate::Configuration, nvs: &mut T, time: u32, channel: api::Channel, note: api::Note) -> (bool, Option<super::MenuFeedback>)
    {
        return self.menu.on_note(config, nvs, time, channel, note);
    }
    #[inline]
    fn off_note(&mut self, config: &mut crate::Configuration, channel: api::Channel, note: api::Note)
    {
        self.menu.off_note(config, channel, note);
    }
    #[inline]
    fn on_reset_switch(&mut self) -> (bool, Option<super::MenuFeedback>)
    {
        return self.menu.on_reset_switch();
    }
    #[inline]
    fn on_message<E: api::Externals>(&mut self, output: &mut crate::Output<E>, message: api::MidiCode)
    {
        return self.menu.on_message(output, message);
    }
    #[inline]
    fn allow_message(&self, message: api::MidiCode) -> bool
    {
        return self.menu.allow_message(message);
    }
}
