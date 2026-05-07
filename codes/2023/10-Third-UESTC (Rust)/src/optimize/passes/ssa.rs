use super::dom::DominatorTreeBuilder;
use crate::frontend::llvm::{function::Function, instr::*, ssa::SSARightValue};
use std::collections::{HashMap, HashSet, VecDeque};

/// record the definition of each register
type BlockId = i32;
type PhiInstructionId = i32;

fn construct_defs(
    func_addr: usize,
    checking_regs: &HashSet<SSARightValue>,
) -> Vec<(SSARightValue, Vec<BlockId>)> {
    // recover func
    let func = unsafe { &mut *(func_addr as *mut Function) };

    let mut defs = HashMap::new();
    func.for_each_instr_closure_mut(|instruction| {
        let bb_id = instruction.bb_id();
        if let Some(reg_write_instr) = instruction.instr().try_as_reg_write_instr() {
            // println!("downcast ref success, construct_defs");
            let des_reg_opt = reg_write_instr.des_register();
            if let Some(des_reg) = des_reg_opt {
                if checking_regs.contains(&des_reg) {
                    defs.entry(des_reg.clone())
                        .or_insert_with(HashSet::new)
                        .insert(bb_id);
                }
            }
        }
    });
    defs.into_iter()
        .map(|(k, v)| (k, v.into_iter().collect::<Vec<_>>()))
        .collect()
}

///
fn phi_insertion(
    checking_reg: SSARightValue,
    func_addr: usize,
    defs: &Vec<BlockId>,
    dominator_frontier_map: &HashMap<BlockId, HashSet<BlockId>>,
    phis: &mut HashSet<PhiInstructionId>,
) {
    // recover func
    let func = unsafe { &mut *(func_addr as *mut Function) };

    let mut work_list: VecDeque<BlockId> = defs.into_iter().copied().collect();
    let mut f: HashSet<BlockId> = HashSet::new();
    let def_set: HashSet<BlockId> = defs.into_iter().copied().collect();

    let mut phi_new = |d1: SSARightValue, node: &BlockId| -> PhiInstructionId {
        let phi = Phi::new_by_reg(d1);
        let phi_box = Box::new(phi);
        let phi_instr = Instruction::new(phi_box, *node);
        func.add_instr2bb_at_front(phi_instr)
        // new a phi instr object
        // phis.insert(phi);
        // insert phi into the function block( df->bb->instrs.push_front)
        // func.bb_mut(df_node).unwrap().add_inst2bb(phis.last().unwrap().clone());
    };

    while let Some(node) = work_list.pop_front() {
        let df = dominator_frontier_map
            .get(&node)
            .expect("dominator frontier map is empty");
        for df_node in df.iter() {
            if !f.contains(df_node) {
                f.insert(*df_node);
                phis.insert(phi_new(checking_reg.clone(), df_node));
                if !def_set.contains(df_node) {
                    work_list.push_back(*df_node);
                }
            }
        }
    }
}

///
///
///
fn update_reaching_def(
    reaching_def: &mut Vec<i32>,
    def_node: &Vec<BlockId>,
    reg_id: i32,
    cur: BlockId,
    dom_tree_builder: &DominatorTreeBuilder,
) {
    assert!(reaching_def.len() == def_node.len());
    let mut x = reaching_def[reg_id as usize];
    while x > 0 {
        let node = def_node[x as usize];
        assert!(node != -1);
        if dom_tree_builder.is_dom(cur, node) {
            break;
        }
        x = reaching_def[x as usize];
    }
    reaching_def[reg_id as usize] = x;
}

