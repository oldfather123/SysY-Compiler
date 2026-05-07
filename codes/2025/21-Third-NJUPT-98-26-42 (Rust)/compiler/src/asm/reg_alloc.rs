//! 寄存器分配
//!
//! 使用两阶段构建

use super::{inst::*, liveness::*, prog::*, reg::*, stack::*};
use crate::{asm::loop_analysis::AsmLoopAnalysis, prelude::*, ssa::ctx::*};
use std::collections::{BTreeMap, BTreeSet};

/// 执行寄存器分配
pub fn allocate_registers(
    cctx: &CoreContext,
    fctx: &FuncContext,
    vprog: &mut VirtProg,
    vfunc: VirtFunc,
    debug_opts: &crate::RegAllocDebugOptions,
) -> CEResult<(VirtFunc, bool)> {
    // 选择不同的寄存器分配算法:
    let env = RegAllocEnv::new(cctx, fctx, vprog, vfunc, debug_opts);
    env.allocate()
}

pub struct RegAllocEnv<'a> {
    // -------- 核心结构 --------
    pub cctx: &'a CoreContext,
    pub fctx: &'a FuncContext,
    pub vprog: &'a mut VirtProg,
    pub vfunc: VirtFunc,

    // -------- 分析字段 --------
    /// 活跃变量分析结果
    pub liveness: Option<LivenessAnalysis>,
    pub loop_analysis: Option<AsmLoopAnalysis>,

    /// 下一个可用的spill槽ID
    pub next_spill_slot_id: usize,

    // -------- 其他字段-------
    /// 是否插入调试注释
    pub debug_comments: bool,
    pub debug_irc: bool,

    pub run_spill: bool,
    pub run_irc: bool,
    pub run_finalize: bool, // 是否继续后处理
}

impl<'a> RegAllocEnv<'a> {
    pub fn new(
        cctx: &'a CoreContext,
        fctx: &'a FuncContext,
        vprog: &'a mut VirtProg,
        vfunc: VirtFunc,
        debug_opts: &crate::RegAllocDebugOptions,
    ) -> Self {
        // 计算下一个可用的spill槽ID（在现有槽之后）
        let next_spill_slot_id = vfunc.stack.slot_count();

        // 从CLI参数获取调试选项
        let debug_comments = debug_opts.debug_comments;
        let run_spill = !debug_opts.skip_spill;
        let run_irc = !debug_opts.skip_irc;
        let debug_irc = debug_opts.debug_irc;
        let run_finalize = !debug_opts.skip_finalize && run_spill && run_irc;

        Self {
            cctx,
            fctx,
            vprog,
            vfunc,
            liveness: None,
            loop_analysis: None,
            debug_comments,
            next_spill_slot_id,
            run_spill,
            run_irc,
            debug_irc,
            run_finalize,
        }
    }

    /// 设置是否生成调试注释
    pub fn with_debug_comments(mut self, enable: bool) -> Self {
        self.debug_comments = enable;
        self
    }

