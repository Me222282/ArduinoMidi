use api::{CCType, Channel};

use crate::{AttenuationSource, VibratoConfig};

pub struct VibratoOp
{
    pub config: VibratoConfig,
    attenuations: [u16; 16]
}

impl VibratoOp
{   
    pub fn on_loop(&mut self, time: usize, pb_offsets: &mut [i16; 16])
    {
        let time = time as f32;
        
        if self.config.global_vibrato
        {
            let offset = (self.config.vibratos[0].function)(time * self.config.vibratos[0].angular_velocity);
            for (i, v) in self.config.vibratos.iter().enumerate()
            {
                if !v.enabled { continue; }
                let atten = match v.attenuation == AttenuationSource::None
                {
                    true => 0x3FFF,
                    false => self.attenuations[i],
                };
                
                let nv = offset * atten as f32 * v.scale;
                pb_offsets[i] = nv as i16;
            }
            return;
        }
        
        for (i, v) in self.config.vibratos.iter().enumerate()
        {
            if !v.enabled { continue; }
            let offset = (self.config.vibratos[i].function)(time * self.config.vibratos[i].angular_velocity);
            let atten = match v.attenuation == AttenuationSource::None
            {
                true => 0x3FFF,
                false => self.attenuations[i],
            };
            
            let nv = offset * atten as f32 * v.scale;
            pb_offsets[i] = nv as i16;
        }
    }
    
    pub fn on_cc(&mut self, cc: CCType, channel: Channel, value: u8)
    {
        if self.config.vibratos[channel as usize].attenuation == AttenuationSource::CC(cc)
        {
            // 7 bit to 14 bit
            self.attenuations[channel as usize] = (value as u16) << 7;
        }
    }
    
    pub fn on_modulation(&mut self, channel: Channel, value: u16)
    {
        if self.config.vibratos[channel as usize].attenuation == AttenuationSource::Modulation
        {
            self.attenuations[channel as usize] = value;
        }
    }
}