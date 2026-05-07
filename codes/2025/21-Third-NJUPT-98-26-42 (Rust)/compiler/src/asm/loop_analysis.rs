//! ASM LoopAnalysis
//!
//! 备注/附录:
//! - SSA使用Block，ASM使用VirtBlockId -- 包含CriticalEdge
//! - 计算最大压力 --一个外层循环必须要取得所有内部块的最大压力，
//!     哪怕有嵌套循环等，也都涵盖在内，防止live_through没有考虑到内层循环的压力，盲目穿透
//!     进而导致内层循环发生溢出(而在循环内的溢出代价和成本都非常高)
//! - 识别循环内部使用的变量 -- 构造 candidates

use super::{cfg::*, inst::*, liveness::*, prog::*, reg::*};
use crate::prelude::*;
use std::collections::{BTreeMap, BTreeSet};

// ===================================================================
// 循环标识符
// ===================================================================

/// 循环标识符
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]
pub struct AsmLoopId(u32);

impl AsmLoopId {
    fn new(id: u32) -> Self {
        Self(id)
    }

    pub fn index(&self) -> u32 {
        self.0
    }
}

// ===================================================================
// 循环信息
// ===================================================================

/// 单个循环的详细信息
#[derive(Debug, Clone)]
pub struct AsmLoopInfo {
    /// 循环标识符
    pub id: AsmLoopId,

    /// 循环头块 -- VirtBlockId
    pub header: VirtBlockId,

    /// 循环体包含的所有块 -- 包括嵌套循环的块
    pub blocks: BTreeSet<VirtBlockId>,

    /// 循环的回边 (from, to)
    pub back_edges: Vec<(VirtBlockId, VirtBlockId)>,

    /// 循环的出口边 (from_inside, to_outside)
    pub exit_edges: Vec<(VirtBlockId, VirtBlockId)>,

    /// 整个循环内的最大整数寄存器压力
    pub max_pressure_x: usize,

    /// 整个循环内的最大浮点寄存器压力
    pub max_pressure_f: usize,

    /// 在循环内部被使用的变量 -- live-in到循环头且在循环内至少被使用一次
    pub used_vars: BTreeSet<Reg>,

    /// 父循环 -- 在循环树中
    pub parent: Option<AsmLoopId>,

    /// 直接子循环
    pub children: Vec<AsmLoopId>,

    /// 循环嵌套深度 -- 顶层循环为1
    pub level: u32,
}

impl AsmLoopInfo {
    fn new(id: AsmLoopId, header: VirtBlockId) -> Self {
        Self {
            id,
            header,
            blocks: BTreeSet::new(),
            back_edges: Vec::new(),
            exit_edges: Vec::new(),
            max_pressure_x: 0,
            max_pressure_f: 0,
            used_vars: BTreeSet::new(),
            parent: None,
            children: Vec::new(),
            level: 1,
        }
    }
}

// ===================================================================
// ASM循环分析
// ===================================================================

/// ASM层的循环分析结果
#[derive(Debug)]
pub struct AsmLoopAnalysis {
    /// 所有循环的信息
    loops: BTreeMap<AsmLoopId, AsmLoopInfo>,

    /// 从块到其所属最内层循环的映射
    block_to_loop: BTreeMap<VirtBlockId, AsmLoopId>,

    /// 从SSA循环到ASM循环的映射 -- 用于转换
    ssa_to_asm_loop: BTreeMap<Loop, AsmLoopId>,

    /// 下一个可用的循环ID
    next_loop_id: u32,
}

// ===================================================================
// 外部查询公共接口
// ===================================================================

impl AsmLoopAnalysis {
    /// 检查块是否是循环头
    pub fn is_loop_header(&self, block: VirtBlockId) -> Option<AsmLoopId> {
        self.block_to_loop.get(&block).and_then(|&loop_id| {
            self.loops.get(&loop_id).and_then(|info| {
                if info.header == block {
                    Some(loop_id)
                } else {
                    None
                }
            })
        })
    }

    /// 获取块所在的最内层循环
    pub fn innermost_loop(&self, block: VirtBlockId) -> Option<AsmLoopId> {
        self.block_to_loop.get(&block).copied()
    }

