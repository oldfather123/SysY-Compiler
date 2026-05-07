extern crate derive_new;
extern crate enum_as_inner;

use super::r#type::Type;
use derive_new::new;
use enum_as_inner::EnumAsInner;
use std::fmt::Display;
use std::hash::{Hash, Hasher};
use std::ops::{Add, Div, Mul, Neg, Not, Rem, Sub};

#[derive(Debug, PartialEq, new, Clone, EnumAsInner, Copy)]
pub enum Immediate {
    Int(i32),
    Float(f32),
}

impl Eq for Immediate {}

impl Hash for Immediate {
    fn hash<H: Hasher>(&self, state: &mut H) {
        match self {
            Immediate::Int(i) => {
                1i32.hash(state);
                i.hash(state)
            }
            Immediate::Float(fl) => {
                2i32.hash(state);
                fl.to_bits().hash(state)
            }
        }
    }
}

impl Immediate {
    pub fn get_type(&self) -> Type {
        match self {
            Immediate::Int(_) => Type::Int,
            Immediate::Float(_) => Type::Float,
        }
    }
    #[inline(always)]
    pub fn is_zero(&self) -> bool {
        match self {
            Immediate::Int(i) => *i == 0,
            Immediate::Float(fl) => *fl == 0.0,
        }
    }
}

impl Display for Immediate {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Immediate::Int(i) => write!(f, "{}", i),
            Immediate::Float(fl) => write!(f, "{}", fl),
        }
    }
}

impl Neg for Immediate {
    type Output = Self;

    fn neg(self) -> Self::Output {
        match self {
            Immediate::Int(i) => Immediate::Int(-i),
            Immediate::Float(fl) => Immediate::Float(-fl),
        }
    }
}

impl Add for Immediate {
    type Output = Self;

    fn add(self, rhs: Self) -> Self::Output {
        match (self, rhs) {
            (Immediate::Int(i1), Immediate::Int(i2)) => Immediate::Int(i1 + i2),
            (Immediate::Float(fl1), Immediate::Float(fl2)) => Immediate::Float(fl1 + fl2),
            (Immediate::Int(i), Immediate::Float(fl)) => Immediate::Float(i as f32 + fl),
            (Immediate::Float(fl), Immediate::Int(i)) => Immediate::Float(fl + i as f32),
        }
    }
}

impl Sub for Immediate {
    type Output = Self;

    fn sub(self, rhs: Self) -> Self::Output {
        match (self, rhs) {
            (Immediate::Int(i1), Immediate::Int(i2)) => Immediate::Int(i1 - i2),
            (Immediate::Float(fl1), Immediate::Float(fl2)) => Immediate::Float(fl1 - fl2),
            (Immediate::Int(i), Immediate::Float(fl)) => Immediate::Float(i as f32 - fl),
            (Immediate::Float(fl), Immediate::Int(i)) => Immediate::Float(fl - i as f32),
        }
    }
}

impl Mul for Immediate {
    type Output = Self;

    fn mul(self, rhs: Self) -> Self::Output {
        match (self, rhs) {
            (Immediate::Int(i1), Immediate::Int(i2)) => Immediate::Int(i1 * i2),
            (Immediate::Float(fl1), Immediate::Float(fl2)) => Immediate::Float(fl1 * fl2),
            (Immediate::Int(i), Immediate::Float(fl)) => Immediate::Float(i as f32 * fl),
            (Immediate::Float(fl), Immediate::Int(i)) => Immediate::Float(fl * i as f32),
        }
    }
}

impl Div for Immediate {
    type Output = Self;

    fn div(self, rhs: Self) -> Self::Output {
        match (self, rhs) {
            (Immediate::Int(i1), Immediate::Int(i2)) => Immediate::Int(i1 / i2),
            (Immediate::Float(fl1), Immediate::Float(fl2)) => Immediate::Float(fl1 / fl2),
            (Immediate::Int(i), Immediate::Float(fl)) => Immediate::Float(i as f32 / fl),
            (Immediate::Float(fl), Immediate::Int(i)) => Immediate::Float(fl / i as f32),
        }
    }
}

impl Rem for Immediate {
    type Output = Self;

    fn rem(self, rhs: Self) -> Self::Output {
        match (self, rhs) {
            (Immediate::Int(i1), Immediate::Int(i2)) => Immediate::Int(i1 % i2),
            (Immediate::Float(fl1), Immediate::Float(fl2)) => Immediate::Float(fl1 % fl2),
            (Immediate::Int(i), Immediate::Float(fl)) => Immediate::Float(i as f32 % fl),
            (Immediate::Float(fl), Immediate::Int(i)) => Immediate::Float(fl % i as f32),
        }
    }
}

impl Not for Immediate {
    type Output = Self;

    fn not(self) -> Self::Output {
        match self {
            Immediate::Int(i) => {
                if i == 0 {
                    Immediate::Int(1)
                } else {
                    Immediate::Int(0)
                }
            }
            Immediate::Float(fl) => {
                if fl == 0.0 {
                    Immediate::Float(1.0)
                } else {
                    Immediate::Float(0.0)
                }
            }
        }
    }
}
