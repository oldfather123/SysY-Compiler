//! # 预先解耦合Spill/Reload
//!
//! 算法基于如下论文进行实现:
//!
//! Register Spilling and Live-Range Splitting for SSA-Form Programs
//! <https://link.springer.com/content/pdf/10.1007/978-3-642-00722-4.pdf>
//!
//! ## 核心思想
//!
//! 传统的寄存器分配器如图着色/线性扫描, 通常将溢出代码的生成与寄存器指派耦合在一起, 当分配启发式算法失败时，
//! 它们才被迫选择一个变量进行溢出, 这种方式存在几个问题:
//! - 决策局部化：溢出决策往往是局部的，可能会选择在循环内部重载一个变量，导致每次循环都产生昂贵的内存访问
//! - 粗粒度溢出：在极端情况下，可能会将一个变量的整个“活跃范围” (live-range) 全部溢出，即在每次定义后都 store，每次使用前都 load，非常低效
//! - 缺乏结构感知：未能充分利用程序的控制流结构（尤其是循环）来做出更全局、更优的溢出决策
//!
//! 因此应该解耦合寄存器分配, 分为溢出+分配两阶段进行:
//! 1. 溢出阶段：首先通过插入 spill/reload 代码，将程序的“最大寄存器压力”, 即为最大同时活跃变量的数量, 降低到物理寄存器数量 k 以下
//! 2. 分配阶段：一旦压力小于等于 k，就可以使用一个简单的线性时间算法完成无冲突的寄存器分配，且此过程不会再引入新的溢出
//!
//! ## 具体算法
//!
//! - W (Working Set): 当前存放在寄存器中的变量的集合
//! - S (Spilled Set): 在栈上存在有效副本的变量集合（S独立于W，不要求S⊆W）
//! - R (Reload Set): 表示为了执行insn需要从内存中Reload回寄存器的变量的集合
//! - m: 表示寄存器数量的上限
//! - k: 物理寄存器的总数
//! - v: 代表一个程序变量
//! - nextUse(insn, v): 一个函数，计算从指令 insn 开始，变量 v 下一次被使用需要经过多少条指令
//! - insn.uses: 指令 insn 使用的变量集合
//! - insn.defs: 指令 insn 定义的变量集合
//!
//! ### 算法 1：MIN 算法
//!
//! Min 算法最初用于操作系统页面替换，其策略是：当必须淘汰一个页面（或寄存器中的变量）时，选择那个下一次使用距离最远的页面进行淘汰
//! 直接将 Min 算法应用于每个基本块是行不通的，因为块与块之间相互影响, 因此定义全局的“下次使用距离”:
//! 这个nextUse是通过类似于活跃变量分析来进行, 只是得出一个准确的距离, 而且为了处理循环, 会给离开循环的边一个极大的权重,
//! 这使得循环内部的任何使用距离都小于循环外部的使用距离, 因此，在循环前做决策时，算法会优先保留在循环内使用的变量，从而避免将 reload 操作放在循环内部
//!     
//! limit：将寄存器集合 W 的大小减少到 m 以内
//! ```rust,no-run
//! def limit(W, S, insn, m):
//!   // 1. 排序: 根据“下次使用距离”对W中的变量进行排序, 距离为升序
//!   sort(W, insn)
//!   // 2. 淘汰: W[m:-1] 表示从第 m 个元素到末尾的所有"最远未来使用"的变量
//!   for v in W[m:-1]:
//!     // 3. 决策是否 spill:
//!     // - v not in S: 如果是第一次被淘汰, 就需要生成一条spill指令; 如果已被spill则无需生成
//!     // - nextUse(insn, v) != infinity: 检查变量 v 是否为"死变量", 如果未来永远不会使用, 则保存是毫无意义的
//!     if v not in S and nextUse(insn, v) != infinity:
//!       add a spill for v before insn
//!     // 4. 【关键】无条件更新状态：无论是否生成spill，v都已不在寄存器中
//!     W = W \ {v}
//!     S = S \ {v} // 维持 S ⊆ W 这个不变式
//!   // 5. 更新 W 集合
//!   W = W[0:m]
//! ```
//!
//! minAlgorithm: 按顺序处理一个基本块中的所有指令处理Spill/Reload过程
//! ```rust,no-run
//! def minAlgorithm(block, W, S):
//!   for insn in block.insnructions:
//!     // 步骤 A: 处理指令的操作数 uses
//!     // 1. 计算需重载的变量：找出指令需要使用、但当前不在寄存器集 W 中的变量
//!     R = insn.uses \ W
//!     // 2. 为每个需重载的变量腾出空间 --  将重载变量加入 W 和 S
//!      W = W U R // 2a. 先假设已将变量use放入寄存器
//!      S = S U R // 2b. 因为是重载，说明内存必有其备份，故加入 S
//!     // 3. 检查寄存器压力 -- 此时 W 的大小可能超过 k, 调用limit淘汰为 uses 腾出空间
//!     limit(W, S, insn, k)
//!     // 步骤 B: 处理指令的定义 (defs)
//!     // 4. 为指令结果预留空间：一条指令会定义 |insn.defs| 个新值
//!     //    注意：此时的 nextUse 是从 insn.next 开始计算
//!     //    因为 insn 本身的 use 已经处理完毕，不再影响未来的决策
//!     limit(W, S, insn.next, k - |insn.defs|)
//!     // 5. 将新定义的变量加入 W： 其定义的新变量现在位于寄存器中
//!     W = W U {insn.defs}
//!     // 步骤 C: 实际插入代码
//!     // 6. 插入重载指令：对于集合 R 中的所有变量，在指令 insn 前插入 Reload
//!     add reloads for vars in R
//!     in front of insn
//! ```
//!
//! ### 算法 2：W 的初始化
//!
//! 一个基本块开始时，寄存器里有哪些变量？这取决于它的所有前驱块执行结束时的状态, 要区分循环头/普通块
//! - 循环头: 为了将重载操作提升到循环外，循环头的 W_entry 不参考其前驱的状态, 而是优先考虑那些在循环内部被使用的变量
//!     将它们放入 W_entry, 这确保了如果一个变量在循环外被溢出了，它的 reload 指令会被放置在进入循环的边上，而不是循环体内部
//!     对于那些贯穿整个循环但未在其中使用的变量live-through, 算法会根据循环自身的寄存器压力进行启发式判断，
//!     只有在确信该变量能在循环中“幸存”而不会被再次溢出的情况下，才将其放入 W_entry
//! - 普通块: 在RPO序下, 对于一个块 B，其 W_entry 入口寄存器集的计算方式是根据是否候选来判断的
//!
//! initLoopHeader: 为循环头块确定初始 W
//! 其核心是不惜一切代价避免在循环内部进行重载, 因此决策完全基于循环自身的需求，而非前驱块的状态
//! ```rust,no-run
//! def initLoopHeader(block):
//!   entry = block.firstInstruction
//!   loop = loopOf(block)
//!   // 1. 识别所有在循环入口活跃的变量
//!   alive = block.phis U block.liveIn
//!   // 2. 将活跃变量分为两类：
//!   // - cand: 在循环内部被使用的变量 Candidates, 这些是首要保留在寄存器中的
//!   // - liveThrough: 贯穿循环但未在内部使用的变量 Live-Through
//!   cand = usedInLoop(loop, alive)
//!   liveThrough = alive \ cand
//!   // 3. 启发式决策：
//!   if |cand| < k:
//!     // 情况A：仅“循环内使用”的变量，数量小于寄存器总数 k
//!     //  此时还有空闲寄存器，可以容纳一些 liveThrough 变量
//!     // freeLoop 计算能容纳多少 liveThrough 变量而不在循环内引发溢出
//!     //  (k - loop.maxPressure): 循环内部最紧张(整个循环内的最大压力)时还剩余的寄存器槽位
//!     //  这些槽位可以给 liveThrough 变量
//!     freeLoop = k - loop.maxPressure
//!     sort(liveThrough, entry)
//!     add = liveThrough[0:freeLoop]
//!   else:
//!     // 情况二：“循环内使用”的变量太多，连循环内都放不下
//!     // 只能从 cand 中选择最重要的 k 个, liveThrough 变量完全不考虑
//!     sort(cand, entry)
//!     cand = cand[0:k]
//!     add = {}
//!   // 4. 返回最终的初始 W 集合
//!   return cand U add
//! ```
//!
//! initUsual: 普通块确定初始 W, 其决策依据是所有前驱块执行结束时的状态
//! ```rust,no-run
//! def initUsual(block):
//!   freq = map()  // 记录每个变量在多少个前驱的出口 W 中出现
//!   take = {}     // 存放“必定”在寄存器中的变量
//!   cand = {}     //  存放“可能”在寄存器中的变量(候选)
//!   // 1. 统计来自所有前驱的信息
//!   for pred in block.preds:
//!     // pred.Wend 是前驱块 pred 的出口 W 集合
//!     for var in pred.Wend:   
//!       freq[var] = freq[var] + 1
//!       cand = cand U {var}
//!       // 如果一个变量在所有前驱的出口 W 中都存在...
//!       if freq[var] == |block.preds|:
//!         cand = cand \ {var} // ...就从候选者中移除...
//!         take = take U {var} // ...并放入“必定”集合
//!   // 2. 排序候选者
//!   entry = block.firstInstruction
//!   // 对那些只在部分前驱中存在的变量，按下次使用距离排序
//!   sort(cand, entry)
//!   // 3. 组合最终结果
//!   // 返回所有“必定”在寄存器的变量，并用最优的候选者填满剩余的 k - |take| 个槽位
//!   return take U cand[0:k - |take|]
//! ```
//!
//! ## 关键语义修正：S集合的独立性
//!
//! ### 观点差异与语义确定
//!
//! 在实现过程中，对于S集合的语义存在两种观点：
//!
//! **观点A（正确）**: S独立于W，记录栈上副本的有效性
//! - S表示"在栈上存在有效副本的变量集合"
//! - S独立于W，不要求S⊆W
//! - 变量可以同时在寄存器和栈上（W∩S），也可以只在栈上（S\W）
//! - 这更符合实际硬件语义：栈上的值不会因为加载到寄存器就消失
//!
//! **观点B（错误）**: S⊆W必须始终成立
//! - S表示"既在寄存器中又有栈副本的变量"
//! - 会导致编译器"忘记"已spill的变量
//! - 产生"尝试Reload一个从未被Spill过的寄存器"错误
//!
//! ### 实现中的难题
//!
//! 1. **Live-through变量的处理**
//!    - 问题：变量在循环外定义，贯穿整个循环但未在循环中使用
//!    - 挑战：第一次进入循环时可能未被spill，但后续迭代需要reload
//!    - 当前解决：在`get_slot_id`中按需分配槽位（临时方案）
//!
//! 2. **延迟耦合代码生成**
//!    - 问题：RPO遍历时，循环回边的前驱还未处理
//!    - 解决：使用`spill_visited`和`inserted_coupling`追踪状态
//!    - 在`after_handle`中处理循环头的延迟coupling
//!
//! 3. **S集合的维护时机**
//!    - Spill时：将变量加入S（表示栈上有副本）
//!    - Reload时：不改变S（栈副本仍有效）
//!    - Def时：从S中移除（新值使栈副本失效）
//!    - 块出口清理：不再执行S∩W（保持S独立性）
//!
//! ### 遗留问题
//!
//! 1. **按需槽位分配**
//!    - 现状：`get_slot_id`会为未spill的变量分配槽位并发出警告
//!    - 根因：某些live-through变量的spill时机不正确
//!    - TODO：确保所有需要spill的变量在正确时机生成spill指令
//!
//! 2. **循环入口的S_entry计算**
//!    - 问题：首次访问循环头时，回边前驱的S_exit未知
//!    - 影响：可能导致不必要的spill/reload
//!    - 需要更精确的循环分析
//!
//! 3. **耦合代码的精确性**
//!    - 当前：生成了一些冗余的spill/reload
//!    - 优化方向：基于更精确的别名分析减少不必要的内存操作
//!
//! ## 重要改进：Call-Aware Spilling
//!
//! 原始MIN算法存在一个关键缺陷：没有考虑函数调用的ABI约束。
//! 在Call点，caller-saved寄存器会被破坏，跨Call的活跃变量只能使用callee-saved寄存器
//! 因此当穿透Call活跃的变量一旦超过了 callee-saved 寄存器的数量，就会导致着色失败!
//!
//! ```rust,no-run
//! // 假设有20个跨Call的活跃变量，k=27（总寄存器数）
//! v1, v2, ..., v20 都活跃
//! result = call(a, b, c)  // Call点
//! 使用 v1, v2, ..., v20  // Call后仍需使用
//!
//! MIN算法：20 < 27，不溢出, 因为已经是"k-colorable"
//! IRC阶段：只有12个callee-saved可用，却会分配失败
//! ```
//! 解决方案
//! 在CallPseudo/RetCallPseudo点实施特殊的压力限制：
//! - 识别ABI绑定变量（参数/返回值，必须在指定caller-saved寄存器）
//! - 限制跨周期变量数量 ≤ CALLEE_SAVED_REGS_LEN
//! - 超出部分提前溢出，保证IRC阶段成功
//!
//! 备注:
//! 即使已经有 pst_slot_map 也不能完全删除 S 变量,
//! 它承载了 pst_slot_map 全局槽位分配表无法提供的一个至关重要的信息维度：栈上副本的validity
use super::{
    inst::{ArgPair, MInst},
    liveness::NextUseDistance,
    prog::*,
    reg::*,
    reg_alloc::RegAllocEnv,
};
use std::collections::{BTreeMap, BTreeSet};