    /// 获取循环的详细信息
    pub fn get_loop_info(&self, loop_id: AsmLoopId) -> Option<&AsmLoopInfo> {
        self.loops.get(&loop_id)
    }

    /// 获取循环的最大寄存器压力 (整数, 浮点)
    /// (max_x_pressure, max_f_pressure)
    pub fn get_loop_max_pressure(&self, loop_id: AsmLoopId) -> Option<(usize, usize)> {
        self.get_loop_info(loop_id)
            .map(|info| (info.max_pressure_x, info.max_pressure_f))
    }

    /// 获取在循环内部被使用的变量集合
    pub fn get_used_in_loop_vars(&self, loop_id: AsmLoopId) -> Option<&BTreeSet<Reg>> {
        self.get_loop_info(loop_id).map(|info| &info.used_vars)
    }

    /// 获取循环的所有出口边
    pub fn get_exit_edges(&self, loop_id: AsmLoopId) -> Option<&[(VirtBlockId, VirtBlockId)]> {
        self.get_loop_info(loop_id).map(|info| &info.exit_edges[..])
    }

    /// 判断边是否是循环出口边
    pub fn is_exit_edge(&self, from: VirtBlockId, to: VirtBlockId) -> bool {
        // 检查是否存在某个循环，使得(from, to)是其出口边
        self.loops.values().any(|loop_info| {
            loop_info
                .exit_edges
                .iter()
                .any(|&(f, t)| f == from && t == to)
        })
    }

    /// 获取循环的父循环
    pub fn get_parent(&self, loop_id: AsmLoopId) -> Option<AsmLoopId> {
        self.get_loop_info(loop_id).and_then(|info| info.parent)
    }

    /// 获取循环的子循环
    pub fn get_children(&self, loop_id: AsmLoopId) -> Option<&[AsmLoopId]> {
        self.get_loop_info(loop_id).map(|info| &info.children[..])
    }

    /// 获取循环的嵌套深度
    pub fn get_loop_level(&self, loop_id: AsmLoopId) -> Option<u32> {
        self.get_loop_info(loop_id).map(|info| info.level)
    }

    /// 获取块的循环嵌套深度
    pub fn get_block_loop_level(&self, block: VirtBlockId) -> u32 {
        self.innermost_loop(block)
            .and_then(|loop_id| self.get_loop_level(loop_id))
            .unwrap_or(0)
    }
}

// ===================================================================
// 初始构建 & 内部方法
// ===================================================================

impl AsmLoopAnalysis {
    pub fn new() -> Self {
        Self {
            loops: BTreeMap::new(),
            block_to_loop: BTreeMap::new(),
            ssa_to_asm_loop: BTreeMap::new(),
            next_loop_id: 0,
        }
    }

    /// 从SSA循环分析和活跃变量分析构建ASM循环分析
    pub fn build(
        ssa_loop_analysis: &crate::ssa::LoopAnalysis,
        liveness: &LivenessAnalysis,
        vfunc: &VirtFunc,
    ) -> Self {
        let mut analysis = Self::new();

        // 1. 转换SSA循环到ASM循环 -- 考虑Critical Edge
        analysis.convert_from_ssa_loops(ssa_loop_analysis, &vfunc.cfg);

        // 2. 识别循环的回边和出口边
        analysis.identify_loop_edges(&vfunc.cfg);

        // 3. 构建循环树 -- 父子关系
        analysis.build_loop_tree();

        // 4. 计算循环嵌套深度
        analysis.compute_loop_levels();

        // 5. 基于循环树构建正确的block_to_loop映射
        analysis.finalize_block_mapping();

        // 6. 计算每个循环的最大寄存器压力和使用变量
        analysis.compute_loop_pressure_and_vars(liveness, vfunc);

        analysis
    }

