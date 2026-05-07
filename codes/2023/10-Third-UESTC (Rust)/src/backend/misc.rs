use super::{
    arch_info::RegConvention,
    block::BlockId,
    instr::{BranchType, FcmpType},
    register::Reg,
};
use crate::common::r#type::Type;
use crate::frontend::llvm::{
    instr::CmpType,
    ssa::{SSALeftValue, SSARightValue},
};
use derive_new::new;
use getset::{Getters, MutGetters, Setters};
use std::{
    cell::RefCell,
    collections::{HashMap, HashSet},
    rc::Rc,
};

#[derive(new, Clone, Debug, Getters, Setters, MutGetters, PartialEq, Eq, Hash)]
pub struct StackObject {
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    position: i32,
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    size: u32,
}

impl StackObject {
    pub fn from_ssa_lvalue(lvalue: &SSALeftValue) -> Self {
        Self::new(-1, lvalue.size())
    }
}

#[derive(Getters, MutGetters)]
pub struct MappingInfo {
    #[getset(get = "pub", get_mut = "pub")]
    obj_mapping: HashMap<i32, Rc<RefCell<StackObject>>>, // SSALeftValue id -> StackObject
    reg_mapping: HashMap<SSARightValue, Reg>, // SSARightValue id -> Reg
    #[getset(get = "pub")]
    block_mapping: HashMap<i32, BlockId>, // bb_id -> BlockId
    #[getset(get = "pub")]
    rev_block_mapping: HashMap<BlockId, i32>, // BlockId -> bb_id
    // constant_value: HashMap<SSARightValue, i32>,
    #[getset(get = "pub")]
    reg_num: i32,
    #[getset(get = "pub")]
    float_regs: HashSet<Reg>,
}

impl MappingInfo {
    pub fn new() -> Self {
        Self {
            obj_mapping: HashMap::new(),
            reg_mapping: HashMap::new(),
            block_mapping: HashMap::new(),
            rev_block_mapping: HashMap::new(),
            // constant_value: HashMap::new(),
            reg_num: std::cmp::max(
                RegConvention::<i32>::COUNT as i32,
                RegConvention::<f32>::COUNT as i32,
            ),
            float_regs: HashSet::new(),
        }
    }
    pub fn new_reg(&mut self, ty: Type) -> Reg {
        assert!(ty != Type::Void);
        self.reg_num += 1;
        Reg::new(self.reg_num - 1, ty)
    }
    pub fn from_ssa_rvalue(&mut self, rvalue: &SSARightValue) -> Reg {
        assert!(!rvalue.is_immediate());
        if let Some(reg) = self.reg_mapping.get(&rvalue) {
            return *reg;
        }
        // float address use int register
        let reg = if rvalue.is_addr() {
            self.new_reg(Type::Int)
        } else {
            self.new_reg(rvalue.ty())
        };
        self.reg_mapping.insert(rvalue.clone(), reg);
        return reg;
    }
    pub fn insert_block(&mut self, block_id: BlockId, bb_id: i32) {
        self.block_mapping.insert(bb_id, block_id);
        self.rev_block_mapping.insert(block_id, bb_id);
    }
}

#[derive(Default, PartialEq, Eq)]
pub enum InstCond {
    #[default]
    Eq,
    Ne,
    Ge,
    Gt,
    Le,
    Lt,
}

impl InstCond {
    pub fn from_llvm_cmp_type(cmp_type: &CmpType) -> Self {
        match cmp_type {
            CmpType::EQ => InstCond::Eq,
            CmpType::NEQ => InstCond::Ne,
            CmpType::SGE => InstCond::Ge,
            CmpType::SGT => InstCond::Gt,
            CmpType::SLE => InstCond::Le,
            CmpType::SLT => InstCond::Lt,
        }
    }
}

#[derive(new, Getters, Setters)]
pub(crate) struct _CmpInfo {
    #[getset(get = "pub", set = "pub")]
    lhs: Reg,
    #[getset(get = "pub", set = "pub")]
    rhs: Reg,
    #[getset(get = "pub", set = "pub")]
    is_float: bool,
    #[getset(get = "pub", set = "pub")]
    cond: InstCond,
}

impl _CmpInfo {
    pub fn _to_risc_v_fcmp(&self) -> (Reg, FcmpType, Reg) {
        assert!(self.is_float);
        match self.cond {
            InstCond::Eq => (self.lhs, FcmpType::FeqS, self.rhs),
            InstCond::Ne => (self.lhs, FcmpType::FeqS, self.rhs),
            InstCond::Ge => (self.rhs, FcmpType::FleS, self.lhs),
            InstCond::Gt => (self.rhs, FcmpType::FltS, self.lhs),
            InstCond::Le => (self.lhs, FcmpType::FleS, self.rhs),
            InstCond::Lt => (self.lhs, FcmpType::FltS, self.rhs),
        }
    }
    pub fn _to_risc_v_branch(&self) -> (Reg, BranchType, Reg) {
        assert!(!self.is_float);
        match self.cond {
            InstCond::Eq => (self.lhs, BranchType::Beq, self.rhs),
            InstCond::Ne => (self.lhs, BranchType::Bne, self.rhs),
            InstCond::Ge => (self.lhs, BranchType::Bge, self.rhs),
            InstCond::Gt => (self.rhs, BranchType::Blt, self.lhs),
            InstCond::Le => (self.rhs, BranchType::Bge, self.lhs),
            InstCond::Lt => (self.lhs, BranchType::Blt, self.rhs),
        }
    }
}

#[derive(Getters, Setters, MutGetters, Debug)]
pub struct AsmContext {
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    temp_sp_offset: i32,
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    epilogue: fn() -> String,
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    prologue: fn() -> String,
}

impl AsmContext {
    pub fn new() -> Self {
        Self {
            temp_sp_offset: 0,
            epilogue: || String::from("unimplemented!(epilogue is not implemented\n"),
            prologue: || String::from("unimplemented!(prologue is not implemented\n"),
        }
    }
}

impl Default for AsmContext {
    fn default() -> Self {
        Self::new()
    }
}
