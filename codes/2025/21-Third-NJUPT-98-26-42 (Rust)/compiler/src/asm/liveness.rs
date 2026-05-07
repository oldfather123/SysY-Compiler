//! 活跃变量分析模块
//!
//! 分离的设计：
//! 1. 标准活跃变量分析（LivenessInfo）
//! 2. Next-use distance 分析（NextUseInfo）
//! 3. 寄存器压力分析（RegisterPressure）
//!
//! Next-use Distance 实现要点：
//! - 必须使用迭代不动点算法，单遍扫描无法处理循环回边
//! - exit_next_use 和 entry_next_use 都必须检测变化并无条件更新，否则信息传播会中断
//! - 距离计算时需跳过纯控制流指令(J)和伪指令(Comment)，它们不占用执行周期
//! - 循环携带变量在循环头的 next-use 距离会通过多次迭代收敛到较小值
//!
//! 备注：
//! - 如果循环变量的 next-use 距离异常大，检查迭代是否正确收敛
//! - RISC-V 的 if-then-else 模式会生成额外的无条件跳转，需要特殊处理
//! - 通过在汇编中插入 Comment 展示分析结果是验证正确性的有效方法

use super::{inst::*, prog::*, reg::*, stack::*};
use std::collections::{BTreeMap, BTreeSet};

// ===================================================================
// 基础活跃变量信息
// ===================================================================

/// 基本块的活跃变量信息
#[derive(Debug, Clone, Default)]
pub struct LivenessInfo {
    /// 块入口的活跃变量
    pub live_in: BTreeSet<Reg>,
    /// 块出口的活跃变量
    pub live_out: BTreeSet<Reg>,
}

impl LivenessInfo {
    pub fn new() -> Self {
        Self::default()
    }

    /// 获取整数寄存器的活跃变量
    pub fn live_in_x(&self) -> BTreeSet<VXId> {
        self.live_in
            .iter()
            .filter_map(|r| match r {
                Reg::VX(id) => Some(*id),
                _ => None,
            })
            .collect()
    }

    /// 获取浮点寄存器的活跃变量
    pub fn live_in_f(&self) -> BTreeSet<VFId> {
        self.live_in
            .iter()
            .filter_map(|r| match r {
                Reg::VF(id) => Some(*id),
                _ => None,
            })
            .collect()
    }
}

// ===================================================================
// Next-use Distance 信息
// ===================================================================

/// Next-use distance 值
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum NextUseDistance {
    /// 立即使用（距离为0）
    Immediate,
    /// 在n条指令后使用
    Distance(u32),
    /// 不再使用（无穷大）
    Infinite,
}

impl NextUseDistance {
    /// 饱和加法
    pub fn saturating_add(self, n: u32) -> Self {
        match self {
            NextUseDistance::Immediate => NextUseDistance::Distance(n),
            NextUseDistance::Distance(d) => {
                if let Some(new_d) = d.checked_add(n) {
                    NextUseDistance::Distance(new_d)
                } else {
                    NextUseDistance::Infinite
                }
            }
            NextUseDistance::Infinite => NextUseDistance::Infinite,
        }
    }

    /// 递增
    pub fn increment(self) -> Self {
        self.saturating_add(1)
    }

    /// 取最小值
    pub fn min(self, other: Self) -> Self {
        match (self, other) {
            (NextUseDistance::Infinite, x) | (x, NextUseDistance::Infinite) => x,
            (NextUseDistance::Immediate, _) | (_, NextUseDistance::Immediate) => {
                NextUseDistance::Immediate
            }
            (NextUseDistance::Distance(a), NextUseDistance::Distance(b)) => {
                NextUseDistance::Distance(a.min(b))
            }
        }
    }

    /// 是否有限
    pub fn is_finite(&self) -> bool {
        !matches!(self, NextUseDistance::Infinite)
    }
}

/// 块级别的 Next-use 信息
#[derive(Debug, Clone, Default, PartialEq)]
pub struct NextUseInfo {
    /// 块入口处每个变量的下次使用距离
    pub entry_next_use: BTreeMap<Reg, NextUseDistance>,
    /// 块出口处每个变量的下次使用距离
    pub exit_next_use: BTreeMap<Reg, NextUseDistance>,
}

// ===================================================================
// 寄存器压力信息
// ===================================================================

/// 寄存器压力信息
#[derive(Debug, Clone, Default)]
pub struct RegisterPressure {
    /// 整数寄存器最大压力
    pub max_x_pressure: usize,
    /// 浮点寄存器最大压力
    pub max_f_pressure: usize,
    /// 每条指令处的压力
    pub inst_pressure: Vec<(usize, usize)>, // (x_pressure, f_pressure)
}

