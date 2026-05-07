use super::arch_info::RegConvention;
use crate::common::r#type::Type;
use crate::frontend::llvm::ssa::SSARightValue;
use derive_new::new;
use getset::{Getters, Setters};
use std::fmt::{Display, Formatter};

#[derive(Debug, Copy, Clone, PartialEq, Eq, Getters, Setters, new, Default, Hash)]
pub struct Reg {
    #[getset(get = "pub", set = "pub")]
    id: i32,
    #[getset(get = "pub")]
    ty: Type,
}

impl Reg {
    pub fn from_ssa_rvalue(rvalue: &SSARightValue) -> Self {
        assert!(!rvalue.is_immediate());
        Self {
            id: *rvalue.id(),
            ty: rvalue.ty(),
        }
    }
    pub fn new_int(id: i32) -> Self {
        Self::new(id, Type::Int)
    }
    pub fn new_float(id: i32) -> Self {
        Self::new(id, Type::Float)
    }
    pub fn is_pseudo(&self) -> bool {
        if self.ty == Type::Int {
            self.id >= RegConvention::<i32>::COUNT as i32
        } else if self.ty == Type::Float {
            self.id >= RegConvention::<f32>::COUNT as i32
        } else {
            panic!("void type register")
        }
    }
    pub fn is_allocable(&self) -> bool {
        if self.is_pseudo() {
            return false;
        }
        if self.ty == Type::Int {
            RegConvention::<i32>::new().is_allocable(self.id as _)
        } else if self.ty == Type::Float {
            RegConvention::<f32>::new().is_allocable(self.id as _)
        } else {
            panic!("void type register")
        }
    }
    pub fn gen_asm(&self) -> String {
        if self.is_pseudo() {
            if self.ty == Type::Int {
                return format!("x{}", self.id);
            } else if self.ty == Type::Float {
                return format!("f{}", self.id);
            } else {
                panic!("void type register")
            }
        }
        if self.ty == Type::Int {
            RegConvention::<i32>::new()
                .gen_asm(self.id as _)
                .to_string()
        } else if self.ty == Type::Float {
            RegConvention::<f32>::new()
                .gen_asm(self.id as _)
                .to_string()
        } else {
            // panic!("void type register")
            "".to_string()
        }
    }
}

impl Display for Reg {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.gen_asm())
    }
}
