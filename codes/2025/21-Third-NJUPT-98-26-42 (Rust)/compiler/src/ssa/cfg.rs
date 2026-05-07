//! Control Flow Graph
//!
//! 备注:
//! - 前驱用 (Block, Inst) : 因为需要知道是哪条具体的分支/跳转指令
//! - 后继用 (Block) : 只需要知道目标块即可

use crate::prelude::*;
use crate::ssa::function::Function;

/// 基本块的前驱信息
/// 包含源块和具体的分支/跳转指令
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct BlockPredecessor {
    /// 前驱块
    pub block: Block,
    /// 该块中导致跳转的指令
    pub inst: Inst,
}

impl BlockPredecessor {
    /// 创建新的前驱信息
    pub fn new(block: Block, inst: Inst) -> Self {
        Self { block, inst }
    }
}

/// CFG节点信息，存储一个块的所有前驱和后继
#[derive(Debug, Clone, Default)]
struct CFGNode {
    /// 前驱列表 - 所有能跳转到此块的指令
    pub predecessors: Vec<BlockPredecessor>,

    /// 后继列表 - 此块能跳转到的所有目标块（保持顺序）
    pub successors: Vec<Block>,
}

/// 控制流图
#[derive(Debug)]
pub struct ControlFlowGraph {
    /// 每个块的CFG节点信息
    data: RefSecMap<Block, CFGNode>,

    /// CFG是否有效（是否已计算）
    valid: bool,
}

impl ControlFlowGraph {
    pub fn new() -> Self {
        Self {
            data: RefSecMap::new(),
            valid: false,
        }
    }

    pub fn clear(&mut self) {
        self.data.clear();
        self.valid = false;
    }

    /// 为函数计算CFG
    pub fn compute(&mut self, func: &Function) {
        self.clear();

        // 遍历所有块
        for (block, _) in func.dfg.blocks.iter_all() {
            self.compute_block(func, block);
        }

        self.valid = true;
    }

    /// 计算单个块的后继，并更新相应的前驱信息
    fn compute_block(&mut self, func: &Function, block: Block) {
        // 获取块的最后一条终结指令
        let term_inst = match func.layout.last_inst(block) {
            Some(inst) => inst,
            None => return, // 空块，没有指令
        };

        // 获取终结指令的数据
        let inst_data = func.dfg.insts.get(term_inst);
        if inst_data.is_none() {
            return;
        }
        let inst_data = inst_data.unwrap();

        // 提取所有目标块
        let targets = match inst_data {
            crate::ssa::ir::InstructionData::Jump { target } => {
                vec![target.block]
            }
            crate::ssa::ir::InstructionData::Brif { targets, .. } => {
                vec![targets[0].block, targets[1].block]
            }
            crate::ssa::ir::InstructionData::BranchTable { table, .. } => {
                let mut target_blocks = Vec::new();
                // 从跳转表中获取所有目标
                if let Some(table_data) = func.dfg.jump_tables.get(*table) {
                    for &entry in &table_data.table {
                        target_blocks.push(entry.block);
                    }
                }
                target_blocks
            }
            _ => return, // 不是控制流指令
        };

        // 为每个目标添加边
        for target in targets {
            self.add_edge(block, term_inst, target);
        }
    }

    /// 添加一条从 from 块的 from_inst 指令到 to 块的边
    fn add_edge(&mut self, from: Block, from_inst: Inst, to: Block) {
        // 更新 from 的后继（避免重复）
        let from_node = self.data.get_or_insert_default(from);
        if !from_node.successors.contains(&to) {
            from_node.successors.push(to);
        }

        // 更新 to 的前驱
        let pred = BlockPredecessor::new(from, from_inst);
        let to_node = self.data.get_or_insert_default(to);

        // 避免重复添加相同的前驱
        if !to_node.predecessors.contains(&pred) {
            to_node.predecessors.push(pred);
        }
    }

    /// 重新计算特定块的CFG信息
    /// 用于块内指令修改后的增量更新
    pub fn recompute_block(&mut self, func: &Function, block: Block) {
        debug_assert!(self.is_valid());

        // 先清除该块的所有后继关系
        self.invalidate_block_successors(block);

        // 重新计算
        self.compute_block(func, block);
    }

    /// 清除块的所有后继关系
    fn invalidate_block_successors(&mut self, block: Block) {
        // 获取该块的所有后继
        let successors: Vec<Block> = if let Some(node) = self.data.get(block) {
            node.successors.to_vec()
        } else {
            return;
        };

        // 从每个后继的前驱列表中移除该块
        for succ in successors {
            if let Some(succ_node) = self.data.get_mut(succ) {
                succ_node.predecessors.retain(|pred| pred.block != block);
            }
        }

        // 清空该块的后继列表
        if let Some(node) = self.data.get_mut(block) {
            node.successors.clear();
        }
    }

    /// 获取块的前驱迭代器
    pub fn pred_iter(&self, block: Block) -> impl Iterator<Item = &BlockPredecessor> {
        self.data
            .get(block)
            .map(|node| node.predecessors.iter())
            .unwrap_or_else(|| [].iter())
    }

    /// 获取块的后继迭代器  
    pub fn succ_iter(&self, block: Block) -> Box<dyn Iterator<Item = Block> + '_> {
        debug_assert!(self.is_valid());
        if let Some(node) = self.data.get(block) {
            Box::new(node.successors.iter().copied())
        } else {
            Box::new(std::iter::empty())
        }
    }

    /// 检查CFG是否有效（已计算）
    pub fn is_valid(&self) -> bool {
        self.valid
    }

    /// 获取块的前驱数量
    pub fn pred_count(&self, block: Block) -> usize {
        self.data
            .get(block)
            .map(|node| node.predecessors.len())
            .unwrap_or(0)
    }

    /// 获取块的后继数量
    pub fn succ_count(&self, block: Block) -> usize {
        self.data
            .get(block)
            .map(|node| node.successors.len())
            .unwrap_or(0)
    }

    /// 判断是否存在从 from 到 to 的边
    pub fn has_edge(&self, from: Block, to: Block) -> bool {
        self.data
            .get(from)
            .map(|node| node.successors.contains(&to))
            .unwrap_or(false)
    }
}

impl Default for ControlFlowGraph {
    fn default() -> Self {
        Self::new()
    }
}
