//! Loop Analysis
//!
//! 备注/附录:
//! - RPO顺序保证: 必须按RPO处理循环头，确保外层循环在内层循环之前，否则会出事情
//! - 逆序遍历: 在discover_loop_blocks中反向遍历，保证内层循环先于外层循环处理

use crate::prelude::*;
use crate::ssa::cfg::ControlFlowGraph;
use crate::ssa::domtree::DominatorTree;
use crate::ssa::function::Function;
use std::collections::HashSet;

/// 循环嵌套深度
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct LoopLevel(u8);

impl LoopLevel {
    /// 无效的循环深度
    const INVALID: u8 = u8::MAX;

    /// 根级别（不在任何循环中）
    pub fn root() -> Self {
        Self(0)
    }

    /// 获取深度值
    pub fn level(self) -> usize {
        self.0 as usize
    }

    /// 无效深度
    pub fn invalid() -> Self {
        Self(Self::INVALID)
    }

    /// 是否为无效深度
    pub fn is_invalid(self) -> bool {
        self.0 == Self::INVALID
    }

    /// 增加一层深度
    pub fn inc(self) -> Self {
        if self.0 == (Self::INVALID - 1) {
            self // 防止溢出
        } else {
            Self(self.0 + 1)
        }
    }

    /// 从 usize 创建，带有上限检查
    pub fn clamped(level: usize) -> Self {
        let max_level = (Self::INVALID - 1) as usize;
        let clamped = level.min(max_level);
        Self(clamped as u8)
    }
}

impl Default for LoopLevel {
    fn default() -> Self {
        Self::invalid()
    }
}

/// 循环数据
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
struct LoopData {
    /// 循环头块
    header: Block,
    /// 父循环（在循环树中）
    parent: Option<Loop>,
    /// 循环嵌套深度
    level: LoopLevel,
}

impl LoopData {
    fn new(header: Block, parent: Option<Loop>) -> Self {
        Self {
            header,
            parent,
            level: LoopLevel::invalid(),
        }
    }
}

/// 循环分析
#[derive(Debug)]
pub struct LoopAnalysis {
    /// 所有循环的数据
    loops: RefMap<Loop, LoopData>,
    /// 块到最内层循环的映射
    block_loop_map: RefSecMap<Block, Option<Loop>>,
    /// 是否有效
    valid: bool,
    /// 是否启用调试打印
    debug: bool,
}

impl LoopAnalysis {
    pub fn new() -> Self {
        Self {
            loops: RefMap::new(),
            block_loop_map: RefSecMap::new(),
            valid: false,
            debug: false,
        }
    }

    pub fn set_debug(&mut self, enabled: bool) {
        self.debug = enabled;
    }

    pub fn is_debug_enabled(&self) -> bool {
        self.debug
    }

    pub fn clear(&mut self) {
        self.loops.clear();
        self.block_loop_map.clear();
        self.valid = false;
    }

    /// 计算循环分析
    pub fn compute(&mut self, func: &Function, cfg: &ControlFlowGraph, domtree: &DominatorTree) {
        // 总是清理并重新计算，确保状态一致
        self.clear();

        if self.debug {
            eprintln!("\n=== 开始循环分析: 函数 {} ===", func.name);
        }

        // 查找所有循环头
        self.find_loop_headers(func, cfg, domtree);

        if self.debug {
            eprintln!("发现 {} 个循环头", self.loops.len());
        }

        // 发现每个循环包含的块
        self.discover_loop_blocks(func, cfg, domtree);

        // 分配循环深度
        self.assign_loop_levels();

        self.valid = true;
    }

    /// 检查分析是否有效
    pub fn is_valid(&self) -> bool {
        self.valid
    }