// ===================================================================
// 块状态定义
// ===================================================================

/// 存储每个块处理完成后的出口状态
#[derive(Debug, Clone, Default)]
struct BlockExitState {
    /// 出口时的整数寄存器工作集 (W)
    w_x: BTreeSet<Reg>,
    /// 出口时的整数溢出集 (栈上有副本的变量)
    s_x: BTreeSet<Reg>,
    /// 出口时的浮点寄存器工作集 (W)
    w_f: BTreeSet<Reg>,
    /// 出口时的浮点溢出集 (栈上有副本的变量)
    s_f: BTreeSet<Reg>,
}

// ===================================================================
// Spiller 状态机
// ===================================================================

/// 溢出算法的核心执行器
struct Spiller<'a, 'b> {
    /// 环境引用
    env: &'a mut RegAllocEnv<'b>,

    /// 存储每个已处理块的出口状态，供后继块查询
    block_states: BTreeMap<VirtBlockId, BlockExitState>,

    /// 暂存需要插入的新指令
    /// Key是块ID，Value是(指令索引, 指令列表)，在指令索引之前插入指令列表
    pending_insts: BTreeMap<VirtBlockId, Vec<(usize, Vec<MInst>)>>,

    /// 记录哪些块已经被处理过（apply_min_on_block_body完成）
    spill_visited: BTreeSet<VirtBlockId>,

    /// 记录哪些块已经插入了coupling code
    inserted_coupling: BTreeSet<VirtBlockId>,

    /// 记录需要延迟插入的 spill
    /// 当遇到需要 reload 但从未 spill 的寄存器时，记录在这里
    /// 格式: (目标块, 寄存器, 插入位置提示)
    delayed_spills: Vec<(VirtBlockId, Reg, String)>,

    /// 物理寄存器数量限制
    k_x: usize,
    k_f: usize,
    /// 要限制跨周期变量到callee-saved数量
    k_call_x: usize,
    k_call_f: usize,

    /// 是否插入详细的Comment注释
    insert_comments: bool,
}

// ===================================================================
// 公共接口
// ===================================================================

/// 模块主入口函数（默认不插入注释）
pub fn run(env: &mut RegAllocEnv) {
    run_with_comments(env, false);
}

/// 带注释控制的入口函数
pub fn run_with_comments(env: &mut RegAllocEnv, insert_comments: bool) {
    let mut spiller = Spiller::new(env, insert_comments);
    spiller.run();
    spiller.commit_changes();
}

