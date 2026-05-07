//! VirtControlFlowGraph
//!
//! ## Critical Edge Splitting
//!
//! 在phi破坏时，phi节点会被替换为大量的mov指令
//! 这些mov指令通常需要放置在前驱块的末尾。但当前驱块有多个后继时，
//! 直接在前驱块末尾放置mov指令会影响其他后继块
//!
//! 解决方案：在CFG的关键边上插入虚拟块，专门用于放置这些mov指令
//!
//! ```plain
//!            0
//!          /   \
//!         1     2
//!        /  \     \
//!       3    4    |
//!       |\  _|____|
//!       | \/ |
//!       | /\ |
//!       5    6
//! (3 -> 5, and 3 -> 6 are critical edges and must be split)
//! ```
//!
//! 按照如上例子:
//! - bb5 BlockParams: 应该往 (3->5) 和 bb2 上放置mv指令
//! - bb6 BlockParams: 应该往 (3->6) 和 bb4 上放置mv指令
//!
//! 关键边定义：一条从有多个后继的块到有多个前驱的块的边
//! 这种边需要被分割，在边上插入一个虚拟块，用于放置phi破坏产生的mv指令

use super::prog::*;
use crate::{
    prelude::*,
    ssa::{ctx::*, domtree::*, function::*, ir::*},
};
use std::collections::{HashMap, HashSet};

// ===================================================================
// ASM CFG节点
// ===================================================================

/// CFG节点
#[derive(Debug, Clone, Default)]
pub struct VirtCfgNode {
    /// 前驱块列表
    pub predecessors: Vec<VirtBlockId>,

    /// 后继块列表
    pub successors: Vec<VirtBlockId>,

    /// 终结指令
    pub terminator: Option<Inst>,

    /// 是否为间接跳转目标 -- 通过跳转表到达
    pub is_indirect_target: bool,
}

impl VirtCfgNode {
    pub fn new() -> Self {
        Self::default()
    }
    /// 添加前驱
    pub fn add_pred(&mut self, pred: VirtBlockId) {
        if !self.predecessors.contains(&pred) {
            self.predecessors.push(pred);
        }
    }
    /// 添加后继
    pub fn add_succ(&mut self, succ: VirtBlockId) {
        if !self.successors.contains(&succ) {
            self.successors.push(succ);
        }
    }
    /// 移除前驱
    pub fn remove_pred(&mut self, pred: &VirtBlockId) {
        self.predecessors.retain(|p| p != pred);
    }
    /// 移除后继
    pub fn remove_succ(&mut self, succ: &VirtBlockId) {
        self.successors.retain(|s| s != succ);
    }
    /// 获取前驱数量
    pub fn pred_count(&self) -> usize {
        self.predecessors.len()
    }
    /// 获取后继数量
    pub fn succ_count(&self) -> usize {
        self.successors.len()
    }
}

// ===================================================================
// ASM控制流图
// ===================================================================

/// ASM层的控制流图
#[derive(Debug)]
pub struct VirtControlFlowGraph {
    /// 所有块的CFG节点信息
    nodes: HashMap<VirtBlockId, VirtCfgNode>,

    /// RPO访问顺序
    block_order: Vec<VirtBlockId>,

    /// 入口块
    entry: VirtBlockId,

    /// 关键边列表
    critical_edges: Vec<(Block, Block, u32)>, // (pred, succ, succ_idx)

    /// 原始块到ASM块的映射
    basic_to_asm: HashMap<Block, VirtBlockId>,
}

impl Default for VirtControlFlowGraph {
    fn default() -> Self {
        Self::new()
    }
}

// ===================================================================
// 公共接口
// ===================================================================

impl VirtControlFlowGraph {
    pub fn new() -> Self {
        Self {
            nodes: HashMap::new(),
            block_order: Vec::new(),
            entry: VirtBlockId::BasicBlock {
                block: Block::default(),
            },
            critical_edges: Vec::new(),
            basic_to_asm: HashMap::new(),
        }
    }

