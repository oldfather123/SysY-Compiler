//! 常量块参数移除优化
//!
//! 识别并移除只接收单一常量值的块参数
//! 内存令牌被特殊处理了

use crate::prelude::*;
use crate::ssa::{
    cfg::ControlFlowGraph, ctx::CoreContext, domtree::DominatorTree, function::Function, ir::*,
};
use std::collections::{HashMap, HashSet};

/// 块参数可能的抽象值
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum AbstractValue {
    /// 没有值流向该参数
    None,
    /// 恰好一个值流向该参数
    One(Value),
    /// 多个不同值流向该参数
    Many,
}

impl AbstractValue {
    /// 格的 join 操作
    fn join(self, other: AbstractValue) -> AbstractValue {
        match (self, other) {
            // None 是底元素
            (AbstractValue::None, v) | (v, AbstractValue::None) => v,
            // Many 是顶元素
            (AbstractValue::Many, _) | (_, AbstractValue::Many) => AbstractValue::Many,
            // 两个具体值的 join
            (AbstractValue::One(v1), AbstractValue::One(v2)) => {
                if v1 == v2 {
                    AbstractValue::One(v1)
                } else {
                    AbstractValue::Many
                }
            }
        }
    }

    fn is_one(&self) -> bool {
        matches!(self, AbstractValue::One(_))
    }
}

/// 前驱边信息
#[derive(Debug)]
struct PredEdge {
    from_block: Block,
    inst: Inst,
    branch_index: usize,
    args: Vec<Value>,
}

/// 块摘要信息
#[derive(Debug, Default)]
struct BlockSummary {
    /// 块的形式参数及其类型
    formals: Vec<(Value, Type)>,
    /// 到达该块的前驱边
    predecessors: Vec<PredEdge>,
}

/// 求解器状态
struct SolverState {
    /// 每个块参数的抽象值
    absvals: HashMap<Value, AbstractValue>,
}

impl SolverState {
    fn new() -> Self {
        Self {
            absvals: HashMap::default(),
        }
    }

    fn get(&self, val: &Value) -> AbstractValue {
        self.absvals
            .get(val)
            .copied()
            .unwrap_or(AbstractValue::None)
    }

    fn set(&mut self, val: Value, absval: AbstractValue) {
        self.absvals.insert(val, absval);
    }
}

