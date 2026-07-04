use api::{MidiCode, Channel, Note};

use crate::{MenuFeedback, MenuWrapper, Panel, ProgramPortsMenu, SpecialOpsMenu, VibratoMenu, create_dynamic_menus};

create_dynamic_menus!(pub Menus:
    A => MenuWrapper<SpecialOpsMenu>,
    B => MenuWrapper<ProgramPortsMenu>,
    C => MenuWrapper<VibratoMenu>);

pub struct Program
{
    menu: Menus,
    panel: Panel,
    
}

impl Program
{
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
        
    }
}
