#![allow(dead_code)]

/// RVGC64 架构指令有100多个，不会一一实现，只实现需要用的指令
/// 指令的分类不依据于指令的功能，而是依据于指令的格式，特殊指令的实现会单独实现
///
use super::{
    arch_info::{RegConvention, SP},
    misc::{AsmContext, InstCond, MappingInfo, StackObject},
    register::Reg,
};
use crate::{backend::arch_info::A0, common::r#type::Type};
use derive_new::new;
use enum_as_inner::EnumAsInner;
use getset::{Getters, MutGetters, Setters};
use itertools::Itertools;
use std::{cell::RefCell, fmt::Debug, rc::Rc};

#[derive(Debug)]
pub(crate) enum TruncType {
    Lo,
    Hi,
}

impl TruncType {
    pub(crate) fn gen_asm(&self) -> String {
        match self {
            TruncType::Lo => format!("%lo"),
            TruncType::Hi => format!("%hi"),
        }
    }
}

#[derive(Debug, EnumAsInner, Clone)]
pub(crate) enum ImmeValueType {
    Direct(i32),
    Symbol(String),
}

fn imme_string(imme: &ImmeValueType, trunc: &Option<TruncType>) -> String {
    match imme {
        ImmeValueType::Direct(i) => format!("{}", i),
        ImmeValueType::Symbol(ref s) => {
            if let Some(trunc) = trunc {
                format!("{}({})", trunc.gen_asm(), s)
            } else {
                panic!("imme instr must have trunc type");
            }
        }
    }
}

// 因为LLVMInstr可能要由多个Instr组成，所以没法直接转换
pub trait InstrTrait: Debug {
    fn gen_asm(&self) -> String;
    fn uses(&self) -> Vec<Reg>;
    fn defs(&self) -> Vec<Reg>;
    fn regs_mut(&mut self) -> Vec<&mut Reg>;
    #[inline(always)]
    fn is_have_side_effect(&self) -> bool {
        false
    }
    fn is_use(&self, reg: &Reg) -> bool {
        self.uses().contains(reg)
    }
    fn is_def(&self, reg: &Reg) -> bool {
        self.defs().contains(reg)
    }
    fn is_relate(&self, reg: &Reg) -> bool {
        self.is_use(reg) || self.is_def(reg)
    }
    fn use_id_vec(&self, reg_type: Type) -> Vec<i32> {
        self.uses()
            .iter()
            .filter(|x| *x.id() != 0 && *x.ty() == reg_type && *x.id() != SP)
            .map(|x| *x.id())
            .collect_vec()
    }
    fn def_id_vec(&self, reg_type: Type) -> Vec<i32> {
        self.defs()
            .iter()
            .filter(|x| *x.id() != 0 && *x.ty() == reg_type && *x.id() != SP)
            .map(|x| *x.id())
            .collect_vec()
    }
    fn get_operands(&self, reg_type: Type) -> (i32, i32, i32) {
        let mut kill = 0;
        let mut gen1 = 0;
        let mut gen2 = 0;

        let ds = self.defs();
        if ds.len() != 0 && *ds[0].ty() == reg_type {
            kill = *ds[0].id();
        }

        let us = self.uses();
        if us.len() != 0 {
            if us.len() >= 1 && *us[0].ty() == reg_type {
                gen1 = *us[0].id();
            }
            if us.len() >= 2 && *us[1].ty() == reg_type {
                gen2 = *us[1].id();
            }
        }

        if kill == SP {
            kill = 0;
        }
        if gen1 == SP {
            gen1 = 0;
        }
        if gen2 == SP {
            gen2 = 0;
        }

        (kill, gen1, gen2)
    }

    fn as_any(&self) -> &dyn std::any::Any;
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any;
}

/* RV64I */
#[derive(Debug)]
pub(crate) enum ImmeType {
    // Real Instruction
    Auipc, // x[rd] = pc + sext(immediate[31:12] << 12)
    // Pseudo Instruction
    Lui, // x[rd] = sext(immediate[31:12] << 12)
    Li,  // x[rd] = immediate
}

// x[rd] = op imme
#[derive(Debug, Getters)]
pub(crate) struct ImmeInstr {
    #[getset(get = "pub")]
    rd: Reg,
    #[getset(get = "pub")]
    ty: ImmeType,
    #[getset(get = "pub")]
    imme: ImmeValueType,
    trunc: Option<TruncType>,
}

impl ImmeInstr {
    pub fn new_load_immediate(rd: Reg, imme: i32) -> Self {
        Self {
            rd,
            ty: ImmeType::Li,
            imme: ImmeValueType::Direct(imme),
            trunc: None,
        }
    }
    pub fn new_auipc(rd: Reg, imme: ImmeValueType, trunc: Option<TruncType>) -> Self {
        Self {
            rd,
            ty: ImmeType::Auipc,
            imme,
            trunc,
        }
    }
    pub fn new_load_upper_immediate(
        rd: Reg,
        imme: ImmeValueType,
        trunc: Option<TruncType>,
    ) -> Self {
        Self {
            rd,
            ty: ImmeType::Lui,
            imme,
            trunc,
        }
    }
}

impl InstrTrait for ImmeInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rd.ty() == Type::Int);
        let imme_string = imme_string(&self.imme, &self.trunc);
        let mut asm = String::new();
        match self.ty {
            ImmeType::Auipc => {
                match self.imme {
                    ImmeValueType::Direct(i) => {
                        assert!(i <= (1 << 19) - 1 && i >= -(1 << 19));
                    }
                    _ => {}
                }
                asm.push_str(&format!("auipc {}, {}", self.rd, imme_string));
            }
            ImmeType::Li => {
                asm.push_str(&format!("li {}, {}", self.rd, imme_string));
            }
            ImmeType::Lui => {
                asm.push_str(&format!("lui {}, {}", self.rd, imme_string));
            }
        }
        asm.push_str("\n");
        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd]
    }
}

