use std::collections::HashSet;

use super::{
    arch_info::{RegConvention, A0, FA0, SP, ZERO},
    function::Function,
    instr::*,
    misc::{InstCond, MappingInfo},
    register::Reg,
};
use crate::common::{
    constant::{ADDRESS_SIZE, BLOCK_LABEL_PREFIX, FLOAT_SIZE, INT_SIZE},
    immediate::Immediate,
    r#type::Type,
};
use crate::frontend::llvm::{
    self,
    function::Function as LLVMFunction,
    instr::{BinaryOp, UnaryOp},
};
use derive_new::new;
use getset::{Getters, MutGetters, Setters};

pub(crate) type BlockId = i32;
#[derive(new, Getters, Setters, MutGetters)]
pub struct Block {
    #[getset(get = "pub", get_mut = "pub")]
    id: BlockId,
    // #[getset(get = "pub")]
    // idx: BlockId, // position in function block vec
    // #[getset(get = "pub")]
    // bb_id: i32,
    #[getset(get = "pub")]
    name: String,
    #[new(default)]
    #[getset(get = "pub")]
    depth: i32,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    instrs: Vec<Box<dyn InstrTrait>>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    in_edges: Vec<BlockId>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    out_edges: Vec<BlockId>,
    // context
    // def: Vec<Reg>,
    // live_in: Vec<Reg>,
    // live_out: Vec<Reg>,
}

