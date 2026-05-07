use std::collections::{HashMap};
use std::hash::Hash;
use crate::allow_nothing;
use crate::mir::{MIRFun,MIRMathOper, MIRMathPrefix, MIRMathRight, MIRRight, MIRStmt, MIRVarComputeAssign, MIRVarCopyAssign, MIRVarRef};
use crate::optimizer::{MIRFunOptPass, PassInstance};


impl PassInstance for MIRCommRightRemove {
    fn instance() -> Self {
        Self {
            available_map: Default::default(),
        }
    }
}


#[derive(Eq, PartialEq, Hash, Clone)]
struct AvailableComputeId {
    first: u64,
    oper: MIRMathOper,
    second: u64,
}

impl MIRVarComputeAssign {
    fn get_right_available_id(&self) -> AvailableComputeId {
        AvailableComputeId {
            first: self.first.global_id(),
            oper: self.oper,
            second: self.second.global_id(),
        }
    }

    fn get_left_available_id(&self) -> u64 {
        self.left.global_id()
    }
}

impl MIRVarCopyAssign {
    fn get_left_available_id(&self) -> u64 {
        self.left.global_id()
    }
}

impl AvailableComputeId {
    fn is_affect(&self, left_var_id: u64) -> bool {
        self.first == left_var_id || self.second == left_var_id
    }
}

pub struct MIRCommRightRemove {
    available_map: HashMap<AvailableComputeId, MIRVarRef>,
}

impl MIRCommRightRemove {
    fn get_available(&mut self, expr: &AvailableComputeId) -> MIRVarRef {
        self.available_map[expr].clone()
    }

    fn is_available(&self, expr: &AvailableComputeId) -> bool {
        self.available_map.contains_key(expr)
    }

    fn affect(&mut self, left: u64) {
        let mut affected_available = vec![];
        for (avail_id, _) in &self.available_map {
            if avail_id.is_affect(left) {
                affected_available.push(avail_id.clone());
            }
        }
        for affected in affected_available {
            self.available_map.remove(&affected);
        }
    }
    fn make_available(&mut self, available_left: MIRVarRef, available_right: AvailableComputeId) {
        self.available_map.insert(available_right, available_left);
    }
}

impl MIRFunOptPass for MIRCommRightRemove {
    fn pass(&mut self, mut input: MIRFun) -> MIRFun {
        self.available_map.clear();
        let stmts = input.take_blocks();
        for mut now_block in stmts {
            let stmts = now_block.take_stmts();
            for stmt_node_rc in stmts.node_iter() {
                let mut stmt_node_mut = stmt_node_rc.borrow_mut();
                let mir_stmt = &stmt_node_mut.value;
                match mir_stmt {
                    MIRStmt::MathVarCopyAssign(copy_assign) => {
                        self.affect(copy_assign.get_left_available_id());
                    }
                    MIRStmt::MathVarComputeAssign(compute_assign) => {
                        let left_id = compute_assign.get_left_available_id();
                        let avail_right = compute_assign.get_right_available_id();
                        let is_available = self.is_available(&avail_right);
                        if is_available {
                            let new_right = self.get_available(&avail_right);
                            let copy_assign = MIRVarCopyAssign::new(compute_assign.is_def, compute_assign.left.clone(), MIRMathRight::new(MIRMathPrefix::None, MIRRight::VarRef(new_right)));
                            (*stmt_node_mut).value = MIRStmt::MathVarCopyAssign(copy_assign);
                            self.affect(left_id);
                        } else {
                            self.affect(compute_assign.get_left_available_id());// 先用当前左边的变量更新available状态，然后加入新的available
                            self.make_available(compute_assign.left.clone(), avail_right);
                        }
                    }
                    _ => {
                        allow_nothing!();
                    }
                }
            }
            now_block.stmts = stmts;
            input.add_block(now_block);
        }
        input
    }
}