#[derive(Debug)]
pub(crate) enum RegType {
    // Real Instruction
    // Pseudo Instruction
    Mv,
    Negw,
    Seqz,
    Snez,
    Sltz,
    Sgtz,
}

// x[rd] = op x[rs]
#[derive(Debug, new, Getters)]
pub(crate) struct RegInstr {
    #[getset(get = "pub")]
    rd: Reg,
    #[getset(get = "pub")]
    rs: Reg,
    #[getset(get = "pub")]
    ty: RegType,
}

impl RegInstr {
    pub fn new_move(rd: Reg, rs: Reg) -> Self {
        Self {
            rd,
            rs,
            ty: RegType::Mv,
        }
    }
    pub fn new_negw(rd: Reg, rs: Reg) -> Self {
        Self {
            rd,
            rs,
            ty: RegType::Negw,
        }
    }
}

impl InstrTrait for RegInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rd.ty() == Type::Int);
        assert!(*self.rs.ty() == Type::Int);
        let mut asm = String::new();
        match self.ty {
            RegType::Mv => {
                asm.push_str(&format!("mv {}, {}", self.rd, self.rs));
            }
            RegType::Negw => {
                asm.push_str(&format!("negw {}, {}", self.rd, self.rs));
            }
            RegType::Seqz => {
                asm.push_str(&format!("seqz {}, {}", self.rd, self.rs));
            }
            RegType::Snez => {
                asm.push_str(&format!("snez {}, {}", self.rd, self.rs));
            }
            RegType::Sltz => {
                asm.push_str(&format!("sltz {}, {}", self.rd, self.rs));
            }
            RegType::Sgtz => {
                asm.push_str(&format!("sgtz {}, {}", self.rd, self.rs));
            }
        }
        asm.push_str("\n");
        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![self.rs]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs]
    }
}

#[derive(Debug)]
#[allow(dead_code)]
pub(crate) enum RegRegType {
    // Real Instruction
    Add,
    Addw,
    Sub,
    Subw,
    Mul,
    Mulw,
    Div,
    Divw,
    Rem,
    Remw,
    Sll,
    Sllw,
    Srl,
    Srlw,
    Sra,
    Sraw,
    And,
    Or,
    // Pseudo Instruction
}

// x[rd] = x[rs1] op x[rs2]
#[derive(Debug, new, Getters)]
pub(crate) struct RegRegInstr {
    #[getset(get = "pub")]
    rd: Reg,
    #[getset(get = "pub")]
    rs1: Reg,
    #[getset(get = "pub")]
    rs2: Reg,
    #[getset(get = "pub")]
    ty: RegRegType,
}