    /// 从 FuncContext 构建 ASM CFG
    pub fn build(func_ctx: &FuncContext) -> Self {
        let func = &func_ctx.func;
        let ssa_cfg = &func_ctx.cfg;
        let domtree = &func_ctx.domtree;

        let mut cfg = Self {
            nodes: HashMap::new(),
            block_order: Vec::new(),
            entry: VirtBlockId::BasicBlock { block: func.entry },
            critical_edges: Vec::new(),
            basic_to_asm: HashMap::new(),
        };

        // 1. 识别所有关键边
        cfg.identify_critical_edges(func, ssa_cfg);

        // 2. 创建所有块节点（包括基本块和关键边块）
        cfg.create_nodes(func, ssa_cfg);

        // 3. 建立控制流关系
        cfg.build_edges(func, ssa_cfg);

        // 4. RPO块访问顺序
        cfg.generate_block_order(domtree);

        // 5. 标记间接跳转目标
        cfg.mark_indirect_targets(func);

        cfg
    }

    /// 克隆块的顺序列表
    pub fn clone_block_order(&self) -> Vec<VirtBlockId> {
        self.block_order.clone()
    }

    /// 获取入口块
    pub fn entry(&self) -> VirtBlockId {
        self.entry
    }

    /// 获取块的CFG节点
    pub fn node(&self, block: VirtBlockId) -> Option<&VirtCfgNode> {
        self.nodes.get(&block)
    }

    /// 获取可变的CFG节点
    pub fn node_mut(&mut self, block: VirtBlockId) -> Option<&mut VirtCfgNode> {
        self.nodes.get_mut(&block)
    }

    /// 获取块的前驱迭代器
    pub fn predecessors(&self, block: VirtBlockId) -> impl Iterator<Item = VirtBlockId> + '_ {
        self.nodes
            .get(&block)
            .map(|node| node.predecessors.iter().copied())
            .into_iter()
            .flatten()
    }

    /// 获取块的后继迭代器
    pub fn successors(&self, block: VirtBlockId) -> impl Iterator<Item = VirtBlockId> + '_ {
        self.nodes
            .get(&block)
            .map(|node| node.successors.iter().copied())
            .into_iter()
            .flatten()
    }

    /// 获取块的前驱数量
    pub fn pred_count(&self, block: VirtBlockId) -> usize {
        self.nodes
            .get(&block)
            .map(|node| node.pred_count())
            .unwrap_or(0)
    }

    /// 获取块的后继数量
    pub fn succ_count(&self, block: VirtBlockId) -> usize {
        self.nodes
            .get(&block)
            .map(|node| node.succ_count())
            .unwrap_or(0)
    }

    /// 获取块的终结指令
    pub fn terminator(&self, block: VirtBlockId) -> Option<Inst> {
        self.nodes.get(&block).and_then(|node| node.terminator)
    }

    /// 判断块是否为间接跳转目标
    pub fn is_indirect_target(&self, block: VirtBlockId) -> bool {
        self.nodes
            .get(&block)
            .map(|node| node.is_indirect_target)
            .unwrap_or(false)
    }

    /// 获取访问顺序中的所有块
    pub fn block_order(&self) -> &[VirtBlockId] {
        &self.block_order
    }

    /// 按访问顺序迭代所有块
    pub fn iter_blocks(&self) -> impl Iterator<Item = VirtBlockId> + '_ {
        self.block_order.iter().copied()
    }

    /// 获取所有关键边
    pub fn critical_edges(&self) -> &[(Block, Block, u32)] {
        &self.critical_edges
    }

    /// 判断是否有从 from 到 to 的边
    pub fn has_edge(&self, from: VirtBlockId, to: VirtBlockId) -> bool {
        self.nodes
            .get(&from)
            .map(|node| node.successors.contains(&to))
            .unwrap_or(false)
    }

    /// 获取原始SSA块对应的ASM块
    pub fn get_asm_block(&self, basic_block: Block) -> Option<VirtBlockId> {
        self.basic_to_asm.get(&basic_block).copied()
    }

    /// 获取关键边块
    pub fn get_critical_edge(&self, pred: Block, succ: Block) -> Option<VirtBlockId> {
        self.critical_edges
            .iter()
            .find(|&&(p, s, _)| p == pred && s == succ)
            .map(|&(p, s, succ_idx)| VirtBlockId::CriticalEdge {
                pred: p,
                succ: s,
                succ_idx,
            })
    }

    /// 访问所有块（包括关键边块）
    pub fn visit_all_blocks<F>(&self, mut visitor: F)
    where
        F: FnMut(VirtBlockId, &VirtCfgNode),
    {
        for &block_id in &self.block_order {
            if let Some(node) = self.nodes.get(&block_id) {
                visitor(block_id, node);
            }
        }
    }

    /// 获取块的数量（包括关键边块）
    pub fn block_count(&self) -> usize {
        self.nodes.len()
    }

    /// 获取关键边的数量
    pub fn critical_edge_count(&self) -> usize {
        self.critical_edges.len()
    }

    /// 调试输出
    pub fn dump(&self) {
        println!("=== ASM Control Flow Graph ===");
        println!("Entry: {}", self.entry);
        println!(
            "Blocks: {} (Basic: {}, Critical Edges: {})",
            self.block_count(),
            self.block_count() - self.critical_edge_count(),
            self.critical_edge_count()
        );

        for &block_id in &self.block_order {
            if let Some(node) = self.nodes.get(&block_id) {
                println!("\n{}: ", block_id);

                if !node.predecessors.is_empty() {
                    print!("  Predecessors: ");
                    for (i, pred) in node.predecessors.iter().enumerate() {
                        if i > 0 {
                            print!(", ");
                        }
                        print!("{}", pred);
                    }
                    println!();
                }

                if !node.successors.is_empty() {
                    print!("  Successors: ");
                    for (i, succ) in node.successors.iter().enumerate() {
                        if i > 0 {
                            print!(", ");
                        }
                        print!("{}", succ);
                    }
                    println!();
                }

                if let Some(term) = node.terminator {
                    println!("  Terminator: {:?}", term);
                }

                if node.is_indirect_target {
                    println!("  [Indirect Jump Target]");
                }
            }
        }
    }
}

