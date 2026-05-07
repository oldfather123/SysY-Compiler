#![allow(dead_code)]

use derive_new::new;

pub(crate) const X0: i32 = 0;
pub(crate) const X1: i32 = 1;
pub(crate) const X2: i32 = 2;
pub(crate) const X3: i32 = 3;
pub(crate) const X4: i32 = 4;
pub(crate) const X5: i32 = 5;
pub(crate) const X6: i32 = 6;
pub(crate) const X7: i32 = 7;
pub(crate) const X8: i32 = 8;
pub(crate) const X9: i32 = 9;
pub(crate) const X10: i32 = 10;
pub(crate) const X11: i32 = 11;
pub(crate) const X12: i32 = 12;
pub(crate) const X13: i32 = 13;
pub(crate) const X14: i32 = 14;
pub(crate) const X15: i32 = 15;
pub(crate) const X16: i32 = 16;
pub(crate) const X17: i32 = 17;
pub(crate) const X18: i32 = 18;
pub(crate) const X19: i32 = 19;
pub(crate) const X20: i32 = 20;
pub(crate) const X21: i32 = 21;
pub(crate) const X22: i32 = 22;
pub(crate) const X23: i32 = 23;
pub(crate) const X24: i32 = 24;
pub(crate) const X25: i32 = 25;
pub(crate) const X26: i32 = 26;
pub(crate) const X27: i32 = 27;
pub(crate) const X28: i32 = 28;
pub(crate) const X29: i32 = 29;
pub(crate) const X30: i32 = 30;
pub(crate) const X31: i32 = 31;

pub(crate) const ZERO: i32 = X0;
pub(crate) const RA: i32 = X1;
pub(crate) const SP: i32 = X2;
pub(crate) const GP: i32 = X3;
pub(crate) const TP: i32 = X4;
pub(crate) const T0: i32 = X5;
pub(crate) const T1: i32 = X6;
pub(crate) const T2: i32 = X7;
pub(crate) const S0: i32 = X8;
pub(crate) const FP: i32 = X8;
pub(crate) const S1: i32 = X9;
pub(crate) const A0: i32 = X10;
pub(crate) const A1: i32 = X11;
pub(crate) const A2: i32 = X12;
pub(crate) const A3: i32 = X13;
pub(crate) const A4: i32 = X14;
pub(crate) const A5: i32 = X15;
pub(crate) const A6: i32 = X16;
pub(crate) const A7: i32 = X17;
pub(crate) const S2: i32 = X18;
pub(crate) const S3: i32 = X19;
pub(crate) const S4: i32 = X20;
pub(crate) const S5: i32 = X21;
pub(crate) const S6: i32 = X22;
pub(crate) const S7: i32 = X23;
pub(crate) const S8: i32 = X24;
pub(crate) const S9: i32 = X25;
pub(crate) const S10: i32 = X26;
pub(crate) const S11: i32 = X27;
pub(crate) const T3: i32 = X28;
pub(crate) const T4: i32 = X29;
pub(crate) const T5: i32 = X30;
pub(crate) const T6: i32 = X31;

#[derive(Copy, Clone, PartialEq, Eq)]
pub enum RegisterUsage {
    CallerSaved,
    CalleeSaved,
    Special,
}

#[derive(Copy, Clone, new)]
pub(crate) struct RegConvention<T> {
    phantom: std::marker::PhantomData<T>,
}

impl RegConvention<i32> {
    pub const COUNT: usize = 32;
    pub const ALLOCABLE_REGISTER_COUNT: i32 = 27;
    pub const ARGUMENT_REGISTER_COUNT: usize = 8;
    pub const ARGUMENT_REGISTERS: [i32; Self::ARGUMENT_REGISTER_COUNT] =
        [A0, A1, A2, A3, A4, A5, A6, A7];