// ===================================================================
// Spiller 核心实现
// ===================================================================

impl<'a, 'b> Spiller<'a, 'b> {
    /// 创建新的 Spiller 实例
    fn new(env: &'a mut RegAllocEnv<'b>, insert_comments: bool) -> Self {
        // 从ISA获取实际的物理寄存器数量
        // 不再使用启发式的k值调整，而是通过正确的Call-Aware Spilling来处理
        let k_x = XREGS_MAX_PRESSURE; // 使用完整的25个寄存器
        let k_f = FREGS_MAX_PRESSURE; // 浮点寄存器保持不变
        let k_call_x = CALLEE_SAVED_XREGS_LEN;
        let k_call_f = CALLEE_SAVED_FREGS_LEN;

        Self {
            env,
            block_states: BTreeMap::new(),
            pending_insts: BTreeMap::new(),
            spill_visited: BTreeSet::new(),
            inserted_coupling: BTreeSet::new(),
            delayed_spills: Vec::new(),
            k_x,
            k_f,
            k_call_x,
            k_call_f,
            insert_comments,
        }
    }

    /// 主驱动函数（单遍RPO处理）
    fn run(&mut self) {
        // 1. 获取 RPO 遍历顺序
        let rpo_order = self.env.vfunc.cfg.block_order().to_vec();

        // 2. 按 RPO 顺序单遍处理所有块
        // 每个块在处理时，其前驱已经被处理完毕
        for &block_id in &rpo_order {
            self.process_block(block_id);
        }

        // 3. 第二阶段：处理所有延迟的 spill
        self.process_delayed_spills();
    }

    /// 第二阶段：处理所有延迟的 spill
    /// 在所有块处理完成后，插入之前记录的延迟 spill
    fn process_delayed_spills(&mut self) {
        eprintln!(
            "\n=== Processing {} delayed spills ===",
            self.delayed_spills.len()
        );

        // 按块分组延迟的 spill
        let mut spills_by_block: BTreeMap<VirtBlockId, Vec<(Reg, String)>> = BTreeMap::new();

        for (block_id, reg, hint) in self.delayed_spills.drain(..) {
            spills_by_block
                .entry(block_id)
                .or_insert_with(Vec::new)
                .push((reg, hint));
        }

        // 处理每个块的延迟 spill
        for (block_id, spills) in spills_by_block {
            eprintln!(
                "Processing {} delayed spills for block {:?}",
                spills.len(),
                block_id
            );

            // 获取块的指令列表
            let block_data = self
                .env
                .vfunc
                .block_datas
                .get(&block_id)
                .expect("Block must exist");

            // 找到合适的插入位置（在第一个 terminator 指令之前）
            // 正向查找第一个 terminator，因为条件跳转会拆分成两条：Bxx 和 Jump
            let insert_pos = block_data
                .insts
                .iter()
                .position(|inst| inst.is_terminator())
                .unwrap_or(block_data.insts.len());

            let mut spill_insts = Vec::new();

            // 为每个延迟的寄存器生成 spill 指令
            for (reg, hint) in spills {
                let slot_id = *self
                    .env
                    .vfunc
                    .pst_slot_map
                    .get(&reg)
                    .expect("Slot must have been allocated");
                let slot_kind = self.env.vfunc.stack.get_disc(slot_id);

                if self.insert_comments {
                    spill_insts.push(MInst::Comment {
                        label: format!("DELAYED SPILL {} ({})", reg, hint),
                    });
                }

                spill_insts.push(MInst::Spill {
                    reg,
                    slot: slot_id,
                    kind: slot_kind,
                });

                eprintln!("  - Generated spill for {} at position {}", reg, insert_pos);
            }

            // 记录需要插入的指令
            if !spill_insts.is_empty() {
                self.pending_insts
                    .entry(block_id)
                    .or_insert_with(Vec::new)
                    .push((insert_pos, spill_insts));
            }
        }

        eprintln!("=== Delayed spills processing complete ===\n");
    }

    /// 处理单个基本块（合并版本）
    fn process_block(&mut self, block_id: VirtBlockId) {
        // 步骤 1: 初始化入口状态 (W_entry, S_entry)
        let (w_entry_x, w_entry_f) = self.initialize_w_entry(block_id);
        let (s_entry_x, s_entry_f) = self.initialize_s_entry(block_id, &w_entry_x, &w_entry_f);

        // 步骤 2: 尝试为当前块插入coupling code
        // 只有当所有前驱都已处理完成时才插入
        self.try_insert_coupling_code(block_id, &w_entry_x, &s_entry_x, &w_entry_f, &s_entry_f);

        // 步骤 3: 在块内应用 Min 算法
        let exit_state =
            self.apply_min_on_block_body(block_id, w_entry_x, s_entry_x, w_entry_f, s_entry_f);

        // 步骤 4: 存储该块的出口状态，供后继使用
        self.block_states.insert(block_id, exit_state);

        // 步骤 5: 标记当前块已处理完成
        self.spill_visited.insert(block_id);

        // 步骤 6: 处理后续工作
        // 检查当前块的后继是否可以插入coupling code
        self.after_handle(block_id);
    }

    /// 在块处理完成后，检查是否需要为其后继块生成coupling code
    fn after_handle(&mut self, block_id: VirtBlockId) {
        // 遍历当前块的所有后继
        let successors: Vec<_> = self.env.vfunc.cfg.successors(block_id).collect();

        for succ_id in successors {
            // 如果后继块是循环头，尝试为它生成coupling code
            let loop_analysis = self.env.loop_analysis.as_ref().unwrap();
            if loop_analysis.is_loop_header(succ_id).is_some() {
                // 重新获取循环头的入口状态
                let (header_w_entry_x, header_w_entry_f) = self.initialize_w_entry(succ_id);
                let (header_s_entry_x, header_s_entry_f) =
                    self.initialize_s_entry(succ_id, &header_w_entry_x, &header_w_entry_f);

                // 尝试为循环头插入coupling code
                self.try_insert_coupling_code(
                    succ_id,
                    &header_w_entry_x,
                    &header_s_entry_x,
                    &header_w_entry_f,
                    &header_s_entry_f,
                );
            }
        }
    }

    /// 尝试为块插入coupling code
    /// 只有当块的所有前驱都已处理完成，且还没有插入过coupling code时才插入
    fn try_insert_coupling_code(
        &mut self,
        block_id: VirtBlockId,
        w_entry_x: &BTreeSet<Reg>,
        s_entry_x: &BTreeSet<Reg>,
        w_entry_f: &BTreeSet<Reg>,
        s_entry_f: &BTreeSet<Reg>,
    ) {
        // 如果已经插入过coupling code，直接返回
        if self.inserted_coupling.contains(&block_id) {
            return;
        }

        // 检查所有前驱是否都已处理完成
        let predecessors: Vec<_> = self.env.vfunc.cfg.predecessors(block_id).collect();
        let all_preds_processed = predecessors
            .iter()
            .all(|pred| self.spill_visited.contains(pred));

        if !all_preds_processed {
            // 还有前驱未处理，暂时跳过
            return;
        }

        // 所有前驱都已处理，可以插入coupling code
        for pred_id in predecessors {
            self.insert_coupling_code_for_edge(
                pred_id, block_id, w_entry_x, s_entry_x, w_entry_f, s_entry_f,
            );
        }

        // 标记已插入coupling code
        self.inserted_coupling.insert(block_id);
    }

