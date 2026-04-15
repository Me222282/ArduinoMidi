use api::{MidiCode, Channel, Note};

use crate::{Panel, InputListener, MenuWrapper, ProgramPortsMenu, SpecialOpsMenu, VibratoMenu, create_dynamic_input_listener};

create_dynamic_input_listener!(pub Listeners:
    A => MenuWrapper<SpecialOpsMenu>,
    B => MenuWrapper<ProgramPortsMenu>,
    C => MenuWrapper<VibratoMenu>);

pub fn on_midi_message(listener: &mut Listeners, panel: &mut Panel, message: MidiCode)
{
    if !listener.allow_message(message)
    {
        return;
    }
    
    match message
    {
        MidiCode::NoteOFF(channel, note) => listener.off_note(channel, note),
        MidiCode::NoteON(channel, note) =>
        {
            // exit
            if listener.on_note(panel, channel, note)
            {
                *listener = Listeners::None;
            }
        },
        MidiCode::ControlChange(channel, cctype, _) => todo!(),
        MidiCode::PitchWheel(channel, _) => todo!(),
        _ => listener.on_message(message)
    }
}