// ===================================================================
// 内部方法
// ===================================================================

impl VirtControlFlowGraph {
    /// 识别关键边
    fn identify_critical_edges(
        &mut self,
        func: &Function,
        ssa_cfg: &crate::ssa::cfg::ControlFlowGraph,
    ) {
        for (block, _) in func.dfg.blocks.iter_all() {
            let succ_count = ssa_cfg.succ_count(block);

            // 如果多后继，检查每个后继是否有多个前驱
            if succ_count > 1 {
                let mut succ_idx = 0;
                for succ_block in ssa_cfg.succ_iter(block) {
                    let pred_count = ssa_cfg.pred_count(succ_block);
                    if pred_count > 1 {
                        // 关键边：pred有多个出边，succ有多个入边
                        self.critical_edges.push((block, succ_block, succ_idx));
                    }
                    succ_idx += 1;
                }
            }
        }
    }

    /// 创建所有节点
    fn create_nodes(&mut self, func: &Function, _ssa_cfg: &crate::ssa::cfg::ControlFlowGraph) {
        // 创建基本块节点
        for (block, _) in func.dfg.blocks.iter_all() {
            let id = VirtBlockId::BasicBlock { block };
            self.nodes.insert(id, VirtCfgNode::new());
            self.basic_to_asm.insert(block, id);
        }

        // 创建关键边节点
        for &(pred, succ, succ_idx) in &self.critical_edges {
            let id = VirtBlockId::CriticalEdge {
                succ_idx,
                pred,
                succ,
            };
            self.nodes.insert(id, VirtCfgNode::new());
        }
    }

    /// 建立控制流边
    fn build_edges(&mut self, func: &Function, ssa_cfg: &crate::ssa::cfg::ControlFlowGraph) {
        for (block, _) in func.dfg.blocks.iter_all() {
            let block_id = VirtBlockId::BasicBlock { block };

            // 设置终结指令
            if let Some(term_inst) = func.layout.last_inst(block) {
                if let Some(node) = self.nodes.get_mut(&block_id) {
                    node.terminator = Some(term_inst);
                }
            }

            // 处理后继关系
            for succ_block in ssa_cfg.succ_iter(block) {
                // 检查是否有关键边
                let critical_edge = self
                    .critical_edges
                    .iter()
                    .find(|&&(p, s, _)| p == block && s == succ_block)
                    .map(|&(p, s, succ_idx)| VirtBlockId::CriticalEdge {
                        pred: p,
                        succ: s,
                        succ_idx,
                    });

                if let Some(edge_id) = critical_edge {
                    // 通过关键边连接：block -> critical_edge -> succ
                    self.add_edge(block_id, edge_id);
                    self.add_edge(edge_id, VirtBlockId::BasicBlock { block: succ_block });
                } else {
                    // 直接连接
                    self.add_edge(block_id, VirtBlockId::BasicBlock { block: succ_block });
                }
            }
        }
    }

