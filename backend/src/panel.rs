use api::{CCType, Channel, Externals, Gate, Note, PanelState};

use crate::{PanelConfig, TriggerSource, VibratoOp};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SlotSelect
{
    ChannelVoice(Channel, u8),
    Channel(Channel),
    Voice(u8),
    Index(u8),
    All
}

// #[derive(Debug, Clone, Copy, PartialEq, Eq)]
// enum VelFunc
// {
//     Velocity,
//     Modulation,
//     CC(CCType, Channel),
//     Trigger(TriggerSource)
// }

pub struct Panel
{
    externals: Externals,
    pub state: PanelState,
    // pub configuration: Configuration,
    pub slot_allocations: [(Channel, u8); 5],
    // pub vel_functions: [VelFunc; 5],
    vibrato_values: [i16; 16],
    pdvs: [u16; 16],
    vels: [u8; 5],
    gate: Gate,
    pub config: PanelConfig
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
    #[inline]
    fn is_vf_mod(&self, slot: usize) -> bool
    {
        unsafe
        {
            return !self.config.trig_enabled.get_unchecked(slot) &&
                !self.config.cc_enabled.get_unchecked(slot) &&
                self.state.modulation;
        }
    }
    #[inline]
    fn is_vf_velocity(&self, slot: usize) -> bool
    {
        unsafe
        {
            return !self.config.trig_enabled.get_unchecked(slot) &&
                !self.config.cc_enabled.get_unchecked(slot) &&
                !self.state.modulation;
        }
    }
    fn get_vf_cc_channel(&self, slot: usize, cc: CCType) -> Option<Channel>
    {
        unsafe
        {
            if *self.config.trig_enabled.get_unchecked(slot) ||
                !self.config.cc_enabled.get_unchecked(slot)
            {
                return None;
            }
            
            let src = *self.config.cc_sources.get_unchecked(slot);
            if src.0 != cc
            {
                return None;
            }
            
            return Some(src.1);
        }
    }
    #[inline]
    fn is_vf_trigger(&self, slot: usize, source: TriggerSource) -> bool
    {
        unsafe
        {
            return *self.config.trig_enabled.get_unchecked(slot) &&
                self.config.triggers.get_unchecked(slot) == &source;
        }
    }
    
