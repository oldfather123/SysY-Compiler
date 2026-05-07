//! Dominator Tree
//!
//! 使用 Semi-NCA 算法

use crate::prelude::*;
use crate::ssa::cfg::ControlFlowGraph;
use crate::ssa::function::Function;

/// 生成树节点，用于支配树计算过程中
#[derive(Clone, Default, Debug)]
struct SpanningTreeNode {
    /// 此节点对应的基本块
    block: Option<Block>,
    /// 生成树中的祖先节点
    ancestor: u32,
    /// 在半支配路径上发现的最小半支配值
    label: u32,
    /// 半支配值
    semi: u32,
    /// 直接支配节点
    idom: u32,
}

/// 未访问节点和虚拟根节点的DFS前序号
const NOT_VISITED: u32 = 0;

/// 生成树，按CFG前序排列
/// 节点0是虚拟根节点，没有对应的基本块
#[derive(Clone, Default, Debug)]
struct SpanningTree {
    nodes: Vec<SpanningTreeNode>,
}

impl SpanningTree {
    fn new() -> Self {
        // 包含虚拟根节点
        Self {
            nodes: vec![Default::default()],
        }
    }

    fn with_capacity(capacity: usize) -> Self {
        // 包含虚拟根节点
        let mut nodes = Vec::with_capacity(capacity + 1);
        nodes.push(Default::default());
        Self { nodes }
    }

    fn len(&self) -> usize {
        self.nodes.len()
    }

    fn reserve(&mut self, capacity: usize) {
        self.nodes.reserve(capacity);
    }

    fn clear(&mut self) {
        self.nodes.resize(1, Default::default());
    }

    /// 添加新节点，返回前序号
    fn push(&mut self, ancestor: u32, block: Block) -> u32 {
        debug_assert!(!self.nodes.is_empty());

        let pre_number = self.nodes.len() as u32;

        self.nodes.push(SpanningTreeNode {
            block: Some(block),
            ancestor,
            label: pre_number,
            semi: pre_number,
            idom: ancestor,
        });

        pre_number
    }
}

impl std::ops::Index<u32> for SpanningTree {
    type Output = SpanningTreeNode;

    fn index(&self, idx: u32) -> &Self::Output {
        &self.nodes[idx as usize]
    }
}

impl std::ops::IndexMut<u32> for SpanningTree {
    fn index_mut(&mut self, idx: u32) -> &mut Self::Output {
        &mut self.nodes[idx as usize]
    }
}

/// 遍历事件，用于同时计算前序生成树和后序块列表
#[derive(Debug)]
enum TraversalEvent {
    Enter(u32, Block),
    Exit(Block),
}

/// 支配树节点
#[derive(Clone, Default, Debug)]
struct DominatorTreeNode {
    /// 直接支配节点，对于不可达块为 None
    idom: Option<Block>,
    /// 前序遍历编号，不可达块为 0
    pre_number: u32,
}

/// 单个函数支配树
#[derive(Debug)]
pub struct DominatorTree {
    /// DFS生成树
    stree: SpanningTree,
    /// CFG后序块列表
    postorder: Vec<Block>,
    /// 支配树节点
    nodes: RefSecMap<Block, DominatorTreeNode>,

    /// 构建生成树的工作栈
    dfs_worklist: Vec<TraversalEvent>,
    /// link-eval过程中处理半支配路径的栈
    eval_worklist: Vec<u32>,

    /// 是否有效
    valid: bool,
}

/// 支配树查询方法
impl DominatorTree {
    pub fn new() -> Self {
        Self {
            stree: SpanningTree::new(),
            nodes: RefSecMap::new(),
            postorder: Vec::new(),
            dfs_worklist: Vec::new(),
            eval_worklist: Vec::new(),
            valid: false,
        }
    }

    pub fn clear(&mut self) {
        self.stree.clear();
        self.nodes.clear();
        self.postorder.clear();
        self.valid = false;
    }

    /// 计算函数的支配树
    /// Linear-Time Algorithms for Dominators and Related Problems.
    /// Loukas Georgiadis, Princeton University, November 2005.
    pub fn compute(&mut self, func: &Function, cfg: &ControlFlowGraph) {
        debug_assert!(cfg.is_valid());

        self.clear();
        self.compute_spanning_tree(func);
        self.compute_domtree(cfg);

        self.valid = true;
    }

    /// 检查支配树是否有效
    pub fn is_valid(&self) -> bool {
        self.valid
    }

    /// 检查块是否从入口块可达
    pub fn is_reachable(&self, block: Block) -> bool {
        self.nodes
            .get(block)
            .map(|node| node.pre_number != NOT_VISITED)
            .unwrap_or(false)
    }