    pub fn allocate(mut self) -> CEResult<(VirtFunc, bool)> {
        // 1. 计算第一遍活跃变量分析
        let liveness = analyze_liveness(&self.vfunc, true); // <= reg_spill 前不处理ABI约束
        self.liveness = Some(liveness.clone());
        if self.debug_comments && !self.run_spill && !self.run_irc {
            self.insert_liveness_comments(); // 如果启用调试注释，插入活跃变量分析结果
        }

        // 2. 执行ASM循环分析（基于SSA循环分析和活跃变量分析）
        let loop_analysis = crate::asm::loop_analysis::AsmLoopAnalysis::build(
            &self.fctx.loop_analysis,
            &liveness,
            &self.vfunc,
        );
        if self.debug_comments && !self.run_spill && !self.run_irc {
            eprintln!("end ASM Loop Analysis==============");
            loop_analysis.dump(); // 如果启用调试注释，输出循环分析结果
        }
        self.loop_analysis = Some(loop_analysis);

        // 3. 执行预先解耦合溢出
        if !self.run_spill {
            eprintln!("跳过预先解耦合溢出 ... ");
            return Ok((self.vfunc, self.run_finalize));
        }
        crate::asm::reg_spill::run_with_comments(&mut self, true);

        // 4. 计算第二遍Liveness（溢出后， 仅仅用于校验压力，因此 include_caller_saved=false）
        let liveness = analyze_liveness(&self.vfunc, true); // <= reg_spill 的校验无需ABI约束
        self.liveness = Some(liveness.clone());
        if self.debug_comments && self.run_spill && !self.run_irc {
            self.insert_liveness_comments(); // 如果启用调试注释，插入活跃变量分析结果
        }

        // DEBUG: 校验最大压力是否已降低到阈值以下
        // ===================================================================
        // 寄存器压力校验的设计说明
        // ===================================================================
        //
        // 两阶段寄存器分配架构：
        // 1. MIN算法（reg_spill）：降低寄存器压力到k以下
        // 2. IRC算法（reg_irc）：在k压力约束下进行着色
        //
        // 关键设计决策：
        // - IRC不再进行spill，所有spill决策由MIN完成
        // - MIN必须保证压力≤k，否则IRC会失败
        // - 这里的校验确保MIN成功完成了任务
        //
        // 常见问题案例（exgcd函数）：
        // - CallPseudo后续会产生新的临时变量
        // - MIN算法需要前瞻性，在CallPseudo时预留空间
        // - 解决方案：CallPseudo时限制到k-persistent_defs-2
        //
        // 双重溢出设计：
        // - 第一次：Spill到outgoing_args（为函数调用）
        // - 第二次：Spill到spill_slots（为降低压力）
        // - 这是完全合理且必要的设计
        // ===================================================================
        self.verify_register_pressure(&liveness);

        // 5. 计算第三遍Liveness（图着色前，include_caller_saved=true）
        if !self.run_irc {
            return Ok((self.vfunc, self.run_finalize));
        }
        let liveness = analyze_liveness(&self.vfunc, true); // <= reg_spill 后需要处理ABI约束
        self.liveness = Some(liveness.clone());

        // 6. 执行IRC图着色
        match self.run_irc() {
            Ok(()) => {
                // IRC成功，继续正常流程
                Ok((self.vfunc, self.run_finalize))
            }
            Err(e) => {
                // IRC失败，但仍然返回虚拟函数以便输出
                eprintln!("\n----------------------------------------");
                eprintln!("⚠️IRC图着色失败: {}", e);
                eprintln!("将输出Spill后的虚拟汇编到.S文件");
                eprintln!("----------------------------------------\n");
                eprintln!("提示：请查看 TEMP/func_irc_*.dot 文件获取干扰图可视化");

                // 返回虚拟函数和一个特殊标志表示IRC失败
                // 使用负值表示IRC失败（-1表示失败但仍要输出）
                Ok((self.vfunc, false))
            }
        }
    }
}

// ===================================================================
// 辅助方法
// ===================================================================
impl RegAllocEnv<'_> {
    /// 获取或创建虚拟寄存器的溢出槽位
    /// 保证每个虚拟寄存器在整个生命周期中只对应一个唯一的栈槽
    pub fn get_or_create_slot_for_vreg(&mut self, reg: Reg) -> (SlotId, SlotKindDisc) {
        // 如果已经有映射，直接返回
        if let Some(&slot_id) = self.vfunc.pst_slot_map.get(&reg) {
            let slot_kind = self.vfunc.stack.get_disc(slot_id);
            return (slot_id, slot_kind);
        }

        // 如果没有映射，创建新槽位
        eprintln!("INFO: Creating new slot for {}", reg);

        let slot_kind = SlotKind::Spill { vreg: reg };
        let slot_data = SlotData {
            id: SlotId::new(0),
            kind: slot_kind,
            size: 8,
            align: 3,
            fp_offset: None,
            sp_offset: None,
        };

        let slot_id = self.vfunc.stack.add_slot(slot_data);
        self.vfunc.stack.add_spill_slot(slot_id);

        // 关键：立即将映射插入 pst_slot_map，确保后续调用返回同一个槽位
        self.vfunc.pst_slot_map.insert(reg, slot_id);

        (slot_id, slot_kind.into())
    }

    // 保留旧函数名作为别名，向后兼容
    #[deprecated(note = "Use get_or_create_slot_for_vreg instead")]
    pub fn make_slot_id(&mut self, reg: Reg) -> (SlotId, SlotKindDisc) {
        self.get_or_create_slot_for_vreg(reg)
    }

    /// 为寄存器分配或获取溢出槽（向后兼容）
    #[deprecated(note = "Use get_or_create_slot_for_vreg instead")]
    pub fn get_slot_id(&mut self, reg: Reg) -> (SlotId, SlotKindDisc) {
        self.get_or_create_slot_for_vreg(reg)
    }
}