    /// 获取所有循环
    pub fn loops(&self) -> impl Iterator<Item = Loop> + '_ {
        self.loops.iter().map(|(lp, _)| lp)
    }

    /// 获取循环头块
    pub fn loop_header(&self, lp: Loop) -> Option<Block> {
        self.loops.get(lp).map(|data| data.header)
    }

    /// 获取循环的父循环
    pub fn loop_parent(&self, lp: Loop) -> Option<Loop> {
        self.loops.get(lp).and_then(|data| data.parent)
    }

    /// 获取块所在的最内层循环
    pub fn innermost_loop(&self, block: Block) -> Option<Loop> {
        self.block_loop_map.get(block).and_then(|&lp| lp)
    }

    /// 检查块是否是循环头
    pub fn is_loop_header(&self, block: Block) -> Option<Loop> {
        self.innermost_loop(block)
            .filter(|&lp| self.loops.get(lp).is_some_and(|data| data.header == block))
    }

    /// 检查块是否在指定循环中
    pub fn is_in_loop(&self, block: Block, lp: Loop) -> bool {
        match self.innermost_loop(block) {
            None => false,
            Some(block_loop) => self.is_child_loop(block_loop, lp),
        }
    }

    /// 检查一个循环是否是另一个循环的子循环（或相同）
    pub fn is_child_loop(&self, child: Loop, parent: Loop) -> bool {
        let mut current = Some(child);
        while let Some(lp) = current {
            if lp == parent {
                return true;
            }
            current = self.loop_parent(lp);
        }
        false
    }

    /// 获取块的循环嵌套深度
    pub fn loop_level(&self, block: Block) -> LoopLevel {
        self.innermost_loop(block)
            .and_then(|lp| self.loops.get(lp))
            .map(|data| data.level)
            .unwrap_or(LoopLevel::root())
    }

    /// 获取循环的嵌套深度
    pub fn loop_level_of_loop(&self, lp: Loop) -> LoopLevel {
        self.loops
            .get(lp)
            .map(|data| data.level)
            .unwrap_or(LoopLevel::invalid())
    }

    /// 检查一个块是否是循环头（内部辅助函数）
    /// 循环头的特征是它支配至少一个前驱
    fn is_block_loop_header(block: Block, cfg: &ControlFlowGraph, domtree: &DominatorTree) -> bool {
        // 检查是否有任何前驱被该块支配（即存在回边）
        cfg.pred_iter(block)
            .any(|pred| domtree.dominates(block, pred.block))
    }

    /// 查找所有循环头
    fn find_loop_headers(
        &mut self,
        _func: &Function,
        cfg: &ControlFlowGraph,
        domtree: &DominatorTree,
    ) {
        // 收集所有循环头块，保持 RPO 顺序
        let loop_headers: Vec<Block> = domtree
            .cfg_postorder()
            .iter()
            .rev() // 转换为RPO顺序
            .filter(|&&block| Self::is_block_loop_header(block, cfg, domtree))
            .copied()
            .collect();

        // 按照 RPO 顺序创建循环
        for block in loop_headers {
            let loop_data = LoopData::new(block, None);
            // 使用 insert 而不是 intern，因为每个循环都是唯一的
            let lp = self.loops.insert(loop_data);
            self.block_loop_map.insert(block, Some(lp));

            if self.debug {
                eprintln!("创建循环 {:?} 头块 {:?}", lp, block);
            }
        }
    }

    /// 发现每个循环包含的块
    fn discover_loop_blocks(
        &mut self,
        _func: &Function,
        cfg: &ControlFlowGraph,
        domtree: &DominatorTree,
    ) {
        let mut stack: Vec<Block> = Vec::new();

        // 收集所有循环及其头块，按照头块的 RPO 顺序排序
        let mut loops_with_headers: Vec<(Loop, Block)> = Vec::new();
        for (lp, data) in self.loops.iter() {
            loops_with_headers.push((lp, data.header));
        }

        // 根据头块在 RPO 中的位置排序 -- 通过 domtree 的 RPO
        let rpo: Vec<Block> = domtree.cfg_postorder().iter().rev().copied().collect();
        let block_to_rpo_idx: RefSecMap<Block, usize> = {
            let mut map = RefSecMap::new();
            for (idx, &block) in rpo.iter().enumerate() {
                map.insert(block, idx);
            }
            map
        };

        loops_with_headers.sort_by_key(|(_, header)| {
            block_to_rpo_idx.get(*header).copied().unwrap_or(usize::MAX)
        });

        // 反向处理每个循环，对应伪后序遍历 -- 确保先处理内层循环，再处理外层循环
        for (lp, _) in loops_with_headers.into_iter().rev() {
            let header = match self.loops.get(lp) {
                Some(data) => data.header,
                None => continue,
            };

            if self.debug {
                eprintln!("处理循环 {:?} (头: {:?})", lp, header);
            }

            // 收集所有回边（被循环头支配的前驱）
            stack.clear();
            stack.extend(
                cfg.pred_iter(header)
                    .filter(|pred| domtree.dominates(header, pred.block))
                    .map(|pred| pred.block),
            );

            if self.debug {
                eprintln!("  回边源: {:?}", stack.clone());
            }

            // DFS遍历循环体
            while let Some(node) = stack.pop() {
                let continue_dfs: Option<Block>;

                match self.block_loop_map.get(node).and_then(|&opt| opt) {
                    None => {
                        // 节点未访问过，标记为当前循环的一部分
                        self.block_loop_map.insert(node, Some(lp));
                        if self.debug {
                            eprintln!("    块 {:?} 加入循环 {:?}", node, lp);
                        }
                        continue_dfs = Some(node);
                    }
                    Some(node_loop) => {
                        // 节点已属于某个循环，需要建立循环树关系
                        let initial_node_loop = node_loop;
                        let mut node_loop = node_loop;
                        let mut node_loop_parent_option =
                            self.loops.get(node_loop).and_then(|d| d.parent);

                        // 向上遍历，找到还没有父循环的最外层循环
                        while let Some(node_loop_parent) = node_loop_parent_option {
                            if node_loop_parent == lp {
                                // 已经遇到 lp，停止（已访问）
                                break;
                            } else {
                                node_loop = node_loop_parent;
                                node_loop_parent_option =
                                    self.loops.get(node_loop).and_then(|d| d.parent);
                            }
                        }

                        // 现在 node_loop_parent_option 要么是：
                        // - None：node_loop 是 lp 的新内层循环
                        // - Some(...)：初始的 node_loop 已经是 lp 的已知内层循环
                        match node_loop_parent_option {
                            Some(_) => {
                                // 已经是 lp 的子循环，停止 DFS
                                if self.debug {
                                    eprintln!(
                                        "    块 {:?} 已在循环 {:?} (是 {:?} 的子循环)，停止",
                                        node, initial_node_loop, lp
                                    );
                                }
                                continue_dfs = None;
                            }
                            None => {
                                if node_loop != lp {
                                    // 设置父循环关系
                                    if self.debug {
                                        eprintln!(
                                            "    设置循环 {:?} 的父循环为 {:?}",
                                            node_loop, lp
                                        );
                                    }

                                    if let Some(loop_data) = self.loops.get_mut(node_loop) {
                                        loop_data.parent = Some(lp);
                                    }
                                    // 关键：从内层循环的头节点继续 DFS
                                    continue_dfs =
                                        self.loops.get(node_loop).map(|data| data.header);

                                    if self.debug {
                                        if let Some(header) = continue_dfs {
                                            eprintln!("    从内层循环头 {:?} 继续 DFS", header);
                                        }
                                    }
                                } else {
                                    // lp 是单块循环，停止
                                    if self.debug {
                                        eprintln!("    遇到循环自身 {:?}，停止", lp);
                                    }
                                    continue_dfs = None;
                                }
                            }
                        }
                    }
                }

                // 继续 DFS：将前驱加入栈
                if let Some(continue_block) = continue_dfs {
                    let preds: Vec<_> = cfg
                        .pred_iter(continue_block)
                        .map(|pred| pred.block)
                        .collect();
                    if self.debug && !preds.is_empty() {
                        eprintln!("    添加 {:?} 的前驱到栈: {:?}", continue_block, preds);
                    }
                    stack.extend(preds);
                }
            }
        }

        if self.debug {
            eprintln!("\n循环树结构:");
            for (lp, data) in self.loops.iter() {
                eprintln!(
                    "  循环 {:?}: 头={:?}, 父={:?}",
                    lp, data.header, data.parent
                );
            }
        }
    }

    /// 分配循环嵌套深度
    fn assign_loop_levels(&mut self) {
        let mut stack: Vec<Loop> = Vec::new();

        // 使用 HashSet 检测循环依赖
        let mut visited = HashSet::new();

        // 遍历所有循环，计算嵌套深度
        for lp in self.loops.iter().map(|(lp, _)| lp).collect::<Vec<_>>() {
            // 跳过已分配深度的循环
            if let Some(data) = self.loops.get(lp) {
                if !data.level.is_invalid() {
                    continue;
                }
            }

            // 使用栈来处理循环树，避免递归
            stack.clear();
            visited.clear();
            stack.push(lp);
            visited.insert(lp);

            while let Some(&current) = stack.last() {
                let parent_opt = self.loops.get(current).and_then(|data| data.parent);

                match parent_opt {
                    Some(parent) => {
                        // 检测循环依赖
                        if visited.contains(&parent) {
                            // 发现循环依赖！这不应该发生
                            panic!(
                                "循环父指针依赖检测！循环 {:?} 的祖先链形成了环。\n\
                                当前栈: {:?}\n\
                                尝试添加的父循环: {:?}\n\
                                这表明 discover_loop_blocks 产生了错误的循环树结构。",
                                current, stack, parent
                            );
                        }

                        // 有父循环，检查父循环的深度
                        let parent_level = self.loops.get(parent).map(|data| data.level);

                        match parent_level {
                            Some(level) if !level.is_invalid() => {
                                // 父循环已有深度，设置当前循环深度
                                if let Some(data) = self.loops.get_mut(current) {
                                    data.level = level.inc();
                                }
                                stack.pop();
                                visited.remove(&current);
                            }
                            Some(_) => {
                                // 父循环还没有深度，先处理父循环
                                stack.push(parent);
                                visited.insert(parent);
                            }
                            None => {
                                // 父循环不存在（不应该发生），设为顶层循环
                                eprintln!(
                                    "警告: 循环 {:?} 引用了不存在的父循环 {:?}",
                                    current, parent
                                );
                                if let Some(data) = self.loops.get_mut(current) {
                                    data.level = LoopLevel::root().inc();
                                }
                                stack.pop();
                                visited.remove(&current);
                            }
                        }
                    }
                    None => {
                        // 没有父循环，这是顶层循环
                        if let Some(data) = self.loops.get_mut(current) {
                            data.level = LoopLevel::root().inc();
                        }
                        stack.pop();
                        visited.remove(&current);
                    }
                }
            }
        }
    }

    /// 获取循环中的所有块（包括嵌套循环的块）
    pub fn get_all_blocks_in_loop(&self, lp: Loop) -> HashSet<Block> {
        let mut result = HashSet::new();

        // 遍历所有块，检查它们是否属于这个循环
        for (block, &block_loop_opt) in self.block_loop_map.iter() {
            if let Some(block_loop) = block_loop_opt {
                // 检查块是否属于该循环或其嵌套循环
                let mut current_loop = Some(block_loop);
                while let Some(curr) = current_loop {
                    if curr == lp {
                        result.insert(block);
                        break;
                    }
                    // 向上遍历父循环
                    current_loop = self.loops.get(curr).and_then(|data| data.parent);
                }
            }
        }

        result
    }

    /// 打印循环分析详情
    #[allow(dead_code)]
    pub fn dump(&self) {
        eprintln!("=== 循环分析详情 ===");
        for (lp, data) in self.loops.iter() {
            eprintln!("循环 {}:", lp);
            eprintln!("  - 循环头: {}", data.header);
            eprintln!("  - 父循环: {:?}", data.parent);
            eprintln!("  - 深度: {:?}", data.level);
        }
        eprintln!("块到循环映射:");
        for (block, lp) in self.block_loop_map.iter() {
            eprintln!("  块 {} -> 循环 {:?}", block, lp);
        }
        // eprintln!("===================");
    }
}

impl Default for LoopAnalysis {
    fn default() -> Self {
        Self::new()
    }
}