// ===================================================================
// 完整的活跃变量分析结果
// ===================================================================

/// 活跃变量分析结果
#[derive(Debug, Clone)]
pub struct LivenessAnalysis {
    /// 基本的活跃变量信息
    pub liveness: BTreeMap<VirtBlockId, LivenessInfo>,
    /// Next-use distance 信息
    pub next_use: BTreeMap<VirtBlockId, NextUseInfo>,
    /// 寄存器压力信息
    pub pressure: BTreeMap<VirtBlockId, RegisterPressure>,
}

impl Default for LivenessAnalysis {
    fn default() -> Self {
        Self::new()
    }
}

impl LivenessAnalysis {
    pub fn new() -> Self {
        Self {
            liveness: BTreeMap::new(),
            next_use: BTreeMap::new(),
            pressure: BTreeMap::new(),
        }
    }

    /// 获取或创建块的活跃变量信息
    pub fn ensure_liveness(&mut self, block: VirtBlockId) -> &mut LivenessInfo {
        self.liveness.entry(block).or_default()
    }

    /// 获取或创建块的 next-use 信息
    pub fn ensure_next_use(&mut self, block: VirtBlockId) -> &mut NextUseInfo {
        self.next_use.entry(block).or_default()
    }

    /// 获取或创建块的压力信息
    pub fn ensure_pressure(&mut self, block: VirtBlockId) -> &mut RegisterPressure {
        self.pressure.entry(block).or_default()
    }
}

// ===================================================================
// 活跃变量分析算法
// ===================================================================

/// 活跃变量分析器
pub struct LivenessAnalyzer<'a> {
    func: &'a VirtFunc,
    analysis: LivenessAnalysis,
    /// 是否包含 caller-saved 寄存器（用于 Call 指令）
    include_caller_saved: bool,
}

impl<'a> LivenessAnalyzer<'a> {
    pub fn new(func: &'a VirtFunc) -> Self {
        Self {
            func,
            analysis: LivenessAnalysis::new(),
            include_caller_saved: false,
        }
    }

    /// 设置是否包含 caller-saved 寄存器
    pub fn with_caller_saved(mut self, include: bool) -> Self {
        self.include_caller_saved = include;
        self
    }

    /// 执行活跃变量分析
    pub fn analyze(mut self) -> LivenessAnalysis {
        // 1. 计算基本的活跃变量信息
        self.compute_liveness();

        // 2. 计算 next-use distance
        self.compute_next_use_distance();

        // 3. 计算寄存器压力
        self.compute_register_pressure();

        self.analysis
    }

    /// 计算基本的活跃变量信息（数据流分析）
    fn compute_liveness(&mut self) {
        // PRO => PO
        let po: Vec<_> = self.func.cfg.block_order().iter().rev().copied().collect();

        let mut changed = true;
        while changed {
            changed = false;

            for &block_id in &po {
                let old_live_in = self
                    .analysis
                    .liveness
                    .get(&block_id)
                    .map(|l| l.live_in.clone())
                    .unwrap_or_default();

                // 计算 live_out = ∪ succ.live_in
                let mut live_out = BTreeSet::new();
                if let Some(node) = self.func.cfg.node(block_id) {
                    for &succ in &node.successors {
                        if let Some(succ_info) = self.analysis.liveness.get(&succ) {
                            live_out.extend(&succ_info.live_in);
                        }
                    }
                }

                // 计算 live_in = use ∪ (live_out - def)
                let mut live_in = live_out.clone();

                // 处理块中的指令（逆序）
                if let Some(block) = self.func.block_datas.get(&block_id) {
                    // 处理所有指令（逆序）
                    for inst in block.insts.iter().rev() {
                        let (uses, defs) = self.get_inst_use_def(inst);
                        for def in defs {
                            live_in.remove(&def);
                        }
                        live_in.extend(uses);
                    }
                }

                // 更新活跃变量信息
                let liveness = self.analysis.ensure_liveness(block_id);
                liveness.live_in = live_in.clone();
                liveness.live_out = live_out;

                // 检查是否有变化
                if old_live_in != live_in {
                    changed = true;
                }
            }
        }
    }