    /// 仅为单条边生成耦合代码
    fn insert_coupling_code_for_edge(
        &mut self,
        pred_id: VirtBlockId,
        succ_id: VirtBlockId,
        w_entry_x: &BTreeSet<Reg>,
        s_entry_x: &BTreeSet<Reg>,
        w_entry_f: &BTreeSet<Reg>,
        s_entry_f: &BTreeSet<Reg>,
    ) {
        if let Some(pred_state) = self.block_states.get(&pred_id).cloned() {
            let mut coupling_insts = Vec::new();

            // 计算需要 reload 的变量
            // 变量需要满足：1) 在entry_W中 2) 不在pred.exit_W中
            // 不需要检查S，因为如果变量需要reload，它要么已经有槽，要么会在下面分配
            let need_reload_x: Vec<_> = w_entry_x.difference(&pred_state.w_x).cloned().collect();
            let need_reload_f: Vec<_> = w_entry_f.difference(&pred_state.w_f).cloned().collect();

            // 计算需要 spill 的变量
            // 标准情况：变量在S_entry中但不在pred.S_exit中，且在pred.W_exit中
            let need_spill_x: Vec<_> = s_entry_x
                .difference(&pred_state.s_x)
                .filter(|r| pred_state.w_x.contains(r))
                .cloned()
                .collect();

            let need_spill_f: Vec<_> = s_entry_f
                .difference(&pred_state.s_f)
                .filter(|r| pred_state.w_f.contains(r))
                .cloned()
                .collect();

            // 特殊情况：如果变量需要reload但从未分配过槽，说明它是live-through但从未spill
            // 这种情况下，我们需要在前驱查找该变量最后出现的位置并生成补偿性spill
            // 暂时先依赖get_slot_id的on-demand分配来处理这种情况

            // 生成 spill 指令
            for reg in need_spill_x {
                if self.insert_comments {
                    coupling_insts.push(MInst::Comment {
                        label: format!(
                            "COUPLING SPILL {} (from {:?} to {:?})",
                            reg, pred_id, succ_id
                        ),
                    });
                }
                // 使用统一接口，确保每个寄存器只对应一个槽位
                let (slot_id, slot_kind) = self.env.get_or_create_slot_for_vreg(reg);
                coupling_insts.push(MInst::Spill {
                    reg,
                    slot: slot_id,
                    kind: slot_kind,
                });
            }
            for reg in need_spill_f {
                if self.insert_comments {
                    coupling_insts.push(MInst::Comment {
                        label: format!(
                            "COUPLING SPILL {} (from {:?} to {:?})",
                            reg, pred_id, succ_id
                        ),
                    });
                }
                // 使用统一接口，确保每个寄存器只对应一个槽位
                let (slot_id, slot_kind) = self.env.get_or_create_slot_for_vreg(reg);
                coupling_insts.push(MInst::Spill {
                    reg,
                    slot: slot_id,
                    kind: slot_kind,
                });
            }

            // 生成 reload 指令
            for reg in need_reload_x {
                // 如果寄存器从未被spill过，需要先分配槽位
                // 这种情况发生在变量是live-through但第一次需要reload时
                if !self.env.vfunc.pst_slot_map.contains_key(&reg) {
                    eprintln!(
                        "DEBUG: Found unspilled register {} needing reload from {:?} to {:?}",
                        reg, pred_id, succ_id
                    );
                    eprintln!(
                        "DEBUG: Is predecessor {:?} already processed? {}",
                        pred_id,
                        self.spill_visited.contains(&pred_id)
                    );

                    // 使用统一接口，确保每个寄存器只对应一个槽位
                    let (slot_id, slot_kind) = self.env.get_or_create_slot_for_vreg(reg);

                    // 记录延迟的 spill，稍后在第二阶段处理
                    let hint = format!("From {:?} to {:?} (coupling code)", pred_id, succ_id);
                    self.delayed_spills.push((pred_id, reg, hint));
                    eprintln!(
                        "INFO: Recorded delayed spill for {} in block {:?}",
                        reg, pred_id
                    );
                }

                if self.insert_comments {
                    coupling_insts.push(MInst::Comment {
                        label: format!(
                            "COUPLING RELOAD {} (from {:?} to {:?})",
                            reg, pred_id, succ_id
                        ),
                    });
                }
                let slot_id = *self
                    .env
                    .vfunc
                    .pst_slot_map
                    .get(&reg)
                    .expect("Slot must exist after allocation");
                let kind = self.env.vfunc.stack.get_disc(slot_id);
                coupling_insts.push(MInst::Reload {
                    slot: slot_id,
                    reg,
                    kind,
                });
            }
            for reg in need_reload_f {
                // 如果寄存器从未被spill过，需要先分配槽位
                if !self.env.vfunc.pst_slot_map.contains_key(&reg) {
                    eprintln!(
                        "DEBUG: Found unspilled float register {} needing reload from {:?} to {:?}",
                        reg, pred_id, succ_id
                    );
                    eprintln!(
                        "DEBUG: Is predecessor {:?} already processed? {}",
                        pred_id,
                        self.spill_visited.contains(&pred_id)
                    );
                    // 使用统一接口，确保每个寄存器只对应一个槽位
                    let (slot_id, slot_kind) = self.env.get_or_create_slot_for_vreg(reg);

                    // 记录延迟的 spill，稍后在第二阶段处理
                    let hint =
                        format!("From {:?} to {:?} (coupling code, float)", pred_id, succ_id);
                    self.delayed_spills.push((pred_id, reg, hint));
                    eprintln!(
                        "INFO: Recorded delayed spill for float {} in block {:?}",
                        reg, pred_id
                    );
                }

                if self.insert_comments {
                    coupling_insts.push(MInst::Comment {
                        label: format!(
                            "COUPLING RELOAD {} (from {:?} to {:?})",
                            reg, pred_id, succ_id
                        ),
                    });
                }
                let slot_id = *self
                    .env
                    .vfunc
                    .pst_slot_map
                    .get(&reg)
                    .expect("Slot must exist after allocation");
                let kind = self.env.vfunc.stack.get_disc(slot_id);
                coupling_insts.push(MInst::Reload {
                    slot: slot_id,
                    reg,
                    kind,
                });
            }

            // 如果有指令需要插入，记录到 pending_insts
            if !coupling_insts.is_empty() {
                // 根据边的类型决定插入位置
                match (pred_id, succ_id) {
                    (VirtBlockId::CriticalEdge { .. }, _) => {
                        // 关键边块：在块的开头插入
                        self.pending_insts
                            .entry(pred_id)
                            .or_default()
                            .push((0, coupling_insts));
                    }
                    _ => {
                        // 普通边：在前驱块的末尾插入
                        // 注意：必须在跳转指令之前插入，否则不会执行
                        if let Some(pred_block) = self.env.vfunc.block_datas.get(&pred_id) {
                            // 从后向前查找跳转指令的位置
                            let mut insert_pos = pred_block.insts.len();

                            // 查找最后一条非注释指令
                            for (i, inst) in pred_block.insts.iter().enumerate().rev() {
                                // 跳过注释
                                if inst.is_comment() {
                                    continue;
                                }
                                // 如果找到跳转指令，必须在它之前插入
                                if inst.is_terminator() {
                                    insert_pos = i; // 在跳转指令的位置插入把跳转往后推
                                } else {
                                    // 最后非注释指令不是跳转
                                    panic!("{i}:{inst} -- 最后非注释指令不是跳转");
                                }
                                break; // 只检查最后一条非注释指令
                            }

                            self.pending_insts
                                .entry(pred_id)
                                .or_default()
                                .push((insert_pos, coupling_insts));
                        }
                    }
                }
            }
        }
    }

    /// 初始化块的入口工作集 W
    fn initialize_w_entry(&self, block_id: VirtBlockId) -> (BTreeSet<Reg>, BTreeSet<Reg>) {
        let loop_analysis = self.env.loop_analysis.as_ref().unwrap();
        // 对没有前驱的入口块进行特殊处理
        if block_id == self.env.vfunc.cfg.entry() {
            let mut w_x = BTreeSet::new();
            let mut w_f = BTreeSet::new();

            for arg_pair in &self.env.vfunc.args {
                if let &ArgPair::RegArg { vreg, .. } = arg_pair {
                    if vreg.is_xreg() {
                        w_x.insert(vreg);
                    } else {
                        w_f.insert(vreg);
                    }
                }
            }
            return (w_x, w_f);
        }

        if let Some(loop_id) = loop_analysis.is_loop_header(block_id) {
            self.init_w_for_loop_header(block_id, loop_id)
        } else {
            self.init_w_for_usual_block(block_id)
        }
    }