    /// 添加控制流边
    fn add_edge(&mut self, from: VirtBlockId, to: VirtBlockId) {
        if let Some(from_node) = self.nodes.get_mut(&from) {
            from_node.add_succ(to);
        }
        if let Some(to_node) = self.nodes.get_mut(&to) {
            to_node.add_pred(from);
        }
    }

    /// 生成RPO块访问顺序
    fn generate_block_order(&mut self, domtree: &DominatorTree) {
        // 先添加所有基本块（按RPO顺序）
        for &block in domtree.cfg_rpo() {
            let block_id = VirtBlockId::BasicBlock { block };
            self.block_order.push(block_id);

            // 在基本块后面立即添加它的关键边块
            for &(pred, succ, succ_idx) in &self.critical_edges {
                if pred == block {
                    let edge_id = VirtBlockId::CriticalEdge {
                        succ_idx,
                        pred,
                        succ,
                    };
                    self.block_order.push(edge_id);
                }
            }
        }
    }

    /// 标记间接跳转目标
    fn mark_indirect_targets(&mut self, func: &Function) {
        let mut indirect_targets = HashSet::new();

        // 遍历所有指令，找出跳转表的目标
        for (block, _) in func.dfg.blocks.iter_all() {
            if let Some(term_inst) = func.layout.last_inst(block) {
                if let Some(InstructionData::BranchTable { table, .. }) =
                    func.dfg.insts.get(term_inst)
                {
                    if let Some(table_data) = func.dfg.jump_tables.get(*table) {
                        for entry in &table_data.table {
                            indirect_targets.insert(entry.block);
                        }
                    }
                }
            }
        }

        // 标记节点
        for block in indirect_targets {
            if let Some(node) = self.nodes.get_mut(&VirtBlockId::BasicBlock { block }) {
                node.is_indirect_target = true;
            }
        }
    }
}

// ===================================================================
// 测试
// ===================================================================

#[cfg(test)]
mod tests {
    use super::*;

    fn create_complex_test_function() -> FuncContext {
        // 测试关键边分割
        // CFG:
        //            0
        //          /   \
        //         1     2
        //        /  \     \
        //       3    4    |
        //       |\  _|____|
        //       | \/ |
        //       | /\ |
        //       5    6
        // (3 -> 5 和 3 -> 6 是关键边，需要被分割)

        let mut func = Function::new();

        // 创建7个基本块
        let bb0 = func.dfg.blocks.insert(BlockData {
            params: func.dfg.value_vecs.insert(vec![]),
        });
        let bb1 = func.dfg.blocks.insert(BlockData {
            params: func.dfg.value_vecs.insert(vec![]),
        });
        let bb2 = func.dfg.blocks.insert(BlockData {
            params: func.dfg.value_vecs.insert(vec![]),
        });
        let bb3 = func.dfg.blocks.insert(BlockData {
            params: func.dfg.value_vecs.insert(vec![]),
        });
        let bb4 = func.dfg.blocks.insert(BlockData {
            params: func.dfg.value_vecs.insert(vec![]),
        });
        let bb5 = func.dfg.blocks.insert(BlockData {
            params: func.dfg.value_vecs.insert(vec![]),
        });
        let bb6 = func.dfg.blocks.insert(BlockData {
            params: func.dfg.value_vecs.insert(vec![]),
        });

        // 设置入口块
        func.entry = bb0;

        // 添加块到布局
        func.layout.append_block(bb0);
        func.layout.append_block(bb1);
        func.layout.append_block(bb2);
        func.layout.append_block(bb3);
        func.layout.append_block(bb4);
        func.layout.append_block(bb5);
        func.layout.append_block(bb6);

        // 创建控制流指令
        // bb0 -> bb1, bb2 (branch)
        let cond0 = func.dfg.make_value(ValueData::int32_val(Type::Int32, 1)); // true = 1
        let inst0 = func.dfg.insts.insert(InstructionData::Brif {
            cond: cond0,
            targets: [
                BlockCall {
                    block: bb1,
                    args: func.dfg.value_vecs.insert(vec![]),
                },
                BlockCall {
                    block: bb2,
                    args: func.dfg.value_vecs.insert(vec![]),
                },
            ],
        });
        func.layout.append_inst(inst0, bb0);

        // bb1 -> bb3, bb4 (branch)
        let cond1 = func.dfg.make_value(ValueData::int32_val(Type::Int32, 1)); // true = 1
        let inst1 = func.dfg.insts.insert(InstructionData::Brif {
            cond: cond1,
            targets: [
                BlockCall {
                    block: bb3,
                    args: func.dfg.value_vecs.insert(vec![]),
                },
                BlockCall {
                    block: bb4,
                    args: func.dfg.value_vecs.insert(vec![]),
                },
            ],
        });
        func.layout.append_inst(inst1, bb1);

        // bb2 -> bb5 (jump)
        let inst2 = func.dfg.insts.insert(InstructionData::Jump {
            target: BlockCall {
                block: bb5,
                args: func.dfg.value_vecs.insert(vec![]),
            },
        });
        func.layout.append_inst(inst2, bb2);

        // bb3 -> bb5, bb6 (branch)
        let cond3 = func.dfg.make_value(ValueData::int32_val(Type::Int32, 1)); // true = 1
        let inst3 = func.dfg.insts.insert(InstructionData::Brif {
            cond: cond3,
            targets: [
                BlockCall {
                    block: bb5,
                    args: func.dfg.value_vecs.insert(vec![]),
                },
                BlockCall {
                    block: bb6,
                    args: func.dfg.value_vecs.insert(vec![]),
                },
            ],
        });
        func.layout.append_inst(inst3, bb3);

        // bb4 -> bb6 (jump)
        let inst4 = func.dfg.insts.insert(InstructionData::Jump {
            target: BlockCall {
                block: bb6,
                args: func.dfg.value_vecs.insert(vec![]),
            },
        });
        func.layout.append_inst(inst4, bb4);

        // bb5 和 bb6 是终止块 (return)
        let inst5 = func.dfg.insts.insert(InstructionData::Return {
            returns: func.dfg.value_vecs.insert(vec![]),
        });
        func.layout.append_inst(inst5, bb5);

        let inst6 = func.dfg.insts.insert(InstructionData::Return {
            returns: func.dfg.value_vecs.insert(vec![]),
        });
        func.layout.append_inst(inst6, bb6);

        // 创建 FuncContext 并计算 CFG 和 domtree
        let mut func_ctx = FuncContext::from_function(func);
        func_ctx.cfg.compute(&func_ctx.func);
        func_ctx.domtree.compute(&func_ctx.func, &func_ctx.cfg);

        func_ctx
    }