// ===================================================================
// 校验寄存器压力
// ===================================================================
impl RegAllocEnv<'_> {
    /// 校验寄存器压力是否符合预期
    ///
    /// MIN算法设计理念：
    /// - Spill/Reload指令本身就是为了降低压力而插入的
    /// - 允许在Spill/Reload指令处临时超过压力阈值
    /// - 这是正常现象，因为这些指令会立即调整W集合
    ///
    /// Call-Aware Spilling 额外约束：
    /// - 在CallPseudo/RetCallPseudo处，跨周期变量数不能超过callee-saved寄存器数
    /// - 这确保了IRC阶段一定能成功着色（k-colorable）
    fn verify_register_pressure(&self, liveness: &LivenessAnalysis) {
        use super::reg::{
            CALLEE_SAVED_FREGS_LEN, CALLEE_SAVED_XREGS_LEN, FREGS_MAX_PRESSURE, XREGS_MAX_PRESSURE,
        };
        use std::collections::BTreeSet;

        let mut violations = Vec::new();
        let mut spill_warnings = Vec::new(); // 单独记录Spill/Reload的临时超压
        let mut call_violations = Vec::new(); // Call-Aware Spilling违规

        // 遍历所有块的压力信息
        for (block_id, pressure_info) in &liveness.pressure {
            // 检查最大压力 - 但不立即判定为violation
            // 需要检查具体是哪条指令导致的超标
            if pressure_info.max_x_pressure > XREGS_MAX_PRESSURE {
                // 暂时不设置has_violation = true
                // 只有在发现非Spill/Reload指令超压时才设置

                // 调试：打印压力数组和指令数组的长度对比
                if self.debug_comments {
                    if let Some(block) = self.vfunc.block_datas.get(block_id) {
                        eprintln!("[DEBUG] Block {:?}:", block_id);
                        eprintln!(
                            "  - inst_pressure长度: {}",
                            pressure_info.inst_pressure.len()
                        );
                        eprintln!("  - block.insts长度: {}", block.insts.len());
                        eprintln!("  - 压力数组应该等于指令数组长度");
                    }
                }

                // 找出具体哪条指令导致超标
                // 注意：压力数组是在插入Comment之前计算的，所以需要建立映射
                for (pressure_idx, &(x_pressure, _)) in
                    pressure_info.inst_pressure.iter().enumerate()
                {
                    if x_pressure > XREGS_MAX_PRESSURE {
                        // 获取该指令信息
                        if let Some(block) = self.vfunc.block_datas.get(block_id) {
                            // 建立从压力索引到当前指令索引的映射
                            // 跳过所有Comment指令
                            let mut non_comment_count = 0;
                            let mut actual_inst_idx = None;

                            for (idx, inst) in block.insts.iter().enumerate() {
                                if !matches!(inst, MInst::Comment { .. }) {
                                    if non_comment_count == pressure_idx {
                                        actual_inst_idx = Some(idx);
                                        break;
                                    }
                                    non_comment_count += 1;
                                }
                            }

                            if let Some(inst_idx) = actual_inst_idx {
                                let inst = &block.insts[inst_idx];

                                // 检查是否是Spill/Reload指令
                                let is_spill_reload =
                                    matches!(inst, MInst::Spill { .. } | MInst::Reload { .. });

                                if is_spill_reload {
                                    // 对于Spill/Reload，只记录为警告，不算真正的违规
                                    // 因为MIN算法允许在这些指令处临时超压
                                    spill_warnings.push(format!(
                                        "  [MIN允许] 指令[{}]: {:?} -> X压力={} (临时超压，正常现象)",
                                        inst_idx, inst, x_pressure
                                    ));
                                } else {
                                    // 其他指令的超压是真正的违规
                                    violations.push(format!(
                                        "  指令[{}]: {:?} -> X压力={}",
                                        inst_idx, inst, x_pressure
                                    ));
                                }

                                // Call-Aware Spilling检查：在CallPseudo/RetCallPseudo处验证跨周期变量约束
                                if matches!(
                                    inst,
                                    MInst::CallPseudo { .. } | MInst::RetCallPseudo { .. }
                                ) {
                                    // 获取当前的活跃寄存器集合
                                    if let Some(liveness_info) = liveness.liveness.get(block_id) {
                                        // 提取ABI绑定的寄存器
                                        let mut abi_bound_x = BTreeSet::new();

                                        match inst {
                                            MInst::CallPseudo { args, ret, .. } => {
                                                // 收集参数寄存器
                                                for arg in args {
                                                    if let super::inst::ArgPair::RegArg {
                                                        vreg,
                                                        ..
                                                    } = arg
                                                    {
                                                        if matches!(vreg, Reg::VX(_) | Reg::X(_)) {
                                                            abi_bound_x.insert(vreg.clone());
                                                        }
                                                    }
                                                }
                                                // 收集返回值寄存器
                                                if let super::inst::ArgPair::RegArg {
                                                    vreg, ..
                                                } = ret
                                                {
                                                    if matches!(vreg, Reg::VX(_) | Reg::X(_)) {
                                                        abi_bound_x.insert(vreg.clone());
                                                    }
                                                }
                                            }
                                            MInst::RetCallPseudo { args, .. } => {
                                                // 只有参数，没有返回值
                                                for arg in args {
                                                    if let super::inst::ArgPair::RegArg {
                                                        vreg,
                                                        ..
                                                    } = arg
                                                    {
                                                        if matches!(vreg, Reg::VX(_) | Reg::X(_)) {
                                                            abi_bound_x.insert(vreg.clone());
                                                        }
                                                    }
                                                }
                                            }
                                            _ => {}
                                        }

                                        // 计算活跃的X寄存器
                                        let live_x: BTreeSet<Reg> = liveness_info
                                            .live_out
                                            .iter()
                                            .filter(|r| matches!(r, Reg::VX(_) | Reg::X(_)))
                                            .cloned()
                                            .collect();

                                        // 计算跨周期变量（活跃但非ABI绑定）
                                        let live_through_x: BTreeSet<_> =
                                            live_x.difference(&abi_bound_x).collect();

                                        // 检查跨周期变量数是否超过callee-saved寄存器数
                                        if live_through_x.len() > CALLEE_SAVED_XREGS_LEN {
                                            call_violations.push(format!(
                                                "  [Call-Aware违规] 块{:?} 指令[{}]: {:?}\n    跨周期X变量: {} > callee-saved: {}\n    跨周期变量: {:?}",
                                                block_id, inst_idx, inst,
                                                live_through_x.len(), CALLEE_SAVED_XREGS_LEN,
                                                live_through_x
                                            ));
                                        }
                                    }
                                }
                            } else {
                                violations.push(format!(
                                    "  压力索引[{}]无法映射到非Comment指令 -> X压力={}",
                                    pressure_idx, x_pressure
                                ));
                            }

                            // 如果有活跃变量信息，显示当时的活跃集合
                            if pressure_idx == 0 {
                                if let Some(liveness_info) = liveness.liveness.get(block_id) {
                                    let x_regs: Vec<_> = liveness_info
                                        .live_in
                                        .iter()
                                        .filter(|r| matches!(r, Reg::VX(_) | Reg::X(_)))
                                        .collect();
                                    if !x_regs.is_empty() {
                                        violations
                                            .push(format!("    入口活跃X寄存器: {:?}", x_regs));
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if pressure_info.max_f_pressure > FREGS_MAX_PRESSURE {
                // 暂时不设置has_violation = true
                // 只有在发现非Spill/Reload指令超压时才设置

                // 找出具体哪条指令导致超标
                for (pressure_idx, &(_, f_pressure)) in
                    pressure_info.inst_pressure.iter().enumerate()
                {
                    if f_pressure > FREGS_MAX_PRESSURE {
                        // 获取该指令信息
                        if let Some(block) = self.vfunc.block_datas.get(block_id) {
                            // 建立从压力索引到当前指令索引的映射
                            // 跳过所有Comment指令
                            let mut non_comment_count = 0;
                            let mut actual_inst_idx = None;

                            for (idx, inst) in block.insts.iter().enumerate() {
                                if !matches!(inst, MInst::Comment { .. }) {
                                    if non_comment_count == pressure_idx {
                                        actual_inst_idx = Some(idx);
                                        break;
                                    }
                                    non_comment_count += 1;
                                }
                            }

                            if let Some(inst_idx) = actual_inst_idx {
                                let inst = &block.insts[inst_idx];

                                // 检查是否是Spill/Reload指令
                                let is_spill_reload =
                                    matches!(inst, MInst::Spill { .. } | MInst::Reload { .. });

                                if is_spill_reload {
                                    // 对于Spill/Reload，只记录为警告，不算真正的违规
                                    spill_warnings.push(format!(
                                        "  [MIN允许] 指令[{}]: {:?} -> F压力={} (临时超压，正常现象)",
                                        inst_idx, inst, f_pressure
                                    ));
                                } else {
                                    // 其他指令的超压是真正的违规
                                    violations.push(format!(
                                        "  指令[{}]: {:?} -> F压力={}",
                                        inst_idx, inst, f_pressure
                                    ));
                                }

                                // Call-Aware Spilling检查：在CallPseudo/RetCallPseudo处验证跨周期变量约束
                                if matches!(
                                    inst,
                                    MInst::CallPseudo { .. } | MInst::RetCallPseudo { .. }
                                ) {
                                    // 获取当前的活跃寄存器集合
                                    if let Some(liveness_info) = liveness.liveness.get(block_id) {
                                        // 提取ABI绑定的浮点寄存器
                                        let mut abi_bound_f = BTreeSet::new();

                                        match inst {
                                            MInst::CallPseudo { args, ret, .. } => {
                                                // 收集参数寄存器
                                                for arg in args {
                                                    if let super::inst::ArgPair::RegArg {
                                                        vreg,
                                                        ..
                                                    } = arg
                                                    {
                                                        if matches!(vreg, Reg::VF(_) | Reg::F(_)) {
                                                            abi_bound_f.insert(vreg.clone());
                                                        }
                                                    }
                                                }
                                                // 收集返回值寄存器
                                                if let super::inst::ArgPair::RegArg {
                                                    vreg, ..
                                                } = ret
                                                {
                                                    if matches!(vreg, Reg::VF(_) | Reg::F(_)) {
                                                        abi_bound_f.insert(vreg.clone());
                                                    }
                                                }
                                            }
                                            MInst::RetCallPseudo { args, .. } => {
                                                // 只有参数，没有返回值
                                                for arg in args {
                                                    if let super::inst::ArgPair::RegArg {
                                                        vreg,
                                                        ..
                                                    } = arg
                                                    {
                                                        if matches!(vreg, Reg::VF(_) | Reg::F(_)) {
                                                            abi_bound_f.insert(vreg.clone());
                                                        }
                                                    }
                                                }
                                            }
                                            _ => {}
                                        }

                                        // 计算活跃的F寄存器
                                        let live_f: BTreeSet<Reg> = liveness_info
                                            .live_out
                                            .iter()
                                            .filter(|r| matches!(r, Reg::VF(_) | Reg::F(_)))
                                            .cloned()
                                            .collect();

                                        // 计算跨周期变量（活跃但非ABI绑定）
                                        let live_through_f: BTreeSet<_> =
                                            live_f.difference(&abi_bound_f).collect();

                                        // 检查跨周期变量数是否超过callee-saved寄存器数
                                        if live_through_f.len() > CALLEE_SAVED_FREGS_LEN {
                                            call_violations.push(format!(
                                                "  [Call-Aware违规] 块{:?} 指令[{}]: {:?}\n    跨周期F变量: {} > callee-saved: {}\n    跨周期变量: {:?}",
                                                block_id, inst_idx, inst,
                                                live_through_f.len(), CALLEE_SAVED_FREGS_LEN,
                                                live_through_f
                                            ));
                                        }
                                    }
                                }

                                // 如果有活跃变量信息，显示当时的活跃集合
                                if let Some(liveness_info) = liveness.liveness.get(block_id) {
                                    let f_regs: Vec<_> = liveness_info
                                        .live_in
                                        .iter()
                                        .filter(|r| matches!(r, Reg::VF(_) | Reg::F(_)))
                                        .collect();
                                    if inst_idx == 0 && !f_regs.is_empty() {
                                        violations
                                            .push(format!("    入口活跃F寄存器: {:?}", f_regs));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 输出警告信息
        // 检查所有类型的违规
        let has_pressure_violation = !violations.is_empty();
        let has_call_aware_violation = !call_violations.is_empty();
        let has_real_violation = has_pressure_violation || has_call_aware_violation;

        if has_real_violation {
            eprintln!("\n========== 寄存器压力校验错误 ==========");
            eprintln!("函数: {:?}", self.vfunc.func_name);

            if has_pressure_violation {
                eprintln!(
                    "\n【压力违规】预期: X<={}, F<={}",
                    XREGS_MAX_PRESSURE, FREGS_MAX_PRESSURE
                );
                eprintln!("严重违规（非Spill/Reload指令超压）:");
                for violation in violations {
                    eprintln!("{}", violation);
                }
            }

            if has_call_aware_violation {
                eprintln!("\n【Call-Aware Spilling违规】");
                eprintln!(
                    "预期: 跨周期X变量<={}, 跨周期F变量<={}",
                    CALLEE_SAVED_XREGS_LEN, CALLEE_SAVED_FREGS_LEN
                );
                eprintln!("以下Call点的跨周期变量过多，无法保证k-colorability:");
                for violation in call_violations {
                    eprintln!("{}", violation);
                }
            }

            eprintln!("\n错误: MIN算法未能正确处理寄存器压力或Call-Aware约束");
            eprintln!("这将导致IRC阶段着色失败！");
        } else if !spill_warnings.is_empty() {
            // 只有Spill/Reload的临时超压，这是MIN算法允许的
            if self.debug_comments {
                eprintln!("\n========== MIN算法临时超压信息 ==========");
                eprintln!("函数: {:?}", self.vfunc.func_name);
                eprintln!("以下Spill/Reload指令处的临时超压是MIN算法设计允许的:");
                for warning in spill_warnings {
                    eprintln!("{}", warning);
                }
                eprintln!("\n✓ 这是正常现象，Spill/Reload会立即调整W集合");
            }
        }

        // 显示已插入的spill槽信息
        if !self.vfunc.pst_slot_map.is_empty() && self.debug_comments {
            eprintln!("\n已分配的Spill槽:");
            for (reg, slot) in &self.vfunc.pst_slot_map {
                eprintln!("  {} -> slot_{}", reg, slot.index());
            }
        }

        if !has_real_violation && self.debug_comments {
            // eprintln!("✓ 寄存器压力校验通过: 所有非Spill/Reload指令的压力均在阈值内");
        }
    }
}
// ===================================================================
// 调试打印
// ===================================================================
impl RegAllocEnv<'_> {
    /// 插入活跃变量分析的调试注释
    fn insert_liveness_comments(&mut self) {
        let liveness = match &self.liveness {
            Some(l) => l,
            None => return,
        };

        // 为每个基本块插入注释
        let block_ids: Vec<_> = self.vfunc.block_datas.keys().cloned().collect();
        for block_id in block_ids {
            let mut new_instructions = Vec::new();

            // 获取该块的活跃变量信息
            let block_liveness = liveness.liveness.get(&block_id);
            let block_next_use = liveness.next_use.get(&block_id);
            let block_pressure = liveness.pressure.get(&block_id);

            // 在块开始插入 live_in 注释
            if let Some(liveness_info) = block_liveness {
                new_instructions.push(MInst::Comment {
                    label: format!("====== Liveness {} ======", block_id),
                });

                // Live-in 信息
                let live_in_str = self.format_reg_set(&liveness_info.live_in);
                new_instructions.push(MInst::Comment {
                    label: format!("LIVE_IN: {}", live_in_str),
                });

                // Next-use 信息（入口处）
                if let Some(next_use_info) = block_next_use {
                    let next_use_str = self.format_next_use(&next_use_info.entry_next_use);
                    if !next_use_str.is_empty() {
                        new_instructions.push(MInst::Comment {
                            label: format!("NEXT_USE_IN: {}", next_use_str),
                        });
                    }
                }

                // 寄存器压力信息
                if let Some(pressure_info) = block_pressure {
                    new_instructions.push(MInst::Comment {
                        label: format!(
                            "PRESSURE: X={}, F={}",
                            pressure_info.max_x_pressure, pressure_info.max_f_pressure
                        ),
                    });
                }
            }

            // 复制原有指令，并在关键指令后插入注释
            if let Some(block) = self.vfunc.block_datas.get(&block_id) {
                for (inst_idx, inst) in block.insts.iter().enumerate() {
                    new_instructions.push(inst.clone());

                    // 在某些关键指令后插入活跃变量快照
                    if self.should_insert_snapshot(inst) {
                        if let Some(pressure_info) = block_pressure {
                            if inst_idx < pressure_info.inst_pressure.len() {
                                let (x_pressure, f_pressure) =
                                    pressure_info.inst_pressure[inst_idx];
                                new_instructions.push(MInst::Comment {
                                    label: format!(
                                        "  after: pressure(X={}, F={})",
                                        x_pressure, f_pressure
                                    ),
                                });
                            }
                        }
                    }
                }

                // 在块结束前插入 live_out 注释
                if let Some(liveness_info) = block_liveness {
                    let live_out_str = self.format_reg_set(&liveness_info.live_out);
                    new_instructions.push(MInst::Comment {
                        label: format!("LIVE_OUT: {}", live_out_str),
                    });

                    // Next-use 信息（出口处）
                    if let Some(next_use_info) = block_next_use {
                        let next_use_str = self.format_next_use(&next_use_info.exit_next_use);
                        if !next_use_str.is_empty() {
                            new_instructions.push(MInst::Comment {
                                label: format!("NEXT_USE_OUT: {}", next_use_str),
                            });
                        }
                    }
                }
            }

            // 更新块的指令
            if let Some(block) = self.vfunc.block_datas.get_mut(&block_id) {
                block.insts = new_instructions;
            }
        }
    }

    /// 判断是否应该在指令后插入活跃变量快照
    fn should_insert_snapshot(&self, inst: &MInst) -> bool {
        matches!(
            inst,
            MInst::Call { .. }
                | MInst::CallPseudo { .. }
                | MInst::Spill { .. }
                | MInst::Reload { .. }
        )
    }

    /// 格式化寄存器集合为字符串
    fn format_reg_set(&self, regs: &BTreeSet<Reg>) -> String {
        if regs.is_empty() {
            return "{}".to_string();
        }

        let mut x_regs = Vec::new();
        let mut f_regs = Vec::new();
        let mut vx_regs = Vec::new();
        let mut vf_regs = Vec::new();

        for reg in regs {
            match reg {
                Reg::X(..) => x_regs.push(format!("{}", reg)),
                Reg::F(..) => f_regs.push(format!("{}", reg)),
                Reg::VX(..) => vx_regs.push(format!("{}", reg)),
                Reg::VF(..) => vf_regs.push(format!("{}", reg)),
            }
        }

        let mut parts = Vec::new();
        if !x_regs.is_empty() {
            x_regs.sort();
            parts.push(format!("X:{{{}}}", x_regs.join(",")));
        }
        if !f_regs.is_empty() {
            f_regs.sort();
            parts.push(format!("F:{{{}}}", f_regs.join(",")));
        }
        if !vx_regs.is_empty() {
            vx_regs.sort();
            parts.push(format!("VX:{{{}}}", vx_regs.join(",")));
        }
        if !vf_regs.is_empty() {
            vf_regs.sort();
            parts.push(format!("VF:{{{}}}", vf_regs.join(",")));
        }

        parts.join(" ")
    }

    /// 格式化 next-use 信息为字符串
    fn format_next_use(&self, next_use: &BTreeMap<Reg, NextUseDistance>) -> String {
        if next_use.is_empty() {
            return String::new();
        }

        let mut items = Vec::new();
        for (reg, dist) in next_use {
            let dist_str = match dist {
                NextUseDistance::Immediate => "imm".to_string(),
                NextUseDistance::Distance(d) => d.to_string(),
                NextUseDistance::Infinite => "∞".to_string(),
            };

            let reg_str = format!("{}", reg);

            items.push(format!("{}:{}", reg_str, dist_str));
        }

        items.sort();
        format!("{{{}}}", items.join(", "))
    }
}
