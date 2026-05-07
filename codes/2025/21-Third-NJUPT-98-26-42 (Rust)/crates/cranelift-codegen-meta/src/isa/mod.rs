//! Define supported ISAs; includes ISA-specific instructions, encodings, registers, settings, etc.
//! This version is simplified to only support RISCV64
use crate::cdsl::isa::TargetIsa;
use std::fmt;

mod riscv64;

/// Represents known ISA target.
#[derive(PartialEq, Copy, Clone)]
pub enum Isa {
    Riscv64,
}

impl Isa {
    /// Creates isa target using name.
    pub fn from_name(name: &str) -> Option<Self> {
        match name {
            "riscv64" => Some(Isa::Riscv64),
            _ => None,
        }
    }

    /// Creates isa target from arch.
    pub fn from_arch(arch: &str) -> Option<Self> {
        match arch {
            "riscv64" | "riscv64gc" | "riscv64imac" => Some(Isa::Riscv64),
            _ => None,
        }
    }

    /// Returns all supported isa targets.
    pub fn all() -> &'static [Isa] {
        &[Isa::Riscv64]
    }
}

impl fmt::Display for Isa {
    // These names should be kept in sync with the crate features.
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match *self {
            Isa::Riscv64 => write!(f, "riscv64"),
        }
    }
}

pub(crate) fn define(isas: &[Isa]) -> Vec<TargetIsa> {
    isas.iter()
        .map(|isa| match isa {
            Isa::Riscv64 => riscv64::define(),
        })
        .collect()
}