    /// 计算 Next-use Distance（使用迭代不动点算法）
    fn compute_next_use_distance(&mut self) {
        // 获取块顺序（RPO），然后反转得到后序
        let po: Vec<_> = self.func.cfg.block_order().iter().rev().copied().collect();

        // 初始化所有块的 next-use 信息为空（隐式无穷大）
        for &block_id in &po {
            self.analysis.ensure_next_use(block_id);
        }

        let mut changed = true;
        while changed {
            changed = false;

            for &block_id in &po {
                // 计算 exit_next_use：从所有后继块合并
                let mut exit_next_use = BTreeMap::new();

                if let Some(node) = self.func.cfg.node(block_id) {
                    for &succ in &node.successors {
                        if let Some(succ_next_use) = self.analysis.next_use.get(&succ) {
                            for (reg, dist) in &succ_next_use.entry_next_use {
                                let new_dist = dist.increment(); // 跨块边界增加1
                                exit_next_use
                                    .entry(*reg)
                                    .and_modify(|d: &mut NextUseDistance| *d = (*d).min(new_dist))
                                    .or_insert(new_dist);
                            }
                        }
                    }
                }

                // 计算 entry_next_use（逆序遍历指令）
                let mut entry_next_use = exit_next_use.clone();

                if let Some(block) = self.func.block_datas.get(&block_id) {
                    // 处理所有指令（逆序）
                    for inst in block.insts.iter().rev() {
                        // 只有在处理使用/定义之前增加距离
                        // 但跳过纯控制流指令（无条件跳转）和伪指令的距离增加
                        if !matches!(inst, MInst::J { .. } | MInst::Comment { .. }) {
                            // 真实指令，所有距离增加1
                            for dist in entry_next_use.values_mut() {
                                *dist = dist.increment();
                            }
                        }
                        self.update_next_use_for_inst(inst, &mut entry_next_use);
                    }
                }

                // 检查是否有变化
                let next_use = self.analysis.ensure_next_use(block_id);

                // 检查 exit 和 entry 是否都有变化
                let exit_changed = next_use.exit_next_use != exit_next_use;
                let entry_changed = next_use.entry_next_use != entry_next_use;

                if exit_changed || entry_changed {
                    changed = true;
                }

                // 无条件更新两者
                next_use.exit_next_use = exit_next_use;
                next_use.entry_next_use = entry_next_use;
            }
        }
    }

    /// 更新单条指令的 next-use
    fn update_next_use_for_inst(
        &self,
        inst: &MInst,
        next_use: &mut BTreeMap<Reg, NextUseDistance>,
    ) {
        let (uses, defs) = self.get_inst_use_def(inst);

        // 定义的寄存器不再使用
        for def in defs {
            next_use.remove(&def);
        }

        // 使用的寄存器立即使用
        for use_reg in uses {
            next_use.insert(use_reg, NextUseDistance::Immediate);
        }
    }

    /// 计算寄存器压力
    fn compute_register_pressure(&mut self) {
        for (block_id, liveness) in self.analysis.liveness.clone() {
            let mut pressure = RegisterPressure::default();
            let mut current_live = liveness.live_out.clone();

            // 计算当前活跃集合的压力
            let (x_count, f_count) = self.count_reg_types(&current_live);
            pressure.max_x_pressure = x_count;
            pressure.max_f_pressure = f_count;

            if let Some(block) = self.func.block_datas.get(&block_id) {
                let mut inst_pressures = Vec::new();

                // 处理每条指令
                for (idx, inst) in block.insts.iter().rev().enumerate() {
                    // 先获取 uses/defs
                    let (uses, defs) = self.get_inst_use_def(inst);

                    // 更新活跃集合
                    for def in defs {
                        current_live.remove(&def);
                    }
                    current_live.extend(uses);

                    // 计算压力（所有指令都要记录，包括伪指令）
                    let (x_count, f_count) = self.count_reg_types(&current_live);
                    pressure.max_x_pressure = pressure.max_x_pressure.max(x_count);
                    pressure.max_f_pressure = pressure.max_f_pressure.max(f_count);
                    inst_pressures.push((x_count, f_count));
                }

                inst_pressures.reverse(); // 恢复正序

                pressure.inst_pressure = inst_pressures;
            }

            self.analysis.pressure.insert(block_id, pressure);
        }
    }

