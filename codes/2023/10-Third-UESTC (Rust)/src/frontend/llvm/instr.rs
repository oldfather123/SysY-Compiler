use super::ssa::*;
use crate::common::immediate::Immediate;
use crate::common::r#type::Type;
use derive_new::new;
use getset::{CopyGetters, Getters, MutGetters};
use std::any::{Any, TypeId};
use std::fmt::{Debug, Display};
use strum_macros::EnumString;

pub trait Instr: Any + Debug {
    fn as_any(&self) -> &dyn Any;
    fn as_any_mut(&mut self) -> &mut dyn Any;
    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        None
    }
    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        None
    }
    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        None
    }
    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        None
    }
    fn try_as_unary_instr(&self) -> Option<&dyn UnaryInstr> {
        None
    }
    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        None
    }

    // (def, use1, use2)
    fn get_operands(&self) -> (i32, i32, i32) {
        let get_id = |x: &SSARightValue| -> i32 {
            if x.is_immediate() {
                0
            } else {
                *x.id()
            }
        };
        let mut kill = 0;
        let mut gen1 = 0;
        let mut gen2 = 0;
        if let Some(w) = self.try_as_reg_write_instr() {
            if let Some(w) = w.des_register() {
                kill = get_id(w);
            }
        }
        if let Some(u) = self.try_as_reg_use_instr() {
            let us = u.uses();
            if us.len() == 2 {
                gen1 = get_id(us[0]);
                gen2 = get_id(us[1]);
            } else if us.len() == 1 {
                gen1 = get_id(us[0]);
            }
        }

        (kill, gen1, gen2)
    }
}

pub trait RegWriteInstr: Instr + Debug {
    // get destiny register
    fn des_register(&self) -> Option<&SSARightValue>;
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue>;
}

pub trait RegUseInstr: Instr + Debug {
    fn uses(&self) -> Vec<&SSARightValue>;
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue>;
}

#[derive(Debug, Hash, PartialEq, Eq, Clone, Copy)]
pub enum UnaryOp {
    Neg,
    Not,
    Fptosi,
    Sitofp,
    // Zext,
    Mov,
    FMov,
}

pub trait UnaryInstr: RegUseInstr + RegWriteInstr {
    fn d1(&self) -> &SSARightValue {
        self.des_register().unwrap()
    }
    fn s1(&self) -> &SSARightValue {
        self.uses()[0]
    }
    fn unary_op(&self) -> UnaryOp;
}

#[derive(Debug, Hash, PartialEq, Eq, Clone, Copy)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Shl,
    LShr,
    AShr,
    // And,
    // Or,
    // Xor,
    FAdd,
    FSub,
    FMul,
    FDiv,
}

pub trait BinaryInstr: RegUseInstr + RegWriteInstr {
    fn d1(&self) -> &SSARightValue {
        self.des_register().unwrap()
    }
    fn s1(&self) -> &SSARightValue {
        self.uses()[0]
    }
    fn s2(&self) -> &SSARightValue {
        self.uses()[1]
    }
    fn binary_op(&self) -> BinaryOp;
}

#[derive(new, Getters, CopyGetters, MutGetters)]
pub struct Instruction {
    #[getset(get = "pub", get_mut = "pub")]
    instr: Box<dyn Instr>,
    #[getset(get_copy = "pub", get_mut = "pub")]
    bb_id: i32,
}

impl Debug for Instruction {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.instr.fmt(f)
    }
}