impl InstrTrait for RegRegInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        // assert!(*self.rd.ty() == Type::Int);
        // assert!(*self.rs1.ty() == Type::Int);
        // assert!(*self.rs2.ty() == Type::Int);
        // assert!(self.rd.is_allocable());
        // assert!(self.rs1.is_allocable(), "not allocable {}", *self.rs1.id());
        // assert!(self.rs2.is_allocable(), "not allocable {}", *self.rs2.id());
        let mut asm = String::new();
        match self.ty {
            RegRegType::Add => {
                asm.push_str(&format!("add {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Addw => {
                asm.push_str(&format!("addw {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Sub => {
                asm.push_str(&format!("sub {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Subw => {
                asm.push_str(&format!("subw {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Mul => {
                asm.push_str(&format!("mul {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Mulw => {
                asm.push_str(&format!("mulw {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Div => {
                asm.push_str(&format!("div {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Divw => {
                asm.push_str(&format!("divw {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Rem => {
                asm.push_str(&format!("rem {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Remw => {
                asm.push_str(&format!("remw {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Sll => {
                asm.push_str(&format!("sll {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Sllw => {
                asm.push_str(&format!("sllw {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Srl => {
                asm.push_str(&format!("srl {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Srlw => {
                asm.push_str(&format!("srlw {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Sra => {
                asm.push_str(&format!("sra {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Sraw => {
                asm.push_str(&format!("sraw {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::And => {
                asm.push_str(&format!("and {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
            RegRegType::Or => {
                asm.push_str(&format!("or {}, {}, {}", self.rd, self.rs1, self.rs2));
            }
        }
        asm.push_str("\n");
        asm
    }

    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1, self.rs2]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs1, &mut self.rs2]
    }
}

#[derive(Debug)]
pub(crate) enum RegImmeType {
    // Real Instruction
    Addi,  // x[rd] = x[rs1] + sext(immediate)
    Addiw, // x[rd] = sext((x[rs1] + sext(immediate))[31:0])
    Slli,
    Slliw, // x[rd] = sext((x[rs1] ≪ shamt)[31: 0])
    Srli,
    Srliw,
    Srai,
    Sraiw,
    // Pseudo Instruction
}

// x[rd] = x[rs1] op offset
#[derive(Debug, new, Getters)]
pub(crate) struct RegImmeInstr {
    #[getset(get = "pub")]
    rd: Reg,
    #[getset(get = "pub")]
    rs1: Reg,
    #[getset(get = "pub")]
    offset: ImmeValueType, // 12 bits
    #[getset(get = "pub")]
    ty: RegImmeType,
    trunc: Option<TruncType>,
}

impl InstrTrait for RegImmeInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rd.ty() == Type::Int);
        assert!(*self.rs1.ty() == Type::Int);

        let mut imme_strings = vec![];
        let mut imme_flag = false;
        let step = 2036;
        match self.offset {
            ImmeValueType::Direct(i) => {
                // assert!(i <= 2047 && i >= -2048, "offset {} out of range", i);
                let mut offset = i;
                imme_flag = true;
                while offset > 2047 {
                    imme_strings.push(format!("{}", step));
                    offset -= step;
                }
                while offset < -2048 {
                    imme_strings.push(format!("{}", -step));
                    offset += step;
                }
                if offset != 0 || imme_strings.is_empty() {
                    imme_strings.push(format!("{}", offset));
                }
            }
            _ => {}
        }

        if !imme_flag {
            let imme_string = imme_string(&self.offset, &self.trunc);
            imme_strings.push(imme_string);
        }

        let mut asm = String::new();

        for imme_string in imme_strings {
            match self.ty {
                RegImmeType::Addi => {
                    asm.push_str(&format!("addi {}, {}, {}", self.rd, self.rs1, imme_string));
                }
                RegImmeType::Addiw => {
                    asm.push_str(&format!("addiw {}, {}, {}", self.rd, self.rs1, imme_string));
                }
                RegImmeType::Slli => {
                    asm.push_str(&format!("slli {}, {}, {}", self.rd, self.rs1, imme_string));
                }
                RegImmeType::Slliw => {
                    asm.push_str(&format!("slliw {}, {}, {}", self.rd, self.rs1, imme_string));
                }
                RegImmeType::Srli => {
                    asm.push_str(&format!("srli {}, {}, {}", self.rd, self.rs1, imme_string));
                }
                RegImmeType::Srliw => {
                    asm.push_str(&format!("srliw {}, {}, {}", self.rd, self.rs1, imme_string));
                }
                RegImmeType::Srai => {
                    asm.push_str(&format!("srai {}, {}, {}", self.rd, self.rs1, imme_string));
                }
                RegImmeType::Sraiw => {
                    asm.push_str(&format!("sraiw {}, {}, {}", self.rd, self.rs1, imme_string));
                }
            }
            asm.push_str("\n");
        }
        asm
    }

    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1]
    }

    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }

    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs1]
    }
}

#[derive(Debug)]
pub(crate) enum LoadType {
    // Real Instruction
    Lb,
    Lh,
    Lw,
    Ld,
    // Pseudo Instruction
}

// x[rd] = sext(M[x[rs1] + sext(offset)[31:0]])
#[derive(Debug, new, Getters)]
pub(crate) struct LoadInstr {
    #[getset(get = "pub")]
    rd: Reg,
    #[getset(get = "pub")]
    rs1: Reg,
    #[getset(get = "pub")]
    offset: i32,
    #[getset(get = "pub")]
    ty: LoadType,
}

impl InstrTrait for LoadInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rd.ty() == Type::Int);
        // assert!(self.offset >= -2048 && self.offset <= 2047);
        let mut asm = String::new();

        let mut steps = vec![];
        let step = 2032;
        let mut offset = self.offset;
        while offset < -2048 {
            asm.push_str(&format!("addi {}, {}, {}\n", self.rs1, self.rs1, -step));
            offset += step;
            steps.push(step);
        }
        while offset > 2047 {
            asm.push_str(&format!("addi {}, {}, {}\n", self.rs1, self.rs1, step));
            offset -= step;
            steps.push(-step);
        }

        match self.ty {
            LoadType::Lb => {
                asm.push_str(&format!("lb {}, {}({})", self.rd, offset, self.rs1));
            }
            LoadType::Lh => {
                asm.push_str(&format!("lh {}, {}({})", self.rd, offset, self.rs1));
            }
            LoadType::Lw => {
                asm.push_str(&format!("lw {}, {}({})", self.rd, offset, self.rs1));
            }
            LoadType::Ld => {
                asm.push_str(&format!("ld {}, {}({})", self.rd, offset, self.rs1));
            }
        }
        asm.push('\n');

        for step in steps {
            asm.push_str(&format!("addi {}, {}, {}\n", self.rs1, self.rs1, step));
        }
        asm
    }

    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1]
    }

    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }

    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs1]
    }
}

#[derive(Debug)]
pub(crate) enum StoreType {
    // Real Instruction
    Sb,
    Sh,
    Sw,
    Sd,
    // Pseudo Instruction
}

// M[x[rs1] + sext(offset)] = x[rs2][31: 0]
#[derive(Debug, new, Getters)]
pub(crate) struct StoreInstr {
    #[getset(get = "pub")]
    rs1: Reg, // base address
    #[getset(get = "pub")]
    rs2: Reg, // value
    #[getset(get = "pub")]
    offset: ImmeValueType,
    #[getset(get = "pub")]
    trunc: Option<TruncType>,
    #[getset(get = "pub")]
    ty: StoreType,
}

impl InstrTrait for StoreInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rs1.ty() == Type::Int);
        assert!(*self.rs2.ty() == Type::Int);
        let mut asm = String::new();
        match &self.offset {
            ImmeValueType::Symbol(_) => {
                match self.ty {
                    StoreType::Sb => {
                        asm.push_str(&format!(
                            "sb {}, {}({})",
                            self.rs2,
                            imme_string(&self.offset, &self.trunc),
                            self.rs1
                        ));
                    }
                    StoreType::Sh => {
                        asm.push_str(&format!(
                            "sh {}, {}({})",
                            self.rs2,
                            imme_string(&self.offset, &self.trunc),
                            self.rs1
                        ));
                    }
                    StoreType::Sw => {
                        asm.push_str(&format!(
                            "sw {}, {}({})",
                            self.rs2,
                            imme_string(&self.offset, &self.trunc),
                            self.rs1
                        ));
                    }
                    StoreType::Sd => {
                        asm.push_str(&format!(
                            "sd {}, {}({})",
                            self.rs2,
                            imme_string(&self.offset, &self.trunc),
                            self.rs1
                        ));
                    }
                }
                asm.push_str("\n");
                return asm;
            }
            _ => {}
        }

        let mut steps = vec![];
        let step = 2032;
        let mut offset = self.offset.clone().into_direct().unwrap();
        while offset < -2048 {
            asm.push_str(&format!("addi {}, {}, {}\n", self.rs1, self.rs1, -step));
            offset += step;
            steps.push(step);
        }
        while offset > 2047 {
            asm.push_str(&format!("addi {}, {}, {}\n", self.rs1, self.rs1, step));
            offset -= step;
            steps.push(-step);
        }

        match self.ty {
            StoreType::Sb => {
                asm.push_str(&format!("sb {}, {}({})", self.rs2, offset, self.rs1));
            }
            StoreType::Sh => {
                asm.push_str(&format!("sh {}, {}({})", self.rs2, offset, self.rs1));
            }
            StoreType::Sw => {
                asm.push_str(&format!("sw {}, {}({})", self.rs2, offset, self.rs1));
            }
            StoreType::Sd => {
                asm.push_str(&format!("sd {}, {}({})", self.rs2, offset, self.rs1));
            }
        }
        asm.push_str("\n");

        for step in steps {
            asm.push_str(&format!("addi {}, {}, {}\n", self.rs1, self.rs1, step));
        }
        asm
    }

    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1, self.rs2]
    }

    fn defs(&self) -> Vec<Reg> {
        vec![]
    }
    fn is_have_side_effect(&self) -> bool {
        true
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rs1, &mut self.rs2]
    }
}

#[derive(Debug)]
pub(crate) enum BranchType {
    // Real Instruction
    Beq, // if (rs1 == rs2) pc += sext(offset)
    Bne, // if (rs1 ≠ rs2) pc += sext(offset)
    Blt, // if (rs1 < rs2) pc += sext(offset)
    Bge, // if (rs1 ≥ rs2) pc += sext(offset)
         // Pseudo Instruction
}

// if (rs1 cond rs2) pc += sext(offset)
#[derive(Debug, new, Getters, Setters)]
pub(crate) struct BranchInstr {
    #[getset(get = "pub")]
    rs1: Reg,
    #[getset(get = "pub")]
    rs2: Reg,
    #[getset(get = "pub", set = "pub")]
    label: String,
    ty: BranchType,
}

impl InstrTrait for BranchInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rs1.ty() == Type::Int);
        assert!(*self.rs2.ty() == Type::Int);
        let mut asm = String::new();
        match self.ty {
            BranchType::Beq => {
                asm.push_str(&format!("beq {}, {}, {}", self.rs1, self.rs2, self.label));
            }
            BranchType::Bne => {
                asm.push_str(&format!("bne {}, {}, {}", self.rs1, self.rs2, self.label));
            }
            BranchType::Blt => {
                asm.push_str(&format!("blt {}, {}, {}", self.rs1, self.rs2, self.label));
            }
            BranchType::Bge => {
                asm.push_str(&format!("bge {}, {}, {}", self.rs1, self.rs2, self.label));
            }
        }
        asm.push_str("\n");
        asm
    }

    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1, self.rs2]
    }

    fn defs(&self) -> Vec<Reg> {
        vec![]
    }

    fn is_have_side_effect(&self) -> bool {
        true
    }

    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rs1, &mut self.rs2]
    }
}

#[derive(Debug)]
pub(crate) enum JumpType {
    // Pseudo Instruction
    J,  // pc += sext(offset); pesudo, equals jal x0, offset
    Jr, // pc = x[rs1]; pesudo, equals jalr x0, 0(x[rs1])
}

#[derive(Debug, Getters, Setters)]
// pc = x[rs] | imme
pub(crate) struct JumpInstr {
    #[getset(get = "pub")]
    rs1: Reg,
    #[getset(get = "pub", set = "pub")]
    label: String,
    #[getset(get = "pub")]
    ty: JumpType,
}

impl InstrTrait for JumpInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        if matches!(self.ty, JumpType::Jr) {
            assert!(*self.rs1.ty() == Type::Int);
        }
        let mut asm = String::new();
        match self.ty {
            JumpType::J => {
                asm.push_str(&format!("j {}", self.label));
            }
            JumpType::Jr => {
                asm.push_str(&format!("jr {}", self.rs1));
            }
        }
        asm.push_str("\n");
        asm
    }

    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1]
    }

    fn defs(&self) -> Vec<Reg> {
        vec![]
    }

    fn is_have_side_effect(&self) -> bool {
        true
    }

    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rs1]
    }
}

