use core::intrinsics;

use api::{CCType, Channel, Note};

use crate::{AttenuationSource, FreqCorrection, Output, VibratoConfig};

struct VCorrData
{
    k: f32,
    key: u8
}

impl VCorrData
{
    fn get_k(&mut self, scale: f32, key: u8) -> f32
    {
        if self.key == key
        {
            return self.k;
        }
        self.key = key;
        
        // let s = scale * 16383.0 / 2048.0;
        let s = scale * 8.0;
        let n = key as f32 / 24.0;
        
        let _1 = intrinsics::powf32(2.0, n);
        const _2: f32 = trig_const::exp(trig_const::ln(2.0) * Note::C4 as f64 * 2.0 / 24.0) as f32;
        let _3 = intrinsics::powf32(2.0, s) - 1.0;
        
        let k =(intrinsics::log2f32(_1 + _2 * _3) - n) / s;
        self.k = k;
        return k;
    }
}

pub struct VibratoOp
{
    pub config: VibratoConfig,
    attenuations: [u16; 16],
    
    vibrato_scales: [VCorrData; 16]
}

impl VibratoOp
{   
    pub fn on_loop(&mut self, time: u32, output: &mut Output)
    {
        let time = time as f32;
        let mut pb_offsets: [i16; 16] = [0; 16];
        
        if self.config.global_vibrato
        {
            let offset = (self.config.vibratos[0].function)(time * self.config.vibratos[0].angular_velocity);
            for (i, (v, pb)) in self.config.vibratos.iter().zip(&mut pb_offsets).enumerate()
            {
                if !v.enabled { continue; }
                let atten = match v.attenuation == AttenuationSource::None
                {
                    true => 0x3FFF,
                    false => self.attenuations[i],
                };
                
                let nv = offset * atten as f32 * v.scale;
                // frequency correction done separately
                *pb = nv as i16;
            }
            return;
        }
        else
        {
            for (i, (v, pb)) in self.config.vibratos.iter().zip(&mut pb_offsets).enumerate()
            {
                if !v.enabled { continue; }
                let offset = (self.config.vibratos[i].function)(time * self.config.vibratos[i].angular_velocity);
                let atten = match v.attenuation == AttenuationSource::None
                {
                    true => 0x3FFF,
                    false => self.attenuations[i],
                };
                
                let nv = offset * atten as f32 * v.scale;
                // frequency correction done separately
                *pb = nv as i16;
            }
        }
        
        output.panel.set_pb_offsets(self, &pb_offsets);
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
    
    pub fn on_reset_switch(&mut self)
    {
        // So that when vibrato scale changes, all k values must be recalculated
        // Happends when exiting the menu
        for vs in &mut self.vibrato_scales
        {
            vs.key = 0;
        }
    }
    
    /// `key` is the note value used by the hardware
    /// `offset` is the currently calculated offset for `channel`
    pub fn frequency_correction(&mut self, key: u8, channel: Channel, offset: isize) -> isize
    {
        let vib = &self.config.vibratos[channel as usize];
        
        match vib.freq_correction
        {
            FreqCorrection::None => return offset,
            FreqCorrection::Half =>
            {
                let k = self.vibrato_scales[channel as usize].get_k(vib.scale, key);
                let k = (k + 1.0) / 2.0;
                return (offset as f32 * k) as isize;
            },
            FreqCorrection::Full =>
            {
                let k = self.vibrato_scales[channel as usize].get_k(vib.scale, key);
                return (offset as f32 * k) as isize;
            }
        }
    }
}