    /// 初始化循环头的工作集
    fn init_w_for_loop_header(
        &self,
        block_id: VirtBlockId,
        loop_id: crate::asm::loop_analysis::AsmLoopId,
    ) -> (BTreeSet<Reg>, BTreeSet<Reg>) {
        let liveness = self.env.liveness.as_ref().unwrap();
        let loop_analysis = self.env.loop_analysis.as_ref().unwrap();

        // 1. 建立一个安全的候选池
        // 关键修正：如果前驱还没处理，使用活跃变量作为候选池
        let mut safe_cand_pool_x = BTreeSet::new();
        let mut safe_cand_pool_f = BTreeSet::new();

        let loop_info = loop_analysis.get_loop_info(loop_id).unwrap();
        let mut has_unprocessed_pred = false;

        for pred_id in self.env.vfunc.cfg.predecessors(block_id) {
            // 只考虑非回边的前驱
            if !loop_info.blocks.contains(&pred_id) {
                if let Some(pred_state) = self.block_states.get(&pred_id) {
                    safe_cand_pool_x.extend(&pred_state.w_x);
                    safe_cand_pool_f.extend(&pred_state.w_f);
                } else {
                    // 前驱还没处理，标记
                    has_unprocessed_pred = true;
                }
            }
        }

        // 如果有未处理的前驱，使用活跃变量作为候选池
        if has_unprocessed_pred {
            let block_liveness = &liveness.liveness[&block_id];
            // 使用活跃变量作为候选池
            for &reg in &block_liveness.live_in {
                if reg.is_xreg() {
                    safe_cand_pool_x.insert(reg);
                } else {
                    safe_cand_pool_f.insert(reg);
                }
            }
        }

        // 2. 获取循环信息和活跃度信息
        let used_in_loop = loop_analysis
            .get_used_in_loop_vars(loop_id)
            .unwrap_or(&BTreeSet::new())
            .clone();
        let (max_p_x, max_p_f) = loop_analysis
            .get_loop_max_pressure(loop_id)
            .unwrap_or((0, 0));
        let entry_next_use = &liveness.next_use[&block_id].entry_next_use;

        // 3. 将安全池中的变量分为两类：
        //    - cand: 在循环内部被使用的变量
        //    - liveThrough: 贯穿循环但未在内部使用的变量
        let (used_in_loop_x, used_in_loop_f): (BTreeSet<_>, BTreeSet<_>) = used_in_loop
            .into_iter()
            .partition(|r| matches!(r, Reg::VX(_) | Reg::X(_)));

        let cand_x: BTreeSet<_> = safe_cand_pool_x
            .intersection(&used_in_loop_x)
            .cloned()
            .collect();
        let live_through_x: BTreeSet<_> = safe_cand_pool_x.difference(&cand_x).cloned().collect();

        let cand_f: BTreeSet<_> = safe_cand_pool_f
            .intersection(&used_in_loop_f)
            .cloned()
            .collect();
        let live_through_f: BTreeSet<_> = safe_cand_pool_f.difference(&cand_f).cloned().collect();

        // 4. 对整数寄存器应用启发式 (与原版逻辑相同，但基于安全的候选集)
        eprintln!(
            "Loop {:?}: cand_x.len()={}, live_through_x.len()={}, k_x={}, safe_pool_x.len()={}",
            loop_id,
            cand_x.len(),
            live_through_x.len(),
            self.k_x,
            safe_cand_pool_x.len()
        );
        let mut w_x = BTreeSet::new();
        if cand_x.len() < self.k_x {
            // 所有候选变量必须在寄存器中
            w_x.extend(cand_x.clone());

            // 使用真实的max_pressure而不是假设的公式
            // 正确的启发式基于循环体内的真实最大压力来决定
            // 可以安全容纳的live-through数量 = k - max_pressure
            let free_slots_for_live_through = self.k_x.saturating_sub(max_p_x);

            if free_slots_for_live_through > 0 {
                // 只选择最重要的live-through变量
                let best_live_through = self.sort_and_take(
                    &live_through_x,
                    entry_next_use,
                    free_slots_for_live_through,
                );
                let num_added = best_live_through.len();
                w_x.extend(best_live_through);
                eprintln!(
                    "  循环{:?}: max_pressure={}, k={}, 候选{}个，安全加入{}个live-through",
                    loop_id,
                    max_p_x,
                    self.k_x,
                    cand_x.len(),
                    num_added
                );
            } else {
                eprintln!(
                    "  循环{:?}: max_pressure={}>=k={}, 不能安全容纳任何live-through变量",
                    loop_id, max_p_x, self.k_x
                );
            }
        } else {
            // 简化方案：候选变量太多时，直接取前k个
            // 按next-use距离排序，选择最近使用的k个
            let best_candidates = self.sort_and_take(&cand_x, entry_next_use, self.k_x);
            w_x.extend(best_candidates);

            eprintln!(
                "  循环{:?}: 候选变量({})>=k({})，按next-use选择{}个进入W_entry",
                loop_id,
                cand_x.len(),
                self.k_x,
                self.k_x
            );
            // live_through变量全部在循环外spill
        }

        // 5. 对浮点寄存器应用启发式（逻辑相同）
        let mut w_f = BTreeSet::new();
        if cand_f.len() < self.k_f {
            // 所有候选变量必须在寄存器中
            w_f.extend(cand_f.clone());

            // 修复缺陷#2：使用真实的max_pressure而不是假设的公式
            let free_slots_for_live_through = self.k_f.saturating_sub(max_p_f);

            if free_slots_for_live_through > 0 {
                // 只选择最重要的live-through变量
                let best_live_through = self.sort_and_take(
                    &live_through_f,
                    entry_next_use,
                    free_slots_for_live_through,
                );
                w_f.extend(best_live_through);
            }
        } else {
            // 简化方案：候选变量太多时，直接取前k个
            // 按next-use距离排序，选择最近使用的k个
            let best_candidates = self.sort_and_take(&cand_f, entry_next_use, self.k_f);
            w_f.extend(best_candidates);

            eprintln!(
                "  循环{:?}: 浮点候选变量({})>=k({})，按next-use选择{}个进入W_entry",
                loop_id,
                cand_f.len(),
                self.k_f,
                self.k_f
            );
        }

        (w_x, w_f)
    }