impl Block {
    pub(crate) fn construct_from_llvm_basic_block(
        blocks: &mut Vec<Block>,
        self_block_id: BlockId,
        next_block_id: Option<BlockId>,
        block_id_offset: BlockId,
        function: &mut Function,
        llvm_function: &LLVMFunction,
        mapping_info: &mut MappingInfo,
    ) {
        let self_bb_id = *mapping_info
            .rev_block_mapping()
            .get(&self_block_id)
            .unwrap();
        for inst_id in llvm_function.layout().inst_iter(self_bb_id) {
            let llvm_instr = llvm_function.instructions().get(&inst_id).unwrap().instr();
            let mut risc_v_instrs: Vec<Box<dyn InstrTrait>> = Vec::new();
            if let Some(alloc_instr) = llvm_instr.as_any().downcast_ref::<llvm::instr::Alloca>() {
                let addr = alloc_instr.addr();
                let reg = mapping_info.from_ssa_rvalue(addr);
                if addr.is_global() {
                    panic!("cannot alloca global variable");
                    // let symbol = addr.global_name().unwrap().to_string();
                    // risc_v_instrs.push(Box::new(LoadAddrInstr::new(reg, symbol.clone())));
                    // function.symbol_reg_mut().insert(reg, symbol);
                } else {
                    let addr_ssa_id = addr.id();
                    risc_v_instrs.push(Box::new(LoadStackAddrInstr::new(
                        reg,
                        mapping_info.obj_mapping().get(addr_ssa_id).unwrap().clone(),
                    )));
                    function.stack_addr_reg_mut().insert(
                        reg,
                        (
                            mapping_info.obj_mapping().get(addr_ssa_id).unwrap().clone(),
                            0,
                        ),
                    );
                }
            } else if let Some(unary_instr) = llvm_instr.try_as_unary_instr() {
                match unary_instr.unary_op() {
                    UnaryOp::Neg | UnaryOp::Not | UnaryOp::Mov => {
                        let rd = mapping_info.from_ssa_rvalue(unary_instr.des_register().unwrap());
                        let s1_ssa = unary_instr.s1();
                        let rs = if s1_ssa.is_immediate() {
                            let rs = mapping_info.new_reg(s1_ssa.ty());
                            let imme = s1_ssa.get_value().unwrap().into_int().unwrap();
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(rs, imme)));
                            rs
                        } else {
                            mapping_info.from_ssa_rvalue(s1_ssa)
                        };
                        let ty = match unary_instr.unary_op() {
                            UnaryOp::Neg => RegType::Negw,
                            UnaryOp::Mov => RegType::Mv,
                            UnaryOp::Not => RegType::Seqz,
                            _ => unreachable!(),
                        };
                        risc_v_instrs.push(Box::new(RegInstr::new(rd, rs, ty)));
                    }
                    UnaryOp::FMov => {
                        let rd = mapping_info.from_ssa_rvalue(unary_instr.des_register().unwrap());
                        let s1_ssa = unary_instr.s1();
                        assert!(s1_ssa.ty().is_float());
                        let rs = if s1_ssa.is_immediate() {
                            let imme = s1_ssa.get_value().unwrap().into_float().unwrap();
                            let int_s1 = mapping_info.new_reg(Type::Int);
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                                int_s1,
                                imme.to_bits() as _,
                            )));
                            let float_s1 = mapping_info.new_reg(Type::Float);
                            risc_v_instrs.push(Box::new(FRegRegInstr::new(
                                float_s1,
                                int_s1,
                                FRegRegConvertType::FmvWX,
                            )));
                            float_s1
                        } else {
                            mapping_info.from_ssa_rvalue(s1_ssa)
                        };
                        risc_v_instrs.push(Box::new(FRegInstr::new(rd, rs, FRegType::FmvS)));
                    }
                    UnaryOp::Fptosi => {
                        let s1_ssa = unary_instr.s1();
                        assert!(s1_ssa.ty().is_float());
                        let rs = if s1_ssa.is_immediate() {
                            let imme = s1_ssa.get_value().unwrap().into_float().unwrap();
                            let int_s1 = mapping_info.new_reg(Type::Int);
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                                int_s1,
                                imme.to_bits() as _,
                            )));
                            let float_s1 = mapping_info.new_reg(Type::Float);
                            risc_v_instrs.push(Box::new(FRegRegInstr::new(
                                float_s1,
                                int_s1,
                                FRegRegConvertType::FmvWX,
                            )));
                            float_s1
                        } else {
                            mapping_info.from_ssa_rvalue(s1_ssa)
                        };
                        let d1_ssa = unary_instr.des_register().unwrap();
                        assert!(d1_ssa.ty().is_int());
                        let rd = mapping_info.from_ssa_rvalue(d1_ssa);
                        risc_v_instrs.push(Box::new(FRegRegInstr::new(
                            rd,
                            rs,
                            FRegRegConvertType::FcvtWS,
                        )));
                    }
                    UnaryOp::Sitofp => {
                        let s1_ssa = unary_instr.s1();
                        assert!(s1_ssa.ty().is_int());
                        let rs = if s1_ssa.is_immediate() {
                            let rs = mapping_info.new_reg(s1_ssa.ty());
                            let imme = s1_ssa.get_value().unwrap().into_int().unwrap();
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(rs, imme)));
                            rs
                        } else {
                            mapping_info.from_ssa_rvalue(s1_ssa)
                        };
                        let d1_ssa = unary_instr.des_register().unwrap();
                        assert!(d1_ssa.ty().is_float());
                        let rd = mapping_info.from_ssa_rvalue(d1_ssa);
                        risc_v_instrs.push(Box::new(FRegRegInstr::new(
                            rd,
                            rs,
                            FRegRegConvertType::FcvtSW,
                        )));
                    }
                }
            } else if let Some(binary_instr) = llvm_instr.try_as_binary_instr() {
                let s1_ssa = binary_instr.s1();
                let s2_ssa = binary_instr.s2();
                let rd = mapping_info.from_ssa_rvalue(binary_instr.des_register().unwrap());
                match binary_instr.binary_op() {
                    BinaryOp::Add
                    | BinaryOp::Sub
                    | BinaryOp::Mul
                    | BinaryOp::Div
                    | BinaryOp::Mod
                    | BinaryOp::Shl
                    | BinaryOp::LShr
                    | BinaryOp::AShr => {
                        let rs1 = if s1_ssa.is_immediate() {
                            let imme = s1_ssa.get_value().unwrap().into_int().unwrap();
                            let rs1 = mapping_info.new_reg(s1_ssa.ty());
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(rs1, imme)));
                            rs1
                        } else {
                            mapping_info.from_ssa_rvalue(s1_ssa)
                        };
                        let rs2 = if s2_ssa.is_immediate() {
                            let imme = s2_ssa.get_value().unwrap().into_int().unwrap();
                            let rs2 = mapping_info.new_reg(s2_ssa.ty());
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(rs2, imme)));
                            rs2
                        } else {
                            mapping_info.from_ssa_rvalue(s2_ssa)
                        };
                        let ty = match binary_instr.binary_op() {
                            BinaryOp::Add => RegRegType::Addw,
                            BinaryOp::Sub => RegRegType::Subw,
                            BinaryOp::Mul => RegRegType::Mulw,
                            BinaryOp::Div => RegRegType::Divw,
                            BinaryOp::Mod => RegRegType::Remw,
                            BinaryOp::Shl => RegRegType::Sllw,
                            BinaryOp::LShr => RegRegType::Srlw,
                            BinaryOp::AShr => RegRegType::Sraw,
                            _ => unreachable!(),
                        };
                        risc_v_instrs.push(Box::new(RegRegInstr::new(rd, rs1, rs2, ty)));
                    }
                    BinaryOp::FAdd | BinaryOp::FSub | BinaryOp::FMul | BinaryOp::FDiv => {
                        let rs1 = if s1_ssa.is_immediate() {
                            let imme = s1_ssa.get_value().unwrap().into_float().unwrap();
                            let int_s1 = mapping_info.new_reg(Type::Int);
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                                int_s1,
                                imme.to_bits() as _,
                            )));
                            let rs1 = mapping_info.new_reg(Type::Float);
                            risc_v_instrs.push(Box::new(FRegRegInstr::new(
                                rs1,
                                int_s1,
                                FRegRegConvertType::FmvWX,
                            )));
                            rs1
                        } else {
                            mapping_info.from_ssa_rvalue(s1_ssa)
                        };
                        let rs2 = if s2_ssa.is_immediate() {
                            let imme = s2_ssa.get_value().unwrap().into_float().unwrap();
                            let int_s2 = mapping_info.new_reg(Type::Int);
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                                int_s2,
                                imme.to_bits() as _,
                            )));
                            let rs2 = mapping_info.new_reg(Type::Float);
                            risc_v_instrs.push(Box::new(FRegRegInstr::new(
                                rs2,
                                int_s2,
                                FRegRegConvertType::FmvWX,
                            )));
                            rs2
                        } else {
                            mapping_info.from_ssa_rvalue(s2_ssa)
                        };
                        let ty = match binary_instr.binary_op() {
                            BinaryOp::FAdd => FRegFRegType::Fadd,
                            BinaryOp::FSub => FRegFRegType::Fsub,
                            BinaryOp::FMul => FRegFRegType::Fmul,
                            BinaryOp::FDiv => FRegFRegType::Fdiv,
                            _ => unreachable!(),
                        };
                        risc_v_instrs.push(Box::new(FRegFRegInstr::new(rd, rs1, rs2, ty)));
                    }
                }
            } else if let Some(load_instr) = llvm_instr.as_any().downcast_ref::<llvm::instr::Load>()
            {
                let d1 = load_instr.d1();
                let rd = mapping_info.from_ssa_rvalue(d1);
                let addr = load_instr.addr();
                if addr.is_global() {
                    let symbol = addr.global_name().unwrap().to_string();
                    if *rd.ty() == Type::Int {
                        risc_v_instrs.push(Box::new(LoadGlobalInstr::new(rd, symbol)))
                    } else {
                        let rt = mapping_info.new_reg(Type::Int);
                        risc_v_instrs.extend(gen_fload_global(rd, symbol, rt))
                    }
                } else {
                    let rs1 = mapping_info.from_ssa_rvalue(addr);
                    if d1.is_addr() {
                        risc_v_instrs.push(Box::new(LoadInstr::new(rd, rs1, 0, LoadType::Ld)));
                    } else if *rd.ty() == Type::Int {
                        risc_v_instrs.push(Box::new(LoadInstr::new(rd, rs1, 0, LoadType::Lw)));
                    } else if *rd.ty() == Type::Float {
                        risc_v_instrs.push(Box::new(FLoadInstr::new(
                            rd,
                            rs1,
                            ImmeValueType::Direct(0),
                            None,
                        )));
                    } else {
                        unreachable!();
                    }
                }
            } else if let Some(store_instr) =
                llvm_instr.as_any().downcast_ref::<llvm::instr::Store>()
            {
                let s1_ssa = store_instr.s1();
                let rs = if s1_ssa.is_immediate() {
                    if s1_ssa.ty() == Type::Int {
                        let rs = mapping_info.new_reg(s1_ssa.ty());
                        let imme = s1_ssa.get_value().unwrap().into_int().unwrap();
                        risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(rs, imme)));
                        rs
                    } else if s1_ssa.ty() == Type::Float {
                        let imme = s1_ssa.get_value().unwrap().into_float().unwrap();
                        let int_rs = mapping_info.new_reg(Type::Int);
                        risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                            int_rs,
                            imme.to_bits() as _,
                        )));
                        let rs = mapping_info.new_reg(Type::Float);
                        risc_v_instrs.push(Box::new(FRegRegInstr::new(
                            rs,
                            int_rs,
                            FRegRegConvertType::FmvWX,
                        )));
                        rs
                    } else {
                        unreachable!()
                    }
                } else {
                    mapping_info.from_ssa_rvalue(s1_ssa)
                };
                let addr = store_instr.addr();
                if addr.is_global() {
                    let symbol = addr.global_name().unwrap().to_string();
                    let rt = mapping_info.new_reg(Type::Int);
                    if *rs.ty() == Type::Int {
                        risc_v_instrs.extend(gen_store_global(rs, symbol, rt));
                    } else {
                        risc_v_instrs.extend(gen_fstore_global(rs, symbol, rt));
                    }
                } else {
                    let rs1 = mapping_info.from_ssa_rvalue(addr);
                    if s1_ssa.is_addr() {
                        risc_v_instrs.push(Box::new(StoreInstr::new(
                            rs1,
                            rs,
                            ImmeValueType::Direct(0),
                            None,
                            StoreType::Sd,
                        )));
                    } else if *rs.ty() == Type::Int {
                        risc_v_instrs.push(Box::new(StoreInstr::new(
                            rs1,
                            rs,
                            ImmeValueType::Direct(0),
                            None,
                            StoreType::Sw,
                        )));
                    } else if *rs.ty() == Type::Float {
                        risc_v_instrs.push(Box::new(FStoreInstr::new(
                            rs1,
                            rs,
                            ImmeValueType::Direct(0),
                            None,
                        )));
                    } else {
                        unreachable!()
                    }
                }
            } else if let Some(gep_instr) = llvm_instr.as_any().downcast_ref::<llvm::instr::Gep>() {
                // todo: address
                let rd = mapping_info.from_ssa_rvalue(gep_instr.d1());
                let base_addr = gep_instr.s1();
                let base = mapping_info.from_ssa_rvalue(base_addr);
                if base_addr.is_global() {
                    risc_v_instrs.push(Box::new(LoadAddrInstr::new(
                        base,
                        base_addr.global_name().unwrap().to_string(),
                    )));
                }
                let index_ssa = gep_instr.index();
                let index = if index_ssa.is_immediate() {
                    let imme = index_ssa.get_value().unwrap().into_int().unwrap();
                    let index = mapping_info.new_reg(Type::Int);
                    risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(index, imme)));
                    index
                } else {
                    mapping_info.from_ssa_rvalue(index_ssa)
                };
                let (_, ty, base_shape, _, _) = base_addr.inner().clone().into_address().unwrap();
                let step: i32 = base_shape[1..].iter().product::<i32>() * ty.size() as i32;
                let step_reg = mapping_info.new_reg(Type::Int);
                risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(step_reg, step)));
                let offset_reg = mapping_info.new_reg(Type::Int);
                risc_v_instrs.push(Box::new(RegRegInstr::new(
                    offset_reg,
                    index,
                    step_reg,
                    RegRegType::Mul,
                )));
                risc_v_instrs.push(Box::new(RegRegInstr::new(
                    rd,
                    base,
                    offset_reg,
                    RegRegType::Add,
                )));
            } else if let Some(icmp_instr) = llvm_instr.as_any().downcast_ref::<llvm::instr::Icmp>()
            {
                let s1_ssa = icmp_instr.s1();
                let rs1 = if s1_ssa.is_immediate() {
                    let imme = s1_ssa.get_value().unwrap().into_int().unwrap();
                    let s1 = mapping_info.new_reg(s1_ssa.ty());
                    risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(s1, imme)));
                    s1
                } else {
                    mapping_info.from_ssa_rvalue(s1_ssa)
                };
                let s2_ssa = icmp_instr.s2();
                let rs2 = if s2_ssa.is_immediate() {
                    let imme = s2_ssa.get_value().unwrap().into_int().unwrap();
                    let s2 = mapping_info.new_reg(s2_ssa.ty());
                    risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(s2, imme)));
                    s2
                } else {
                    mapping_info.from_ssa_rvalue(s2_ssa)
                };
                let rd = mapping_info.from_ssa_rvalue(icmp_instr.d1());
                let cmp_instrs = gen_icmp_riscv_instrs(
                    rs1,
                    rs2,
                    InstCond::from_llvm_cmp_type(icmp_instr.cmp_type()),
                    rd,
                    mapping_info,
                );
                risc_v_instrs.extend(cmp_instrs);
            } else if let Some(fcmp_instr) = llvm_instr.as_any().downcast_ref::<llvm::instr::Fcmp>()
            {
                let s1_ssa = fcmp_instr.s1();
                let rs1 = if s1_ssa.is_immediate() {
                    let imme = s1_ssa.get_value().unwrap().into_float().unwrap();
                    let int_s1 = mapping_info.new_reg(Type::Int);
                    risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                        int_s1,
                        imme.to_bits() as _,
                    )));
                    let float_s1 = mapping_info.new_reg(Type::Float);
                    risc_v_instrs.push(Box::new(FRegRegInstr::new(
                        float_s1,
                        int_s1,
                        FRegRegConvertType::FmvWX,
                    )));
                    float_s1
                } else {
                    mapping_info.from_ssa_rvalue(s1_ssa)
                };
                let s2_ssa = fcmp_instr.s2();
                let rs2 = if s2_ssa.is_immediate() {
                    let imme = s2_ssa.get_value().unwrap().into_float().unwrap();
                    let int_s2 = mapping_info.new_reg(Type::Int);
                    risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                        int_s2,
                        imme.to_bits() as _,
                    )));
                    let rs2 = mapping_info.new_reg(Type::Float);
                    risc_v_instrs.push(Box::new(FRegRegInstr::new(
                        rs2,
                        int_s2,
                        FRegRegConvertType::FmvWX,
                    )));
                    rs2
                } else {
                    mapping_info.from_ssa_rvalue(s2_ssa)
                };
                let rd = mapping_info.from_ssa_rvalue(fcmp_instr.d1());
                let cmp_instrs = gen_fcmp_riscv_instr(
                    rs1,
                    rs2,
                    InstCond::from_llvm_cmp_type(fcmp_instr.cmp_type()),
                    rd,
                );
                risc_v_instrs.extend(cmp_instrs);
            } else if let Some(ret_instr) = llvm_instr.as_any().downcast_ref::<llvm::instr::Ret>() {
                if let Some(ret_value) = ret_instr.value() {
                    if ret_value.ty().is_float() {
                        if ret_value.is_immediate() {
                            let imme = ret_value.get_value().unwrap().into_float().unwrap();
                            let int_ret_value = mapping_info.new_reg(Type::Int);
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                                int_ret_value,
                                imme.to_bits() as _,
                            )));
                            risc_v_instrs.push(Box::new(FRegRegInstr::new(
                                Reg::new(FA0, Type::Float), // f10 f11 are return value register
                                int_ret_value,
                                FRegRegConvertType::FmvWX,
                            )));
                        } else {
                            let float_ret_value = mapping_info.from_ssa_rvalue(ret_value);
                            risc_v_instrs.push(Box::new(FRegInstr::new(
                                Reg::new(FA0, Type::Float), // f10 f11 are return value register
                                float_ret_value,
                                FRegType::FmvS,
                            )));
                        }
                    } else if ret_value.ty().is_int() {
                        if ret_value.is_immediate() {
                            let imme = ret_value.get_value().unwrap().into_int().unwrap();
                            risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                                Reg::new(A0, Type::Int), // x10 x11 are return value register
                                imme,
                            )));
                        } else {
                            let int_ret_value = mapping_info.from_ssa_rvalue(ret_value);
                            risc_v_instrs.push(Box::new(RegInstr::new(
                                Reg::new(A0, Type::Int), // f10 f11 are return value register
                                int_ret_value,
                                RegType::Mv,
                            )))
                        }
                    } else {
                        unimplemented!("unimplemented ret value type: {:?}", ret_value.ty());
                    }
                }
                risc_v_instrs.push(Box::new(ReturnInstr::new(function.ctx().clone())));
            } else if let Some(branch_instr) =
                llvm_instr.as_any().downcast_ref::<llvm::instr::Branch>()
            {
                let true_target_block_id = *mapping_info
                    .block_mapping()
                    .get(&branch_instr.label1)
                    .unwrap();
                if let Some(cond) = &branch_instr.cond {
                    let rd = mapping_info.from_ssa_rvalue(&cond);
                    let false_target_block_id = *mapping_info
                        .block_mapping()
                        .get(&branch_instr.label2.unwrap())
                        .unwrap();
                    // 如果条件为真的语句块不是下一个语句块，那么需要跳转到真的语句块
                    if next_block_id.is_none() || true_target_block_id != next_block_id.unwrap() {
                        let label = BLOCK_LABEL_PREFIX.to_string()
                            + true_target_block_id.to_string().as_str();
                        risc_v_instrs.push(Box::new(BranchInstr::new(
                            rd,
                            Reg::new(ZERO, Type::Int),
                            label,
                            BranchType::Bne,
                        )));
                        // 如果条件为假的语句块是下一个语句块，那么就不需要插入条件为假跳转到假的语句块
                        if next_block_id.is_none()
                            || false_target_block_id != next_block_id.unwrap()
                        {
                            let label = BLOCK_LABEL_PREFIX.to_string()
                                + false_target_block_id.to_string().as_str();
                            risc_v_instrs.push(Box::new(JumpInstr::new_jump(label)));
                        }
                    }
                    // 如果条件为真的语句块是下一个语句块，那么直接插入条件为假跳转到假的语句块
                    else {
                        // 条件为假的目标语句块一定不是下一个语句块
                        let label = BLOCK_LABEL_PREFIX.to_string()
                            + false_target_block_id.to_string().as_str();
                        risc_v_instrs.push(Box::new(BranchInstr::new(
                            rd,
                            Reg::new(ZERO, Type::Int),
                            label,
                            BranchType::Beq,
                        )));
                    }

                    blocks[(self_block_id - block_id_offset) as usize]
                        .add_out_edge(true_target_block_id);
                    blocks[(true_target_block_id - block_id_offset) as usize]
                        .add_in_edge(self_block_id);
                    blocks[(self_block_id - block_id_offset) as usize]
                        .add_out_edge(false_target_block_id);
                    blocks[(false_target_block_id - block_id_offset) as usize]
                        .add_in_edge(self_block_id);
                } else {
                    if next_block_id.is_none() || true_target_block_id != next_block_id.unwrap() {
                        risc_v_instrs.push(Box::new(JumpInstr::new_jump(
                            BLOCK_LABEL_PREFIX.to_string()
                                + true_target_block_id.to_string().as_str(),
                        )));
                    }
                    blocks[(self_block_id - block_id_offset) as usize]
                        .add_out_edge(true_target_block_id);
                    blocks[(true_target_block_id - block_id_offset) as usize]
                        .add_in_edge(self_block_id);
                }
            } else if let Some(call_instr) = llvm_instr.as_any().downcast_ref::<llvm::instr::Call>()
            {
                let mut is_addr_set = HashSet::new();
                let mut int_arg_cnt = 0;
                let mut float_arg_cnt = 0;
                let arg_reg_vec = call_instr
                    .args()
                    .iter()
                    .map(|arg| {
                        if arg.is_immediate() {
                            if arg.ty().is_int() {
                                int_arg_cnt += 1;
                            } else if arg.ty().is_float() {
                                float_arg_cnt += 1;
                            } else {
                                unimplemented!("unimplemented arg type: {:?}", arg.ty());
                            }
                            let imme = arg.get_value().unwrap();
                            match imme {
                                Immediate::Int(int) => {
                                    let rs = mapping_info.new_reg(arg.ty());
                                    risc_v_instrs
                                        .push(Box::new(ImmeInstr::new_load_immediate(rs, int)));
                                    rs
                                }
                                Immediate::Float(float) => {
                                    let rs_int = mapping_info.new_reg(Type::Int);
                                    risc_v_instrs.push(Box::new(ImmeInstr::new_load_immediate(
                                        rs_int,
                                        float.to_bits() as _,
                                    )));
                                    let rs_float = mapping_info.new_reg(Type::Float);
                                    risc_v_instrs.push(Box::new(FRegRegInstr::new(
                                        rs_float,
                                        rs_int,
                                        FRegRegConvertType::FmvWX,
                                    )));
                                    rs_float
                                }
                            }
                        } else {
                            let reg = mapping_info.from_ssa_rvalue(arg);
                            if arg.is_addr() {
                                is_addr_set.insert(reg);
                                int_arg_cnt += 1;
                            } else if arg.ty().is_int() {
                                int_arg_cnt += 1;
                            } else if arg.ty().is_float() {
                                float_arg_cnt += 1;
                            } else {
                                unimplemented!("unimplemented arg type: {:?}", arg.ty());
                            }
                            reg
                        }
                    })
                    .collect::<Vec<_>>();
                let int_arg_size = int_arg_cnt;
                let float_arg_size = float_arg_cnt;
                let mut stack_passed = 0;
                if int_arg_size > RegConvention::<i32>::ARGUMENT_REGISTER_COUNT {
                    let mut in_reg_int_arg = 0;
                    for stack_arg in &arg_reg_vec {
                        if stack_arg.ty() == &Type::Int {
                            in_reg_int_arg += 1;
                            if in_reg_int_arg <= RegConvention::<i32>::ARGUMENT_REGISTER_COUNT {
                                continue;
                            }
                            if is_addr_set.contains(stack_arg) {
                                stack_passed += ADDRESS_SIZE as i32;
                            } else {
                                stack_passed += INT_SIZE as i32;
                            }
                        }
                    }
                }
                if float_arg_size > RegConvention::<f32>::ARGUMENT_REGISTER_COUNT {
                    stack_passed += (float_arg_size - RegConvention::<f32>::ARGUMENT_REGISTER_COUNT)
                        as i32
                        * FLOAT_SIZE as i32;
                }

                // stack address align, 16 byte
                let align_size = if stack_passed % 16 != 0 {
                    let align_size = 16 - stack_passed % 16;
                    stack_passed += align_size;
                    align_size
                } else {
                    0
                };

                let mut offset = -align_size;
                for rs in arg_reg_vec.iter().rev() {
                    if rs.ty().is_int() {
                        int_arg_cnt -= 1;
                        if int_arg_cnt >= RegConvention::<i32>::ARGUMENT_REGISTER_COUNT {
                            if is_addr_set.contains(&rs) {
                                offset -= ADDRESS_SIZE as i32;
                                risc_v_instrs.push(Box::new(StoreInstr::new(
                                    Reg::new_int(SP),
                                    *rs,
                                    ImmeValueType::Direct(offset),
                                    None,
                                    StoreType::Sd,
                                )));
                            } else {
                                offset -= INT_SIZE as i32;
                                risc_v_instrs.push(Box::new(StoreInstr::new(
                                    Reg::new_int(SP),
                                    *rs,
                                    ImmeValueType::Direct(offset),
                                    None,
                                    StoreType::Sw,
                                )));
                            }
                        } else {
                            risc_v_instrs.push(Box::new(RegInstr::new(
                                Reg::new_int(
                                    RegConvention::<i32>::ARGUMENT_REGISTERS[int_arg_cnt as usize],
                                ),
                                *rs,
                                RegType::Mv,
                            )));
                        }
                    } else if rs.ty().is_float() {
                        float_arg_cnt -= 1;
                        if float_arg_cnt >= RegConvention::<f32>::ARGUMENT_REGISTER_COUNT {
                            offset -= FLOAT_SIZE as i32;
                            risc_v_instrs.push(Box::new(FStoreInstr::new(
                                Reg::new_int(SP),
                                *rs,
                                ImmeValueType::Direct(offset),
                                None,
                            )));
                        } else {
                            risc_v_instrs.push(Box::new(FRegInstr::new(
                                Reg::new_float(
                                    RegConvention::<f32>::ARGUMENT_REGISTERS
                                        [float_arg_cnt as usize],
                                ),
                                *rs,
                                FRegType::FmvS,
                            )));
                        }
                    } else {
                        unimplemented!("unimplemented arg type: {:?}", rs.ty());
                    }
                }
                {
                    // if function.name() == "params_mix" && call_instr.func_name() == "params_mix" {
                    //     println!("stack passed: {}", stack_passed);
                    //     for arg_ssa in call_instr.args() {
                    //         if let Some(addr) = arg_ssa.inner().as_address() {
                    //             println!("addr: {:?}", addr);
                    //         } else {
                    //             println!("scalar: {:?}", arg_ssa.inner());
                    //         }
                    //     }
                    // }
                    assert!(
                        offset == (stack_passed as i32 * -1),
                        "offset: {}, stack_passed: -{}, function {} call for {}",
                        offset,
                        stack_passed,
                        function.name(),
                        call_instr.func_name()
                    );
                }

                if stack_passed > 0 {
                    risc_v_instrs.push(Box::new(ChangeSPInstr::new(stack_passed * -1)));
                }
                risc_v_instrs.push(Box::new(CallInstr::new(
                    call_instr.func_name().to_string(),
                    int_arg_size,
                    float_arg_size,
                )));
                if stack_passed > 0 {
                    risc_v_instrs.push(Box::new(ChangeSPInstr::new(stack_passed)));
                }

                if let Some(ret) = call_instr.ret() {
                    let rd = mapping_info.from_ssa_rvalue(ret);
                    if ret.ty().is_float() {
                        risc_v_instrs.push(Box::new(FRegInstr::new(
                            rd,
                            Reg::new(FA0, Type::Float),
                            FRegType::FmvS,
                        )));
                    } else if ret.ty().is_int() {
                        risc_v_instrs.push(Box::new(RegInstr::new(
                            rd,
                            Reg::new(A0, Type::Int),
                            RegType::Mv,
                        )));
                    } else {
                        unimplemented!("unimplemented ret type: {:?}", ret.ty());
                    }
                }
            } else {
                unimplemented!(
                    "unimplemented llvm instr: ({}-{}): {:?}",
                    self_bb_id,
                    inst_id,
                    llvm_instr
                );
            };
            blocks[(self_block_id - block_id_offset) as usize]
                .instrs
                .extend(risc_v_instrs);
        }
    }
    pub(crate) fn gen_asm(&self) -> String {
        let mut asm = String::new();
        asm.push_str(format!("{}:\n", self.name).as_str());
        for instr in &self.instrs {
            asm.push_str(&instr.gen_asm());
        }
        asm
    }
    pub(crate) fn push_back(&mut self, instr: Box<dyn InstrTrait>) {
        self.instrs.push(instr);
    }
    pub(crate) fn add_out_edge(&mut self, block_id: BlockId) {
        assert!(!self.out_edges.contains(&block_id));
        self.out_edges.push(block_id);
    }
    pub(crate) fn add_in_edge(&mut self, block_id: BlockId) {
        assert!(!self.in_edges.contains(&block_id));
        self.in_edges.push(block_id);
    }
}
