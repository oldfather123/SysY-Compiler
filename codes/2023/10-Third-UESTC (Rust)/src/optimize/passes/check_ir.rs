use crate::frontend::llvm::{function::Function, instr::*, llvm_module::LLVMModule};
use std::collections::{HashMap, HashSet};

fn build_defs(f: &Function) -> HashMap<i32, i32> {
    let mut defs = HashMap::new();
    for arg in f.arg_list().as_normal().unwrap().iter() {
        defs.insert(*arg.id(), 0); // instruction id 0 is reserved
    }
    for bb_id in f.layout().block_iter() {
        for instr_id in f.layout().inst_iter(bb_id) {
            let instr = f.instructions().get(&instr_id).unwrap();
            if let Some(regwr_ir) = instr.instr().try_as_reg_write_instr() {
                if let Some(d1) = regwr_ir.des_register() {
                    defs.insert(*d1.id(), instr_id);
                }
            }
        }
    }
    defs
}

fn check_ir(f: &Function) {
    let defs = build_defs(f);
    for bb_id in f.layout().block_iter() {
        let bb = f.bb(bb_id).unwrap();
        let prev = bb.prev_bb();
        for instr_id in f.layout().inst_iter(bb_id) {
            let instr = f.instructions().get(&instr_id).unwrap();
            if let Some(phi_instr) = instr.instr().as_any().downcast_ref::<Phi>() {
                let mut flag = false;
                let mut flag1 = false;
                let mut set: HashSet<_> = HashSet::new();
                for (_, v) in phi_instr.uses() {
                    if !prev.contains(v) {
                        flag = true;
                    }
                    if !set.insert(v) {
                        flag1 = true;
                    }
                }
                if flag || flag1 || phi_instr.uses().len() != prev.len() {
                    for bb0 in prev {
                        log::debug!("{}", bb0);
                    }
                    log::debug!("bb:{}", bb_id);
                    panic!("phi instruction {} is not valid", instr_id);
                }
            } else if let Some(br_instr) = instr.instr().as_any().downcast_ref::<Branch>() {
                if br_instr.label2.is_some() {
                    assert!(br_instr.label1 != br_instr.label2.unwrap());
                }
            } else if let Some(regwr_ir) = instr.instr().try_as_reg_write_instr() {
                if regwr_ir.des_register().is_some() {
                    assert!(defs[regwr_ir.des_register().unwrap().id()] == instr_id);
                }
            }

            if let Some(reg_use_instr) = instr.instr().try_as_reg_use_instr() {
                for reg in reg_use_instr.uses() {
                    if reg.is_immediate() || reg.is_global() {
                        continue;
                    }
                    if !defs.contains_key(reg.id()) {
                        panic!("reg {} is not defined", reg.id());
                    }
                }
            }
        }
        assert!(f.layout().inst_iter(bb_id).next() != None);
        let last_instr_id = f
            .layout()
            .basic_blocks()
            .get(&bb_id)
            .unwrap()
            .last_inst()
            .unwrap();
        let last_instr = f.instructions().get(&last_instr_id).unwrap();
        if last_instr.is_branch() || last_instr.is_ret() {
            continue;
        } else {
            panic!("bb {} does not end with branch or ret", bb_id);
        }
    }
}

pub fn check_module(module: &LLVMModule) {
    module.for_each_user_func(check_ir);
}