impl Instruction {
    pub fn is_branch(&self) -> bool {
        self.instr.as_any().downcast_ref::<Branch>().is_some()
    }
    pub fn is_ret(&self) -> bool {
        self.instr.as_any().downcast_ref::<Ret>().is_some()
    }
    pub fn is_cmp(&self) -> bool {
        self.instr.as_any().downcast_ref::<Icmp>().is_some()
            || self.instr.as_any().downcast_ref::<Fcmp>().is_some()
    }
    pub fn is_phi(&self) -> bool {
        self.instr.as_any().downcast_ref::<Phi>().is_some()
    }
    pub fn is_reg_write(&self) -> bool {
        self.instr.as_any().type_id() == TypeId::of::<dyn RegWriteInstr>()
    }
    pub fn is_load(&self) -> bool {
        self.instr.as_any().downcast_ref::<Load>().is_some()
    }
    pub fn is_store(&self) -> bool {
        self.instr.as_any().downcast_ref::<Store>().is_some()
    }
    pub fn is_call(&self) -> bool {
        self.instr.as_any().downcast_ref::<Call>().is_some()
    }
    pub fn is_alloca(&self) -> bool {
        self.instr.as_any().downcast_ref::<Alloca>().is_some()
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct GlobalDecl {
    pub var: SSALeftValue,
}

impl Debug for GlobalDecl {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut code = String::new();
        code.push_str("@");
        code.push_str(&self.var.get_name());
        code.push_str(" = ");
        if *self.var.is_const() {
            code.push_str("constant ");
        } else {
            code.push_str("global ");
        }

        // todo array
        let var_copy = self.var.init_value().clone();
        if let Some(mut v) = var_copy {
            code.push_str(&array_init_value_string(1, self.var.get_shape(), &mut v));
        } else {
            code.push_str(&array_shape_string(
                &self.var.get_shape(),
                self.var.get_type(),
            ));
            code.push_str(" ");
            if self.var.get_shape().len() != 0 {
                code.push_str("zeroinitializer");
            } else {
                code.push_str("0");
            }
        }
        writeln!(f, "{}", code)
    }
}

impl Instr for GlobalDecl {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }
}

#[derive(PartialEq, Clone, new, MutGetters, Getters)]
pub struct Alloca {
    #[getset(get = "pub")]
    addr: SSARightValue,
}

impl Debug for Alloca {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.addr.is_addr());
        writeln!(
            f,
            "{} = alloca {}",
            self.addr,
            address_type_string(&self.addr)
        )
    }
}

impl Instr for Alloca {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Alloca {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.addr)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.addr)
    }
}

impl RegUseInstr for Alloca {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![]
    }

    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![]
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Load {
    #[getset(get = "pub")]
    addr: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub d1: SSARightValue,
}

impl Debug for Load {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.addr.is_addr());
        assert!(self.d1.get_type() == self.addr.get_type());
        let type_str = address_type_string(&self.addr);
        writeln!(
            f,
            "{} = load {}, {}* {}",
            self.d1,
            if self.d1.is_addr() {
                address_type_string(&self.d1)
            } else {
                self.d1.get_type().to_string()
            },
            type_str,
            self.addr,
        )
    }
}

impl Instr for Load {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn get_operands(&self) -> (i32, i32, i32) {
        (*self.d1.id(), 0, 0)
    }
}

impl RegWriteInstr for Load {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Load {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.addr]
    }

    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.addr]
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Store {
    #[getset(get = "pub")]
    addr: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
}

impl Debug for Store {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.addr.is_addr(), "{} is not address", self.addr);
        assert!(self.addr.get_type() == self.s1.get_type());
        let type_str = address_type_string(&self.addr);
        writeln!(
            f,
            "store {} {}, {}* {}",
            if self.s1.is_addr() {
                address_type_string(&self.s1)
            } else {
                self.s1.get_type().to_string()
            },
            self.s1,
            type_str,
            self.addr
        )
    }
}

impl Instr for Store {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn get_operands(&self) -> (i32, i32, i32) {
        (0, *self.s1.id(), 0)
    }
}

impl RegWriteInstr for Store {
    fn des_register(&self) -> Option<&SSARightValue> {
        None
    }

    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        None
    }
}

impl RegUseInstr for Store {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.addr]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.addr]
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Mov {
    #[getset(get = "pub", get_mut = "pub")]
    d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    s1: SSARightValue,
}

impl Mov {
    pub fn set_s1(&mut self, s1: SSARightValue) {
        assert!(s1.get_type() == Type::Int);
        self.s1 = s1;
    }
}

impl Debug for Mov {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        writeln!(f, "{} = mov i32 {}", self.d1, self.s1)
    }
}

impl Instr for Mov {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_unary_instr(&self) -> Option<&dyn UnaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Mov {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Mov {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1]
    }
}

impl UnaryInstr for Mov {
    fn unary_op(&self) -> UnaryOp {
        UnaryOp::Mov
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct FMov {
    #[getset(get = "pub", get_mut = "pub")]
    d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    s1: SSARightValue,
}

impl Debug for FMov {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Float);
        assert!(self.s1.get_type() == Type::Float);
        writeln!(f, "{} = mov float {}", self.d1, self.s1)
    }
}