    pub fn output_note(&mut self, slots: SlotSelect, mut note: Note)// -> Gate
    {
        // let mut result = Gate::zero();
        note.key = shift_note(note.key, self.state.octave);
        
        match slots
        {
            SlotSelect::ChannelVoice(c, v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com != (c, v) { continue; }
                    
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                    self.vels[i] = note.velocity;
                    // result.on(i as u8);
                }
            },
            SlotSelect::Channel(Channel::All) | 
            SlotSelect::All =>
            {
                for i in 0..5
                {
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                    self.vels[i] = note.velocity;
                }
                // result = Gate::all_on();
            },
            SlotSelect::Channel(c) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.0 != c { continue; }
                    
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                    self.vels[i] = note.velocity;
                    // result.on(i as u8);
                }
            },
            SlotSelect::Voice(v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.1 != v { continue; }
                    
                    self.set_note(i, note.key);
                    self.set_vel(i, note.velocity);
                    self.vels[i] = note.velocity;
                    // result.on(i as u8);
                }
            },
            SlotSelect::Index(i) =>
            {
                // result.on(i);
                let i = i as usize;
                self.set_note(i, note.key);
                self.set_vel(i, note.velocity);
                self.vels[i] = note.velocity;
            }
        }
        
        // return result;
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
    pub fn output_control_change(&self, cc: CCType, channel: Channel, value: u8)
    {
        if self.config.per_channel_cc
        {
            // counts the number of slots with this channel
            let mut i = 0;
            for (slot, &com) in self.slot_allocations.iter().enumerate()
            {
                if com.0 != channel { continue; }
                
                if self.get_vf_cc_channel(i, cc).is_some()
                {
                    // 7 bit to 8 bit
                    (self.externals.set_vel)(slot, value << 1);
                }
                
                i += 1;
            }
            return;
        }
        
        for i in 0..5
        {
            if self.get_vf_cc_channel(i, cc) == Some(channel)
            {
                // 7 bit to 8 bit
                (self.externals.set_vel)(i, value << 1);
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
    
    #[inline]
    fn calculate_pb(&self, vibrato: &mut VibratoOp, pb: isize, offset: isize, channel: Channel, slot: usize) -> u16
    {
        if offset == 0
        {
            // 14 bit to 12 bit
            return (pb >> 2) as u16;
        }
        
        let key = (self.externals.get_note)(slot);
        let offset = vibrato.frequency_correction(key, channel, offset);
        
        // 14 bit to 12 bit
        let nv = (pb >> 2) + offset;
        return nv.clamp(0, 0xFFF) as u16;
    }
    pub fn set_pitch_bend(&mut self, vibrato: &mut VibratoOp, channel: Channel, mut value: u16)
    {
        // pitch bend select switch
        let vf = (value as i16 - 2048) as f32;
        value = ((vf * PB_DIV[self.state.pitch_bend as usize]) as i16 + 2048) as u16;
        
        unsafe
        {
            *self.pdvs.get_unchecked_mut(channel as usize) = value;
        }
        let offset = unsafe {
            *self.vibrato_values.get_unchecked(channel as usize)
        };
        
        for (i, &com) in self.slot_allocations.iter().enumerate()
        {
            if com.0 != channel { continue; }
            
            let nv = self.calculate_pb(vibrato, value as isize, offset as isize, channel, i);
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
    pub fn set_pb_offsets(&mut self, vibrato: &mut VibratoOp, values: &[i16; 16])
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
            
            let nv = self.calculate_pb(vibrato, pb_value as isize, offset as isize, channel, i);
            (self.externals.set_pitch_bend)(i, nv);
        }
    }
    
    #[inline]
    pub fn output_gate_on(&mut self, slots: SlotSelect)
    {
        self.output_gate_on_base(slots, self.gate);
    }
    pub fn output_gate_on_base(&mut self, slots: SlotSelect, base: Gate)
    {
        let mut value = base;
        
        match slots
        {
            SlotSelect::ChannelVoice(c, v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com != (c, v) { continue; }
                    
                    value.on(i as u8);
                }
            },
            SlotSelect::Channel(Channel::All) | 
            SlotSelect::All =>
            {
                value = Gate::all_on();
            },
            SlotSelect::Channel(c) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.0 != c { continue; }
                    
                    value.on(i as u8);
                }
            },
            SlotSelect::Voice(v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.1 != v { continue; }
                    
                    value.on(i as u8);
                }
            },
            SlotSelect::Index(i) =>
            {
                value.on(i);
            }
        }
        
        self.gate = value;
        (self.externals.set_gate)(value);
    }
    #[inline]
    pub fn output_gate_off(&mut self, slots: SlotSelect)
    {
        self.output_gate_off_base(slots, self.gate);
    }
    pub fn output_gate_off_base(&mut self, slots: SlotSelect, base: Gate)
    {
        let mut value = base;
        
        match slots
        {
            SlotSelect::ChannelVoice(c, v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com != (c, v) { continue; }
                    
                    value.off(i as u8);
                }
            },
            SlotSelect::Channel(c) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.0 != c { continue; }
                    
                    value.off(i as u8);
                }
            },
            SlotSelect::Voice(v) =>
            {
                for (i, &com) in self.slot_allocations.iter().enumerate()
                {
                    if com.1 != v { continue; }
                    
                    value.off(i as u8);
                }
            },
            SlotSelect::Index(i) =>
            {
                value.off(i);
            },
            SlotSelect::All =>
            {
                value = Gate::zero();
            }
        }
        
        self.gate = value;
        (self.externals.set_gate)(value);
    }
    // #[inline]
    // pub fn output_gate(&self, value: Gate)
    // {
    //     (self.externals.set_gate)(value);
    // }
    #[inline]
    pub fn delay(&self, value: u32)
    {
        (self.externals.delay)(value);
    }
    
    #[inline]
    fn set_note(&self, slot: usize, key: u8)
    {
        let key = match self.config.micro_tone
        {
            true => key,
            false => key << 1
        };
        
        (self.externals.set_note)(slot, key);
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
    
    pub fn update_notes(&self)
    {
        for i in 0..5
        {
            let key = (self.externals.get_note)(i);
            self.set_note(i, shift_note(key, self.state.octave));
        }
    }
    pub fn update_vels(&self)
    {
        for i in 0..5
        {
            self.set_vel(i, self.vels[i]);
        }
    }
    
    /// Returns cd ordered by channel
    pub fn update_slot_allocations(&mut self, cd: &mut [(Channel, u8); 5]) -> u8
    {
        if self.config.use_custom_allocations
        {
            self.slot_allocations = self.config.custom_allocations;
            
            return determine_channel_data(&self.slot_allocations, cd);
        }
        
        let c = self.state.channels;
        let v = self.state.voices;
        if self.config.alternate_allocations
        {
            if c == 0
            {
                if v == 1
                {
                    self.slot_allocations = SS15;
                    return determine_channel_data(&self.slot_allocations, cd);
                }
                else if v == 2
                {
                    self.slot_allocations = SS16;
                    return determine_channel_data(&self.slot_allocations, cd);
                }
                else if v == 3
                {
                    self.slot_allocations = SS17;
                    return determine_channel_data(&self.slot_allocations, cd);
                }
            }
            else if c == 1 && v == 1
            {
                self.slot_allocations = SS18;
                return determine_channel_data(&self.slot_allocations, cd);
            }
        }
        
        // default to normal
        self.slot_allocations = *SLOT_SETTINGS[c as usize - 1][v as usize - 1];
        return determine_channel_data(&self.slot_allocations, cd);
    }
}