    pub const REGISTER_USAGE: [RegisterUsage; Self::COUNT] = [
        RegisterUsage::Special,     // zero
        RegisterUsage::Special,     // ra
        RegisterUsage::Special,     // sp
        RegisterUsage::Special,     // gp
        RegisterUsage::Special,     // tp
        RegisterUsage::CallerSaved, // t0
        RegisterUsage::CallerSaved, // t1
        RegisterUsage::CallerSaved, // t2
        RegisterUsage::CalleeSaved, // s0
        RegisterUsage::CalleeSaved, // s1
        RegisterUsage::CallerSaved, // a0
        RegisterUsage::CallerSaved, // a1
        RegisterUsage::CallerSaved, // a2
        RegisterUsage::CallerSaved, // a3
        RegisterUsage::CallerSaved, // a4
        RegisterUsage::CallerSaved, // a5
        RegisterUsage::CallerSaved, // a6
        RegisterUsage::CallerSaved, // a7
        RegisterUsage::CalleeSaved, // s2
        RegisterUsage::CalleeSaved, // s3
        RegisterUsage::CalleeSaved, // s4
        RegisterUsage::CalleeSaved, // s5
        RegisterUsage::CalleeSaved, // s6
        RegisterUsage::CalleeSaved, // s7
        RegisterUsage::CalleeSaved, // s8
        RegisterUsage::CalleeSaved, // s9
        RegisterUsage::CalleeSaved, // s10
        RegisterUsage::CalleeSaved, // s11
        RegisterUsage::CallerSaved, // t3
        RegisterUsage::CallerSaved, // t4
        RegisterUsage::CallerSaved, // t5
        RegisterUsage::CallerSaved, // t6
    ];

    pub const REGISTER_NAME: [&str; Self::COUNT] = [
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", "a3", "a4",
        "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4",
        "t5", "t6",
    ];

    pub fn is_allocable(&self, reg: i32) -> bool {
        assert!(reg >= 0 && reg < Self::COUNT as i32);
        Self::REGISTER_USAGE[reg as usize] != RegisterUsage::Special
    }

    pub fn gen_asm(&self, reg: i32) -> &str {
        // assert!(reg >= 0 && reg < Self::COUNT as i32);
        Self::REGISTER_NAME[reg as usize]
    }
}

pub(crate) const F0: i32 = 0;
pub(crate) const F1: i32 = 1;
pub(crate) const F2: i32 = 2;
pub(crate) const F3: i32 = 3;
pub(crate) const F4: i32 = 4;
pub(crate) const F5: i32 = 5;
pub(crate) const F6: i32 = 6;
pub(crate) const F7: i32 = 7;
pub(crate) const F8: i32 = 8;
pub(crate) const F9: i32 = 9;
pub(crate) const F10: i32 = 10;
pub(crate) const F11: i32 = 11;
pub(crate) const F12: i32 = 12;
pub(crate) const F13: i32 = 13;
pub(crate) const F14: i32 = 14;
pub(crate) const F15: i32 = 15;
pub(crate) const F16: i32 = 16;
pub(crate) const F17: i32 = 17;
pub(crate) const F18: i32 = 18;
pub(crate) const F19: i32 = 19;
pub(crate) const F20: i32 = 20;
pub(crate) const F21: i32 = 21;
pub(crate) const F22: i32 = 22;
pub(crate) const F23: i32 = 23;
pub(crate) const F24: i32 = 24;
pub(crate) const F25: i32 = 25;
pub(crate) const F26: i32 = 26;
pub(crate) const F27: i32 = 27;
pub(crate) const F28: i32 = 28;
pub(crate) const F29: i32 = 29;
pub(crate) const F30: i32 = 30;
pub(crate) const F31: i32 = 31;

