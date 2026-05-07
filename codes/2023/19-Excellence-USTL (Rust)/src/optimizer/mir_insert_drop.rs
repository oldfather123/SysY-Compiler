use crate::allow_nothing;
use crate::mir::{MIRBlock, MIRFun, MIRMathLRef, MIRName, MIRRight, MIRStmt, MIRVarActiveType, MIRVarRef};
use crate::optimizer::{MIRFunOptPass, PassInstance};
use crate::util::{ElementRef, HashInsertVec};
use std::cell::RefCell;
use std::collections::{HashMap, VecDeque};

pub struct MIRInsertDrop {
    long_live_drop_map: HashMap<MIRName, ElementRef<MIRStmt>>,
    block_stmts: VecDeque<HashInsertVec<MIRStmt>>,
    blocks: VecDeque<MIRBlock>,
}

impl MIRInsertDrop {
    fn drop_var(&mut self, current_list: &mut HashInsertVec<MIRStmt>, ele_ref: &ElementRef<MIRStmt>, var: &MIRVarRef) {
        if self.long_live_drop_map.contains_key(var.get_name()) {
            let drop_ref = self.long_live_drop_map[var.get_name()].clone();
            if current_list.contains(&drop_ref) {
                let drop_stmt = current_list.remove(&drop_ref);
                let new_ref = current_list.insert_after(drop_stmt, ele_ref);
                self.long_live_drop_map.insert(var.copy_name(), new_ref);
            } else {
                for block_stmt in &mut self.block_stmts {
                    if block_stmt.contains(ele_ref) {
                        block_stmt.remove(ele_ref);
                        break;
                    }
                }
            }
        } else {
            // 临时变量，立即消除即可
            let drop_ref = current_list.insert_after(MIRStmt::Drop(var.copy_name()), ele_ref);
            self.long_live_drop_map.insert(var.copy_name(), drop_ref);
        }
    }

    fn check_right_mathl_drop_status(&mut self, math_lref: &MIRMathLRef, need_update_drops: &mut Vec<MIRVarRef>) {
        match &math_lref {
            MIRMathLRef::Var(var) => {
                if !need_update_drops.contains(var) {
                    need_update_drops.push(var.clone());
                }
            }
            MIRMathLRef::ArrayEle(ele) => {
                let var = ele.offset;
                if !need_update_drops.contains(&var) {
                    need_update_drops.push(var.clone());
                }
            }
        }
    }

    fn check_left_var_drop_status(
        &mut self,
        is_temp: bool,
        is_def: bool,
        math_lref: &MIRVarRef,
        need_update_drops: &mut Vec<MIRVarRef>,
        need_new_drops: &mut Vec<MIRVarRef>,
    ) {
        if !is_temp && is_def {
            // 临时变量不加入消除查询表
            let var = math_lref.clone();
            need_new_drops.push(var);
        } else if !is_def {
            if !need_update_drops.contains(&math_lref) {
                need_update_drops.push(math_lref.clone());
            }
        }
    }
}

impl MIRFunOptPass for MIRInsertDrop {
    fn pass(&mut self, mut input: MIRFun) -> MIRFun {
        let blocks = input.take_blocks();
        self.long_live_drop_map = HashMap::<MIRName, ElementRef<MIRStmt>>::new();
        let mut need_update_drops = vec![];
        let mut need_new_drops = vec![];
        for block in blocks {
            let now_new_block = MIRBlock::new(block.get_name().clone());
            let mut current_list = block.to_link_list();
            let node_iter = current_list.node_iter();
            for node in node_iter {
                need_update_drops.clear();
                need_new_drops.clear();
                let stmt = RefCell::borrow(&node);
                match &stmt.value {
                    MIRStmt::MathVarComputeAssign(assign) => {
                        self.check_right_mathl_drop_status(&assign.first, &mut need_update_drops);
                        self.check_right_mathl_drop_status(&assign.second, &mut need_update_drops);
                        self.check_left_var_drop_status(assign.is_temp(), assign.is_def, &assign.left, &mut need_update_drops, &mut need_new_drops);
                    }
                    MIRStmt::MathVarCopyAssign(assign) => {
                        // match &assign.right.math {
                        //     MIRRight::Lit(_) => {
                        //         allow_nothing!();
                        //     }
                        //     MIRRight::VarRef(var) => {
                        //         if !need_update_drops.contains(var) {
                        //             need_update_drops.push(var.clone());
                        //         }
                        //     }
                        //     MIRRight::ArrEleRef(ele) => {
                        //         let var = &ele.offset;
                        //         if !need_update_drops.contains(var) {
                        //             need_update_drops.push(var.clone());
                        //         }
                        //     }
                        //     MIRRight::CmpRef(_) => {
                        //         todo!("NOT IMPLEMENTED")
                        //     }
                        //     MIRRight::Invoke(_invoke) => {
                        //         todo!("NOT IMPLEMENTED")
                        //     }
                        // }
                        self.check_left_var_drop_status(assign.is_temp(), assign.is_def, &assign.left, &mut need_update_drops, &mut need_new_drops);
                    }
                    MIRStmt::MathCvt(_cvt) => {
                        todo!("not implemented")
                    }
                    MIRStmt::BoolCmpAssign(_assign) => {
                        todo!("not implemented")
                    }
                    // MIRStmt::BoolCopyAssign(_copy) => {
                    //     todo!("not implemented")
                    // }
                    MIRStmt::Return(ret) => match &ret.value {
                        None => {}
                        Some(math) => {
                            self.check_right_mathl_drop_status(math, &mut need_update_drops);
                        }
                    },
                    _ => {
                        // 无关语句或者已经加入的drop语句，无需关注
                        continue;
                    }
                }
                let ele_ref = stmt.get_node_ref();
                drop(stmt);
                drop(node);
                for need_update_drop_var in &need_update_drops {
                    if need_update_drop_var.active_type != MIRVarActiveType::Global {
                        // 全局变量的活跃问题特殊处理
                        self.drop_var(&mut current_list, &ele_ref, &need_update_drop_var);
                    }
                }
                for need_new_drop in &need_new_drops {
                    if need_new_drop.active_type == MIRVarActiveType::Global {
                        // 全局变量的活跃问题特殊处理
                        continue;
                    }
                    let drop_stmt = MIRStmt::Drop(need_new_drop.copy_name());
                    let new_drop = current_list.insert_after(drop_stmt, &ele_ref);
                    self.long_live_drop_map.insert(need_new_drop.copy_name(), new_drop);
                }
                // 更新drops
            }
            self.blocks.push_back(now_new_block);
            self.block_stmts.push_back(current_list);
        }
        loop {
            let block = self.blocks.pop_front();
            match block {
                None => break,
                Some(mut block) => {
                    let stmt = self.block_stmts.pop_front();
                    match stmt {
                        None => {
                            panic!("NEVER HERE")
                        }
                        Some(stmt) => {
                            block.stmts = stmt;
                            input.add_block(block);
                        }
                    }
                }
            }
        }

        input
    }
}

impl PassInstance for MIRInsertDrop {
    fn instance() -> Self {
        MIRInsertDrop {
            long_live_drop_map: Default::default(),
            block_stmts: VecDeque::new(),
            blocks: VecDeque::new(),
        }
    }
}