    /// 获取块的直接支配节点
    pub fn idom(&self, block: Block) -> Option<Block> {
        self.nodes.get(block).and_then(|node| node.idom)
    }

    /// 检查 a 是否支配 b
    /// 块被认为支配自己
    pub fn dominates(&self, a: Block, b: Block) -> bool {
        self.block_dominates(a, b)
    }

    /// 内部方法：检查块级别的支配关系
    fn block_dominates(&self, block_a: Block, mut block_b: Block) -> bool {
        let pre_a = match self.nodes.get(block_a) {
            Some(node) => node.pre_number,
            None => return false,
        };

        // 从 b 向上遍历支配树直到找到 a
        while let Some(node_b) = self.nodes.get(block_b) {
            if pre_a >= node_b.pre_number {
                break;
            }
            match node_b.idom {
                Some(idom) => block_b = idom,
                None => return false, // 到达根节点
            }
        }

        block_a == block_b
    }

    /// 获取CFG后序遍历
    pub fn cfg_postorder(&self) -> &[Block] {
        debug_assert!(self.is_valid());
        &self.postorder
    }

    /// 获取CFG逆后序遍历迭代器
    pub fn cfg_rpo(&self) -> impl Iterator<Item = &Block> {
        debug_assert!(self.is_valid());
        self.postorder.iter().rev()
    }

    /// 构建生成树和计算后序遍历
    fn compute_spanning_tree(&mut self, func: &Function) {
        // 预分配空间
        let num_blocks = func.dfg.blocks.len();
        self.nodes.clear();
        self.stree.reserve(num_blocks);

        // 从入口块开始
        if func.entry != Block::default() {
            self.dfs_worklist.push(TraversalEvent::Enter(0, func.entry));
        }

        loop {
            match self.dfs_worklist.pop() {
                Some(TraversalEvent::Enter(parent, block)) => {
                    // 检查是否已访问
                    if let Some(node) = self.nodes.get(block) {
                        if node.pre_number != NOT_VISITED {
                            continue;
                        }
                    } else {
                        // 为新块创建节点
                        self.nodes.insert(block, DominatorTreeNode::default());
                    }

                    self.dfs_worklist.push(TraversalEvent::Exit(block));

                    let pre_number = self.stree.push(parent, block);
                    if let Some(node) = self.nodes.get_mut(block) {
                        node.pre_number = pre_number;
                    }

                    // 访问后继块（反向顺序）
                    let successors: Vec<Block> = func.successors(block).collect();
                    self.dfs_worklist.extend(
                        successors
                            .into_iter()
                            .rev()
                            .filter(|&successor| {
                                self.nodes
                                    .get(successor)
                                    .map(|n| n.pre_number == NOT_VISITED)
                                    .unwrap_or(true)
                            })
                            .map(|successor| TraversalEvent::Enter(pre_number, successor)),
                    );
                }
                Some(TraversalEvent::Exit(block)) => {
                    self.postorder.push(block);
                }
                None => break,
            }
        }
    }

    /// Eval-link 过程
    /// 返回半支配路径上的最小值
    fn eval(&mut self, v: u32, last_linked: u32) -> u32 {
        if self.stree[v].ancestor < last_linked {
            return self.stree[v].label;
        }

        // 跟随半支配路径
        let mut root = v;
        loop {
            self.eval_worklist.push(root);
            root = self.stree[root].ancestor;

            if self.stree[root].ancestor < last_linked {
                break;
            }
        }

        let mut prev = root;
        let root = self.stree[prev].ancestor;

        // 路径压缩
        while let Some(curr) = self.eval_worklist.pop() {
            if self.stree[prev].label < self.stree[curr].label {
                self.stree[curr].label = self.stree[prev].label;
            }

            self.stree[curr].ancestor = root;
            prev = curr;
        }

        self.stree[v].label
    }

    /// 计算支配树
    fn compute_domtree(&mut self, cfg: &ControlFlowGraph) {
        // 计算半支配节点
        for w in (1..self.stree.len() as u32).rev() {
            let block = match self.stree[w].block {
                Some(b) => b,
                None => continue,
            };

            let mut semi = self.stree[w].ancestor;
            let last_linked = w + 1;

            // 检查所有前驱
            for pred in cfg.pred_iter(block) {
                // 跳过不可达节点
                let pred_pre = match self.nodes.get(pred.block) {
                    Some(node) if node.pre_number != NOT_VISITED => node.pre_number,
                    _ => continue,
                };

                let semi_candidate = self.eval(pred_pre, last_linked);
                semi = std::cmp::min(semi, semi_candidate);
            }

            self.stree[w].label = semi;
            self.stree[w].semi = semi;
        }

        // 计算直接支配节点
        for v in 1..self.stree.len() as u32 {
            let semi = self.stree[v].semi;
            let block = match self.stree[v].block {
                Some(b) => b,
                None => continue,
            };
            let mut idom = self.stree[v].idom;

            while idom > semi {
                idom = self.stree[idom].idom;
            }

            self.stree[v].idom = idom;

            if let Some(node) = self.nodes.get_mut(block) {
                node.idom = self.stree[idom].block;
            }
        }
    }
}

