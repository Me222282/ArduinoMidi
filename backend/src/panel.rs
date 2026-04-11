use api::{CCType, Gate, Note, Externals, PanelState};

use crate::{Configuration, TriggerSource};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SlotSelect
{
    ChannelVoice(u8, u8),
    Channel(u8),
    Voice(u8),
    Index(u8),
    All
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VelFunc
{
    Velocity,
    Modulation,
    CC(CCType),
    Trigger(TriggerSource)
}

pub struct Panel
{
    externals: Externals,
    pub state: PanelState,
    pub configuration: Configuration,
    pub slot_allocations: [(u8, u8); 5],
    pub vel_functions: [VelFunc; 5],
    vibrato_values: [i16; 16],
    pdvs: [u16; 16]
}

impl Panel
{
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
            },
            SlotSelect::All =>
            {
                for i in 0..5
                {
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                }
                result = Gate::all_on();
            }
        }
        
        return result;
    }
    pub fn output_modulation(&self, channel: u8, value: u16)
    {
        for (i, (&com, vf)) in self.slot_allocations.iter().zip(self.vel_functions).enumerate()
        {
            if com.0 != channel { continue; }
            
            if vf == VelFunc::Modulation
            {
                // 14 bit to 8 bit
                (self.externals.set_vel)(i, (value >> 6) as u8);
            }
        }
        
        if !self.state.modulation && channel == 0
        {
            // 14 bit to 12 bit
            (self.externals.set_mod)(value >> 2)
        }
    }
    pub fn output_control_change(&self, slots: SlotSelect, cc: CCType, value: u8)
    {
        match slots
        {
            SlotSelect::ChannelVoice(c, v) =>
            {
                for (i, (&com, vf)) in self.slot_allocations.iter().zip(self.vel_functions).enumerate()
                {
                    if com != (c, v) { continue; }
                    
                    if vf == VelFunc::CC(cc)
                    {
                        // 7 bit to 8 bit
                        (self.externals.set_vel)(i, value << 1);
                    }
                }
            },
            SlotSelect::Channel(c) =>
            {
                for (i, (&com, vf)) in self.slot_allocations.iter().zip(self.vel_functions).enumerate()
                {
                    if com.0 != c { continue; }
                    
                    if vf == VelFunc::CC(cc)
                    {
                        // 7 bit to 8 bit
                        (self.externals.set_vel)(i, value << 1);
                    }
                }
            },
            SlotSelect::Voice(v) =>
            {
                for (i, (&com, vf)) in self.slot_allocations.iter().zip(self.vel_functions).enumerate()
                {
                    if com.1 != v { continue; }
                    
                    if vf == VelFunc::CC(cc)
                    {
                        // 7 bit to 8 bit
                        (self.externals.set_vel)(i, value << 1);
                    }
                }
            },
            SlotSelect::Index(i) =>
            {
                let i = i as usize;
                unsafe
                {
                    if self.vel_functions.get_unchecked(i) == &VelFunc::CC(cc)
                    {
                        // 7 bit to 8 bit
                        (self.externals.set_vel)(i, value << 1);
                    }
                }
            },
            SlotSelect::All =>
            {
                for (i, vf) in (0..5).zip(self.vel_functions)
                {
                    if vf == VelFunc::CC(cc)
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
        for (i, &vf) in self.vel_functions.iter().enumerate()
        {
            if vf == VelFunc::Trigger(source)
            {
                (self.externals.set_vel)(i, v);
            }
        }
    }
    pub fn set_pitch_bend(&mut self, channel: u8, value: u16)
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
    pub fn set_pb_offset(&mut self, channel: u8, value: i16)
    {
        unsafe
        {
            *self.vibrato_values.get_unchecked_mut(channel as usize) = value;
        }
        let pb_value = unsafe {
            *self.pdvs.get_unchecked(channel as usize)
        };
        
        // 14 bit to 12 bit
        let nv = (pb_value >> 2) as isize + value as isize;
        let nv = nv.clamp(0, 0xFFF) as u16;
        
        for (i, &com) in self.slot_allocations.iter().enumerate()
        {
            if com.0 != channel { continue; }
            
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
    pub fn get_time(&self) -> usize
    {
        return (self.externals.get_time)();
    }
    
    #[inline]
    fn set_note(&self, slot: usize, key: u8)
    {
        match self.configuration.micro_tone
        {
            true => (self.externals.set_note)(slot, key),
            false => (self.externals.set_note)(slot, key << 1)
        }
    }
    fn set_vel(&self, slot: usize, value: u8)
    {
        unsafe
        {
            if self.vel_functions.get_unchecked(slot) == &VelFunc::Velocity
            {
                // 7 bit to 8 bit
                (self.externals.set_vel)(slot, value << 1);
            }
        }
        
        if self.state.modulation && slot == 0
        {
            // 7 bit to 12 bit
            (self.externals.set_mod)((value as u16) << 5)
        }
    }
}