    /// 初始化普通块的工作集
    fn init_w_for_usual_block(&self, block_id: VirtBlockId) -> (BTreeSet<Reg>, BTreeSet<Reg>) {
        let liveness = self.env.liveness.as_ref().unwrap();
        let entry_next_use = &liveness.next_use[&block_id].entry_next_use;

        // 特殊处理入口块
        if block_id == self.env.vfunc.cfg.entry() {
            // 入口块：使用VirtFunc::args中的信息
            let mut w_x = BTreeSet::new();
            let mut w_f = BTreeSet::new();

            for arg_pair in &self.env.vfunc.args {
                match arg_pair {
                    &ArgPair::RegArg { vreg, .. } => {
                        // 寄存器传递的参数加入工作集
                        if vreg.is_xreg() {
                            w_x.insert(vreg);
                        } else {
                            w_f.insert(vreg);
                        }
                    }
                    ArgPair::StackArg { .. } => {
                        // StackArg 应该完全忽略，视而不见!!!
                        // 原因：
                        // 1. StackArg 只是一个信息标记，对寄存器分配没有任何意义
                        // 2. StackArg.vreg 的值已经在函数调用约定的栈位置上
                        // 3. 后续使用 vreg 时会从正确的栈位置加载（不是 spill slot）
                        // 4. StackArg 与 spill/reload 机制完全无关
                        // 5. 将 StackArg.vreg 加入工作集会导致错误的寄存器分配决策
                    }
                    ArgPair::None => {}
                }
            }

            return (w_x, w_f);
        }

        // 非入口块的处理逻辑
        // 统计变量在前驱中出现的频率
        let mut freq_x: BTreeMap<Reg, usize> = BTreeMap::new();
        let mut freq_f: BTreeMap<Reg, usize> = BTreeMap::new();
        let mut cand_x = BTreeSet::new();
        let mut cand_f = BTreeSet::new();

        let preds: Vec<_> = self.env.vfunc.cfg.predecessors(block_id).collect();
        let pred_count = preds.len();
        if pred_count == 0 {
            // 如果一个非入口块没有前驱，那就是死代码返回空集合 (虽然SSA已经进行过死代码优化)
            return (BTreeSet::new(), BTreeSet::new());
        }

        for pred in preds {
            if let Some(pred_state) = self.block_states.get(&pred) {
                for &reg in &pred_state.w_x {
                    *freq_x.entry(reg).or_insert(0) += 1;
                    cand_x.insert(reg);
                }
                for &reg in &pred_state.w_f {
                    *freq_f.entry(reg).or_insert(0) += 1;
                    cand_f.insert(reg);
                }
            }
        }

        // 分离必定在寄存器中的变量（在所有前驱中都存在）
        let mut take_x = BTreeSet::new();
        let mut take_f = BTreeSet::new();

        cand_x.retain(|&reg| {
            if freq_x.get(&reg).is_some_and(|&f| f == pred_count) {
                take_x.insert(reg);
                false
            } else {
                true
            }
        });

        cand_f.retain(|&reg| {
            if freq_f.get(&reg).is_some_and(|&f| f == pred_count) {
                take_f.insert(reg);
                false
            } else {
                true
            }
        });

        // 排序候选者并填充剩余槽位
        let mut w_x = take_x;
        let remaining_x = self.k_x.saturating_sub(w_x.len());
        w_x.extend(self.sort_and_take(&cand_x, entry_next_use, remaining_x));

        let mut w_f = take_f;
        let remaining_f = self.k_f.saturating_sub(w_f.len());
        w_f.extend(self.sort_and_take(&cand_f, entry_next_use, remaining_f));

        (w_x, w_f)
    }

    /// 初始化块的入口溢出集 S
    fn initialize_s_entry(
        &self,
        block_id: VirtBlockId,
        _w_entry_x: &BTreeSet<Reg>,
        _w_entry_f: &BTreeSet<Reg>,
    ) -> (BTreeSet<Reg>, BTreeSet<Reg>) {
        // 存疑修复: 为了维持S属于W的不变量，不能把栈上参数丢入S:
        // 对没有前驱的入口块进行特殊处理
        // if block_id == self.env.vfunc.cfg.entry() {
        //     let mut s_x = BTreeSet::new();
        //     let mut s_f = BTreeSet::new();

        //     for arg_pair in &self.env.vfunc.args {
        //         if let &ArgPair::StackArg { vreg, .. } = arg_pair {
        //             if vreg.is_xreg() {
        //                 s_x.insert(vreg);
        //             } else {
        //                 s_f.insert(vreg);
        //             }
        //             // self.env.vfunc.pst_slot_map.insert(vreg, slot); compiler/src/asm/ctx.rs:230 已经对IncomingArg进行操作了
        //         }
        //     }
        //     return (s_x, s_f);
        // }

        let mut union_s_x = BTreeSet::new();
        let mut union_s_f = BTreeSet::new();

        // 收集所有前驱的出口溢出集
        for pred in self.env.vfunc.cfg.predecessors(block_id) {
            if let Some(pred_state) = self.block_states.get(&pred) {
                union_s_x.extend(&pred_state.s_x);
                union_s_f.extend(&pred_state.s_f);
            }
        }

        // S_entry = ∪ pred.S_exit
        // S独立记录栈状态，不需要与W_entry求交集
        let s_entry_x = union_s_x;
        let s_entry_f = union_s_f;

        (s_entry_x, s_entry_f)
    }