///
///
///
fn variable_renaming(
    func_addr: usize,
    checking_regs: &HashSet<SSARightValue>,
    phi_ids: &mut HashSet<i32>,
    dom_tree_builder: &DominatorTreeBuilder,
) {
    let func = unsafe { &mut *(func_addr as *mut Function) };
    let mut reaching_def: Vec<i32> = Vec::new();
    let mut version_count: Vec<i32> = Vec::new();
    let mut origin_name: Vec<i32> = Vec::new();
    let mut def_node: Vec<BlockId> = Vec::new();
    reaching_def.resize((func.id() + 1) as usize, -1);
    version_count.resize((func.id() + 1) as usize, 0);
    origin_name.resize((func.id() + 1) as usize, 0);
    def_node.resize((func.id() + 1) as usize, -1);
    for reg in checking_regs {
        origin_name[*reg.id() as usize] = *reg.id();
    }

    for &node in dom_tree_builder.dfsn_order() {
        for instr_id in func.layout().inst_iter(node) {
            let func = unsafe { &mut *(func_addr as *mut Function) };
            let instr = func.instructions_mut().get_mut(&instr_id).unwrap();
            if instr.is_phi() {
                // skip phi
            } else {
                if let Some(reg_use_instr) = instr.instr_mut().try_as_reg_use_instr_mut() {
                    for use_reg in reg_use_instr.uses_mut() {
                        if checking_regs.contains(use_reg) {
                            update_reaching_def(
                                &mut reaching_def,
                                &def_node,
                                *use_reg.id(),
                                node,
                                dom_tree_builder,
                            );
                            assert!(
                                reaching_def[*use_reg.id() as usize] > -1,
                                "{}-{}: {:?}",
                                node,
                                instr_id,
                                reg_use_instr,
                            );
                            use_reg.set_id(reaching_def[*use_reg.id() as usize]);
                        }
                    }
                }
            }

            if let Some(reg_write_instr) = instr.instr_mut().try_as_reg_write_instr_mut() {
                if let Some(d1) = reg_write_instr.des_register_mut() {
                    let d1_id = *d1.id();
                    if checking_regs.contains(d1) {
                        update_reaching_def(
                            &mut reaching_def,
                            &def_node,
                            d1_id,
                            node,
                            dom_tree_builder,
                        );
                        let func = unsafe { &mut *(func_addr as *mut Function) }; // 无奈的选择
                        let ty = d1.get_type();
                        let mut new_version = func.new_reg(ty);
                        new_version.set_origin_id_and_version(Some((
                            d1_id,
                            version_count[d1_id as usize],
                        )));
                        version_count[d1_id as usize] += 1;
                        reaching_def.resize((func.id() + 1) as usize, -1);
                        version_count.resize((func.id() + 1) as usize, 0);
                        origin_name.resize((func.id() + 1) as usize, 0);
                        def_node.resize((func.id() + 1) as usize, -1);

                        let new_version_id = *new_version.id();
                        reaching_def[new_version_id as usize] = reaching_def[d1_id as usize];
                        def_node[new_version_id as usize] = node;
                        origin_name[new_version_id as usize] = origin_name[d1_id as usize];
                        reaching_def[d1_id as usize] = new_version_id;
                        d1.set_id(new_version_id);
                    }
                }
            }
        }

        let succ_bb_ids = func.basic_blocks().get(&node).unwrap().succ_bb().to_owned();
        for succ_bb_id in succ_bb_ids {
            for succ_bb_instr_id in func.layout().inst_iter(succ_bb_id) {
                let func = unsafe { &mut *(func_addr as *mut Function) };
                let instr = func.instructions_mut().get_mut(&succ_bb_instr_id).unwrap();
                if let Some(phi_instr) = instr.instr_mut().as_any_mut().downcast_mut::<Phi>() {
                    if phi_ids.contains(&succ_bb_instr_id) {
                        let reg_id = origin_name[*phi_instr.des_register().unwrap().id() as usize];
                        update_reaching_def(
                            &mut reaching_def,
                            &def_node,
                            reg_id,
                            node,
                            dom_tree_builder,
                        );
                        assert!(
                            reaching_def[*phi_instr.des_register().unwrap().id() as usize] > -1,
                            "current bb: {}, successor: {}-{}: {:?}",
                            node,
                            succ_bb_id,
                            succ_bb_instr_id,
                            phi_instr
                        );
                        let ty = phi_instr.d1().get_type();
                        phi_instr.add_use(
                            SSARightValue::new_reg(reaching_def[reg_id as usize], ty),
                            node,
                        );
                        // println!("{}: {:?}", succ_bb_instr_id, phi_instr);
                    } else {
                        for (reg, bb_id) in phi_instr.uses_mut() {
                            let reg_id = *reg.id();
                            if *bb_id == node && checking_regs.contains(reg) {
                                update_reaching_def(
                                    &mut reaching_def,
                                    &def_node,
                                    reg_id,
                                    node,
                                    dom_tree_builder,
                                );
                                let ty = reg.get_type();
                                *reg = SSARightValue::new_reg(reaching_def[reg_id as usize], ty);
                            }
                        }
                    }
                }
            }
        }
    }
}

///
pub fn ssa_construction(func: &mut Function, checking_regs: &HashSet<SSARightValue>) {
    let func_addr = func as *mut Function as usize;
    // build each basic block dominator frontier
    let mut dom_tree_builder = DominatorTreeBuilder::new(func);
    dom_tree_builder.build_dominators_map();
    dom_tree_builder.build_dominator_frontier();
    let dominator_frontier_map = dom_tree_builder.dominator_frontier_map();

    // define phi instructions
    let mut phis = HashSet::new();

    // construct defs
    let reg_with_defs = construct_defs(func_addr, checking_regs);
    // println!("reg_with_defs: {:?}", reg_with_defs);

    // insert phi
    for reg_with_def in reg_with_defs {
        // phase 1: phi insertion
        phi_insertion(
            reg_with_def.0,
            func_addr,
            &reg_with_def.1,
            dominator_frontier_map,
            &mut phis,
        );
    }

    // record the definition of each register
    variable_renaming(func_addr, checking_regs, &mut phis, &dom_tree_builder);
}

pub fn ssa_construction_without_renaming(
    func: &mut Function,
    checking_regs: &HashSet<SSARightValue>,
) {
    let func_addr = func as *mut Function as usize;
    // build each basic block dominator frontier
    let mut dom_tree_builder = DominatorTreeBuilder::new(func);
    dom_tree_builder.build_dominators_map();
    dom_tree_builder.build_dominator_frontier();
    let dominator_frontier_map = dom_tree_builder.dominator_frontier_map();

    // define phi instructions
    let mut phis = HashSet::new();

    // construct defs
    let reg_with_defs = construct_defs(func_addr, checking_regs);
    // println!("reg_with_defs: {:?}", reg_with_defs);

    // insert phi
    for reg_with_def in reg_with_defs {
        // phase 1: phi insertion
        phi_insertion(
            reg_with_def.0,
            func_addr,
            &reg_with_def.1,
            dominator_frontier_map,
            &mut phis,
        );
    }
}