impl JumpInstr {
    pub(crate) fn new_jump(label: String) -> Self {
        Self {
            rs1: Reg::default(),
            label,
            ty: JumpType::J,
        }
    }
    pub(crate) fn new_jump_reg(rs1: Reg) -> Self {
        Self {
            rs1,
            label: String::new(),
            ty: JumpType::Jr,
        }
    }
}

#[derive(Debug)]
pub(crate) enum JumpAndLinkType {
    // Real Instruction
    Jal,  // x[rd] = pc+4; pc += sext(offset);
    Jalr, // x[rd] = pc+4; pc = (x[rs1]+sext(offset))&~1;
}

#[derive(Debug, new)]
// pc = x[rs] | imme, x[rd] = pc+4
pub(crate) struct JumpAndLinkInstr {
    rd: Reg,
    rs1: Reg,
    offset: i32,
    ty: JumpAndLinkType,
}

impl InstrTrait for JumpAndLinkInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rd.ty() == Type::Int);
        assert!(*self.rs1.ty() == Type::Int);
        let mut asm = String::new();
        match self.ty {
            JumpAndLinkType::Jal => {
                assert!(self.offset >= -(1 << 19) && self.offset <= (1 << 19) - 1);
                asm.push_str(&format!("jal {}, {}", self.rd, self.offset));
            }
            JumpAndLinkType::Jalr => {
                assert!(self.offset >= -(1 << 11) && self.offset <= (1 << 11) - 1);
                asm.push_str(&format!("jalr {}, {}, {}", self.rd, self.rs1, self.offset));
            }
        }
        asm.push_str("\n");
        asm
    }

    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1]
    }

    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }

    fn is_have_side_effect(&self) -> bool {
        true
    }

    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs1]
    }
}