    /// 在块内应用 MIN 算法
    fn apply_min_on_block_body(
        &mut self,
        block_id: VirtBlockId,
        mut w_x: BTreeSet<Reg>,
        mut s_x: BTreeSet<Reg>,
        mut w_f: BTreeSet<Reg>,
        mut s_f: BTreeSet<Reg>,
    ) -> BlockExitState {
        let block = self.env.vfunc.block_datas.get(&block_id).unwrap().clone();

        // 先提取需要的 liveness 信息，然后释放借用
        let (before_inst_next_uses, after_inst_next_uses, exit_next_use) = {
            let liveness = self.env.liveness.as_ref().unwrap();
            let block_next_use = &liveness.next_use[&block_id];

            // 预先计算块内所有指令点的 next-use 信息（执行前和执行后）
            let (before, after) = self.build_inst_next_use(&block, block_next_use);

            // 克隆 exit_next_use 以避免后续借用冲突
            let exit = block_next_use.exit_next_use.clone();

            (before, after, exit)
        };

        // 遍历块中的所有指令
        for (i, inst) in block.insts.iter().enumerate() {
            // 检查：预先存在的Spill/Reload指令的合法性
            // 注意：对于超过寄存器数量的参数，可能会有预先分配的持久化槽位
            match inst {
                &MInst::Spill { slot, reg, kind } => {
                    // 允许以下情况的持久化Spill:
                    // 1. 临时槽位（OutgoingArg等）
                    // 2. 已经在pst_slot_map中的槽位（预先分配的）
                    if kind.is_persistent_slot() && !self.env.vfunc.pst_slot_map.contains_key(&reg)
                    {
                        // 将预先存在的持久化槽位记录到pst_slot_map中
                        self.env.vfunc.pst_slot_map.insert(reg, slot);
                    }
                }
                &MInst::Reload { slot, .. } => {
                    let slot_data = self
                        .env
                        .vfunc
                        .stack
                        .get_slotdata(slot)
                        .expect("必须要有预分配插入的栈槽");
                    // Reload通常应该从持久化位置加载，但也允许从临时位置（如OutgoingArg）
                    // 不再强制断言
                }
                _ => {}
            };
            // 使用预计算的 next-use 信息
            // 指令执行前的 next-use（用于处理uses）
            let before_inst_next_use = &before_inst_next_uses[i];
            // 指令执行后的 next-use（用于处理defs）
            let after_inst_next_use = &after_inst_next_uses[i];

            // 获取指令的 use 和 def
            let (raw_uses, raw_defs) = self.get_inst_use_def(inst);
            let (uses_x, uses_f) = self.partition_regs(&raw_uses);
            let (defs_x, defs_f) = self.partition_regs(&raw_defs);

            // 调试 VX781 相关指令
            if raw_uses.iter().any(|r| r.to_string().contains("VX781"))
                || raw_defs.iter().any(|r| r.to_string().contains("VX781"))
            {
                eprintln!("DEBUG inst {}: {:?}", i, inst);
                eprintln!("  uses: {:?}", raw_uses);
                eprintln!("  defs: {:?}", raw_defs);
                // 检查 VX781 的 next-use 距离
                if let Some(vx781) = raw_uses
                    .iter()
                    .chain(raw_defs.iter())
                    .find(|r| r.to_string().contains("VX781"))
                {
                    let dist_before = before_inst_next_use.get(vx781);
                    let dist_after = after_inst_next_use.get(vx781);
                    eprintln!("  VX781 next-use before inst: {:?}", dist_before);
                    eprintln!("  VX781 next-use after inst: {:?}", dist_after);
                }
            }

            // 在指令 298 附近特别调试
            if i >= 296 && i <= 300 {
                if let Some(vx781_reg) = w_x
                    .iter()
                    .chain(w_f.iter())
                    .find(|r| r.to_string().contains("VX781"))
                {
                    eprintln!(
                        "DEBUG inst {}: VX781 in W, next-use = {:?}",
                        i,
                        before_inst_next_use.get(vx781_reg)
                    );
                }
            }

            // 存储为当前指令新生成的spill/reload指令
            let mut new_insts = Vec::new();

            if inst.is_pseudo_call_like() {
                // ==========================================================
                // Call 指令的特殊处理路径：正确处理Call-Aware Spilling
                // ==========================================================

                // --- XREGS ---
                // 1. Reload uses (和普通指令一样)
                let reload_x: BTreeSet<Reg> = uses_x
                    .iter()
                    .filter(|r| !w_x.contains(r))
                    .cloned()
                    .collect();
                for reg in &reload_x {
                    w_x.insert(*reg);
                    if !s_x.contains(reg) {
                        self.ensure_spill_slot(reg, &mut s_x);
                    }
                }

                // 2. 分离变量：ABI绑定 vs 跨周期
                let abi_bound_x: BTreeSet<Reg> = uses_x.union(&defs_x).cloned().collect();
                let mut live_through_x: BTreeSet<Reg> =
                    w_x.difference(&abi_bound_x).cloned().collect();

                // 3. 对跨周期变量应用 CALLEE-SAVED 限制（关键！）
                let spills_from_live_through_x = self.apply_limit(
                    &mut live_through_x, // 注意：只对live_through集合操作
                    &mut s_x,
                    before_inst_next_use,
                    self.k_call_x,    // 使用callee-saved寄存器数量作为k
                    &BTreeSet::new(), // 在这个子问题中没有需要保护的变量
                );

                // 4. 更新 W 集合：live_through(限制后) + ABI绑定 + defs
                w_x = live_through_x.union(&abi_bound_x).cloned().collect();

                // --- FREGS (逻辑同上) ---
                let reload_f: BTreeSet<Reg> = uses_f
                    .iter()
                    .filter(|r| !w_f.contains(r))
                    .cloned()
                    .collect();
                for reg in &reload_f {
                    w_f.insert(*reg);
                    if !s_f.contains(reg) {
                        self.ensure_spill_slot(reg, &mut s_f);
                    }
                }

                let abi_bound_f: BTreeSet<Reg> = uses_f.union(&defs_f).cloned().collect();
                let mut live_through_f: BTreeSet<Reg> =
                    w_f.difference(&abi_bound_f).cloned().collect();

                let spills_from_live_through_f = self.apply_limit(
                    &mut live_through_f,
                    &mut s_f,
                    before_inst_next_use,
                    self.k_call_f,
                    &BTreeSet::new(),
                );

                w_f = live_through_f.union(&abi_bound_f).cloned().collect();

                // 5. 生成spill/reload指令
                self.generate_spill_reload_insts(
                    &mut new_insts,
                    &spills_from_live_through_x,
                    &reload_x,
                    i,
                    "CALL-AWARE X",
                );
                self.generate_spill_reload_insts(
                    &mut new_insts,
                    &spills_from_live_through_f,
                    &reload_f,
                    i,
                    "CALL-AWARE F",
                );
            } else {
                // ==========================================================
                // 普通指令的标准 MIN 算法路径
                // ==========================================================

                // --- XREGS ---
                // 1. Reload uses
                let reload_x: BTreeSet<Reg> = uses_x
                    .iter()
                    .filter(|r| !w_x.contains(r))
                    .cloned()
                    .collect();
                for reg in &reload_x {
                    w_x.insert(*reg);
                    if !s_x.contains(reg) {
                        self.ensure_spill_slot(reg, &mut s_x);
                    }
                }

                // 2. 限制 W 到 k
                let protected_uses_x: BTreeSet<Reg> = uses_x.clone();
                let mut spills_x = self.apply_limit(
                    &mut w_x,
                    &mut s_x,
                    before_inst_next_use,
                    self.k_x,
                    &protected_uses_x,
                );

                // 3. 处理 defs
                for &def in &defs_x {
                    s_x.remove(&def); // def使栈上的旧副本失效
                }
                let limit_for_defs = self.k_x.saturating_sub(defs_x.len());
                spills_x.extend(self.apply_limit(
                    &mut w_x,
                    &mut s_x,
                    after_inst_next_use,
                    limit_for_defs,
                    &protected_uses_x,
                ));
                for &def in &defs_x {
                    w_x.insert(def);
                }

                // 如果新定义导致 W 超限，需要再次限制
                if w_x.len() > self.k_x {
                    let protected_post_def: BTreeSet<Reg> = defs_x.clone();
                    spills_x.extend(self.apply_limit(
                        &mut w_x,
                        &mut s_x,
                        after_inst_next_use,
                        self.k_x,
                        &protected_post_def,
                    ));
                }

                // --- FREGS (逻辑同上) ---
                let reload_f: BTreeSet<Reg> = uses_f
                    .iter()
                    .filter(|r| !w_f.contains(r))
                    .cloned()
                    .collect();
                for reg in &reload_f {
                    w_f.insert(*reg);
                    if !s_f.contains(reg) {
                        self.ensure_spill_slot(reg, &mut s_f);
                    }
                }

                let protected_uses_f: BTreeSet<Reg> = uses_f.clone();
                let mut spills_f = self.apply_limit(
                    &mut w_f,
                    &mut s_f,
                    before_inst_next_use,
                    self.k_f,
                    &protected_uses_f,
                );

                for &def in &defs_f {
                    s_f.remove(&def);
                }
                let limit_for_defs_f = self.k_f.saturating_sub(defs_f.len());
                spills_f.extend(self.apply_limit(
                    &mut w_f,
                    &mut s_f,
                    after_inst_next_use,
                    limit_for_defs_f,
                    &protected_uses_f,
                ));
                for &def in &defs_f {
                    w_f.insert(def);
                }

                // 如果新定义导致 W 超限，需要再次限制
                if w_f.len() > self.k_f {
                    let protected_post_def: BTreeSet<Reg> = defs_f.clone();
                    spills_f.extend(self.apply_limit(
                        &mut w_f,
                        &mut s_f,
                        after_inst_next_use,
                        self.k_f,
                        &protected_post_def,
                    ));
                }

                // 生成指令
                self.generate_spill_reload_insts(&mut new_insts, &spills_x, &reload_x, i, "MIN X");
                self.generate_spill_reload_insts(&mut new_insts, &spills_f, &reload_f, i, "MIN F");
            }

            // 将生成的新指令记录到pending_insts
            if !new_insts.is_empty() {
                self.pending_insts
                    .entry(block_id)
                    .or_default()
                    .push((i, new_insts));
            }
        } // 结束指令循环

        // ===================================================================
        // 在块末尾清理所有死变量
        // ===================================================================
        // 必须确保块的出口工作集 (W) 只包含在该点仍然活跃 (live-out) 的变量
        // 1. 清理整数寄存器工作集 (w_x)
        // 只保留那些在块出口处 next_use 距离不是无穷大的寄存器
        w_x.retain(|reg| {
            exit_next_use
                .get(reg)
                .map_or(false, |dist| *dist != NextUseDistance::Infinite)
        });

        // 2. 清理浮点寄存器工作集 (w_f)
        w_f.retain(|reg| {
            exit_next_use
                .get(reg)
                .map_or(false, |dist| *dist != NextUseDistance::Infinite)
        });

        // 3. S独立维护，不需要与W同步
        // S记录的是栈上副本的有效性，与是否在寄存器中无关

        // 返回出口状态
        BlockExitState { w_x, s_x, w_f, s_f }
    }

    /// 确保寄存器有spill槽位分配，并将其标记为在栈上
    fn ensure_spill_slot(&mut self, reg: &Reg, s: &mut BTreeSet<Reg>) {
        // 使用统一接口确保槽位分配
        let (_slot_id, _) = self.env.get_or_create_slot_for_vreg(*reg);
        s.insert(*reg);
    }

    /// 生成spill和reload指令
    fn generate_spill_reload_insts(
        &self,
        new_insts: &mut Vec<MInst>,
        spills: &BTreeSet<Reg>,
        reloads: &BTreeSet<Reg>,
        i: usize,
        label: &str,
    ) {
        // 先生成所有Spill指令
        for reg in spills {
            if self.insert_comments {
                new_insts.push(MInst::Comment {
                    label: format!("{} SPILL {} at inst {}", label, reg, i),
                });
            }
            let slot_id = *self.env.vfunc.pst_slot_map.get(reg).unwrap();
            let kind = self.env.vfunc.stack.get_disc(slot_id);
            new_insts.push(MInst::Spill {
                reg: *reg,
                slot: slot_id,
                kind,
            });
        }

        // 再生成所有Reload指令
        for reg in reloads {
            if self.insert_comments {
                new_insts.push(MInst::Comment {
                    label: format!("{} RELOAD {} at inst {}", label, reg, i),
                });
            }
            let slot_id = *self.env.vfunc.pst_slot_map.get(reg).unwrap();
            let kind = self.env.vfunc.stack.get_disc(slot_id);
            new_insts.push(MInst::Reload {
                reg: *reg,
                slot: slot_id,
                kind,
            });
        }
    }