/// 常量块参数移除优化
pub fn remove_const_phi(
    func: &mut Function,
    _ctx: &mut CoreContext,
    cfg: &ControlFlowGraph,
    domtree: &DominatorTree,
) -> CEResult<()> {
    debug!("开始常量块参数移除优化: 函数 {}", func.name);

    // 如果没有块参数，直接返回
    let mut has_block_params = false;
    for (_, block) in func.dfg.blocks.0.iter() {
        let params_vec = block.params;
        if !func.dfg.value_vecs[params_vec].is_empty() {
            has_block_params = true;
            break;
        }
    }

    if !has_block_params {
        debug!("没有块参数，跳过优化");
        return Ok(());
    }

    let entry_block = func
        .layout
        .entry_block()
        .ok_or_else(|| CErr::ssa_err("No entry block"))?;

    // Phase 1: 构建块摘要
    let mut summaries: HashMap<Block, BlockSummary> = HashMap::default();

    for &block in domtree.cfg_rpo() {
        if block == entry_block {
            continue; // 跳过入口块
        }

        let mut summary = BlockSummary::default();

        // 收集形式参数
        let _block_data = &func.dfg.blocks[block];
        let params = func.dfg.block_params(block);
        debug!("块 {:?} 有 {} 个参数", block, params.len());
        for &param in params {
            let ty = func.dfg.value_type(param);
            summary.formals.push((param, ty));
        }

        // 收集前驱边
        for pred in cfg.pred_iter(block) {
            let pred_block = pred.block;
            let pred_inst = pred.inst;
            // 获取跳转指令数据
            if let Some(inst_data) = func.dfg.insts.get(pred_inst) {
                // 根据指令类型提取跳转目标和参数
                let args_opt = match inst_data {
                    InstructionData::Jump { target } => {
                        if target.block == block {
                            let args: Vec<Value> = func.dfg.value_vecs[target.args]
                                .iter()
                                .map(|&arg| func.dfg.resolve_aliases(arg))
                                .collect();
                            Some((0, args))
                        } else {
                            None
                        }
                    }
                    InstructionData::Brif { targets, .. } => {
                        let mut result = None;
                        for (idx, target) in targets.iter().enumerate() {
                            if target.block == block {
                                let args: Vec<Value> = func.dfg.value_vecs[target.args]
                                    .iter()
                                    .map(|&arg| func.dfg.resolve_aliases(arg))
                                    .collect();
                                result = Some((idx, args));
                                break;
                            }
                        }
                        result
                    }
                    InstructionData::BranchTable { table, .. } => {
                        let mut result = None;
                        if let Some(jt) = func.dfg.jump_tables.get(*table) {
                            for (idx, target) in jt.table.iter().enumerate() {
                                if target.block == block {
                                    let args: Vec<Value> = func.dfg.value_vecs[target.args]
                                        .iter()
                                        .map(|&arg| func.dfg.resolve_aliases(arg))
                                        .collect();
                                    result = Some((idx, args));
                                    break;
                                }
                            }
                        }
                        result
                    }
                    _ => None,
                };

                if let Some((branch_idx, args)) = args_opt {
                    summary.predecessors.push(PredEdge {
                        from_block: pred_block,
                        inst: pred_inst,
                        branch_index: branch_idx,
                        args,
                    });
                }
            }
        }

        if !summary.formals.is_empty() {
            summaries.insert(block, summary);
        }
    }

    // Phase 2: 数据流分析
    let mut state = SolverState::new();

    // 初始化所有块参数为 None
    for summary in summaries.values() {
        for &(param, _) in &summary.formals {
            state.set(param, AbstractValue::None);
        }
    }

    // 迭代求解直到不动点
    let mut changed = true;
    let mut iterations = 0;

    while changed && iterations < 100 {
        changed = false;
        iterations += 1;

        for &block in domtree.cfg_rpo() {
            if let Some(summary) = summaries.get(&block) {
                // 对每个形式参数计算新的抽象值
                for (param_idx, &(formal, formal_ty)) in summary.formals.iter().enumerate() {
                    // 特殊处理：内存令牌总是 Many
                    if formal_ty == Type::MemToken {
                        let old_val = state.get(&formal);
                        if old_val != AbstractValue::Many {
                            state.set(formal, AbstractValue::Many);
                            changed = true;
                        }
                        continue;
                    }

                    // 计算所有前驱传入的值
                    let mut new_absval = AbstractValue::None;

                    for pred in &summary.predecessors {
                        if param_idx < pred.args.len() {
                            let actual = pred.args[param_idx];

                            // 计算实际参数的抽象值
                            let actual_absval = match func.dfg.valuedata(actual) {
                                ValueData::BlockParam { .. } => state.get(&actual),
                                ValueData::Alias { .. } => {
                                    // 解析别名链
                                    let resolved = func.dfg.resolve_aliases(actual);
                                    match func.dfg.valuedata(resolved) {
                                        ValueData::BlockParam { .. } => state.get(&resolved),
                                        _ => AbstractValue::One(resolved),
                                    }
                                }
                                _ => AbstractValue::One(actual),
                            };

                            new_absval = new_absval.join(actual_absval);
                        }
                    }

                    // 更新抽象值
                    let old_absval = state.get(&formal);
                    if new_absval != old_absval {
                        state.set(formal, new_absval);
                        changed = true;
                    }
                }
            }
        }
    }

    // 统计可优化的参数
    let mut const_params = 0;
    for absval in state.absvals.values() {
        if absval.is_one() {
            const_params += 1;
        }
    }

    if const_params == 0 {
        return Ok(());
    }

    debug!(
        "常量块参数移除: {} 次迭代, {} 个参数可优化",
        iterations, const_params
    );

    // Phase 3: 应用变换

    // 收集需要编辑的块
    let mut blocks_to_edit: HashSet<Block> = HashSet::default();
    for (&block, summary) in &summaries {
        for &(formal, _) in &summary.formals {
            if state.get(&formal).is_one() {
                blocks_to_edit.insert(block);
                break;
            }
        }
    }

    // 处理每个需要编辑的块
    for &block in &blocks_to_edit {
        let summary = &summaries[&block];
        let mut params_to_remove = Vec::new();

        // 收集要移除的参数
        for (idx, &(formal, _)) in summary.formals.iter().enumerate() {
            if let AbstractValue::One(replacement) = state.get(&formal) {
                params_to_remove.push((idx, formal, replacement));
            }
        }

        // 反向移除参数（避免索引变化）
        for (idx, formal, replacement) in params_to_remove.into_iter().rev() {
            debug!(
                "移除常量块参数 {:?} -> {:?} 在块 {:?}",
                formal, replacement, block
            );

            // 创建别名
            func.dfg.change_to_alias(formal, replacement);

            // 从块参数列表中移除 - 需要获取正确的 ValueVec
            let params_vec = func.dfg.blocks[block].params;
            func.dfg.value_vecs[params_vec].remove(idx);
        }
    }

    // 更新跳转指令的参数
    for (&target_block, summary) in &summaries {
        if !blocks_to_edit.contains(&target_block) {
            continue;
        }

        // 对每个前驱更新跳转参数
        for pred in &summary.predecessors {
            // 构建新的参数列表，跳过已优化的参数
            let mut new_args = Vec::new();

            for (idx, &arg) in pred.args.iter().enumerate() {
                if idx < summary.formals.len() {
                    let (formal, _) = summary.formals[idx];
                    if !state.get(&formal).is_one() {
                        new_args.push(arg);
                    }
                }
            }

            // 更新指令的参数列表
            if let Some(inst_data) = func.dfg.insts.get_mut(pred.inst) {
                match inst_data {
                    InstructionData::Jump { target } => {
                        let block = target.block;
                        *target =
                            BlockCall::new(block, new_args.into_iter(), &mut func.dfg.value_vecs);
                    }
                    InstructionData::Brif { targets, .. } => {
                        if pred.branch_index < targets.len() {
                            let block = targets[pred.branch_index].block;
                            targets[pred.branch_index] = BlockCall::new(
                                block,
                                new_args.into_iter(),
                                &mut func.dfg.value_vecs,
                            );
                        }
                    }
                    InstructionData::BranchTable { table, .. } => {
                        // 处理跳转表
                        if let Some(jt) = func.dfg.jump_tables.get_mut(*table) {
                            if pred.branch_index < jt.table.len() {
                                let block = jt.table[pred.branch_index].block;
                                jt.table[pred.branch_index] = BlockCall::new(
                                    block,
                                    new_args.clone().into_iter(),
                                    &mut func.dfg.value_vecs,
                                );
                            }
                        }
                    }
                    _ => {}
                }
            }
        }
    }

    Ok(())
}