// pc = x[1]; pesudo, equals jalr x0, 0(x1)
#[derive(Debug, new)]
pub(crate) struct ReturnInstr {
    ctx: Rc<RefCell<AsmContext>>,
}

impl InstrTrait for ReturnInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        let mut asm = String::new();
        // asm.push_str(self.ctx.borrow().epilogue()().as_str());
        asm.push_str(format!("ret\n").as_str());
        asm
    }

    fn uses(&self) -> Vec<Reg> {
        vec![Reg::new_int(A0), Reg::new_float(A0)]
        // vec![]
    }

    fn defs(&self) -> Vec<Reg> {
        vec![]
    }

    fn is_have_side_effect(&self) -> bool {
        true
    }

    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![]
    }
}

// Homemade Pesudo Instruction: Load Stack
#[derive(Debug, new)]
pub(crate) struct LoadStackInstr {
    rd: Reg,
    offset: i32,
    stack_object: Rc<RefCell<StackObject>>,
}

impl InstrTrait for LoadStackInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        let size = *self.stack_object.borrow().size();
        let mut offset = if self.offset != -1 {
            self.offset
        } else {
            *self.stack_object.borrow().position()
        };
        let step = 2032;
        let mut asm = String::new();
        let mut offsets = vec![];

        while offset > 2047 {
            asm.push_str(&format!("addi sp, sp, {}\n", step));
            offset -= step;
            offsets.push(-step);
        }
        while offset < -2048 {
            asm.push_str(&format!("addi sp, sp, {}\n", -step));
            offset += step;
            offsets.push(step);
        }

        if size == 4 {
            if self.rd.ty() == &Type::Int {
                asm.push_str(&format!("lw {}, {}(sp)\n", self.rd, offset));
            } else if self.rd.ty() == &Type::Float {
                asm.push_str(&format!("flw {}, {}(sp)\n", self.rd, offset));
            } else {
                unreachable!("load stack must be int or float");
            }
        } else if size == 8 {
            // address, saved in int register
            assert!(self.rd.ty() == &Type::Int);
            asm.push_str(&format!("ld {}, {}(sp)\n", self.rd, offset));
        } else {
            unreachable!("size of stack object is not 4 or 8");
        }
        for offset in offsets {
            asm.push_str(&format!("addi sp, sp, {}\n", offset));
        }
        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd]
    }
}

// Pesudo Instruction: Load address, most for global variables address
#[derive(Debug, new)]
pub(crate) struct LoadAddrInstr {
    rd: Reg,
    symbol: String,
}

impl InstrTrait for LoadAddrInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rd.ty() == Type::Int);
        format!("la {}, {}\n", self.rd, self.symbol)
    }
    fn uses(&self) -> Vec<Reg> {
        vec![]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd]
    }
}

// Pesudo Instruction: load global, most for global int variables
#[derive(Debug, new)]
pub(crate) struct LoadGlobalInstr {
    rd: Reg,
    symbol: String,
}

impl InstrTrait for LoadGlobalInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rd.ty() == Type::Int);
        format!("lw {}, {}\n", self.rd, self.symbol)
    }
    fn uses(&self) -> Vec<Reg> {
        vec![]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd]
    }
}

pub fn gen_store_global(rs: Reg, symbol: String, rt: Reg) -> Vec<Box<dyn InstrTrait>> {
    let mut instrs: Vec<Box<dyn InstrTrait>> = vec![];
    assert!(*rs.ty() == Type::Int);
    assert!(*rt.ty() == Type::Int);
    instrs.push(Box::new(ImmeInstr::new_load_upper_immediate(
        rt,
        ImmeValueType::Symbol(symbol.clone()),
        Some(TruncType::Hi),
    )));
    instrs.push(Box::new(StoreInstr::new(
        rt,
        rs,
        ImmeValueType::Symbol(symbol),
        Some(TruncType::Lo),
        StoreType::Sw,
    )));
    instrs
}

// bogus instruction, will be replaced by real instruction
#[derive(Debug, new, Getters, MutGetters, Setters)]
pub(crate) struct LoadStackAddrInstr {
    rd: Reg,
    #[getset(get = "pub", get_mut = "pub")]
    stack_object: Rc<RefCell<StackObject>>,
}

impl InstrTrait for LoadStackAddrInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        // unreachable!("load stack addr instr must be replaced by real instr");
        let mut offset = *self.stack_object.borrow().position();

        let mut offsets = vec![];
        let step = 2036;

        while offset > 2047 {
            offsets.push(format!("{}", step));
            offset -= step;
        }
        while offset < -2048 {
            offsets.push(format!("{}", -step));
            offset += step;
        }
        if offset != 0 || offsets.is_empty() {
            offsets.push(format!("{}", offset));
        }

        let mut asm = String::new();
        let mut first_flag = true;
        for offset in offsets {
            if first_flag {
                asm.push_str(&format!("addi {}, sp, {}\n", self.rd.gen_asm(), offset));
                first_flag = false;
            } else {
                asm.push_str(&format!(
                    "addi {}, {}, {}\n",
                    self.rd.gen_asm(),
                    self.rd.gen_asm(),
                    offset
                ));
            }
        }
        asm

        // format!("LSA {} {}", self.rd.gen_asm(), self.offset)
    }
    fn uses(&self) -> Vec<Reg> {
        vec![]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd]
    }
}

// Homemade Pesudo Instruction: Change Stack Pointer
#[derive(Debug, new)]
pub(crate) struct ChangeSPInstr {
    change: i32,
}

impl InstrTrait for ChangeSPInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        let mut offset = self.change;
        let mut asm = String::new();

        while offset > 2047 {
            asm.push_str(&format!("addi sp, sp, {}\n", 2047));
            offset -= 2047;
        }

        while offset < -2048 {
            asm.push_str(&format!("addi sp, sp, {}\n", -2048));
            offset += 2048;
        }

        if offset != 0 {
            asm.push_str(&format!("addi sp, sp, {}\n", offset));
        }

        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![]
    }
    fn is_have_side_effect(&self) -> bool {
        true
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![]
    }
}

