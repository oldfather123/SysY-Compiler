use super::ssa::{ssa_construction, ssa_construction_without_renaming};
use crate::common::{immediate::Immediate, r#type::Type};
use crate::frontend::llvm::{
    function::Function,
    instr::{Alloca, FMov, Instruction, Load, Mov, Store},
    llvm_module::LLVMModule,
    ssa::SSARightValue,
};
use std::collections::{HashMap, HashSet};

/// mem2reg for function
fn mem2reg_for_func(func: &mut Function) -> HashSet<SSARightValue> {
    let mut mem2reg = HashMap::new();
    let mut value_regs = HashSet::new();

    let func_addr = func as *const Function as usize;
    func.for_each_mut_instr_closure_mut(|instruction| {
        if let Some(alloc_instr) = instruction.instr().as_any().downcast_ref::<Alloca>() {
            let addr = alloc_instr.addr();
            let func = unsafe { &mut *(func_addr as *mut Function) }; // 无奈的选择
            let mem_obj = func.mem_scope().objects().get(addr.id()).unwrap();
            if mem_obj.is_promotable() {
                if !mem2reg.contains_key(addr) {
                    let value_reg = func.new_reg(addr.get_type());
                    value_regs.insert(value_reg.clone());
                    mem2reg.insert(addr.clone(), value_reg.clone());
                    if value_reg.get_type() == Type::Int {
                        *instruction = Instruction::new(
                            Box::new(Mov::new(
                                value_reg.clone(),
                                SSARightValue::new_imme(Immediate::new_int(0)),
                            )),
                            instruction.bb_id(),
                        );
                    } else if value_reg.get_type() == Type::Float {
                        *instruction = Instruction::new(
                            Box::new(FMov::new(
                                value_reg.clone(),
                                SSARightValue::new_imme(Immediate::new_float(0.0)),
                            )),
                            instruction.bb_id(),
                        );
                    } else {
                        panic!("mem2reg_for_func: unknown type");
                    }
                }
                // addr2mem
            }
        } else if let Some(load_instr) = instruction.instr().as_any().downcast_ref::<Load>() {
            let addr = load_instr.addr();
            let reg = load_instr.d1();
            if mem2reg.contains_key(addr) {
                let value_reg = mem2reg.get(addr).unwrap();
                if value_reg.get_type() == Type::Int {
                    *instruction = Instruction::new(
                        Box::new(Mov::new(reg.clone(), value_reg.clone())),
                        instruction.bb_id(),
                    );
                } else if value_reg.get_type() == Type::Float {
                    *instruction = Instruction::new(
                        Box::new(FMov::new(reg.clone(), value_reg.clone())),
                        instruction.bb_id(),
                    );
                } else {
                    panic!("mem2reg_for_func: unknown type");
                }
            }
        } else if let Some(store_instr) = instruction.instr().as_any().downcast_ref::<Store>() {
            let addr = store_instr.addr();
            let reg = store_instr.s1();
            if mem2reg.contains_key(addr) {
                let value_reg = mem2reg.get(addr).unwrap();
                if value_reg.get_type() == Type::Int {
                    *instruction = Instruction::new(
                        Box::new(Mov::new(value_reg.clone(), reg.clone())),
                        instruction.bb_id(),
                    );
                } else if value_reg.get_type() == Type::Float {
                    *instruction = Instruction::new(
                        Box::new(FMov::new(value_reg.clone(), reg.clone())),
                        instruction.bb_id(),
                    );
                } else {
                    panic!("mem2reg_for_func: unknown type");
                }
            }
        } else {
            // do nothing
        }
    });
    value_regs
}

/// mem2reg for llvm module
pub fn mem2reg(module: &mut LLVMModule) {
    // ssa for each user function
    module.for_each_user_func_mut(|f| {
        let value_reg = mem2reg_for_func(f);
        // println!("mem2reg_for_func {}: {:?}", f.name(), value_reg);
        ssa_construction(f, &value_reg);
    });
}

/// mem2reg for llvm module
pub fn only_mem2reg(module: &mut LLVMModule) {
    // ssa for each user function
    module.for_each_user_func_mut(|f| {
        let value_reg = mem2reg_for_func(f);
        println!("mem2reg_for_func {}: {:?}", f.name(), value_reg);
    });
}

pub fn mem2reg_without_renaming(module: &mut LLVMModule) {
    // ssa for each user function
    module.for_each_user_func_mut(|f| {
        let value_reg = mem2reg_for_func(f);
        println!("mem2reg_for_func {}: {:?}", f.name(), value_reg);
        ssa_construction_without_renaming(f, &value_reg);
    });
}
