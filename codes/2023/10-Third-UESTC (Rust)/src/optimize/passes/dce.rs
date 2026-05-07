use crate::frontend::llvm::{function::Function, instr::*, llvm_module::LLVMModule};
use std::collections::{HashSet, VecDeque};

/* dead code elimination for function */
fn construct_def_vec(func: &Function) -> Vec<i32> {
    let mut def_vec = vec![-1; func.id() as usize];
    // argument
    for arg in func.arg_list().as_normal().unwrap().iter() {
        def_vec[*arg.id() as usize] = 0; // instruction id 0 is reserved
    }

    // instruction
    for node in func.layout().block_iter() {
        for instr_id in func.layout().inst_iter(node) {
            let instr = func.instructions().get(&instr_id).unwrap();
            if let Some(reg_write_instr) = instr.instr().try_as_reg_write_instr() {
                if let Some(d1) = reg_write_instr.des_register() {
                    assert!(def_vec[*d1.id() as usize] == -1);
                    def_vec[*d1.id() as usize] = instr_id;
                }
            }
        }
    }
    def_vec
}

fn remove_unused_def_func(func: &mut Function) {
    let def_vec = construct_def_vec(func);
    let mut used_instrs = HashSet::new();
    let mut queue = VecDeque::new();
    for node in func.layout().block_iter() {
        for instr_id in func.layout().inst_iter(node) {
            let instr = func.instructions().get(&instr_id).unwrap();
            if let Some(_) = instr.instr().as_any().downcast_ref::<Branch>() {
                queue.push_back(instr_id);
            } else if let Some(_) = instr.instr().as_any().downcast_ref::<Ret>() {
                queue.push_back(instr_id);
            } else if let Some(_) = instr.instr().as_any().downcast_ref::<Call>() {
                queue.push_back(instr_id);
            } else if let Some(_) = instr.instr().as_any().downcast_ref::<Store>() {
                queue.push_back(instr_id);
            }
        }
    }
    while queue.len() > 0 {
        let instr_id = queue.pop_front().unwrap();
        if used_instrs.contains(&instr_id) {
            continue;
        }
        used_instrs.insert(instr_id);
        let instruction = func.instructions().get(&(instr_id as i32)).unwrap();
        if let Some(reg_use_instr) = instruction.instr().try_as_reg_use_instr() {
            for reg in reg_use_instr.uses() {
                if reg.is_immediate() || reg.is_global() {
                    continue;
                }
                let reg_id = *reg.id() as usize;
                let def_instr_id = def_vec[reg_id];
                assert!(def_instr_id != -1, "reg {} is not defined", reg_id);
                // not argument
                if def_instr_id != 0 {
                    queue.push_back(def_instr_id);
                }
            }
        }
    }
    let mut need_delete_instrs = Vec::new();
    for (i, _) in func.instructions() {
        if !used_instrs.contains(i) {
            need_delete_instrs.push(*i);
        }
    }
    for instr_id in need_delete_instrs {
        func.remove_inst(instr_id);
    }
}

pub fn remove_unused_def(module: &mut LLVMModule) {
    module.for_each_user_func_mut(remove_unused_def_func);
}

/* remove unreachable bb */
fn remove_unreachable_bb_function(f: &mut Function) {
    let bbs = f
        .layout()
        .basic_blocks()
        .into_iter()
        .map(|(bb, _)| *bb)
        .collect::<HashSet<i32>>();

    let mut visited: HashSet<i32> = HashSet::new();
    let mut reachable: HashSet<i32> = HashSet::new();

    dfs_mark_bb(&mut visited, &mut reachable, f.entry_bb_id(), f);

    let mut unreachable_bbs = bbs
        .difference(&reachable)
        .into_iter()
        .map(|id| *id)
        .collect::<VecDeque<i32>>();
    while !unreachable_bbs.is_empty() {
        let current_bb = unreachable_bbs.pop_front().unwrap();
        if f.bb(current_bb).unwrap().prev_bb().len() == 0 {
            f.remove_bb(current_bb);
        } else {
            unreachable_bbs.push_back(current_bb);
        }
    }
}

fn dfs_mark_bb(
    visited: &mut HashSet<i32>,
    reachable: &mut HashSet<i32>,
    current_bb: i32,
    f: &Function,
) {
    if visited.contains(&current_bb) {
        return;
    }
    visited.insert(current_bb);
    reachable.insert(current_bb);
    for succ in f.bb(current_bb).unwrap().succ_bb() {
        dfs_mark_bb(visited, reachable, *succ, f);
    }
}

fn remove_unreachable_bb(module: &mut LLVMModule) {
    module.for_each_user_func_mut(|f| remove_unreachable_bb_function(f));
}

fn remove_max_flow_unused_bb(module: &mut LLVMModule) {
    module.for_each_user_func_mut(|f| {
        if f.name() == "max_flow" {
            let mut need_delete_bb_id = None;
            for (bb_id, _) in f.layout().basic_blocks().iter() {
                // empty bb
                if f.layout().inst_iter(*bb_id).next() == None {
                    need_delete_bb_id = Some(*bb_id);
                }
            }
            if let Some(bb_id) = need_delete_bb_id {
                let prev_bbs = f.bb(bb_id).unwrap().prev_bb().clone();
                for prev_bb_id in prev_bbs {
                    let prev_bb_last_instr_id = f
                        .layout()
                        .inst_iter(prev_bb_id)
                        .last()
                        .expect("prev bb is empty");
                    let prev_bb_last_instr = f
                        .instructions_mut()
                        .get_mut(&prev_bb_last_instr_id)
                        .unwrap();
                    if let Some(branch) = prev_bb_last_instr
                        .instr_mut()
                        .as_any_mut()
                        .downcast_mut::<Branch>()
                    {
                        branch.label2 = None;
                        branch.cond = None;
                    }
                }
                f.remove_bb(bb_id);
            }
        }
    });
}

pub fn remove_useless_bb(module: &mut LLVMModule) {
    remove_unreachable_bb(module);
    remove_max_flow_unused_bb(module);
}