// Pesudo yet Real Instruction: Call
#[derive(Debug, new, Getters)]
pub(crate) struct CallInstr {
    #[getset(get = "pub")]
    label: String,
    #[getset(get = "pub")]
    int_arg_cnt: usize,
    #[getset(get = "pub")]
    float_arg_cnt: usize,
}

impl InstrTrait for CallInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        let func_name = if self.label == "starttime" {
            "_sysy_starttime"
        } else if self.label == "stoptime" {
            "_sysy_stoptime"
        } else {
            &self.label
        };
        format!("call {}\n", func_name)
    }
    fn uses(&self) -> Vec<Reg> {
        let mut regs = vec![];
        for i in 0..std::cmp::min(
            self.int_arg_cnt,
            RegConvention::<i32>::ARGUMENT_REGISTER_COUNT,
        ) {
            regs.push(Reg::new_int(RegConvention::<i32>::ARGUMENT_REGISTERS[i]));
        }
        for i in 0..std::cmp::min(
            self.float_arg_cnt,
            RegConvention::<f32>::ARGUMENT_REGISTER_COUNT,
        ) {
            regs.push(Reg::new_float(RegConvention::<f32>::ARGUMENT_REGISTERS[i]));
        }
        regs
    }
    fn defs(&self) -> Vec<Reg> {
        let mut regs = vec![];
        // for i in 0..std::cmp::min(self.int_arg_cnt, RegConvention::<i32>::COUNT) {
        //     if RegConvention::<i32>::REGISTER_USAGE[i] == RegisterUsage::CallerSaved {
        //         regs.push(Reg::new_int(i as _));
        //     }
        // }
        // for i in 0..std::cmp::min(self.float_arg_cnt, RegConvention::<f32>::COUNT) {
        //     if RegConvention::<f32>::REGISTER_USAGE[i] == RegisterUsage::CallerSaved {
        //         regs.push(Reg::new_float(i as _));
        //     }
        // }
        // regs.push(Reg::new_int(RA));
        regs.push(Reg::new_int(A0));
        regs.push(Reg::new_float(A0));
        regs
    }
    fn is_have_side_effect(&self) -> bool {
        true
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![] // todo!("regs use for call instr")
    }
    fn get_operands(&self, _reg_type: Type) -> (i32, i32, i32) {
        (10, 0, 0)
    }
}

/* RV64F */
// f[rd] = M[x[rs1] + sext(offset)][31:0]
#[derive(Debug, new)]
pub(crate) struct FLoadInstr {
    rd: Reg,               // destination register
    rs1: Reg,              // base address
    offset: ImmeValueType, // 12 bits
    trunc: Option<TruncType>,
}

impl InstrTrait for FLoadInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rs1.ty() == Type::Int);
        assert!(*self.rd.ty() == Type::Float);

        if let ImmeValueType::Direct(i) = self.offset {
            let mut s = String::new();
            let mut offsets = vec![];

            let mut offset = i;
            let step = 2032;
            while offset > 2047 {
                s.push_str(&format!("addi sp, sp, {}\n", step));
                offset -= step;
                offsets.push(-step);
            }
            while offset < -2048 {
                s.push_str(&format!("addi sp, sp, {}\n", -step));
                offset += step;
                offsets.push(step);
            }

            s.push_str(&format!("flw {}, {}({})\n", self.rd, offset, self.rs1));

            for ele in offsets {
                s.push_str(&format!("addi sp, sp, {}\n", ele));
            }

            s
        } else {
            format!(
                "flw {}, {}({})\n",
                self.rd,
                imme_string(&self.offset, &self.trunc),
                self.rs1
            )
        }
    }
    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs1]
    }
}

// M[x[rs1] + sext(offset)] = f[rs2][31:0]
#[derive(Debug, new)]
pub(crate) struct FStoreInstr {
    rs1: Reg,              // base address
    rs2: Reg,              // value
    offset: ImmeValueType, // 12 bits
    trunc: Option<TruncType>,
}

impl InstrTrait for FStoreInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rs1.ty() == Type::Int);
        assert!(*self.rs2.ty() == Type::Float);

        if let ImmeValueType::Direct(i) = self.offset {
            let mut s = String::new();
            let mut offsets = vec![];

            let mut offset = i;
            let step = 2032;
            while offset > 2047 {
                s.push_str(&format!("addi sp, sp, {}\n", step));
                offset -= step;
                offsets.push(-step);
            }
            while offset < -2048 {
                s.push_str(&format!("addi sp, sp, {}\n", -step));
                offset += step;
                offsets.push(step);
            }

            s.push_str(&format!("fsw {}, {}({})\n", self.rs2, offset, self.rs1));

            for ele in offsets {
                s.push_str(&format!("addi sp, sp, {}\n", ele));
            }

            s
        } else {
            format!(
                "fsw {}, {}({})\n",
                self.rs2,
                imme_string(&self.offset, &self.trunc),
                self.rs1
            )
        }
    }
    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1, self.rs2]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![]
    }
    fn is_have_side_effect(&self) -> bool {
        true
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rs1, &mut self.rs2]
    }
}

#[derive(Debug)]
pub(crate) enum FRegType {
    // Real Instruction
    FsqrtS,
    // Pseudo Instruction
    FmvS,
    FabsS,
    FnegS,
}

// f[rd] = op f[rs]
#[derive(Debug, new, Getters)]
pub(crate) struct FRegInstr {
    #[getset(get = "pub")]
    rd: Reg,
    #[getset(get = "pub")]
    rs: Reg,
    #[getset(get = "pub")]
    ty: FRegType,
}

impl FRegInstr {
    pub fn new_fmove(rd: Reg, rs: Reg) -> Self {
        Self::new(rd, rs, FRegType::FmvS)
    }
}