impl Default for DominatorTree {
    fn default() -> Self {
        Self::new()
    }
}

/// 支配树前序遍历信息，提供额外的功能：
/// - 通过 children() 迭代器进行前向遍历
/// - 支配树前序排序
/// - 常量时间的块级支配检查
#[derive(Debug)]
pub struct DominatorTreePreorder {
    nodes: RefSecMap<Block, ExtraNode>,
    /// 用于计算的临时栈
    stack: Vec<Block>,
}

#[derive(Default, Clone, Debug)]
struct ExtraNode {
    /// 支配树中的第一个子节点
    child: Option<Block>,
    /// 支配树中的下一个兄弟节点
    sibling: Option<Block>,
    /// 支配树前序遍历的序号
    pre_number: u32,
    /// 子树中最大的前序号
    pre_max: u32,
}

impl DominatorTreePreorder {
    pub fn new() -> Self {
        Self {
            nodes: RefSecMap::new(),
            stack: Vec::new(),
        }
    }

    /// 根据支配树重新计算
    pub fn compute(&mut self, domtree: &DominatorTree, func: &Function) {
        self.nodes.clear();
        self.stack.clear();

        // 步骤1：建立子节点和兄弟节点链接
        // 按照CFG后序遍历，确保兄弟列表按CFG逆后序排序
        for &block in domtree.cfg_postorder() {
            if let Some(idom) = domtree.idom(block) {
                // 先获取 idom 的 child
                let idom_child = self.nodes.get(idom).and_then(|n| n.child);

                // 然后更新 block 的 sibling
                let node = self.nodes.get_or_insert_default(block);
                node.sibling = idom_child;

                // 最后更新 idom 的 child
                let idom_node = self.nodes.get_or_insert_default(idom);
                idom_node.child = Some(block);
            } else {
                // 只有入口块没有直接支配节点
                self.stack.push(block);
            }
        }

        // 步骤2：从支配树的DFS分配前序号
        debug_assert!(self.stack.len() <= 1);
        let mut n = 0;
        while let Some(block) = self.stack.pop() {
            n += 1;
            let node = self.nodes.get_or_insert_default(block);
            node.pre_number = n;
            node.pre_max = n;

            // 先处理兄弟，再处理子节点（为了正确的栈顺序）
            if let Some(sibling) = node.sibling {
                self.stack.push(sibling);
            }
            if let Some(child) = node.child {
                self.stack.push(child);
            }
        }

        // 步骤3：向上传播 pre_max 值
        for &block in domtree.cfg_postorder() {
            if let Some(idom) = domtree.idom(block) {
                let block_pre_max = self.nodes.get(block).map(|n| n.pre_max).unwrap_or(0);
                if let Some(idom_node) = self.nodes.get_mut(idom) {
                    idom_node.pre_max = std::cmp::max(idom_node.pre_max, block_pre_max);
                }
            }
        }
    }

    /// 获取块在支配树中的直接子节点迭代器
    pub fn children(&self, block: Block) -> ChildIter<'_> {
        ChildIter {
            dtpo: self,
            next: self.nodes.get(block).and_then(|n| n.child),
        }
    }

    /// 快速常量时间的支配检查
    /// 块被认为支配自己
    pub fn dominates(&self, a: Block, b: Block) -> bool {
        match (self.nodes.get(a), self.nodes.get(b)) {
            (Some(na), Some(nb)) => na.pre_number <= nb.pre_number && na.pre_max >= nb.pre_max,
            _ => false,
        }
    }
}

impl Default for DominatorTreePreorder {
    fn default() -> Self {
        Self::new()
    }
}

/// 支配树子节点迭代器
pub struct ChildIter<'a> {
    dtpo: &'a DominatorTreePreorder,
    next: Option<Block>,
}

impl Iterator for ChildIter<'_> {
    type Item = Block;

    fn next(&mut self) -> Option<Block> {
        let block = self.next?;
        self.next = self.dtpo.nodes.get(block).and_then(|n| n.sibling);
        Some(block)
    }
}