    /// 获取指令的 use 和 def 集合
    fn get_inst_use_def(&self, inst: &MInst) -> (Vec<Reg>, Vec<Reg>) {
        let mut collector = RegCollector::new();

        // 特殊处理 Spill/Reload 指令，根据 SlotKind 判断
        match inst {
            MInst::Spill { reg, slot, kind } => {
                // 检查槽的类型
                use SlotKindDisc::*;
                match kind {
                    OutgoingArg => {
                        // ===================================================================
                        // Spill到outgoing_args的特殊处理
                        // ===================================================================
                        //
                        // 设计决策：
                        // Spill到outgoing_args是为函数调用准备参数，是临时存储
                        // 变量在调用后可能仍然需要使用，所以：
                        // 1. 标记为use（变量值被读取）
                        // 2. 但不从活跃集移除（变量仍然活跃）
                        //
                        // 这与Spill到spill_slots的行为不同：
                        // - spill_slots是持久化存储，变量不再活跃
                        // - outgoing_args是临时存储，变量仍然活跃
                        //
                        // 这个设计支持"双重溢出"：
                        // - 第一次：Spill到outgoing_args -- 为函数调用
                        // - 第二次：Spill到spill_slots -- 为降低压力
                        //
                        // 示例场景（exgcd函数）：
                        // Spill VX4 -> outgoing_slot  # 第一次：为CallPseudo准备
                        // Spill VX4 -> spill_slot     # 第二次：MIN算法为降低压力
                        // CallPseudo ... StackArg{VX4}
                        // Reload spill_slot -> VX4    # 后续使用时reload
                        // ===================================================================
                        collector.uses.push(*reg);
                    }
                    Spill | IncomingArg => {
                        // Spill 到持久化栈位置
                        // Spill指令只是读取寄存器值并存储到内存，是一个use操作
                        // 寄存器生命周期在此结束不是因为被def，而是因为这是最后一个use
                        // 活跃变量分析会自然地处理这个情况
                        collector.uses.push(*reg);
                        // 不应该def！Spill不修改寄存器的值
                    }
                    FixedObject | CalleeSaved => {
                        panic!("不应该 Spill 到 {} 类型的槽", kind);
                    }
                    ExtendTailArg => {
                        // ===================================================================
                        // Spill到ExtendTailArg的处理
                        // ===================================================================
                        //
                        // 尾调用的特殊性：
                        // 1. 复用当前函数的栈帧
                        // 2. 可以使用incoming_args + extend_tail_args作为调用参数
                        // 3. 与OutgoingArg类似，这是为函数调用准备的临时存储
                        //
                        // 处理方式与OutgoingArg相同：
                        // - 标记为use（读取变量值）
                        // - 变量仍然活跃（不从活跃集移除）
                        // ===================================================================
                        collector.uses.push(*reg);
                    }
                    ReturnAddress | FpAddress => {
                        panic!("不可能 Spill 到 {} 类型的槽", kind);
                    }
                }
            }
            MInst::Reload { reg, slot, kind } => {
                // Reload 总是定义寄存器
                // 但也要检查是从哪种槽加载
                use SlotKindDisc::*;
                match kind {
                    OutgoingArg => {
                        panic!("不应该从 OutgoingArg 槽进行 Reload");
                    }
                    Spill | IncomingArg => {
                        // 从持久化栈位置 Reload，正常
                        collector.defs.push(*reg);
                    }
                    FixedObject => {
                        // 从固定对象 Reload 也是合理的
                        collector.defs.push(*reg);
                    }
                    CalleeSaved => {
                        panic!("不应该从 CalleeSaved 槽进行 Reload");
                    }
                    ExtendTailArg | ReturnAddress | FpAddress => {
                        panic!("不可能从 {} 类型的槽进行 Reload", kind);
                    }
                }
            }
            _ => {
                // 其他指令，使用默认的 visit_regs
                inst.visit_regs(&mut collector);
            }
        }

        // 特殊处理 Call 指令： 如果包含 caller-saved 寄存器
        if self.include_caller_saved && inst.is_call() {
            for xreg in CALLER_SAVED_XREGS.iter() {
                collector.defs.push(xreg.to_reg());
            }
            for freg in CALLER_SAVED_FREGS.iter() {
                collector.defs.push(freg.to_reg());
            }
        }

        (collector.uses, collector.defs)
    }

    /// 统计寄存器类型数量
    fn count_reg_types(&self, regs: &BTreeSet<Reg>) -> (usize, usize) {
        let mut x_count = 0;
        let mut f_count = 0;

        for reg in regs {
            match reg {
                Reg::VX(_) | Reg::X(_) => x_count += 1,
                Reg::VF(_) | Reg::F(_) => f_count += 1,
            }
        }

        (x_count, f_count)
    }
}

// ===================================================================
// 公共接口
// ===================================================================

/// 执行活跃变量分析
pub fn analyze_liveness(func: &VirtFunc, include_caller_saved: bool) -> LivenessAnalysis {
    LivenessAnalyzer::new(func)
        .with_caller_saved(include_caller_saved)
        .analyze()
}
