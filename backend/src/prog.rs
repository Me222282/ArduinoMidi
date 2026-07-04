use api::{MidiCode, Channel, Note};

use crate::{Configuration, MenuFeedback, MenuWrapper, NoteCollection, NoteConfig, OtherConfig, Panel, ProgramPortsMenu, SequencerConfig, SpecialOpsMenu, VibratoMenu, create_dynamic_menus, vibrato_loop};

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
    note_manager: NoteCollection,
    sequen_config: SequencerConfig
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
            output: &mut self.panel.config
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
        
        vibrato_loop(time, &mut self.panel, &self.other_config);
    }
}