    fn apply_limit(
        &mut self,
        w: &mut BTreeSet<Reg>,
        s: &mut BTreeSet<Reg>,
        next_use: &BTreeMap<Reg, NextUseDistance>,
        k: usize,
        protected: &BTreeSet<Reg>,
    ) -> BTreeSet<Reg> {
        let mut spilled = BTreeSet::new();

        // 如果W的大小不超过k，不需要溢出
        if w.len() <= k {
            return spilled;
        }

        // 需要溢出 w.len() - k 个寄存器
        let need_to_spill = w.len() - k;

        // 创建一个可溢出的候选列表（排除protected的寄存器）
        let mut spillable: Vec<_> = w.difference(protected).cloned().collect();

        // 按next-use距离排序（远的优先溢出）
        spillable.sort_by(|a, b| {
            let dist_a = next_use
                .get(a)
                .cloned()
                .unwrap_or(NextUseDistance::Infinite);
            let dist_b = next_use
                .get(b)
                .cloned()
                .unwrap_or(NextUseDistance::Infinite);
            // 降序排序：距离远的在前
            dist_b.cmp(&dist_a)
        });

        // 溢出前need_to_spill个寄存器
        for i in 0..need_to_spill.min(spillable.len()) {
            let reg = spillable[i];
            w.remove(&reg);

            // 只有当寄存器未来还会被使用时，才生成 spill
            let dist = next_use
                .get(&reg)
                .cloned()
                .unwrap_or(NextUseDistance::Infinite);

            // 调试输出
            if reg.to_string().contains("VX781") {
                eprintln!(
                    "DEBUG apply_limit: Deciding spill for VX781, next_use = {:?}",
                    dist
                );
            }

            if dist != NextUseDistance::Infinite {
                spilled.insert(reg);
                // 使用统一接口确保槽位分配，并标记为在栈上
                let (_slot_id, _) = self.env.get_or_create_slot_for_vreg(reg);
                s.insert(reg);
            } else {
                // 死变量，只从 W 中移除，不生成 spill
                if reg.to_string().contains("VX781") {
                    eprintln!(
                        "DEBUG apply_limit: VX781 is dead (Infinite), skipping spill generation"
                    );
                }
            }
        }

        spilled
    }

    /// 将所有暂存的指令修改提交到 VirtFunc
    fn commit_changes(&mut self) {
        for (block_id, edits) in std::mem::take(&mut self.pending_insts) {
            if let Some(block) = self.env.vfunc.block_datas.get_mut(&block_id) {
                let original_insts = std::mem::take(&mut block.insts);
                let mut new_insts = Vec::new();

                // 为每个位置收集要插入的指令
                let mut inserts_at_pos: BTreeMap<usize, Vec<MInst>> = BTreeMap::new();
                for (idx, insts) in edits {
                    inserts_at_pos.entry(idx).or_default().extend(insts);
                }

                // 将原始指令和新插入的指令合并
                for i in 0..=original_insts.len() {
                    if let Some(inserts) = inserts_at_pos.remove(&i) {
                        new_insts.extend(inserts);
                    }
                    if i < original_insts.len() {
                        new_insts.push(original_insts[i].clone());
                    }
                }

                block.insts = new_insts;
            }
        }
    }

    // ===================================================================
    // 辅助方法
    // ===================================================================

    /// 根据 next-use 距离排序并取前 n 个
    fn sort_and_take(
        &self,
        regs: &BTreeSet<Reg>,
        next_use: &BTreeMap<Reg, NextUseDistance>,
        n: usize,
    ) -> BTreeSet<Reg> {
        let mut sorted: Vec<_> = regs.iter().cloned().collect();
        sorted.sort_by_key(|r| {
            next_use
                .get(r)
                .cloned()
                .unwrap_or(NextUseDistance::Infinite)
        });
        sorted.into_iter().take(n).collect()
    }

    /// 按 next-use 距离排序
    fn sort_by_next_use(
        &self,
        w: &BTreeSet<Reg>,
        next_use: &BTreeMap<Reg, NextUseDistance>,
        protected: &BTreeSet<Reg>,
    ) -> Vec<Reg> {
        // 1. 将排序后的 protected 变量放在最前面
        let mut protected_list: Vec<_> = protected.iter().cloned().collect();
        protected_list.sort_by_key(|r| {
            next_use
                .get(r)
                .cloned()
                .unwrap_or(NextUseDistance::Infinite)
        });

        // 2. 非protected变量按next-use距离排序
        let mut non_protected: Vec<_> = w
            .iter()
            .filter(|r| !protected.contains(*r))
            .cloned()
            .collect();
        non_protected.sort_by_key(|r| {
            next_use
                .get(r)
                .cloned()
                .unwrap_or(NextUseDistance::Infinite)
        });

        // 3. 合并：protected在前，non_protected在后
        protected_list
            .into_iter()
            .chain(non_protected.into_iter())
            .collect()
    }

    /// 构建每条指令处的 next-use 信息
    /// 返回每个指令位置处的next-use信息（包括执行前和执行后的状态）
    fn build_inst_next_use(
        &self,
        block: &VirtBlockData,
        block_next_use: &crate::asm::liveness::NextUseInfo,
    ) -> (
        Vec<BTreeMap<Reg, NextUseDistance>>,
        Vec<BTreeMap<Reg, NextUseDistance>>,
    ) {
        let mut before_inst_next_uses = Vec::with_capacity(block.insts.len());
        let mut after_inst_next_uses = Vec::with_capacity(block.insts.len());

        // 从块的出口状态开始逆向推导
        let mut current_next_use = block_next_use.exit_next_use.clone();

        for inst in block.insts.iter().rev() {
            // 1. 当前状态就是指令执行后的状态
            let next_use_after_inst = current_next_use.clone();

            // 2. 基于当前状态，计算指令执行前的状态
            let mut next_use_before_inst = current_next_use; // a. 直接移动所有权

            // b. 所有距离增加1 (除了伪指令)
            if !inst.is_comment() && !inst.is_terminator() {
                for dist in next_use_before_inst.values_mut() {
                    *dist = dist.increment();
                }
            }

            // c. 应用指令的use/def效果
            let (uses, defs) = self.get_inst_use_def(inst);
            for def in defs {
                // 在这条指令之前，def的变量是不存在的，所以next-use信息无效
                next_use_before_inst.remove(&def);
            }
            for use_reg in uses {
                // 在这条指令之前，use的变量即将被使用
                next_use_before_inst.insert(use_reg, NextUseDistance::Immediate);
            }

            // 3. 存储计算出的 before 和 after 状态
            before_inst_next_uses.push(next_use_before_inst.clone());
            after_inst_next_uses.push(next_use_after_inst);

            // 4. 更新当前状态，为下一条指令(前一条)做准备
            current_next_use = next_use_before_inst;
        }

        // 5. 因为是逆向收集的，需要反转回来
        before_inst_next_uses.reverse();
        after_inst_next_uses.reverse();

        (before_inst_next_uses, after_inst_next_uses)
    }

    /// 获取指令的 use 和 def 集合
    fn get_inst_use_def(&self, inst: &MInst) -> (BTreeSet<Reg>, BTreeSet<Reg>) {
        // 使用指令自身的 use 和 def
        let uses = inst.get_uses().into_iter().collect();
        let defs = inst.get_defs().into_iter().collect();
        (uses, defs)
    }

    /// 将寄存器集合分为整数和浮点
    fn partition_regs(&self, regs: &BTreeSet<Reg>) -> (BTreeSet<Reg>, BTreeSet<Reg>) {
        regs.iter().cloned().partition(|r| r.is_xreg())
    }
}