pub(crate) const FT0: i32 = F0;
pub(crate) const FT1: i32 = F1;
pub(crate) const FT2: i32 = F2;
pub(crate) const FT3: i32 = F3;
pub(crate) const FT4: i32 = F4;
pub(crate) const FT5: i32 = F5;
pub(crate) const FT6: i32 = F6;
pub(crate) const FT7: i32 = F7;
pub(crate) const FS0: i32 = F8;
pub(crate) const FS1: i32 = F9;
pub(crate) const FA0: i32 = F10;
pub(crate) const FA1: i32 = F11;
pub(crate) const FA2: i32 = F12;
pub(crate) const FA3: i32 = F13;
pub(crate) const FA4: i32 = F14;
pub(crate) const FA5: i32 = F15;
pub(crate) const FA6: i32 = F16;
pub(crate) const FA7: i32 = F17;
pub(crate) const FS2: i32 = F18;
pub(crate) const FS3: i32 = F19;
pub(crate) const FS4: i32 = F20;
pub(crate) const FS5: i32 = F21;
pub(crate) const FS6: i32 = F22;
pub(crate) const FS7: i32 = F23;
pub(crate) const FS8: i32 = F24;
pub(crate) const FS9: i32 = F25;
pub(crate) const FS10: i32 = F26;
pub(crate) const FS11: i32 = F27;
pub(crate) const FT8: i32 = F28;
pub(crate) const FT9: i32 = F29;
pub(crate) const FT10: i32 = F30;
pub(crate) const FT11: i32 = F31;

impl RegConvention<f32> {
    pub const COUNT: usize = 32;
    pub const ALLOCABLE_REGISTER_COUNT: i32 = 32;
    pub const ARGUMENT_REGISTER_COUNT: usize = 8;
    pub const ARGUMENT_REGISTERS: [i32; Self::ARGUMENT_REGISTER_COUNT] =
        [FA0, FA1, FA2, FA3, FA4, FA5, FA6, FA7];

    pub const REGISTER_USAGE: [RegisterUsage; Self::COUNT] = [
        RegisterUsage::CallerSaved, // ft0
        RegisterUsage::CallerSaved, // ft1
        RegisterUsage::CallerSaved, // ft2
        RegisterUsage::CallerSaved, // ft3
        RegisterUsage::CallerSaved, // ft4
        RegisterUsage::CallerSaved, // ft5
        RegisterUsage::CallerSaved, // ft6
        RegisterUsage::CallerSaved, // ft7
        RegisterUsage::CalleeSaved, // fs0
        RegisterUsage::CalleeSaved, // fs1
        RegisterUsage::CallerSaved, // fa0
        RegisterUsage::CallerSaved, // fa1
        RegisterUsage::CallerSaved, // fa2
        RegisterUsage::CallerSaved, // fa3
        RegisterUsage::CallerSaved, // fa4
        RegisterUsage::CallerSaved, // fa5
        RegisterUsage::CallerSaved, // fa6
        RegisterUsage::CallerSaved, // fa7
        RegisterUsage::CalleeSaved, // fs2
        RegisterUsage::CalleeSaved, // fs3
        RegisterUsage::CalleeSaved, // fs4
        RegisterUsage::CalleeSaved, // fs5
        RegisterUsage::CalleeSaved, // fs6
        RegisterUsage::CalleeSaved, // fs7
        RegisterUsage::CalleeSaved, // fs8
        RegisterUsage::CalleeSaved, // fs9
        RegisterUsage::CalleeSaved, // fs10
        RegisterUsage::CalleeSaved, // fs11
        RegisterUsage::CallerSaved, // ft8
        RegisterUsage::CallerSaved, // ft9
        RegisterUsage::CallerSaved, // ft10
        RegisterUsage::CallerSaved, // ft11
    ];

    pub const REGISTER_NAME: [&str; Self::COUNT] = [
        "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "fs0", "fs1", "fa0", "fa1", "fa2",
        "fa3", "fa4", "fa5", "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9",
        "fs10", "fs11", "ft8", "ft9", "ft10", "ft11",
    ];

    pub fn is_allocable(&self, reg: i32) -> bool {
        assert!(reg >= 0 && reg < Self::COUNT as i32);
        true
    }

