use api::{CCType, Channel, Externals, Gate, Note, PanelState};

use crate::{OutputConfig, TriggerSource};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SlotSelect
{
    ChannelVoice(Channel, u8),
    Channel(Channel),
    Voice(u8),
    Index(u8),
    All
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VelFunc
{
    Velocity,
    Modulation,
    CC(CCType, Channel),
    Trigger(TriggerSource)
}

pub struct Panel
{
    externals: Externals,
    pub state: PanelState,
    // pub configuration: Configuration,
    pub slot_allocations: [(Channel, u8); 5],
    // pub vel_functions: [VelFunc; 5],
    vibrato_values: [i16; 16],
    pdvs: [u16; 16],
    pub config: OutputConfig
}

impl Panel
{
    // OUTPUT PRIORITY:
    // Trigger, CC, Modulation, Velocity
    // #[inline]
    // fn vel_function(&self, slot: usize) -> VelFunc
    // {
    //     unsafe
    //     {
    //         if *self.config.trig_enabled.get_unchecked(slot)
    //         {
    //             return VelFunc::Trigger(*self.config.triggers.get_unchecked(slot));
    //         }
    //         if *self.config.cc_enabled.get_unchecked(slot)
    //         {
    //             let d = *self.config.cc_sources.get_unchecked(slot);
    //             return VelFunc::CC(d.0, d.1);
    //         }
    //         if self.state.modulation
    //         {
    //             return VelFunc::Modulation;
    //         }
            
    //         return VelFunc::Velocity;
    //     }
    // }
    fn is_vf_mod(&self, slot: usize) -> bool
    {
        unsafe
        {
            return !self.config.trig_enabled.get_unchecked(slot) &&
                !self.config.cc_enabled.get_unchecked(slot) &&
                self.state.modulation;
        }
    }
    fn is_vf_velocity(&self, slot: usize) -> bool
    {
        unsafe
        {
            return !self.config.trig_enabled.get_unchecked(slot) &&
                !self.config.cc_enabled.get_unchecked(slot) &&
                !self.state.modulation;
        }
    }
    fn is_vf_cc(&self, slot: usize, cc: CCType, channel: Channel) -> bool
    {
        unsafe
        {
            return !self.config.trig_enabled.get_unchecked(slot) &&
                *self.config.cc_enabled.get_unchecked(slot) &&
                self.config.cc_sources.get_unchecked(slot) == &(cc, channel);
        }
    }
    fn is_vf_trigger(&self, slot: usize, source: TriggerSource) -> bool
    {
        unsafe
        {
            return *self.config.trig_enabled.get_unchecked(slot) &&
                self.config.triggers.get_unchecked(slot) == &source;
        }
    }
    
    pub fn output_note(&self, slots: SlotSelect, note: Note) -> Gate
    {
        let mut result = Gate::zero();
        
        match slots
        {
            SlotSelect::ChannelVoice(c, v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com != (c, v) { continue; }
                    
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                    result.on(i as u8);
                }
            },
            SlotSelect::Channel(Channel::All) | 
            SlotSelect::All =>
            {
                for i in 0..5
                {
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                }
                result = Gate::all_on();
            },
            SlotSelect::Channel(c) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.0 != c { continue; }
                    
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                    result.on(i as u8);
                }
            },
            SlotSelect::Voice(v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.1 != v { continue; }
                    
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                    result.on(i as u8);
                }
            },
            SlotSelect::Index(i) =>
            {
                result.on(i);
                let i = i as usize;
                self.set_note(i, note.key);
                self.set_vel(i, note.velocity);
            }
        }
        
        return result;
    }
    pub fn output_modulation(&self, channel: Channel, value: u16)
    {
        for (i, &com) in self.slot_allocations.iter().enumerate()
        {
            if com.0 != channel { continue; }
            
            if self.is_vf_mod(i)
            {
                // 14 bit to 8 bit
                (self.externals.set_vel)(i, (value >> 6) as u8);
            }
        }
        
        if !self.state.modulation && channel == Channel::C1
        {
            // 14 bit to 12 bit
            (self.externals.set_mod)(value >> 2)
        }
    }
    pub fn output_control_change(&self, slots: SlotSelect, cc: CCType, channel: Channel, value: u8)
    {
        match slots
        {
            SlotSelect::ChannelVoice(c, v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com != (c, v) { continue; }
                    
                    if self.is_vf_cc(i, cc, channel)
                    {
                        // 7 bit to 8 bit
                        (self.externals.set_vel)(i, value << 1);
                    }
                }
            },
            SlotSelect::Channel(c) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.0 != c { continue; }
                    
                    if self.is_vf_cc(i, cc, channel)
                    {
                        // 7 bit to 8 bit
                        (self.externals.set_vel)(i, value << 1);
                    }
                }
            },
            SlotSelect::Voice(v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.1 != v { continue; }
                    
                    if self.is_vf_cc(i, cc, channel)
                    {
                        // 7 bit to 8 bit
                        (self.externals.set_vel)(i, value << 1);
                    }
                }
            },
            SlotSelect::Index(i) =>
            {
                let i = i as usize;
                if self.is_vf_cc(i, cc, channel)
                {
                    // 7 bit to 8 bit
                    (self.externals.set_vel)(i, value << 1);
                }
            },
            SlotSelect::All =>
            {
                for i in 0..5
                {
                    if self.is_vf_cc(i, cc, channel)
                    {
                        // 7 bit to 8 bit
                        (self.externals.set_vel)(i, value << 1);
                    }
                }
            }
        }
    }
    pub fn output_trigger(&self, source: TriggerSource, value: bool)
    {
        let v: u8 = match value
        {
            true => 0xFF,
            false => 0x00,
        };
        for i in 0..5
        {
            if self.is_vf_trigger(i, source)
            {
                (self.externals.set_vel)(i, v);
            }
        }
    }
    pub fn set_pitch_bend(&mut self, channel: Channel, value: u16)
    {
        unsafe
        {
            *self.pdvs.get_unchecked_mut(channel as usize) = value;
        }
        let offset = unsafe {
            *self.vibrato_values.get_unchecked(channel as usize)
        };
        
        // 14 bit to 12 bit
        let nv = (value >> 2) as isize + offset as isize;
        let nv = nv.clamp(0, 0xFFF) as u16;
        
        for (i, &com) in self.slot_allocations.iter().enumerate()
        {
            if com.0 != channel { continue; }
            
            (self.externals.set_pitch_bend)(i, nv);
        }
    }
    // pub fn set_pb_offset(&mut self, channel: Channel, value: i16)
    // {
    //     unsafe
    //     {
    //         *self.vibrato_values.get_unchecked_mut(channel as usize) = value;
    //     }
    //     let pb_value = unsafe {
    //         *self.pdvs.get_unchecked(channel as usize)
    //     };
        
    //     // 14 bit to 12 bit
    //     let nv = (pb_value >> 2) as isize + value as isize;
    //     let nv = nv.clamp(0, 0xFFF) as u16;
        
    //     for (i, &com) in self.slot_allocations.iter().enumerate()
    //     {
    //         if com.0 != channel { continue; }
            
    //         (self.externals.set_pitch_bend)(i, nv);
    //     }
    // }
    pub fn set_pf_offsets(&mut self, values: &[i16; 16])
    {
        self.vibrato_values.copy_from_slice(values);
        
        for (i, &(channel, _)) in self.slot_allocations.iter().enumerate()
        {
            let pb_value = unsafe {
                *self.pdvs.get_unchecked(channel as usize)
            };
            let offset = unsafe {
                *values.get_unchecked(channel as usize)
            };
            
            // 14 bit to 12 bit
            let nv = (pb_value >> 2) as isize + offset as isize;
            let nv = nv.clamp(0, 0xFFF) as u16;
            
            (self.externals.set_pitch_bend)(i, nv);
        }
    }
    
    // pub fn output_gate(&self, slots: SlotSelect)
    // {
    //     let mut value = Gate::zero();
        
    //     match slots
    //     {
    //         SlotSelect::ChannelVoice(c, v) =>
    //         {
    //             for (i, &com) in self.slot_allocations.iter().enumerate()
    //             {
    //                 if com != (c, v) { continue; }
                    
    //                 value.on(i as u8);
    //             }
    //         },
    //         SlotSelect::Channel(c) =>
    //         {
    //             for (i, &com) in self.slot_allocations.iter().enumerate()
    //             {
    //                 if com.0 != c { continue; }
                    
    //                 value.on(i as u8);
    //             }
    //         },
    //         SlotSelect::Voice(v) =>
    //         {
    //             for (i, &com) in self.slot_allocations.iter().enumerate()
    //             {
    //                 if com.1 != v { continue; }
                    
    //                 value.on(i as u8);
    //             }
    //         },
    //         SlotSelect::Index(i) =>
    //         {
    //             value.on(i);
    //         },
    //         SlotSelect::All =>
    //         {
    //             value = Gate::all_on();
    //         }
    //     }
        
    //     (self.externals.set_gate)(value);
    // }
    #[inline]
    pub fn output_gate(&self, value: Gate)
    {
        (self.externals.set_gate)(value);
    }
    #[inline]
    pub fn delay(&self, value: u32)
    {
        (self.externals.delay)(value);
    }
    
    #[inline]
    fn set_note(&self, slot: usize, key: u8)
    {
        match self.config.micro_tone
        {
            true => (self.externals.set_note)(slot, key),
            false => (self.externals.set_note)(slot, key << 1)
        }
    }
    fn set_vel(&self, slot: usize, value: u8)
    {
        if self.is_vf_velocity(slot)
        {
            // 7 bit to 8 bit
            (self.externals.set_vel)(slot, value << 1);
        }
        
        if self.state.modulation && slot == 0
        {
            // 7 bit to 12 bit
            (self.externals.set_mod)((value as u16) << 5)
        }
    }
    
    // pub fn toggle_trigger(&mut self, slot: usize) -> bool
    // {
    //     let vs = &mut self.vel_functions[slot];
    //     if let VelFunc::Trigger(_) = vs
    //     {
    //         if self.config.cc_enabled[slot]
    //         {
    //             let cc = self.config.cc_sources[slot];
    //             self.vel_functions[slot] = VelFunc::CC(cc.0, cc.1);
    //         }
    //         else if self.state.modulation
    //         {
    //             self.vel_functions[slot] = VelFunc::Modulation;
    //         }
    //         else
    //         {
    //             self.vel_functions[slot] = VelFunc::Velocity;
    //         }
    //         return false;
    //     }
        
    //     *vs = VelFunc::Trigger(self.config.triggers[slot]);
    //     return true;
    // }
    // // set source without channel
    // pub fn set_trigger(&mut self, slot: usize, mut source: TriggerSource)
    // {
    //     let ts = &mut self.config.triggers[slot];
    //     source.set_channel(ts.get_channel());
        
    //     *ts = source;
    //     if let VelFunc::Trigger(s) = &mut self.vel_functions[slot]
    //     {
    //         *s = source;
    //     }
    // }
    // pub fn set_trigger_channel(&mut self, slot: usize, channel: Channel)
    // {
    //     let trig = &mut self.config.triggers[slot];
    //     trig.set_channel(channel);
    //     if let VelFunc::Trigger(s) = &mut self.vel_functions[slot]
    //     {
    //         *s = *trig;
    //     }
    // }
    // pub fn toggle_cc(&mut self, slot: usize) -> bool
    // {
    //     let enabled = !self.config.cc_enabled[slot];
    //     self.config.cc_enabled[slot] = enabled;
    //     if enabled
    //     {
    //         if let VelFunc::Trigger(_) = self.vel_functions[slot]
    //         {
    //             return enabled;
    //         }
    //         let cc = self.config.cc_sources[slot];
    //         self.vel_functions[slot] = VelFunc::CC(cc.0, cc.1);
    //     }
    //     else if self.state.modulation
    //     {
    //         self.vel_functions[slot] = VelFunc::Modulation;
    //     }
    //     else
    //     {
    //         self.vel_functions[slot] = VelFunc::Velocity;
    //     }
        
    //     return enabled;
    // }
    // pub fn set_cc(&mut self, slot: usize, source: CCType, channel: Channel)
    // {
    //     self.config.cc_sources[slot] = (source, channel);
    //     if let VelFunc::CC(s, c) = &mut self.vel_functions[slot]
    //     {
    //         *s = source;
    //         *c = channel;
    //     }
    // }
}