    /// 调试输出
    pub fn dump(&self) {
        eprintln!("=== ASM Loop Analysis ===");
        eprintln!("Total loops: {}", self.loops.len());

        // 按深度排序输出
        let mut sorted_loops: Vec<_> = self.loops.values().collect();
        sorted_loops.sort_by_key(|info| (info.level, info.id.index()));

        for info in sorted_loops {
            eprintln!("\nLoop {:?} (level {}):", info.id, info.level);
            eprintln!("  Header: {:?}", info.header);
            eprintln!("  Parent: {:?}", info.parent);
            eprintln!("  Children: {:?}", info.children);
            eprintln!("  Blocks: {} blocks", info.blocks.len());
            eprintln!("  Back edges: {:?}", info.back_edges);
            eprintln!("  Exit edges: {:?}", info.exit_edges);
            eprintln!(
                "  Max pressure: X={}, F={}",
                info.max_pressure_x, info.max_pressure_f
            );
            eprintln!("  Used vars: {} vars", info.used_vars.len());
        }

        // 添加block_to_loop映射的调试输出
        eprintln!("\n=== Block to Loop Mapping ===");
        let mut block_mappings: Vec<_> = self.block_to_loop.iter().collect();
        block_mappings.sort_by_key(|(block, _)| format!("{:?}", block));

        for (block, loop_id) in block_mappings {
            eprintln!("  Block {:?} -> Loop {:?}", block, loop_id);
        }

        eprintln!("\n=========================");
    }

    /// 转换SSA循环到ASM循环
    fn convert_from_ssa_loops(
        &mut self,
        ssa_loop_analysis: &crate::ssa::LoopAnalysis,
        cfg: &VirtControlFlowGraph,
    ) {
        // 第一步：创建所有ASM循环并建立映射
        for ssa_loop in ssa_loop_analysis.loops() {
            // 获取SSA循环头
            if let Some(ssa_header) = ssa_loop_analysis.loop_header(ssa_loop) {
                // 转换循环头到ASM块
                if let Some(asm_header) = cfg.get_asm_block(ssa_header) {
                    // 创建新的ASM循环
                    let loop_id = self.allocate_loop_id();
                    let mut loop_info = AsmLoopInfo::new(loop_id, asm_header);

                    // 收集循环体中的所有块
                    self.collect_loop_blocks(
                        ssa_loop,
                        ssa_loop_analysis,
                        cfg,
                        &mut loop_info.blocks,
                    );

                    // 记录映射关系
                    self.ssa_to_asm_loop.insert(ssa_loop, loop_id);
                    self.loops.insert(loop_id, loop_info);
                }
            }
        }

        // 第二步：设置父子关系 -- 此时所有映射都已建立
        for ssa_loop in ssa_loop_analysis.loops() {
            if let Some(&asm_loop_id) = self.ssa_to_asm_loop.get(&ssa_loop) {
                // 转换SSA父循环
                if let Some(ssa_parent) = ssa_loop_analysis.loop_parent(ssa_loop) {
                    if let Some(&asm_parent_id) = self.ssa_to_asm_loop.get(&ssa_parent) {
                        // 设置父循环
                        if let Some(loop_info) = self.loops.get_mut(&asm_loop_id) {
                            loop_info.parent = Some(asm_parent_id);
                        }
                    }
                }
            }
        }
    }

    /// 收集循环体中的所有ASM块 -- 包括Critical Edge块
    fn collect_loop_blocks(
        &self,
        ssa_loop: Loop,
        ssa_loop_analysis: &crate::ssa::LoopAnalysis,
        cfg: &VirtControlFlowGraph,
        blocks: &mut BTreeSet<VirtBlockId>,
    ) {
        // 从SSA分析获取循环中的所有块
        let ssa_blocks = ssa_loop_analysis.get_all_blocks_in_loop(ssa_loop);

        // 将SSA块转换为ASM块
        for ssa_block in ssa_blocks {
            // 添加基本块
            if let Some(asm_block) = cfg.get_asm_block(ssa_block) {
                blocks.insert(asm_block);
            }

            // 检查并添加关键边块
            // 遍历该块的所有后继，查找关键边
            for succ in cfg.successors(VirtBlockId::BasicBlock { block: ssa_block }) {
                if let VirtBlockId::CriticalEdge {
                    pred,
                    succ: edge_succ,
                    ..
                } = succ
                {
                    // 如果关键边的两端都在循环内，则关键边块也属于循环
                    if pred == ssa_block && ssa_loop_analysis.is_in_loop(edge_succ, ssa_loop) {
                        blocks.insert(succ);
                    }
                }
            }
        }
    }