impl Instr for FMov {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_unary_instr(&self) -> Option<&dyn UnaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for FMov {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for FMov {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1]
    }
}

impl UnaryInstr for FMov {
    fn unary_op(&self) -> UnaryOp {
        UnaryOp::FMov
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Add {
    #[getset(get = "pub", get_mut = "pub")]
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for Add {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(f, "{} = add i32 {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for Add {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Add {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Add {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for Add {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::Add
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct FAdd {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for FAdd {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Float);
        assert!(self.s1.get_type() == Type::Float);
        assert!(self.s2.get_type() == Type::Float);
        writeln!(f, "{} = fadd float {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for FAdd {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for FAdd {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for FAdd {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for FAdd {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::FAdd
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Sub {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for Sub {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(f, "{} = sub i32 {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for Sub {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Sub {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Sub {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for Sub {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::Sub
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct FSub {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for FSub {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Float);
        assert!(self.s1.get_type() == Type::Float);
        assert!(self.s2.get_type() == Type::Float);
        writeln!(f, "{} = fsub float {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for FSub {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for FSub {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for FSub {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for FSub {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::FSub
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Mul {
    #[getset(get = "pub", get_mut = "pub")]
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for Mul {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(f, "{} = mul i32 {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for Mul {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Mul {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Mul {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for Mul {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::Mul
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct FMul {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for FMul {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Float);
        assert!(self.s1.get_type() == Type::Float);
        assert!(self.s2.get_type() == Type::Float);
        writeln!(f, "{} = fmul float {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for FMul {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for FMul {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for FMul {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for FMul {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::FMul
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Shl {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for Shl {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(f, "{} = shl i32 {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for Shl {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Shl {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Shl {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for Shl {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::Shl
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct LShr {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for LShr {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(f, "{} = lshr i32 {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for LShr {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for LShr {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for LShr {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for LShr {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::LShr
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct AShr {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for AShr {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(f, "{} = ashr i32 {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for AShr {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for AShr {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for AShr {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for AShr {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::AShr
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Div {
    #[getset(get = "pub", get_mut = "pub")]
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for Div {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(f, "{} = sdiv i32 {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for Div {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Div {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Div {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for Div {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::Div
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct FDiv {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for FDiv {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Float);
        assert!(self.s1.get_type() == Type::Float);
        assert!(self.s2.get_type() == Type::Float);
        writeln!(f, "{} = fdiv float {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for FDiv {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for FDiv {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for FDiv {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for FDiv {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::FDiv
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Mod {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

impl Debug for Mod {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(f, "{} = srem i32 {}, {}", self.d1, self.s1, self.s2)
    }
}

impl Instr for Mod {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_binary_instr(&self) -> Option<&dyn BinaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Mod {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Mod {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

impl BinaryInstr for Mod {
    fn binary_op(&self) -> BinaryOp {
        BinaryOp::Mod
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Neg {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
}

impl Debug for Neg {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        writeln!(f, "{} = sub i32 0, {}", self.d1, self.s1)
    }
}

impl Instr for Neg {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_unary_instr(&self) -> Option<&dyn UnaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Neg {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Neg {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1]
    }
}

impl UnaryInstr for Neg {
    fn unary_op(&self) -> UnaryOp {
        UnaryOp::Neg
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Not {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
}

impl Debug for Not {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        writeln!(f, "{} = not i32 {}", self.d1, self.s1)
    }
}

impl Instr for Not {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_unary_instr(&self) -> Option<&dyn UnaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Not {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Not {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1]
    }
}

impl UnaryInstr for Not {
    fn unary_op(&self) -> UnaryOp {
        UnaryOp::Not
    }
}

#[derive(Debug, PartialEq, EnumString, Clone, new, Eq, Hash, Copy)]
pub enum CmpType {
    #[strum(serialize = "==")]
    EQ,
    #[strum(serialize = "!=")]
    NEQ,
    #[strum(serialize = ">")]
    SGT,
    #[strum(serialize = ">=")]
    SGE,
    #[strum(serialize = "<")]
    SLT,
    #[strum(serialize = "<=")]
    SLE,
}

impl Display for CmpType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CmpType::EQ => {
                write!(f, "eq")
            }
            CmpType::NEQ => {
                write!(f, "ne")
            }
            CmpType::SGT => {
                write!(f, "sgt")
            }
            CmpType::SGE => {
                write!(f, "sge")
            }
            CmpType::SLT => {
                write!(f, "slt")
            }
            CmpType::SLE => {
                write!(f, "sle")
            }
        }
    }
}

#[derive(PartialEq, Clone, Getters, MutGetters)]
pub struct Icmp {
    #[getset(get = "pub", get_mut = "pub")]
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub cmp_type: CmpType,
}

impl Icmp {
    pub fn new(d1: SSARightValue, s1: SSARightValue, s2: SSARightValue, cmp_type: CmpType) -> Self {
        assert!(d1.get_type() == Type::Int);
        assert!(s1.get_type() == Type::Int);
        assert!(s2.get_type() == Type::Int);
        Self {
            d1,
            s1,
            s2,
            cmp_type,
        }
    }
}

impl Debug for Icmp {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Int);
        assert!(self.s2.get_type() == Type::Int);
        writeln!(
            f,
            "{} = icmp {} i32 {}, {}",
            self.d1, self.cmp_type, self.s1, self.s2
        )
    }
}

impl Instr for Icmp {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Icmp {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Icmp {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Fcmp {
    #[getset(get = "pub", get_mut = "pub")]
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub cmp_type: CmpType,
}

impl Debug for Fcmp {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Float);
        assert!(self.s2.get_type() == Type::Float);
        writeln!(
            f,
            "{} = fcmp {} float {}, {}",
            self.d1, self.cmp_type, self.s1, self.s2
        )
    }
}

impl Instr for Fcmp {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Fcmp {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Fcmp {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1, &self.s2]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1, &mut self.s2]
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Call {
    #[getset(get = "pub", get_mut = "pub")]
    pub ret: Option<SSARightValue>,
    #[getset(get = "pub", get_mut = "pub")]
    pub func_name: String,
    #[getset(get = "pub", get_mut = "pub")]
    pub args: Vec<SSARightValue>,
}

impl Debug for Call {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut code = String::new();
        if let Some(ret) = &self.ret {
            code += format!("{} = ", ret).as_str();
        }
        code += format!(
            "call {} @{}(",
            if let Some(ret) = &self.ret {
                ret.get_type()
            } else {
                Type::Void
            },
            self.func_name
        )
        .as_str();
        for (i, arg) in self.args.iter().enumerate() {
            if i != 0 {
                code += ", ";
            }
            code += format!("{} {}", arg.get_type(), arg).as_str();
        }
        code += ")";
        writeln!(f, "{}", code)
    }
}

impl Instr for Call {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Call {
    fn des_register(&self) -> Option<&SSARightValue> {
        self.ret.as_ref()
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        self.ret.as_mut()
    }
}

impl RegUseInstr for Call {
    fn uses(&self) -> Vec<&SSARightValue> {
        self.args.iter().map(|arg| arg).collect()
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        self.args.iter_mut().map(|arg| arg).collect()
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Ret {
    #[getset(get = "pub")]
    value: Option<SSARightValue>,
}

impl Debug for Ret {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut code = String::new();
        code += "ret ";
        if let Some(value) = &self.value {
            code += format!("{} {}", value.get_type(), value).as_str();
        }
        writeln!(f, "{}", code)
    }
}

impl Instr for Ret {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }
}

impl RegUseInstr for Ret {
    fn uses(&self) -> Vec<&SSARightValue> {
        if let Some(s1) = &self.value {
            vec![s1]
        } else {
            vec![]
        }
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        if let Some(s1) = self.value.as_mut() {
            vec![s1]
        } else {
            vec![]
        }
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Xor {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s2: SSARightValue,
}

// impl Debug for Xor {
//     fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
//         assert!(self.d1.get_type() == Type::Int);
//         assert!(self.s1.get_type() == Type::Int);
//         assert!(self.s2.get_type() == Type::Int);
//         writeln!(f, "{} = xor i32 {}, {}", self.d1, self.s1, self.s2)
//     }
// }

// impl Instr for Xor {
//     fn as_any(&self) -> &dyn Any {
//         self
//     }

//     fn as_any_mut(&mut self) -> &mut dyn Any {
//         self
//     }

//     fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
//         Some(self)
//     }

//     fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
//         Some(self)
//     }

//     fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
//         Some(self)
//     }

//     fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
//         Some(self)
//     }
// }

// impl RegWriteInstr for Xor {
//     fn des_register(&self) -> Option<&SSARightValue> {
//         Some(&self.d1)
//     }
//     fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
//         Some(&mut self.d1)
//     }
// }

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Zext {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
}

// impl Debug for Zext {
//     fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
//         assert!(self.d1.get_type() == Type::Int);
//         assert!(self.s1.get_type() == Type::Int);
//         writeln!(f, "{} = zext i1 {}, i32", self.d1, self.s1)
//     }
// }

// impl Instr for Zext {
//     fn as_any(&self) -> &dyn Any {
//         self
//     }

//     fn as_any_mut(&mut self) -> &mut dyn Any {
//         self
//     }

//     fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
//         Some(self)
//     }

//     fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
//         Some(self)
//     }

//     fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
//         Some(self)
//     }

//     fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
//         Some(self)
//     }
// }

// impl RegWriteInstr for Zext {
//     fn des_register(&self) -> Option<&SSARightValue> {
//         Some(&self.d1)
//     }
//     fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
//         Some(&mut self.d1)
//     }
// }

#[derive(PartialEq, Clone, new, MutGetters, Getters)]
pub struct Gep {
    #[getset(get = "pub")]
    s1: SSARightValue,
    #[getset(get = "pub")]
    d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    index: SSARightValue,
}

impl Debug for Gep {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.s1.is_addr());
        assert!(self.d1.is_addr());
        assert!(self.s1.get_type() == self.d1.get_type());
        writeln!(
            f,
            "{} = getelementptr {}, {}* {}, i32 {}",
            self.d1,
            address_type_string(&self.s1),
            address_type_string(&self.s1),
            self.s1,
            self.index
        )
    }
}

impl Instr for Gep {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Gep {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Gep {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.index, &self.s1]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.index, &mut self.s1]
    }
}

type Blkid = i32;
#[derive(PartialEq, Clone, new, Getters, MutGetters, Hash, Eq)]
pub struct Phi {
    #[getset(get = "pub")]
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub uses: Vec<(SSARightValue, Blkid)>,
}

impl Phi {
    pub fn add_use(self: &mut Self, r: SSARightValue, bid: Blkid) {
        self.uses.push((r, bid));
    }

    pub fn new_by_reg(d1: SSARightValue) -> Self {
        Self { d1, uses: vec![] }
    }
}

impl Debug for Phi {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        for (reg, _) in &self.uses {
            assert!(reg.get_type() == self.d1().get_type());
        }
        let mut code = String::new();
        code += format!("{} = phi {}", self.d1, self.d1().get_type()).as_str();
        for (reg, bid) in &self.uses {
            code += format!(" [ {}, %{} ],", reg, bid).as_str();
        }
        writeln!(f, "{}", code)
    }
}

impl Instr for Phi {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Phi {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Phi {
    fn uses(&self) -> Vec<&SSARightValue> {
        self.uses.iter().map(|(reg, _)| reg).collect()
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        self.uses.iter_mut().map(|(reg, _)| reg).collect()
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Branch {
    pub label1: i32,
    pub label2: Option<i32>,
    pub cond: Option<SSARightValue>,
}

impl Branch {
    pub fn new_label(label1: i32) -> Branch {
        Branch {
            label1: label1,
            label2: None,
            cond: None,
        }
    }
}

impl Instr for Branch {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }
}

impl Debug for Branch {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if let Some(cond) = &self.cond {
            assert!(cond.get_type() == Type::Int);
        }
        if let Some(label2) = self.label2 {
            assert!(label2 != self.label1);
        }
        let mut code = String::new();
        code += "br ";
        if let Some(cond) = &self.cond {
            code += format!("i1 {}, ", cond).as_str();
        }
        code += format!("label %{}", self.label1).as_str();
        if let Some(label2) = &self.label2 {
            code += format!(", label %{}", label2).as_str();
        }
        writeln!(f, "{}", code)
    }
}

impl RegUseInstr for Branch {
    fn uses(&self) -> Vec<&SSARightValue> {
        if let Some(cond) = &self.cond {
            vec![cond]
        } else {
            vec![]
        }
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        if let Some(cond) = self.cond.as_mut() {
            vec![cond]
        } else {
            vec![]
        }
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Sitofp {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
}

impl Debug for Sitofp {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Float);
        assert!(self.s1.get_type() == Type::Int);
        writeln!(f, "{} = {} int2float", self.d1, self.s1)
    }
}

impl Instr for Sitofp {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_unary_instr(&self) -> Option<&dyn UnaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Sitofp {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Sitofp {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1]
    }
}

impl UnaryInstr for Sitofp {
    fn unary_op(&self) -> UnaryOp {
        UnaryOp::Sitofp
    }
}

#[derive(PartialEq, Clone, new, Getters, MutGetters)]
pub struct Fptosi {
    pub d1: SSARightValue,
    #[getset(get = "pub", get_mut = "pub")]
    pub s1: SSARightValue,
}

impl Debug for Fptosi {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        assert!(self.d1.get_type() == Type::Int);
        assert!(self.s1.get_type() == Type::Float);
        writeln!(f, "{} = {} float2int", self.d1, self.s1)
    }
}

impl Instr for Fptosi {
    fn as_any(&self) -> &dyn Any {
        self
    }

    fn as_any_mut(&mut self) -> &mut dyn Any {
        self
    }

    fn try_as_reg_write_instr(&self) -> Option<&dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_write_instr_mut(&mut self) -> Option<&mut dyn RegWriteInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr(&self) -> Option<&dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_reg_use_instr_mut(&mut self) -> Option<&mut dyn RegUseInstr> {
        Some(self)
    }

    fn try_as_unary_instr(&self) -> Option<&dyn UnaryInstr> {
        Some(self)
    }
}

impl RegWriteInstr for Fptosi {
    fn des_register(&self) -> Option<&SSARightValue> {
        Some(&self.d1)
    }
    fn des_register_mut(&mut self) -> Option<&mut SSARightValue> {
        Some(&mut self.d1)
    }
}

impl RegUseInstr for Fptosi {
    fn uses(&self) -> Vec<&SSARightValue> {
        vec![&self.s1]
    }
    fn uses_mut(&mut self) -> Vec<&mut SSARightValue> {
        vec![&mut self.s1]
    }
}

impl UnaryInstr for Fptosi {
    fn unary_op(&self) -> UnaryOp {
        UnaryOp::Fptosi
    }
}

pub fn address_type_string(address: &SSARightValue) -> String {
    let (_, &ty, shape, _, _) = address.inner().as_address().unwrap();
    if shape.len() == 0 {
        format!("{}", ty)
    } else {
        if shape[0] == -1 {
            format!("{}*", array_shape_string(&shape[1..], ty))
        } else {
            format!("{}", array_shape_string(&shape, ty))
        }
    }
}

fn array_shape_string(shape: &[i32], type_: Type) -> String {
    let mut str = String::new();
    if shape.len() == 0 {
        str.push_str(format!("{}", type_).as_str());
    } else {
        str.push_str("[");
        str.push_str(shape[0].to_string().as_str());
        str.push_str(" x ");
        str.push_str(&array_shape_string(&shape[1..], type_));
        str.push_str("]");
    }
    str
}

pub fn array_init_value_string(
    high_dim: i32,
    shape: Vec<i32>,
    values: &mut Vec<Immediate>,
) -> String {
    let mut str = String::new();
    if shape.len() != 0 {
        let new_high_dim = shape[0];
        let new_shape = shape[1..].to_vec();
        for i in 0..high_dim {
            str.push_str(&array_shape_string(
                &shape.clone(),
                values.last().unwrap().get_type(),
            ));
            str.push_str(" [");
            str.push_str(&array_init_value_string(
                new_high_dim,
                new_shape.clone(),
                values,
            ));
            str.push_str("]");
            if i < high_dim - 1 {
                str.push_str(", ");
            }
        }
    } else {
        for i in 0..high_dim {
            let value = values.first().unwrap();
            str.push_str(format!("{}", value.get_type()).as_str());
            str.push_str(" ");
            str.push_str(format!("{}", value).as_str());
            if i != high_dim - 1 {
                str.push_str(", ");
            }
            values.remove(0);
        }
    }
    str
}
