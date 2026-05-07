use crate::mir::{MIRBlock, MIRMathPrefix, MIRRight, MIRStmt, MIRVarActiveType, MIRVarRef};
use crate::optimizer::{MIRBlockOptPass, PassInstance};
use crate::util::ElementRef;
use std::collections::HashMap;

pub struct MIRRemoveLazyRight {
    temp_init: HashMap<MIRVarRef, ElementRef<MIRStmt>>,
}

impl PassInstance for MIRRemoveLazyRight {
    fn instance() -> Self {
        MIRRemoveLazyRight {
            temp_init: HashMap::new(),
        }
    }
}

// 临时变量只允许使用一次
impl MIRBlockOptPass for MIRRemoveLazyRight {
    fn pass(&mut self, input: MIRBlock) -> MIRBlock {
        let mut block = MIRBlock::new(input.get_name().clone());
        let mut link_list = input.to_link_list();

        for now_node in link_list.node_iter() {
            let now_node_inner_ref = now_node.borrow();
            if let MIRStmt::MathVarComputeAssign(comp) = &now_node_inner_ref.value {
                if comp.left.active_type == MIRVarActiveType::Temp && comp.is_def() {
                    let node_ref = now_node_inner_ref.get_node_ref();
                    self.temp_init.insert(comp.left.clone(), node_ref);
                }
            } else if let MIRStmt::MathVarCopyAssign(copy_assign) = &now_node_inner_ref.value {
                if copy_assign.is_just_define() {
                    continue;
                }
                let right = copy_assign.unwrap_math_right();
                if right.get_prefix() != &MIRMathPrefix::None {
                    continue;
                }
                let right = right.get_right();
                // 可以替换
                if let MIRRight::VarRef(var) = right {
                    let stmt = self.temp_init.remove(var);
                    match stmt {
                        None => {
                            // 没查到可替换信息 , 但是可以看一下当前是不是临时语句，如果是的话，加入可用池
                            if copy_assign.is_temp() && copy_assign.is_def {
                                // 这里应当加入左边而不是右边
                                let left = copy_assign.left.clone();
                                let node_ref = now_node_inner_ref.get_node_ref();
                                self.temp_init.insert(left.clone(), node_ref);
                            }
                        }
                        Some(stmt_ref) => {
                            let new_left = copy_assign.left.clone();
                            let new_is_def = copy_assign.is_def;
                            drop(now_node_inner_ref);
                            let stmt = link_list.remove(&stmt_ref);
                            let new_stmt = match stmt {
                                MIRStmt::MathVarComputeAssign(mut assign) => {
                                    assign.left = new_left;
                                    assign.is_def = new_is_def;
                                    MIRStmt::MathVarComputeAssign(assign)
                                }
                                MIRStmt::MathVarCopyAssign(mut assign) => {
                                    assign.left = new_left;
                                    assign.is_def = new_is_def;
                                    MIRStmt::MathVarCopyAssign(assign)
                                }
                                _ => {
                                    panic!("NEVER HERE");
                                }
                            };
                            let mut now_node_inner_ref_mut = now_node.borrow_mut();
                            (*now_node_inner_ref_mut).value = new_stmt;
                        }
                    }
                }
            }
        }

        self.temp_init.clear();
        block.stmts = link_list;
        block
    }
}
