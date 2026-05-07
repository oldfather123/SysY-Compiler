//! 不可达代码消除
//!
//! 删除从入口块无法到达的整个基本块
//! 但注意不会删除结果未使用的单个指令!

use crate::prelude::*;
use crate::ssa::{
    cfg::ControlFlowGraph,
    ctx::CoreContext,
    cursor::{Cursor, FuncCursor},
    domtree::DominatorTree,
    function::Function,
};
use std::collections::HashSet;

/// 不可达代码消除
pub fn remove_unreach(
    func: &mut Function,
    ctx: &mut CoreContext,
    cfg: &mut ControlFlowGraph,
    domtree: &DominatorTree,
) -> CEResult<()> {
    // 创建游标
    let mut pos = FuncCursor::new(func, ctx);
    let mut used_jump_tables = HashSet::new();

    // 第一遍：收集可达块使用的跳转表
    while let Some(block) = pos.next_block() {
        if domtree.is_reachable(block) {
            // 检查块的最后一条指令
            if let Some(inst) = pos.layout().last_inst(block) {
                if let Some(inst_data) = pos.func.dfg.insts.get(inst) {
                    if let crate::ssa::ir::InstructionData::BranchTable { table, .. } = inst_data {
                        used_jump_tables.insert(*table);
                    }
                }
            }
        }
    }

    // 重置游标位置
    pos.set_position(crate::ssa::cursor::CursorPosition::Nowhere);

    // 第二遍：删除不可达块
    while let Some(block) = pos.next_block() {
        if !domtree.is_reachable(block) {
            trace!("消除不可达块 {:?}", block);

            // 移动游标到前一个块，以确保下一次迭代正确
            pos.prev_block();

            // 清空块中的所有指令
            let insts_to_remove: Vec<_> = pos.func.layout.block_insts(block).collect();
            for inst in insts_to_remove {
                trace!("  - 删除指令 {:?}", inst);
                pos.func.dfg.insts.remove(inst);
            }

            // 更新 CFG
            cfg.recompute_block(pos.func, block);

            // 从布局中删除块
            pos.func.layout.remove_block(block);

            // 从DFG中删除块
            pos.func.dfg.blocks.remove(block);
        }
    }

    // 清理未使用的跳转表
    let all_jump_tables: Vec<_> = func.dfg.jump_tables.iter().map(|(k, _)| k).collect();
    for jump_table in all_jump_tables {
        if !used_jump_tables.contains(&jump_table) {
            trace!("清理未使用的跳转表 {:?}", jump_table);
            func.dfg.jump_tables.remove(jump_table);
        }
    }

    Ok(())
}

// #[cfg(test)]
// mod tests {
//     use super::*;
//     use crate::ssa::ir::*;

//     #[test]
//     fn test_eliminate_unreachable_basic() {
//         let mut func = Function::new();
//         let mut ctx = CoreContext::new();

//         // 创建入口块
//         let entry = func.dfg.make_block();
//         func.entry = entry;

//         // 创建可达块
//         let reachable = func.dfg.make_block();

//         // 创建不可达块
//         let unreachable = func.dfg.make_block();

//         // 在入口块添加跳转到可达块
//         {
//             let block_node = func.block_nodes.get_or_insert_default(entry);
//             let jump_data = InstructionData::Jump {
//                 target: BlockCall {
//                     block: reachable,
//                     args: func.dfg.value_vecs.insert(vec![]),
//                 },
//             };
//             let inst = func.dfg.make_inst(jump_data);
//             func.inst_nodes.get_or_insert_default(inst).block = entry;
//             block_node.insts.push(inst);
//             block_node.is_terminated = true;
//         }

//         // 在不可达块添加一些指令
//         {
//             let block_node = func.block_nodes.get_or_insert_default(unreachable);
//             // 创建常量值
//             let val1 = func.dfg.make_value(ValueData::int32_val(Type::Int32, 1));
//             let val2 = func.dfg.make_value(ValueData::int32_val(Type::Int32, 2));
//             let const_data = InstructionData::Binary {
//                 op: BinOp::IAdd,
//                 lhs: val1,
//                 rhs: val2,
//             };
//             let inst = func.dfg.make_inst(const_data);
//             func.inst_nodes.get_or_insert_default(inst).block = unreachable;
//             block_node.insts.push(inst);
//         }

//         // 计算 CFG 和支配树
//         let mut cfg = ControlFlowGraph::new();
//         cfg.compute(&func);

//         let mut domtree = DominatorTree::new();
//         domtree.compute(&func, &cfg);

//         // 记录原始块数量
//         let blocks_before = func.dfg.blocks.len();

//         // 执行不可达代码消除
//         remove_unreach(&mut func, &mut ctx, &mut cfg, &domtree).unwrap();

//         // 验证结果
//         let blocks_after = func.dfg.blocks.len();
//         assert_eq!(blocks_before - 1, blocks_after, "应该删除一个不可达块");

//         // 验证不可达块已被删除
//         assert!(
//             func.dfg.blocks.get(unreachable).is_none(),
//             "不可达块应该被删除"
//         );

//         // 验证可达块仍然存在
//         assert!(func.dfg.blocks.get(entry).is_some(), "入口块应该保留");
//         assert!(func.dfg.blocks.get(reachable).is_some(), "可达块应该保留");
//     }

//     #[test]
//     fn test_eliminate_unreachable_with_jump_table() {
//         let mut func = Function::new();
//         let mut ctx = CoreContext::new();

//         // 创建入口块
//         let entry = func.dfg.make_block();
//         func.entry = entry;

//         // 创建可达块（跳转表目标）
//         let target1 = func.dfg.make_block();
//         let target2 = func.dfg.make_block();

//         // 创建不可达块（不在跳转表中）
//         let unreachable = func.dfg.make_block();

//         // 创建跳转表
//         let jump_table_data = JumpTableData {
//             table: vec![
//                 BlockCall {
//                     block: target1,
//                     args: func.dfg.value_vecs.insert(vec![]),
//                 },
//                 BlockCall {
//                     block: target2,
//                     args: func.dfg.value_vecs.insert(vec![]),
//                 },
//             ],
//         };
//         let jump_table = func.dfg.jump_tables.insert(jump_table_data);

//         // 在入口块添加跳转表指令
//         {
//             let block_node = func.block_nodes.get_or_insert_default(entry);
//             let switch_val = func.dfg.make_value(ValueData::int32_val(Type::Int32, 0));
//             let branch_table_data = InstructionData::BranchTable {
//                 cond: switch_val,
//                 table: jump_table,
//             };
//             let inst = func.dfg.make_inst(branch_table_data);
//             func.inst_nodes.get_or_insert_default(inst).block = entry;
//             block_node.insts.push(inst);
//             block_node.is_terminated = true;
//         }

//         // 计算 CFG 和支配树
//         let mut cfg = ControlFlowGraph::new();
//         cfg.compute(&func);

//         let mut domtree = DominatorTree::new();
//         domtree.compute(&func, &cfg);

//         // 执行不可达代码消除
//         remove_unreach(&mut func, &mut ctx, &mut cfg, &domtree).unwrap();

//         // 验证跳转表仍然存在（因为被使用）
//         assert!(
//             func.dfg.jump_tables.get(jump_table).is_some(),
//             "使用的跳转表应该保留"
//         );
//     }
// }