impl InstrTrait for FRegInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rs.ty() == Type::Float);
        assert!(*self.rd.ty() == Type::Float);
        let mut asm = match self.ty {
            FRegType::FabsS => format!("fabs.s {}, {}", self.rd, self.rs),
            FRegType::FmvS => format!("fmv.s {}, {}", self.rd, self.rs),
            FRegType::FsqrtS => format!("fsqrt.s {}, {}", self.rd, self.rs),
            FRegType::FnegS => format!("fneg.s {}, {}", self.rd, self.rs),
        };
        asm.push_str("\n");
        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![self.rs]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs]
    }
}

#[derive(Debug)]
pub(crate) enum FRegRegConvertType {
    // Real Instruction
    FcvtSW,  // 把寄存器 x[rs1]中的 32 位二进制补码表示的整数转化为单精度浮点数，再写入 f[rd]中
    FcvtWS,  // 把寄存器 f[rs1]中的单精度浮点数转化为 32 位二进制补码表示的整数，再写入 x[rd]中
    FclassS, // x[rd] = classifys(f[rs1])
    FmvXW,   // x[rd] = sext(f[rs1][31:0])
    FmvWX,   // f[rd][31:0] = x[rs1]
}

// f[rd] = op x[rs]
#[derive(Debug, new)]
pub(crate) struct FRegRegInstr {
    rd: Reg,
    rs: Reg,
    ty: FRegRegConvertType,
}

impl InstrTrait for FRegRegInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        let mut asm = match self.ty {
            FRegRegConvertType::FcvtSW => {
                assert!(self.rd.ty() == &Type::Float);
                assert!(self.rs.ty() == &Type::Int);
                format!("fcvt.s.w {}, {}", self.rd, self.rs)
            }
            FRegRegConvertType::FcvtWS => {
                assert!(self.rd.ty() == &Type::Int);
                assert!(self.rs.ty() == &Type::Float);
                format!("fcvt.w.s {}, {}, rtz", self.rd, self.rs)
            }
            FRegRegConvertType::FclassS => {
                assert!(self.rd.ty() == &Type::Int);
                assert!(self.rs.ty() == &Type::Float);
                format!("fclass.s {}, {}", self.rd, self.rs)
            }
            FRegRegConvertType::FmvXW => {
                assert!(self.rd.ty() == &Type::Int);
                assert!(self.rs.ty() == &Type::Float);
                format!("fmv.x.w {}, {}", self.rd, self.rs)
            }
            FRegRegConvertType::FmvWX => {
                assert!(self.rd.ty() == &Type::Float);
                assert!(self.rs.ty() == &Type::Int);
                format!("fmv.w.x {}, {}", self.rd, self.rs)
            }
        };
        asm.push_str("\n");
        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![self.rs]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs]
    }
}

#[derive(Debug)]
pub(crate) enum FRegFRegType {
    // Real Instruction
    Fadd,
    Fsub,
    Fmul,
    Fdiv,
    Fmin,
    Fmax,
    // Pseudo Instruction
}

// f[rd] = op f[rs1] f[rs2]
#[derive(Debug, new)]
pub(crate) struct FRegFRegInstr {
    rd: Reg,
    rs1: Reg,
    rs2: Reg,
    ty: FRegFRegType,
}

impl InstrTrait for FRegFRegInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rs1.ty() == Type::Float);
        assert!(*self.rs2.ty() == Type::Float);
        assert!(*self.rd.ty() == Type::Float);
        let mut asm = match self.ty {
            FRegFRegType::Fadd => format!("fadd.s {}, {}, {}", self.rd, self.rs1, self.rs2),
            FRegFRegType::Fsub => format!("fsub.s {}, {}, {}", self.rd, self.rs1, self.rs2),
            FRegFRegType::Fmul => format!("fmul.s {}, {}, {}", self.rd, self.rs1, self.rs2),
            FRegFRegType::Fdiv => format!("fdiv.s {}, {}, {}", self.rd, self.rs1, self.rs2),
            FRegFRegType::Fmin => format!("fmin.s {}, {}, {}", self.rd, self.rs1, self.rs2),
            FRegFRegType::Fmax => format!("fmax.s {}, {}, {}", self.rd, self.rs1, self.rs2),
        };
        asm.push_str("\n");
        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1, self.rs2]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs1, &mut self.rs2]
    }
}

#[derive(Debug)]
pub(crate) enum FcmpType {
    FeqS, // if f[rs1] == f[rs2] then x[rd] = 1 else x[rd] = 0
    FltS, // if f[rs1] < f[rs2] then x[rd] = 1 else x[rd] = 0
    FleS, // if f[rs1] <= f[rs2] then x[rd] = 1 else x[rd] = 0
}

// x[rd] = op f[rs1] f[rs2]
#[derive(Debug, new)]
pub(crate) struct FcmpInstr {
    rd: Reg,
    rs1: Reg,
    rs2: Reg,
    ty: FcmpType,
}

impl InstrTrait for FcmpInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rs1.ty() == Type::Float);
        assert!(*self.rs2.ty() == Type::Float);
        assert!(*self.rd.ty() == Type::Int);
        let asm = match self.ty {
            FcmpType::FeqS => format!("feq.s {}, {}, {}\n", self.rd, self.rs1, self.rs2),
            FcmpType::FltS => format!("flt.s {}, {}, {}\n", self.rd, self.rs1, self.rs2),
            FcmpType::FleS => format!("fle.s {}, {}, {}\n", self.rd, self.rs1, self.rs2),
        };
        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![self.rs1, self.rs2]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![self.rd]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rd, &mut self.rs1, &mut self.rs2]
    }
}

// Pseudo Instruction, Floating-point Swap Rounding Mode
#[derive(Debug, new)]
pub(crate) struct FsrmInstr {
    rs: Reg,
}