    pub fn gen_asm(&self, reg: i32) -> &str {
        assert!(reg >= 0 && reg < Self::COUNT as i32);
        Self::REGISTER_NAME[reg as usize]
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_int_register() {
        let reg = RegConvention::<i32>::new();
        assert_eq!(reg.is_allocable(ZERO), false);
        assert_eq!(reg.is_allocable(RA), false);
        assert_eq!(reg.is_allocable(SP), false);
        assert_eq!(reg.is_allocable(GP), false);
        assert_eq!(reg.is_allocable(TP), false);
        assert_eq!(reg.is_allocable(T0), true);
        assert_eq!(reg.is_allocable(T1), true);
        assert_eq!(reg.is_allocable(T2), true);
        assert_eq!(reg.is_allocable(S0), true);
        assert_eq!(reg.is_allocable(FP), true);
        assert_eq!(reg.is_allocable(S1), true);
        assert_eq!(reg.is_allocable(A0), true);
        assert_eq!(reg.is_allocable(A1), true);
        assert_eq!(reg.is_allocable(A2), true);
        assert_eq!(reg.is_allocable(A3), true);
        assert_eq!(reg.is_allocable(A4), true);
        assert_eq!(reg.is_allocable(A5), true);
        assert_eq!(reg.is_allocable(A6), true);
        assert_eq!(reg.is_allocable(A7), true);
        assert_eq!(reg.is_allocable(S2), true);
        assert_eq!(reg.is_allocable(S3), true);
        assert_eq!(reg.is_allocable(S4), true);
        assert_eq!(reg.is_allocable(S5), true);
        assert_eq!(reg.is_allocable(S6), true);
        assert_eq!(reg.is_allocable(S7), true);
        assert_eq!(reg.is_allocable(S8), true);
        assert_eq!(reg.is_allocable(S9), true);
        assert_eq!(reg.is_allocable(S10), true);
        assert_eq!(reg.is_allocable(T3), true);
        assert_eq!(reg.is_allocable(T4), true);
        assert_eq!(reg.is_allocable(T5), true);
        assert_eq!(reg.is_allocable(T6), true);
    }

    #[test]
    fn test_float_register() {
        let reg = RegConvention::<f32>::new();
        assert_eq!(reg.is_allocable(FT0), true);
        assert_eq!(reg.is_allocable(FT1), true);
        assert_eq!(reg.is_allocable(FT2), true);
        assert_eq!(reg.is_allocable(FT3), true);
        assert_eq!(reg.is_allocable(FT4), true);
        assert_eq!(reg.is_allocable(FT5), true);
        assert_eq!(reg.is_allocable(FT6), true);
        assert_eq!(reg.is_allocable(FT7), true);
        assert_eq!(reg.is_allocable(FS0), true);
        assert_eq!(reg.is_allocable(FS1), true);
        assert_eq!(reg.is_allocable(FA0), true);
        assert_eq!(reg.is_allocable(FA1), true);
        assert_eq!(reg.is_allocable(FA2), true);
        assert_eq!(reg.is_allocable(FA3), true);
        assert_eq!(reg.is_allocable(FA4), true);
        assert_eq!(reg.is_allocable(FA5), true);
        assert_eq!(reg.is_allocable(FA6), true);
        assert_eq!(reg.is_allocable(FA7), true);
        assert_eq!(reg.is_allocable(FS2), true);
        assert_eq!(reg.is_allocable(FS3), true);
        assert_eq!(reg.is_allocable(FS4), true);
        assert_eq!(reg.is_allocable(FS5), true);
        assert_eq!(reg.is_allocable(FS6), true);
        assert_eq!(reg.is_allocable(FS7), true);
        assert_eq!(reg.is_allocable(FS8), true);
        assert_eq!(reg.is_allocable(FS9), true);
        assert_eq!(reg.is_allocable(FS10), true);
        assert_eq!(reg.is_allocable(FT8), true);
        assert_eq!(reg.is_allocable(FT9), true);
        assert_eq!(reg.is_allocable(FT10), true);
        assert_eq!(reg.is_allocable(FT11), true);
    }
}
