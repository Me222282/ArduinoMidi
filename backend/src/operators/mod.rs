use api::Channel;

use crate::{OtherConfig, Panel};

pub fn vibrato_loop(time: usize, panel: &mut Panel, config: &OtherConfig)
{
    let time = time as f32;
    
    if config.global_vibrato
    {
        let offset = (config.vibratos[0].function)(time * config.vibratos[0].angular_velocity);
        for (i, v) in config.vibratos.iter().enumerate()
        {
            if !v.enabled { continue; }
            let nv = offset * /* mod */ v.scale;
            panel.set_pb_offset(Channel::from_u8(i as u8), nv as i16);
        }
        return;
    }
    
    for (i, v) in config.vibratos.iter().enumerate()
    {
        if !v.enabled { continue; }
        let offset = (config.vibratos[i].function)(time * config.vibratos[i].angular_velocity);
        let nv = offset * /* mod */ v.scale;
        panel.set_pb_offset(Channel::from_u8(i as u8), nv as i16);
    }
}