impl InstrTrait for FsrmInstr {
    fn as_any(&self) -> &dyn std::any::Any {
        self
    }
    fn as_any_mut(&mut self) -> &mut dyn std::any::Any {
        self
    }
    fn gen_asm(&self) -> String {
        assert!(*self.rs.ty() == Type::Int);
        let asm = format!("fsrm x0, {}\n", self.rs);
        asm
    }
    fn uses(&self) -> Vec<Reg> {
        vec![self.rs]
    }
    fn defs(&self) -> Vec<Reg> {
        vec![]
    }
    fn regs_mut(&mut self) -> Vec<&mut Reg> {
        vec![&mut self.rs]
    }
}

pub fn gen_fload_global(rd: Reg, symbol: String, rt: Reg) -> Vec<Box<dyn InstrTrait>> {
    assert!(rd.ty() == &Type::Float);
    assert!(rt.ty() == &Type::Int);
    let mut instrs: Vec<Box<dyn InstrTrait>> = vec![];
    instrs.push(Box::new(ImmeInstr::new_load_upper_immediate(
        rt,
        ImmeValueType::Symbol(symbol.clone()),
        Some(TruncType::Hi),
    )));
    instrs.push(Box::new(FLoadInstr::new(
        rd,
        rt,
        ImmeValueType::Symbol(symbol),
        Some(TruncType::Lo),
    )));
    instrs
}

pub fn gen_fstore_global(rs: Reg, symbol: String, rt: Reg) -> Vec<Box<dyn InstrTrait>> {
    assert!(rs.ty() == &Type::Float);
    assert!(rt.ty() == &Type::Int);
    let mut instrs: Vec<Box<dyn InstrTrait>> = vec![];
    instrs.push(Box::new(ImmeInstr::new_load_upper_immediate(
        rt,
        ImmeValueType::Symbol(symbol.clone()),
        Some(TruncType::Hi),
    )));
    instrs.push(Box::new(FStoreInstr::new(
        rt,
        rs,
        ImmeValueType::Symbol(symbol),
        Some(TruncType::Lo),
    )));
    instrs
}

pub fn gen_icmp_riscv_instrs(
    rs1: Reg,
    rs2: Reg,
    cond_type: InstCond,
    rd: Reg,
    mapping_info: &mut MappingInfo,
) -> Vec<Box<dyn InstrTrait>> {
    assert!(*rs1.ty() == Type::Int);
    assert!(*rs2.ty() == Type::Int);
    assert!(*rd.ty() == Type::Int);
    let mut instrs: Vec<Box<dyn InstrTrait>> = vec![];
    match cond_type {
        InstCond::Eq => {
            instrs.push(Box::new(RegRegInstr::new(rd, rs1, rs2, RegRegType::Sub)));
            instrs.push(Box::new(RegInstr::new(rd, rd, RegType::Seqz)));
        }
        InstCond::Ne => {
            instrs.push(Box::new(RegRegInstr::new(rd, rs1, rs2, RegRegType::Sub)));
            instrs.push(Box::new(RegInstr::new(rd, rd, RegType::Snez)));
        }
        InstCond::Lt => {
            instrs.push(Box::new(RegRegInstr::new(rd, rs1, rs2, RegRegType::Sub)));
            instrs.push(Box::new(RegInstr::new(rd, rd, RegType::Sltz)));
        }
        InstCond::Le => {
            instrs.push(Box::new(RegRegInstr::new(rd, rs1, rs2, RegRegType::Sub)));
            let tmp = mapping_info.new_reg(Type::Int);
            instrs.push(Box::new(RegInstr::new(tmp, rd, RegType::Seqz)));
            instrs.push(Box::new(RegInstr::new(rd, rd, RegType::Sltz)));
            instrs.push(Box::new(RegRegInstr::new(rd, tmp, rd, RegRegType::Or)));
        }
        InstCond::Gt => {
            instrs.push(Box::new(RegRegInstr::new(rd, rs1, rs2, RegRegType::Sub)));
            instrs.push(Box::new(RegInstr::new(rd, rd, RegType::Sgtz)));
        }
        InstCond::Ge => {
            instrs.push(Box::new(RegRegInstr::new(rd, rs1, rs2, RegRegType::Sub)));
            let tmp = mapping_info.new_reg(Type::Int);
            instrs.push(Box::new(RegInstr::new(tmp, rd, RegType::Seqz)));
            instrs.push(Box::new(RegInstr::new(rd, rd, RegType::Sgtz)));
            instrs.push(Box::new(RegRegInstr::new(rd, tmp, rd, RegRegType::Or)));
        }
    }
    instrs
}

pub fn gen_fcmp_riscv_instr(
    rs1: Reg,
    rs2: Reg,
    cond_type: InstCond,
    rd: Reg,
) -> Vec<Box<dyn InstrTrait>> {
    assert!(*rs1.ty() == Type::Float);
    assert!(*rs2.ty() == Type::Float);
    assert!(*rd.ty() == Type::Int);
    let mut instrs: Vec<Box<dyn InstrTrait>> = vec![];
    match cond_type {
        InstCond::Eq => instrs.push(Box::new(FcmpInstr::new(rd, rs1, rs2, FcmpType::FeqS))),
        InstCond::Ne => {
            instrs.push(Box::new(FcmpInstr::new(rd, rs1, rs2, FcmpType::FeqS)));
            instrs.push(Box::new(RegInstr::new(rd, rd, RegType::Seqz)));
        }
        InstCond::Lt => instrs.push(Box::new(FcmpInstr::new(rd, rs1, rs2, FcmpType::FltS))),
        InstCond::Le => instrs.push(Box::new(FcmpInstr::new(rd, rs1, rs2, FcmpType::FleS))),
        InstCond::Gt => instrs.push(Box::new(FcmpInstr::new(rd, rs2, rs1, FcmpType::FltS))),
        InstCond::Ge => instrs.push(Box::new(FcmpInstr::new(rd, rs2, rs1, FcmpType::FleS))),
    }
    instrs
}