    #[test]
    fn test_complex_critical_edges() {
        let func_ctx = create_complex_test_function();
        let asm_cfg = VirtControlFlowGraph::build(&func_ctx);

        // 验证结果
        // 应该有7个基本块 + 2个关键边 = 9个降级块
        let basic_block_count = asm_cfg.block_order.iter().filter(|b| b.is_basic()).count();
        assert_eq!(basic_block_count, 7);

        // 应该有2个关键边：bb3 -> bb5 和 bb3 -> bb6
        assert_eq!(asm_cfg.critical_edge_count(), 2);

        // 收集所有关键边
        let mut critical_edges = Vec::new();
        for block_id in asm_cfg.iter_blocks() {
            if let VirtBlockId::CriticalEdge { pred, succ, .. } = block_id {
                critical_edges.push((pred, succ));
            }
        }

        // 验证关键边
        assert_eq!(critical_edges.len(), 2);

        // 找出 bb3 对应的块（可能是 bb3、bb4 或其他）
        // 由于我们不知道具体的块分配，需要根据控制流找出具有2个后继且后继有多个前驱的块
        let mut found_critical_edges = 0;
        for &(pred, succ, _) in asm_cfg.critical_edges() {
            println!("Critical edge: {:?} -> {:?}", pred, succ);
            found_critical_edges += 1;
        }
        assert_eq!(found_critical_edges, 2);

        // 验证关键边块的属性
        for block_id in asm_cfg.iter_blocks() {
            if let VirtBlockId::CriticalEdge { pred, succ, .. } = block_id {
                // 关键边块应该有且仅有一个前驱和一个后继
                assert_eq!(asm_cfg.pred_count(block_id), 1);
                assert_eq!(asm_cfg.succ_count(block_id), 1);

                // 验证连接关系
                let preds: Vec<_> = asm_cfg.predecessors(block_id).collect();
                assert_eq!(preds[0], VirtBlockId::BasicBlock { block: pred });

                let succs: Vec<_> = asm_cfg.successors(block_id).collect();
                assert_eq!(succs[0], VirtBlockId::BasicBlock { block: succ });
            }
        }
    }
}