    /// 识别循环的回边和出口边
    fn identify_loop_edges(&mut self, cfg: &VirtControlFlowGraph) {
        let loop_ids: Vec<_> = self.loops.keys().copied().collect();

        for loop_id in loop_ids {
            let header = self.loops[&loop_id].header;
            let blocks = self.loops[&loop_id].blocks.clone();

            let mut back_edges = Vec::new();
            let mut exit_edges = Vec::new();

            // 遍历循环内的所有块
            for &block in &blocks {
                // 检查所有后继
                for succ in cfg.successors(block) {
                    if succ == header {
                        // 回边：从循环内跳到循环头
                        back_edges.push((block, succ));
                    } else if !blocks.contains(&succ) {
                        // 出口边：从循环内跳到循环外
                        exit_edges.push((block, succ));
                    }
                }
            }

            // 更新循环信息
            if let Some(loop_info) = self.loops.get_mut(&loop_id) {
                loop_info.back_edges = back_edges;
                loop_info.exit_edges = exit_edges;
            }
        }
    }

    /// 构建循环树 -- 基于从SSA继承的父子关系
    fn build_loop_tree(&mut self) {
        // 基于已经设置好的parent关系，构建children列表
        let loop_ids: Vec<_> = self.loops.keys().copied().collect();

        for &loop_id in &loop_ids {
            // 获取父循环 -- 在convert_from_ssa_loops中已经设置
            if let Some(parent_id) = self.loops[&loop_id].parent {
                // 将当前循环添加到父循环的children列表中
                if let Some(parent_loop) = self.loops.get_mut(&parent_id) {
                    if !parent_loop.children.contains(&loop_id) {
                        parent_loop.children.push(loop_id);
                    }
                }
            }
        }
    }

    /// 计算循环嵌套深度
    fn compute_loop_levels(&mut self) {
        // 找到所有顶层循环 -- 没有父循环的
        let top_loops: Vec<_> = self
            .loops
            .iter()
            .filter(|(_, info)| info.parent.is_none())
            .map(|(&id, _)| id)
            .collect();

        // 递归设置深度
        for &loop_id in &top_loops {
            self.set_loop_level_recursive(loop_id, 1);
        }
    }

    /// 递归设置循环深度
    fn set_loop_level_recursive(&mut self, loop_id: AsmLoopId, level: u32) {
        // 设置当前循环的深度
        let children = if let Some(loop_info) = self.loops.get_mut(&loop_id) {
            loop_info.level = level;
            loop_info.children.clone()
        } else {
            return;
        };

        // 递归处理子循环
        for child_id in children {
            self.set_loop_level_recursive(child_id, level + 1);
        }
    }

    /// 基于循环树构建正确的block_to_loop映射
    /// 这个方法在循环树构建完成后调用，确保每个块被映射到其最内层循环
    fn finalize_block_mapping(&mut self) {
        // 清空旧的映射 -- 如果有的话
        self.block_to_loop.clear();

        // 按照循环深度从内到外的顺序处理 -- 确保内层循环优先
        let mut sorted_loops: Vec<_> = self.loops.keys().copied().collect();
        sorted_loops.sort_by_key(|&loop_id| {
            // 按深度降序排序 -- 深度越大的内层循环先处理
            std::cmp::Reverse(self.loops[&loop_id].level)
        });

        for loop_id in sorted_loops {
            let blocks = self.loops[&loop_id].blocks.clone();
            for block in blocks {
                // 使用entry API确保只有在块还没有被映射时才插入
                // 这保证了块被映射到最内层的循环
                self.block_to_loop.entry(block).or_insert(loop_id);
            }
        }
    }

    /// 检查child_id是否是parent_id的子循环 -- 或相同
    fn is_child_loop_of(&self, child_id: AsmLoopId, parent_id: AsmLoopId) -> bool {
        if child_id == parent_id {
            return true;
        }

        let mut current = self.loops.get(&child_id).and_then(|info| info.parent);
        while let Some(loop_id) = current {
            if loop_id == parent_id {
                return true;
            }
            current = self.loops.get(&loop_id).and_then(|info| info.parent);
        }
        false
    }

