extern crate enum_as_inner;
extern crate strum_macros;

use enum_as_inner::EnumAsInner;
use std::fmt::Display;
use strum_macros::EnumString;

#[derive(Debug, Eq, PartialEq, EnumString, Clone, EnumAsInner, Default, Copy, Hash)]
pub enum Type {
    #[default]
    #[strum(serialize = "void")]
    Void,
    #[strum(serialize = "int")]
    Int,
    #[strum(serialize = "float")]
    Float,
}

impl Display for Type {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Type::Void => write!(f, "void"),
            Type::Int => write!(f, "int"),
            Type::Float => write!(f, "float"),
        }
    }
}

impl Type {
    pub fn new_type(a: Type, b: Type) -> Type {
        if a == Type::Void || b == Type::Void {
            panic!("Type::new_type: cannot have void type");
        }
        if a == b {
            return a;
        }
        if a == Type::Float || b == Type::Float {
            return Type::Float;
        }
        Type::Int
    }
    pub fn size(&self) -> u32 {
        match self {
            Type::Void => 0,
            Type::Int => 4,
            Type::Float => 4,
        }
    }
}