fn shift_note(key: u8, octave: i8) -> u8
{
    let nk = key as isize + octave as isize * 12;
    return nk.clamp(0, 127) as u8;
}

/// Returns cd ordered by channel
fn determine_channel_data(slot_alloc: &[(Channel, u8); 5], cd: &mut [(Channel, u8); 5]) -> u8
{
    let mut ai = 0;
    let mut table = [0; 16];
    
    for &(c, v) in slot_alloc
    {
        let v = v + 1;
        let current = table[c as usize];
        if v > current
        {
            table[c as usize] = v;
        }
    }
    
    for (i, v) in table.into_iter().enumerate()
    {
        if v > 0
        {
            cd[ai] = (Channel::from_u8(i as u8), v);
            ai += 1;
        }
    }
    
    return ai as u8;
}

const PB_DIV: [f32; 6] = [ 1.0 / 24.0, 1.0 / 12.0, 1.0 / 6.0, 5.0 / 12.0, 7.0 / 12.0, 1.0 ];

const SS0: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 0)];
const SS1: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 1)];
const SS2: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 1), (Channel::C1, 2)];
const SS3: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 2), (Channel::C1, 3)];
const SS4: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 2), (Channel::C1, 3), (Channel::C1, 4)];
const SS5: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 0), (Channel::C2, 0), (Channel::C2, 0)];
const SS6: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 0), (Channel::C1, 1), (Channel::C2, 0), (Channel::C2, 1)];
const SS7: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 2), (Channel::C2, 0), (Channel::C2, 1)];
const SS8: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 2), (Channel::C1, 3), (Channel::C2, 0)];
const SS9: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 0), (Channel::C2, 0), (Channel::C2, 0), (Channel::C3, 0)];
const SS10: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C2, 0), (Channel::C2, 1), (Channel::C3, 0)];
const SS11: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 2), (Channel::C2, 0), (Channel::C3, 0)];
const SS12: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 0), (Channel::C2, 0), (Channel::C3, 0), (Channel::C4, 0)];
const SS13: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C2, 0), (Channel::C3, 0), (Channel::C4, 0)];
const SS14: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C2, 0), (Channel::C3, 0), (Channel::C4, 0), (Channel::C5, 0)];

const SS15: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 0)];
const SS16: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 2), (Channel::C1, 0), (Channel::C1, 1)];
const SS17: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 2), (Channel::C1, 3), (Channel::C1, 0)];
const SS18: [(Channel, u8); 5] = [(Channel::C1, 0), (Channel::C1, 1), (Channel::C1, 0), (Channel::C2, 0), (Channel::C2, 1)];

const SLOT_SETTINGS: [[&[(Channel, u8); 5]; 5]; 5] = [
    // voice priority
    // // 1 Channel
    // [&SS0, &SS1, &SS2, &SS3, &SS4],
    // // 2 Channels
    // [&SS5, &SS6, &SS7, &SS8, &SS4],
    // // 3 Channels
    // [&SS9, &SS10, &SS11, &SS8, &SS4],
    // // 4 Channels
    // [&SS12, &SS13, &SS11, &SS8, &SS4],
    // // 5 Channels
    // [&SS14, &SS13, &SS11, &SS8, &SS4]
    // channel priority
    // 1 Channel
    [&SS0, &SS1, &SS2, &SS3, &SS4],
    // 2 Channels
    [&SS5, &SS6, &SS7, &SS8, &SS8],
    // 3 Channels
    [&SS9, &SS10, &SS11, &SS11, &SS11],
    // 4 Channels
    [&SS12, &SS13, &SS13, &SS13, &SS13],
    // 5 Channels
    [&SS14, &SS14, &SS14, &SS14, &SS14]
];