    /// 计算每个循环的最大寄存器压力和使用变量
    fn compute_loop_pressure_and_vars(&mut self, liveness: &LivenessAnalysis, vfunc: &VirtFunc) {
        // 使用后序遍历，确保子循环的压力先被计算
        let loop_post_order = self.get_loop_tree_post_order();

        for loop_id in loop_post_order {
            let (header, blocks, children) = {
                let loop_info = &self.loops[&loop_id];
                (
                    loop_info.header,
                    loop_info.blocks.clone(),
                    loop_info.children.clone(),
                )
            };

            // 获取循环头的live-in集合
            let header_live_in = liveness
                .liveness
                .get(&header)
                .map(|info| &info.live_in)
                .cloned()
                .unwrap_or_default();

            let mut max_pressure_x = 0;
            let mut max_pressure_f = 0;
            let mut used_vars = BTreeSet::new();

            // 第一步：计算当前循环直接包含的块的压力 -- 不包括子循环的块
            for &block in &blocks {
                // 只考虑直接属于当前循环的块 -- 不属于任何子循环
                let is_direct_member = self.block_to_loop.get(&block) == Some(&loop_id);

                if is_direct_member {
                    // 更新最大压力
                    if let Some(pressure) = liveness.pressure.get(&block) {
                        max_pressure_x = max_pressure_x.max(pressure.max_x_pressure);
                        max_pressure_f = max_pressure_f.max(pressure.max_f_pressure);
                    }
                }

                // 收集在块中使用的变量 -- 包括子循环的块
                if let Some(block_data) = vfunc.block_datas.get(&block) {
                    for inst in &block_data.insts {
                        // 收集指令使用的寄存器
                        let mut collector = RegCollector::new();
                        inst.visit_regs(&mut collector);

                        // 检查使用的寄存器是否在循环头的live-in中
                        for reg in &collector.uses {
                            if header_live_in.contains(reg) {
                                used_vars.insert(*reg);
                            }
                        }
                    }
                }
            }

            // 第二步：合并所有子循环的最大压力
            // 由于是后序遍历，所有子循环的压力此时已经计算完毕
            for &child_id in &children {
                if let Some(child_info) = self.loops.get(&child_id) {
                    max_pressure_x = max_pressure_x.max(child_info.max_pressure_x);
                    max_pressure_f = max_pressure_f.max(child_info.max_pressure_f);
                }
            }

            // 更新循环信息
            if let Some(loop_info) = self.loops.get_mut(&loop_id) {
                loop_info.max_pressure_x = max_pressure_x;
                loop_info.max_pressure_f = max_pressure_f;
                loop_info.used_vars = used_vars;
            }
        }
    }

    /// 获取循环树的后序遍历 -- 子循环在前，父循环在后
    fn get_loop_tree_post_order(&self) -> Vec<AsmLoopId> {
        let mut result = Vec::new();
        let mut visited = BTreeSet::new();

        // 找到所有顶层循环 -- 没有父循环的
        let top_loops: Vec<_> = self
            .loops
            .iter()
            .filter(|(_, info)| info.parent.is_none())
            .map(|(&id, _)| id)
            .collect();

        // 对每个顶层循环进行后序遍历
        for &loop_id in &top_loops {
            self.post_order_dfs(loop_id, &mut visited, &mut result);
        }

        result
    }

    /// 辅助函数：后序DFS遍历
    fn post_order_dfs(
        &self,
        loop_id: AsmLoopId,
        visited: &mut BTreeSet<AsmLoopId>,
        result: &mut Vec<AsmLoopId>,
    ) {
        if !visited.insert(loop_id) {
            return;
        }

        // 先递归访问所有子循环
        if let Some(loop_info) = self.loops.get(&loop_id) {
            for &child_id in &loop_info.children {
                self.post_order_dfs(child_id, visited, result);
            }
        }

        // 最后访问当前循环 -- 后序
        result.push(loop_id);
    }

    /// 分配新的循环ID
    fn allocate_loop_id(&mut self) -> AsmLoopId {
        let id = AsmLoopId::new(self.next_loop_id);
        self.next_loop_id += 1;
        id
    }
}

impl Default for AsmLoopAnalysis {
    fn default() -> Self {
        Self::new()
    }
}
