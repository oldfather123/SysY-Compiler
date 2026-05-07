//! 核心降级上下文

use super::{cfg::*, generated::selection::*, inst::*, prog::*, reg::*, stack::*};
use crate::{
    asm::inst,
    prelude::*,
    ssa::{ctx::*, ir::*},
};
use std::collections::{HashMap, HashSet};

/// 主降级上下文
pub struct Lower<'a> {
    // -------- 核心结构 --------
    cctx: &'a CoreContext,
    fctx: &'a FuncContext,
    vprog: &'a mut VirtProg, // 用于存放数据段
    /// 虚拟寄存器分配器
    vregs: VRegAllocator,

    // -------- VirtFunc 各个字段 --------
    // 为了降低耦合度, 直接把VirtFunc的各个字段拆分到Lower中
    func_ref: FuncRef,
    func_name: FuncName,
    exported: bool,
    block_datas: HashMap<VirtBlockId, VirtBlockData>,
    cfg: VirtControlFlowGraph,
    stack: StackLayout,
    pst_slot_map: HashMap<Reg, SlotId>, // 注意: IncomingArg是持久化槽位
    args: ArgPairVec,

    // -------- 状态映射 --------
    /// SSA Value => Reg 映射
    value_regs: RefSecMap<Value, Reg>,
    /// 已被下沉融合的指令
    inst_sunk: HashSet<Inst>,
    /// 已被记录为常量的指令
    inst_const: RefSparseMap<Inst, u64>,
    /// 需要插入mv的块参数位置
    edge_params: Vec<EdgeParams>,

    // -------- 副作用跟踪 --------
    /// 颜色计数器
    next_color: InstColor,
    /// 每个块结束的颜色
    block_end_color: RefSecMap<Block, InstColor>,
    /// 每指令进入前的颜色
    inst_entry_color: RefSparseMap<Inst, InstColor>,

    // -------- 指令融合 --------
    /// 分析每个 SSA 值的“传递性”使用状态
    value_ir_uses: RefSecMap<Value, ValueUseState>,
    /// 记录每个值实际被使用了多少次
    value_lowered_uses: RefSecMap<Value, u32>,

    // -------- 临时状态 --------
    /// 当前块
    cur_block: Option<VirtBlockId>,
    /// 当前指令
    cur_inst: Option<Inst>,
    /// 当前插入的指令
    cur_minsts: Vec<MInst>,
    /// 当前正在处理的指令的颜色
    cur_inst_entry_color: Option<InstColor>,
    /// 从SSA IR的StackSlot映射到后端的SlotId
    ir_slot_map: RefSecMap<StackSlot, SlotId>,

    // -------- 其他字段 --------
    debug_show: bool,
}

impl<'a> Lower<'a> {
    /// 创建新的 Lower 上下文
    pub fn new(
        cctx: &'a CoreContext,
        func_ref: FuncRef,
        fctx: &'a FuncContext,
        vprog: &'a mut VirtProg,
    ) -> Self {
        let debug_show = false;
        let debug_show = true;
        let mut lower = Self {
            cctx,
            fctx,
            vprog,
            vregs: VRegAllocator::new(),
            func_ref,
            func_name: fctx.func.name.clone(),
            exported: true,
            block_datas: HashMap::new(),
            cfg: VirtControlFlowGraph::build(fctx), // 使用build从FuncContext构建CFG
            stack: StackLayout::new(),
            pst_slot_map: HashMap::new(),
            args: Vec::new(),
            edge_params: Vec::new(),
            value_regs: RefSecMap::new(),
            inst_sunk: HashSet::new(),
            inst_const: RefSparseMap::new(),
            next_color: InstColor::new(1),
            block_end_color: RefSecMap::new(),
            inst_entry_color: RefSparseMap::new(),
            value_ir_uses: RefSecMap::new(),
            value_lowered_uses: RefSecMap::new(),
            cur_block: None,
            cur_inst: None,
            cur_minsts: Vec::new(),
            cur_inst_entry_color: None,
            ir_slot_map: RefSecMap::new(),
            debug_show,
        };

        // 预分配虚拟寄存器 & 初始化栈布局
        lower.init_alloc_vregs();
        lower.init_stack_layout();

        // 颜色初始化 & 指令融合初始化
        lower.init_inst_color();
        lower.init_value_use_state();

        lower
    }
}

// ===================================================================
// Layout 初始化 & 虚拟寄存器分配
// ===================================================================
impl Lower<'_> {
    /// 预分配虚拟寄存器给所有SSA值
    fn init_alloc_vregs(&mut self) {
        let func = &self.fctx.func;
        let dfg = &func.dfg;

        // 为所有块参数分配虚拟寄存器，并将它们添加到 VirtBlockData.params
        for (block, block_data) in dfg.blocks.iter() {
            let params_vec = dfg.valuevec(block_data.params).to_vec();

            // 过滤掉内存令牌，并分配虚拟寄存器
            let mut virt_params = Vec::new();
            for &param in &params_vec {
                let ty = dfg.value_type(param);
                if ty != Type::MemToken {
                    // 为非内存令牌参数分配虚拟寄存器
                    if let Some(vreg) = self.ensure_vreg(param) {
                        virt_params.push(vreg);
                    }
                }
            }

            // 将块参数添加到对应的 VirtBlockData
            let virt_block = VirtBlockId::BasicBlock { block };
            let block_data = self.ensure_block_data(virt_block);
            block_data.params = virt_params;
        }

        // 为所有指令结果分配虚拟寄存器
        let all_values: Vec<Value> = dfg.all_values().keys().collect();
        for value in all_values {
            self.ensure_vreg(value);
        }
    }

    /// 初始化栈布局
    fn init_stack_layout(&mut self) {
        // 1. 处理 incoming_args
        self.process_incoming_args();

        // 2. 处理 outgoing_args
        self.process_outgoing_args();

        // 3. 处理 extend_tail_args
        self.process_tail_call_args();

        // 4. 处理 fixed_objects
        self.process_fixed_objects();
    }

    /// 处理传入参数
    fn process_incoming_args(&mut self) {
        let func = &self.fctx.func;
        let entry_block = func.layout.entry_block().unwrap();
        let entry_params = func.dfg.block_params(entry_block);

        // 按照原始参数顺序处理，同时跟踪整数和浮点寄存器的使用
        let mut int_count = 0;
        let mut float_count = 0;

        for (&param, param_ty) in entry_params.iter().zip(&func.signature.params) {
            // 跳过内存令牌参数
            if self.is_memory_token(param_ty) {
                continue;
            }

            let &vreg = self
                .value_regs
                .get(param)
                .expect("需要为入口块参数预分配寄存器");

            let arg_pair = match param_ty {
                Type::Float32 | Type::Float64 => {
                    // 浮点参数
                    if float_count < CALL_FARGS_LEN {
                        // 寄存器传递
                        let preg = CALL_FARGS[float_count].into();
                        float_count += 1;
                        ArgPair::RegArg { vreg, preg }
                    } else {
                        // 栈传递
                        let (arg_pair, slot_id) =
                            self.create_stack_arg(vreg, param_ty, SlotKind::IncomingArg { vreg });
                        self.stack.add_incoming_arg(slot_id);
                        self.pst_slot_map.insert(vreg, slot_id); // IncomingArg是持久化槽位
                        arg_pair
                    }
                }
                _ => {
                    // 整数参数
                    if int_count < CALL_XARGS_LEN {
                        // 寄存器传递
                        let preg = CALL_XARGS[int_count].into();
                        int_count += 1;
                        ArgPair::RegArg { vreg, preg }
                    } else {
                        // 栈传递
                        let (arg_pair, slot_id) =
                            self.create_stack_arg(vreg, param_ty, SlotKind::IncomingArg { vreg });
                        self.stack.add_incoming_arg(slot_id);
                        self.pst_slot_map.insert(vreg, slot_id); // IncomingArg是持久化槽位
                        arg_pair
                    }
                }
            };

            self.args.push(arg_pair);
        }
    }

    /// 辅助方法: 计算函数调用需要的栈参数数量
    fn count_stack_args(&self, args: &ValueVec) -> usize {
        let dfg = &self.fctx.func.dfg;
        let arg_values = dfg.valuevec(*args);
        let mut int_count = 0;
        let mut float_count = 0;
        let mut stack_args_count = 0;

        for &arg_val in arg_values {
            let arg_ty = dfg.value_type(arg_val);

            // 过滤内存令牌
            if self.is_memory_token(&arg_ty) {
                continue;
            }

            let needs_stack = match arg_ty {
                Type::Float32 | Type::Float64 => {
                    float_count += 1;
                    float_count > CALL_FARGS_LEN
                }
                _ => {
                    int_count += 1;
                    int_count > CALL_XARGS_LEN
                }
            };

            if needs_stack {
                stack_args_count += 1;
            }
        }
        stack_args_count
    }

    /// 处理传出参数
    fn process_outgoing_args(&mut self) {
        let dfg = &self.fctx.func.dfg;
        let mut max_stack_args = 0;

        // 遍历所有Call指令
        for (_inst, inst_data) in dfg.insts.iter() {
            if let InstructionData::Call { args, .. } = inst_data {
                max_stack_args = max_stack_args.max(self.count_stack_args(args));
            }
        }

        // 创建传出参数栈槽
        self.create_stack_slots(max_stack_args, SlotKind::OutgoingArg, |stack, slot_id| {
            stack.add_outgoing_arg(slot_id)
        });

        self.stack
            .set_outgoing_args_size((max_stack_args * 8) as u32);
    }

    /// ReturnCall扩展incoming_args_size大小
    /// 理论上 incoming_args_size 应该由上层调用者负责
    /// 但为了ReturnCall直接复用栈帧, 选择调整大小
    /// 这部分扩展大小不是栈帧的一部分
    fn process_tail_call_args(&mut self) {
        let dfg = &self.fctx.func.dfg;
        let mut max_tail_stack_args = 0;

        // 预扫所有ReturnCall
        for (_inst, inst_data) in dfg.insts.iter() {
            if let InstructionData::ReturnCall { args, .. } = inst_data {
                max_tail_stack_args = max_tail_stack_args.max(self.count_stack_args(args));
            }
        }
        // 需要扩展size
        let incoming_size = self.stack.incoming_args_count();
        if max_tail_stack_args > incoming_size {
            let extend_count = max_tail_stack_args - incoming_size;

            self.create_stack_slots(extend_count, SlotKind::ExtendTailArg, |stack, slot_id| {
                stack.add_extend_tail_arg(slot_id)
            });

            self.stack.set_tail_args_size((extend_count * 8) as u32);
        }
    }

    /// 辅助方法: 创建栈槽
    fn create_stack_slots<F>(&mut self, count: usize, kind: SlotKind, mut add_slot_fn: F)
    where
        F: FnMut(&mut StackLayout, SlotId),
    {
        for _ in 0..count {
            let slot_data = SlotData {
                id: SlotId::invalid(),
                kind,
                size: 8,  // RISC-V 64-bit
                align: 3, // 2^3 = 8 bytes
                fp_offset: None,
                sp_offset: None,
            };
            let slot_id = self.stack.add_slot(slot_data);
            add_slot_fn(&mut self.stack, slot_id);
        }
    }

    /// 处理固定栈对象
    fn process_fixed_objects(&mut self) {
        let dfg = &self.fctx.func.dfg;
        // 处理所有的 StackSlot
        for (stack_slot, slot_data) in dfg.stack_slots.iter() {
            let slot = SlotData {
                id: SlotId::invalid(), // 由 add_slot 设置正确的 id
                kind: SlotKind::FixedObject {
                    ir_slot: stack_slot,
                },
                size: slot_data.size,
                align: slot_data.align,
                fp_offset: None,
                sp_offset: None,
            };

            // 初始化在核心lower中

            let slot_id = self.stack.add_slot(slot);
            self.stack.add_fixed_object(slot_id);

            // 记录 SSA StackSlot 到后端 SlotId 的映射
            self.ir_slot_map.insert(stack_slot, slot_id);
        }
    }
}

// ===================================================================
// 栈槽初始化
// ===================================================================

// 32字节以内内联，以上则调用库函数
const INLINE_INIT_THRESHOLD: usize = 32;

impl Lower<'_> {
    /// 基于阈值判断是否调用memcpy/memset来初始化
    fn should_invoke_libcall(size: usize) -> bool {
        size > INLINE_INIT_THRESHOLD
    }

    /// 栈槽初始化生成
    fn lower_stack_slot_init(&mut self) {
        self.emit(&MInst::Comment {
            label: "lower_stack_slot_init start".to_string(),
        });
        let dfg = &self.fctx.func.dfg;

        for (ir_slot, ir_slot_data) in dfg.stack_slots.iter() {
            let slot_id = self.ir_slot_map[ir_slot];
            let kind = self.stack.get_disc(slot_id);
            self.generate_slot_init(slot_id, kind, ir_slot_data);
        }

        self.emit(&MInst::Comment {
            label: "lower_stack_slot_init end".to_string(),
        });
    }

    /// 生成单个栈槽的初始化指令
    fn generate_slot_init(
        &mut self,
        slot_id: SlotId,
        kind: SlotKindDisc,
        slot_data: &StackSlotData,
    ) {
        if self.debug_show {
            eprintln!(
                "SlotInit: {slot_id} , slot_size={}, init={:?}",
                slot_data.size, slot_data.init
            );
        }
        let size = slot_data.size;
        match &slot_data.init {
            ArrayInit::Zero => {
                if Self::should_invoke_libcall(slot_data.size) {
                    self.init_by_memset(slot_id, kind, size)
                } else {
                    self.init_zero_inline(slot_id, kind, size)
                }
            }
            ArrayInit::List { vals } => {
                if Self::should_invoke_libcall(size) {
                    let data_bytes = self.collect_const_bytes(vals);
                    let label = self.vprog.ensure_rodata(data_bytes);
                    self.init_by_memcpy(slot_id, kind, label, size)
                } else {
                    self.init_const_inline(slot_id, kind, vals)
                }
            }
            ArrayInit::Undef => {}
        }
    }

    /// 使用memset初始化
    fn init_by_memset(&mut self, slot_id: SlotId, kind: SlotKindDisc, size: usize) {
        let addr_reg = self.vregs.alloc_x();
        let zero_reg = self.zero_reg();
        let size_reg = self.vregs.alloc_x();

        let _ = [
            MInst::AddrSlot {
                dst: addr_reg,
                slot: slot_id,
                kind,
            },
            MInst::Li {
                rd: size_reg,
                imm: size as i64,
            },
        ]
        .iter()
        .map(|inst| self.emit(inst))
        .collect::<Vec<_>>();

        // memset(void *s, int c, size_t n)
        let _ = self.emit_call_pseudo(
            &"memset".to_string(),
            None, //不需要使用返回值
            &vec![addr_reg.into(), zero_reg.into(), size_reg.into()],
            false,
        );
    }

    /// 使用memcpy初始化
    fn init_by_memcpy(&mut self, slot_id: SlotId, kind: SlotKindDisc, label: String, size: usize) {
        let dst_reg = self.vregs.alloc_x();
        let src_reg = self.vregs.alloc_x();
        let size_reg = self.vregs.alloc_x();

        let _ = [
            MInst::AddrSlot {
                dst: dst_reg,
                slot: slot_id,
                kind,
            },
            MInst::La { rd: src_reg, label },
            MInst::Li {
                rd: size_reg,
                imm: size as i64,
            },
        ]
        .iter()
        .map(|inst| self.emit(inst))
        .collect::<Vec<_>>();

        // memcpy(void *dest, const void *src, size_t n)
        let _ = self.emit_call_pseudo(
            &"memcpy".to_string(),
            None, //不需要使用返回值
            &vec![dst_reg.into(), src_reg.into(), size_reg.into()],
            false,
        );
    }

    /// 内联零初始化
    fn init_zero_inline(&mut self, slot_id: SlotId, kind: SlotKindDisc, size: usize) {
        use super::{inst::*, reg::*};
        let base_reg = self.vregs.alloc_x();
        let mut insts = vec![MInst::AddrSlot {
            dst: base_reg,
            slot: slot_id,
            kind,
        }];
        let zero = XReg::phys(RvXReg::Zero);

        let mut offset = 0;
        // 8字节对齐存储
        while offset + 8 <= size {
            insts.extend(
                self.emit_store_with_offset(base_reg, offset as i64, |r, o| MInst::Sd {
                    rs2: zero,
                    rs1: r,
                    offset: o,
                }),
            );
            offset += 8;
        }
        // 剩余4字节
        if offset + 4 <= size {
            insts.extend(
                self.emit_store_with_offset(base_reg, offset as i64, |r, o| MInst::Sw {
                    rs2: zero,
                    rs1: r,
                    offset: o,
                }),
            );
        }
        let _ = insts.iter().map(|inst| self.emit(inst)).collect::<Vec<_>>();
    }

    /// 内联常量初始化
    fn init_const_inline(&mut self, slot_id: SlotId, kind: SlotKindDisc, vals: &ValueVec) {
        use super::inst::*;
        let dfg = &self.fctx.func.dfg;
        let values = dfg.valuevec(*vals).to_vec();
        if values.is_empty() {
            return;
        }

        let base_reg = self.vregs.alloc_x();
        let mut insts = vec![MInst::AddrSlot {
            dst: base_reg,
            slot: slot_id,
            kind,
        }];

        // 处理每个常量值
        let mut offset = 0i64;
        for &val_ref in &values {
            if let ValueData::Const { c, .. } = dfg.valuedata(val_ref) {
                let (imm, is_float, size) = Self::parse_const_value(c);

                if is_float {
                    // 浮点：li -> fmv.x.x -> fsw/fsd
                    let xreg = self.vregs.alloc_x();
                    let freg = self.vregs.alloc_f();
                    insts.push(MInst::Li { rd: xreg, imm });
                    insts.push(if size == 4 {
                        MInst::FmvWX { fd: freg, rs: xreg }
                    } else {
                        MInst::FmvDX { fd: freg, rs: xreg }
                    });
                    insts.extend(self.emit_store_with_offset(base_reg, offset, |r, o| {
                        if size == 4 {
                            MInst::Fsw {
                                fs: freg,
                                rs: r,
                                offset: o,
                            }
                        } else {
                            MInst::Fsd {
                                fs: freg,
                                rs: r,
                                offset: o,
                            }
                        }
                    }));
                } else {
                    // 整数：li -> sw/sd
                    let val_reg = self.vregs.alloc_x();
                    insts.push(MInst::Li { rd: val_reg, imm });
                    insts.extend(self.emit_store_with_offset(base_reg, offset, |r, o| {
                        if size == 4 {
                            MInst::Sw {
                                rs2: val_reg,
                                rs1: r,
                                offset: o,
                            }
                        } else {
                            MInst::Sd {
                                rs2: val_reg,
                                rs1: r,
                                offset: o,
                            }
                        }
                    }));
                }
                offset += size as i64;
            }
        }
        for inst in &insts {
            self.emit(inst);
        }
    }
}

// ===================================================================
// 副作用跟踪 & 指令融合
// ===================================================================
impl Lower<'_> {
    /// 正向扫描颜色初始化
    fn init_inst_color(&mut self) {
        let func = &self.fctx.func;
        for ir_bb in func.layout.blocks() {
            // eprintln!("DEBUG: 处理块 {:?}, 当前颜色 {:?}", ir_bb, self.next_color);
            self.next_color = self.next_color.next();
            for inst in func.layout.block_insts(ir_bb) {
                let inst_data = func.dfg.insts[inst];
                // eprintln!(
                //     "  指令 {:?}: {:?}, 颜色 {:?}",
                //     inst, inst_data, self.next_color
                // );
                if inst_data.has_side_effect() {
                    //       ^^^^^^^^^^^^^^^^  控制流也被算作副作用指令
                    self.inst_entry_color.insert(inst, self.next_color);
                    self.next_color = self.next_color.next(); // <= 要确保副作用指令应该位于同色指令的最底部
                } else {
                    // ⚠️TODO: 不会记录每一条inst的颜色, 通过block_end_color反推即可?
                    // UPDATE: 直接记录完事
                    self.inst_entry_color.insert(inst, self.next_color);
                }
            }
            self.block_end_color.insert(ir_bb, self.next_color);
        }
    }

    /// 初始化传递唯一性分析
    fn init_value_use_state(&mut self) {
        let func = &self.fctx.func;

        // 获取value依赖的其他值. 用于传播Multiple
        let uses = |value: Value| {
            // 注意: 必须使用 value_def 自动解析别名
            match func.dfg.value_def(value) {
                &ValueData::Inst { inst, .. } => {
                    if self.is_root_inst(inst) {
                        None // 根指令阻断Multiple传播
                    } else {
                        // 返回指令的操作数
                        Some(func.dfg.inst_values(inst))
                    }
                }
                &ValueData::Const { .. } => None, // 常量可以任意复制，不需要传播Multiple
                &ValueData::BlockParam { .. } => None, // 块参数没有依赖的值
                &ValueData::GlobalSymbol { .. } => None, // 全局符号没有依赖的值
                &ValueData::Union { .. } | &ValueData::Alias { .. } => {
                    panic!("Lower阶段有未处理的ValueData")
                }
            }
        };

        // DFS 栈，用于传播 Multiple 状态
        let mut stack = Vec::new();

        // 初始化所有值为 Unused
        // 无需手动初始化, 已经设置了 RefSecMap 的IndexMut会自动调用 default()方法
        // 已经是 ValueUseState::Unused -- 但前提必须是 IndexMut, 如果调用Index则会panic
        let mut value_ir_uses: RefSecMap<Value, ValueUseState> = RefSecMap::new();

        for inst in func
            .layout
            .blocks()
            .flat_map(|ir_bb| func.layout.block_insts(ir_bb))
        {
            for arg in func.dfg.inst_values(inst) {
                let old_state = {
                    let _ = &mut value_ir_uses[arg];
                    value_ir_uses[arg]
                };
                value_ir_uses[arg].inc();
                let new_state = value_ir_uses[arg];

                // 当值第一次变为Multiple时，才启动 DFS 传播
                if old_state == ValueUseState::Multiple || new_state != ValueUseState::Multiple {
                    continue;
                }

                // 获取该值依赖的其他值(操作数)
                if let Some(deps) = uses(arg) {
                    stack.push(deps)
                }

                // DFS: 将整个依赖树标记为 Multiple
                while let Some(deps) = stack.last_mut() {
                    if let Some(dep_value) = deps.pop() {
                        // DFS剪枝: 如果已经是 Multiple，跳过
                        let dep_state = {
                            let _ = &mut value_ir_uses[dep_value];
                            value_ir_uses[dep_value]
                        };
                        if dep_state == ValueUseState::Multiple {
                            continue;
                        }
                        // 标记为 Multiple
                        value_ir_uses[dep_value] = ValueUseState::Multiple;
                        // 继续传播到这个值的依赖
                        if let Some(more_deps) = uses(dep_value) {
                            stack.push(more_deps)
                        }
                    } else {
                        // 当前层已处理完，弹出
                        stack.pop();
                    }
                }
            }
        }
        self.value_ir_uses = value_ir_uses;
    }

    #[inline]
    fn is_inst_sunk(&self, inst: Inst) -> bool {
        self.inst_sunk.contains(&inst)
    }

    #[inline]
    fn is_any_inst_result_need(&mut self, inst: Inst) -> bool {
        self.fctx.func.dfg.inst_results(inst).iter().any(|&result| {
            // 初始化所有值为 Unused (通过 IndexMut 触发 default())
            let _ = &mut self.value_lowered_uses[result];
            let use_count = self.value_lowered_uses[result];
            // eprintln!("检查值 {:?} 的使用计数: {}", result, use_count);
            use_count > 0
        })
    }
}
// ===================================================================
// 寄存器处理
// ===================================================================
impl Lower<'_> {
    /// 根据类型分配虚拟寄存器
    fn alloc_ty(&mut self, ty: &Type) -> Reg {
        match ty {
            Type::Float32 | Type::Float64 => self.vregs.alloc_f().into(),
            _ => self.vregs.alloc_x().into(),
        }
    }

    /// Value -> Reg 并过滤内存令牌
    pub fn ensure_vreg(&mut self, value: Value) -> Option<Reg> {
        if let Some(&reg) = self.value_regs.get(value) {
            return Some(reg);
        }
        let dfg = &self.fctx.func.dfg;
        let value_data = dfg.all_values().get(value)?;
        let ty = value_data.ty();
        // 如果是内存令牌，返回 None
        if self.is_memory_token(&ty) {
            return None;
        }
        // 对于指令结果，检查是否应该跳过
        if let ValueData::Inst { inst, .. } = value_data {
            if let Some(inst_data) = dfg.insts.get(*inst) {
                if self.should_skip_inst(inst_data) {
                    return None; //TODO: 跳过高级内存操作指令, 理论上Lower阶段这些指令不应该存在
                }
            }
        }
        let vreg = self.alloc_ty(&ty);
        self.value_regs.insert(value, vreg);
        Some(vreg)
    }

    /// Vec<Value> -> Vec<Reg>
    pub fn ensure_vregs<'v>(&mut self, values: impl IntoIterator<Item = &'v Value>) -> Vec<Reg> {
        values
            .into_iter()
            .filter_map(|&v| self.ensure_vreg(v))
            .collect()
    }

    /// 记录块参数  
    fn record_block_call(&mut self, this: VirtBlockId, inst: Inst) {
        let bcalls: &[BlockCall] = self.fctx.func.dfg.inst_destination(inst);
        let successors: Vec<_> = self.cfg.successors(this).collect();
        let succ_count = successors.len();

        use crate::asm::prog::ParamTransfer;
        use VirtBlockId::*;

        // 先收集所有的块参数和目标参数
        let param_data: Vec<_> = bcalls
            .iter()
            .map(|bcall| {
                let ir_block = bcall.block;
                let ir_args = self.fctx.func.dfg.valuevec(bcall.args);
                let ir_args_vec: Vec<_> = ir_args.iter().collect();
                let target_params = self.fctx.func.dfg.block_params(ir_block);
                let target_params_vec: Vec<_> = target_params.iter().collect();
                (ir_block, ir_args_vec, target_params_vec)
            })
            .collect();
        // 批量转换为寄存器并生成EdgeParams
        let edge_params_to_add: Vec<_> = param_data
            .into_iter()
            .zip(successors)
            .filter_map(|((ir_block, ir_args, target_params), succ)| {
                // 构建参数传递列表
                let mut transfers = Vec::new();

                // 先过滤掉 MemToken 类型的参数，确保参数列表和寄存器列表对齐
                let non_token_pairs: Vec<(Value, Value)> = ir_args
                    .iter()
                    .zip(target_params.iter())
                    .filter_map(|(&&arg, &&target)| {
                        // 检查目标参数类型，过滤掉 MemToken
                        let target_ty = self.fctx.func.dfg.value_type(target);
                        if matches!(target_ty, Type::MemToken) {
                            None
                        } else {
                            Some((arg, target))
                        }
                    })
                    .collect();

                // 提取过滤后的参数列表
                let filtered_args: Vec<Value> =
                    non_token_pairs.iter().map(|&(arg, _)| arg).collect();
                let filtered_targets: Vec<Value> =
                    non_token_pairs.iter().map(|&(_, target)| target).collect();

                // 为过滤后的目标参数分配寄存器
                let target_regs: Vec<Reg> = self
                    .ensure_vregs(filtered_targets.iter())
                    .into_iter()
                    .collect();

                // 处理每个参数对
                for ((&arg_value, &target_param), &target_reg) in filtered_args
                    .iter()
                    .zip(&filtered_targets)
                    .zip(&target_regs)
                {
                    // 先解析别名，获取实际的值
                    let resolved_arg = self.fctx.func.dfg.resolve_aliases(arg_value);
                    self.value_lowered_uses[resolved_arg] += 1;

                    // 检查是否为常量
                    if let ValueData::Const { c, .. } = self.fctx.func.dfg.valuedata(resolved_arg) {
                        // 常量参数，需要加载
                        transfers.push(ParamTransfer::LoadConst(*c, target_reg));
                    } else if let Some(src_reg) = self.ensure_vreg(resolved_arg) {
                        // 非常量参数，寄存器移动
                        transfers.push(ParamTransfer::Move(src_reg, target_reg));
                    }
                }

                // 如果没有参数需要传递，返回None
                (!transfers.is_empty())
                    .then(|| {
                        // 根据不同情况决定 mv 指令的插入位置
                        match (this, succ) {
                            // 情况一: Block A(当前) ---> CriticalEdge(插入mv) ---> Block C
                            (_, edge @ CriticalEdge { .. }) => Some(EdgeParams {
                                transfers: transfers.clone(),
                                move_loc: edge,
                            }),
                            // 情况二: Block A ---> CriticalEdge(当前) ---> Block C(不做任何操作)
                            (CriticalEdge { .. }, _) => None,
                            // 情况三: Block A(当前,插入mv) ---> Block C(单后继)
                            // 情况四: Block A(当前) ---> Block C(多后继,插入mv)
                            (_, _) => Some(EdgeParams {
                                transfers,
                                move_loc: if succ_count == 1 { this } else { succ },
                            }),
                        }
                    })
                    .flatten()
            })
            .collect();

        // DEBUG: 打印即将添加的 edge_params
        // eprintln!("  Adding {} edge_params to list", edge_params_to_add.len());
        // for ep in &edge_params_to_add {
        // eprintln!(
        // "    EdgeParams: move_loc={:?}, transfers={:?}",
        // ep.move_loc, ep.transfers
        // );
        // }

        self.edge_params.extend(edge_params_to_add);
    }
}

// ===================================================================
// 块数据处理
// ===================================================================
impl Lower<'_> {
    #[inline]
    pub fn ensure_block_data(&mut self, bidx: VirtBlockId) -> &mut VirtBlockData {
        self.block_datas.entry(bidx).or_default()
    }
}

// ===================================================================
// 指令生成
// ===================================================================
impl Lower<'_> {
    /// 生成带偏移的存储指令
    fn emit_store_with_offset<F>(
        &mut self,
        base: XReg,
        offset: i64,
        mut store_fn: F,
    ) -> Vec<VirtInst>
    where
        F: FnMut(XReg, Imm12) -> VirtInst,
    {
        use super::inst::*;
        if is_imm12_valid(offset) {
            vec![store_fn(base, offset as Imm12)]
        } else {
            let offset_reg = self.vregs.alloc_x();
            let addr_reg = self.vregs.alloc_x();
            vec![
                MInst::Li {
                    rd: offset_reg,
                    imm: offset,
                },
                MInst::Add {
                    rd: addr_reg,
                    rs1: base,
                    rs2: offset_reg,
                },
                store_fn(addr_reg, 0),
            ]
        }
    }

    #[inline]
    fn emit_cur_minsts(&mut self) {
        let cur_block = self.cur_block.expect("no current block");
        // 使用 std::mem::take 避免借用冲突，并直接反转原地修改
        let mut minsts = std::mem::take(&mut self.cur_minsts);
        minsts.reverse();
        let block_data = self.ensure_block_data(cur_block);
        block_data.extend_insts(minsts);
    }
}

// ===================================================================
// Context trait 实现
// ===================================================================

use super::generated::selection::Context;

impl Context for Lower<'_> {
    fn i32_add(&mut self, arg0: i32, arg1: i32) -> i32 {
        arg0.wrapping_add(arg1)
    }

    fn i64_add(&mut self, arg0: i64, arg1: i64) -> i64 {
        arg0.wrapping_add(arg1)
    }

    fn imm64_from_iconst_add(&mut self, x: Value, y: Value) -> Imm64 {
        let x_val = self.iconst(x).unwrap_or(0);
        let y_val = self.iconst(y).unwrap_or(0);
        x_val.wrapping_add(y_val)
    }

    fn option_reg_from_reg(&mut self, arg0: Reg) -> OptionReg {
        Some(arg0)
    }

    fn option_reg_from_xreg(&mut self, arg0: XReg) -> OptionReg {
        Some(arg0.into())
    }

    fn option_reg_from_freg(&mut self, arg0: FReg) -> OptionReg {
        Some(arg0.into())
    }

    fn put_in_reg(&mut self, value: Value) -> Reg {
        // eprintln!("DEBUG: put_in_reg 开始，值 {:?}", value);
        // 增加使用计数:
        self.value_lowered_uses[value] += 1;

        // 尝试获取已分配的寄存器
        if let Some(&reg) = self.value_regs.get(value) {
            // eprintln!("DEBUG: put_in_reg - 值 {:?} 已经有寄存器 {:?}", value, reg);
            return reg;
        }

        // 调试：打印更多信息
        eprintln!(
            "DEBUG: put_in_reg - 值 {:?} 没有找到预分配的寄存器！",
            value
        );
        eprintln!("  value_regs 中的所有条目数量: {}", self.value_regs.len());

        // 检查该值是否在 dfg.all_values() 中
        if let Some(value_data) = self.fctx.func.dfg.all_values().get(value) {
            eprintln!("  值在 dfg.all_values() 中存在: {:?}", value_data);
            eprintln!("  值的类型: {:?}", value_data.ty());

            // 检查是否是指令结果
            if let ValueData::Inst { inst, .. } = value_data {
                eprintln!("  这是指令 {:?} 的结果", inst);
                if let Some(inst_data) = self.fctx.func.dfg.insts.get(*inst) {
                    eprintln!("  指令数据: {:?}", inst_data);
                }
            }
        } else {
            eprintln!("  值不在 dfg.all_values() 中！");
        }

        // 处理常量
        let dfg = &self.fctx.func.dfg;
        eprintln!(
            "DEBUG: put_in_reg - 检查值 {:?}, valuedata = {:?}",
            value,
            dfg.valuedata(value)
        );
        if let ValueData::Const { c, .. } = dfg.valuedata(value) {
            let ty = dfg.value_type(value);
            let reg = self.alloc_ty(&ty);

            // 生成常量加载指令
            match c {
                ConstValue::Int32 { val } => {
                    let xreg = XReg::try_from(reg).unwrap();
                    self.emit(&MInst::Li {
                        rd: xreg,
                        imm: *val as i64,
                    });
                }
                ConstValue::Int64 { val } => {
                    let xreg = XReg::try_from(reg).unwrap();
                    self.emit(&MInst::Li {
                        rd: xreg,
                        imm: *val,
                    });
                }
                ConstValue::Float32 { val } => {
                    let freg = FReg::try_from(reg).unwrap();
                    let label = self
                        .vprog
                        .ensure_rodata(val.to_bits().to_le_bytes().to_vec());
                    // 生成浮点加载指令: la temp_reg, label; flw freg, 0(temp_reg)
                    let temp_xreg = self.vregs.alloc_x();
                    self.emit(&MInst::La {
                        rd: temp_xreg,
                        label: label.clone(),
                    });
                    self.emit(&MInst::Flw {
                        fd: freg,
                        rs: temp_xreg,
                        offset: 0,
                    });
                }
                ConstValue::Float64 { val } => {
                    let freg = FReg::try_from(reg).unwrap();
                    let label = self
                        .vprog
                        .ensure_rodata(val.to_bits().to_le_bytes().to_vec());
                    // 生成浮点加载指令: la temp_reg, label; fld freg, 0(temp_reg)
                    let temp_xreg = self.vregs.alloc_x();
                    self.emit(&MInst::La {
                        rd: temp_xreg,
                        label: label.clone(),
                    });
                    self.emit(&MInst::Fld {
                        fd: freg,
                        rs: temp_xreg,
                        offset: 0,
                    });
                }
                _ => {}
            }

            self.value_regs.insert(value, reg);
            reg
        } else {
            // 分配新寄存器
            let ty = dfg.value_type(value);
            let reg = self.alloc_ty(&ty);
            self.value_regs.insert(value, reg);
            reg
        }
    }

    fn put_in_xreg(&mut self, value: Value) -> XReg {
        // eprintln!("DEBUG: put_in_xreg 调用，值 {:?}", value);
        let reg = self.put_in_reg(value);
        // eprintln!("DEBUG: put_in_xreg 返回寄存器 {:?}", reg);
        XReg::try_from(reg).unwrap()
    }

    fn put_in_freg(&mut self, value: Value) -> FReg {
        FReg::try_from(self.put_in_reg(value)).unwrap()
    }

    fn put_in_regs_vec(&mut self, args: &ValueSlice) -> RegsVec {
        // 过滤内存令牌，只转换实际需要传递的参数
        let dfg = &self.fctx.func.dfg;
        let mut regs = Vec::new();

        for &v in args.iter() {
            let ty: Type = dfg.value_type(v);

            // 跳过内存令牌
            if matches!(ty, Type::MemToken) {
                continue;
            }

            // 根据类型选择合适的处理方式
            let reg = if let Some(_) = self.ty_fit_freg(&ty) {
                // 浮点类型使用 f_operand
                let freg = constructor_f_operand(self, v);
                freg.into()
            } else {
                // 整数类型使用 x_operand
                let xreg = constructor_x_operand(self, v);
                xreg.into()
            };

            regs.push(reg);
        }

        regs
    }

    fn reg_new(&mut self, ty: &Type) -> Reg {
        self.alloc_ty(ty)
    }

    fn xreg_new(&mut self) -> XReg {
        self.vregs.alloc_x()
    }

    fn freg_new(&mut self) -> FReg {
        self.vregs.alloc_f()
    }

    fn xreg_to_reg(&mut self, arg0: XReg) -> Reg {
        arg0.into()
    }

    fn freg_to_reg(&mut self, arg0: FReg) -> Reg {
        arg0.into()
    }

    fn reg_to_xreg(&mut self, arg0: XReg) -> Option<Reg> {
        Some(arg0.into())
    }

    fn reg_to_freg(&mut self, arg0: FReg) -> Option<Reg> {
        Some(arg0.into())
    }

    fn zero_reg(&mut self) -> XReg {
        XReg::phys(RvXReg::Zero)
    }

    fn imm12_from_offset(&mut self, offset: Offset) -> Imm12 {
        offset as Imm12
    }

    fn imm12_from_i64(&mut self, val: i64) -> Option<Imm12> {
        if is_imm12_valid(val) {
            Some(val as Imm12)
        } else {
            None
        }
    }

    fn imm64_from_i64(&mut self, val: i64) -> Imm64 {
        val
    }

    fn imm64_from_i32(&mut self, val: i32) -> Imm64 {
        val as i64
    }

    fn label_from_fconst32(&mut self, val: f32) -> Label {
        let bytes = val.to_bits().to_le_bytes().to_vec();
        self.vprog.ensure_rodata(bytes)
    }

    fn label_from_fconst64(&mut self, val: f64) -> Label {
        let bytes = val.to_bits().to_le_bytes().to_vec();
        self.vprog.ensure_rodata(bytes)
    }

    fn global_to_label(&mut self, global: Global) -> Label {
        self.vprog
            .global_map
            .get(&global)
            .cloned()
            .unwrap_or_else(|| format!("global_{}", global.index()))
    }

    fn func_to_label(&mut self, func_ref: FuncRef) -> Label {
        // 从外部函数信息获取函数名
        self.cctx
            .ext_funcs
            .get(func_ref)
            .map(|f| f.name.clone())
            .unwrap_or_else(|| {
                // 如果不是外部函数，从当前函数上下文获取
                self.fctx.func.name.clone()
            })
    }

    fn stackslot_to_slotid(&mut self, stack_slot: StackSlot) -> SlotId {
        self.ir_slot_map[stack_slot]
    }

    fn stackslot_to_kind(&mut self, arg0: StackSlot) -> SlotKindDisc {
        let id = self.ir_slot_map[arg0];
        self.stack
            .get_slotdata(id)
            .expect("居然有未分配的StackSlot!")
            .kind
            .into()
    }

    fn unit(&mut self) -> Unit {
        Unit
    }

    fn i32_to_i64(&mut self, val: i32) -> i64 {
        val as i64
    }

    fn valuevec_slice(&mut self, vec: ValueVec) -> ValueSlice {
        // ValueSlice是Vec<Value>，而ValueVec是向量池的键
        self.fctx.func.dfg.valuevec(vec).clone()
    }

    fn value_slice_first(&mut self, arg0: &ValueSlice) -> Value {
        arg0[0]
    }

    fn value_is_unused(&mut self, value: Value) -> bool {
        // 检查值是否被使用
        self.value_lowered_uses[value] == 0
    }

    fn def_inst(&mut self, value: Value) -> Option<Inst> {
        let dfg = &self.fctx.func.dfg;
        match dfg.value_def(value) {
            ValueData::Inst { inst, .. } => Some(*inst),
            _ => None,
        }
    }

    fn value_type(&mut self, value: Value) -> Type {
        self.fctx.func.dfg.value_type(value)
    }

    fn value_data(&mut self, value: Value) -> ValueData {
        *self.fctx.func.dfg.valuedata(value)
    }

    fn inst_results(&mut self, inst: Inst) -> ValueSlice {
        let results = self.fctx.func.dfg.inst_results(inst);
        // 返回结果的值向量
        results.to_vec()
    }

    fn first_result(&mut self, inst: Inst) -> Option<Value> {
        let dfg = &self.fctx.func.dfg;
        let results = dfg.inst_results(inst);
        // 过滤掉内存令牌，返回第一个非内存令牌的值
        self.first_valid_value(results)
    }

    fn no_inst_result(&mut self, inst: Inst) -> Option<Inst> {
        let dfg = &self.fctx.func.dfg;
        let results = dfg.inst_results(inst);
        if self.first_valid_value(results).is_none() {
            Some(inst)
        } else {
            None
        }
    }

    #[inline]
    fn first_argument(&mut self, arg0: Inst) -> Option<Value> {
        self.fctx.func.dfg.inst_args(arg0).first().copied()
    }

    #[inline]
    fn inst_data_value(&mut self, inst: Inst) -> InstructionData {
        self.fctx.func.dfg.insts[inst]
    }

    // Type extractors
    fn ty_unit(&mut self, ty: &Type) -> Option<Type> {
        match ty {
            Type::Unit => Some(*ty),
            _ => None,
        }
    }

    fn ty_bool(&mut self, ty: &Type) -> Option<Type> {
        match ty {
            Type::Bool => Some(*ty),
            _ => None,
        }
    }

    fn ty_i32(&mut self, ty: &Type) -> Option<Type> {
        match ty {
            Type::Int32 => Some(*ty),
            _ => None,
        }
    }

    fn ty_i64(&mut self, ty: &Type) -> Option<Type> {
        match ty {
            Type::Int64 => Some(*ty),
            _ => None,
        }
    }

    fn ty_f32(&mut self, ty: &Type) -> Option<Type> {
        match ty {
            Type::Float32 => Some(*ty),
            _ => None,
        }
    }

    fn ty_f64(&mut self, ty: &Type) -> Option<Type> {
        match ty {
            Type::Float64 => Some(*ty),
            _ => None,
        }
    }

    fn ty_no_ret(&mut self, ty: &Type) -> Option<()> {
        match ty {
            Type::Unit => Some(()),
            _ => None,
        }
    }

    fn ty_fit_xreg(&mut self, ty: &Type) -> Option<()> {
        match ty {
            Type::Bool | Type::Int32 | Type::Int64 => Some(()),
            _ => None,
        }
    }

    fn ty_fit_freg(&mut self, ty: &Type) -> Option<()> {
        match ty {
            Type::Float32 | Type::Float64 => Some(()),
            _ => None,
        }
    }

    /// 注意只有Int32具是有符号的
    fn ty_fit_signed(&mut self, ty: &Type) -> Option<()> {
        match ty {
            Type::Int32 => Some(()),
            _ => None,
        }
    }

    /// Int64是无符号的
    fn ty_fit_unsigned(&mut self, ty: &Type) -> Option<()> {
        match ty {
            Type::Int64 => Some(()),
            _ => None,
        }
    }

    fn iconst(&mut self, value: Value) -> Option<i64> {
        let dfg = &self.fctx.func.dfg;
        let value_data = dfg.valuedata(value);
        // eprintln!(
        //     "DEBUG: iconst checking value {:?}, valuedata = {:?}",
        //     value, value_data
        // );
        match value_data {
            ValueData::Const { c, .. } => {
                // eprintln!("DEBUG: iconst found Const, c = {:?}", c);
                match c {
                    ConstValue::Int32 { val } => {
                        // eprintln!("DEBUG: iconst returning Some({}) for Int32", val);
                        Some(*val as i64)
                    }
                    ConstValue::Int64 { val } => {
                        // eprintln!("DEBUG: iconst returning Some({}) for Int64", val);
                        Some(*val)
                    }
                    _ => {
                        // eprintln!("DEBUG: iconst returning None - not Int32/Int64");
                        None
                    }
                }
            }
            _ => {
                // eprintln!("DEBUG: iconst returning None - not a Const");
                None
            }
        }
    }

    fn i64_from_iconst(&mut self, value: Value) -> Option<i64> {
        self.iconst(value)
    }

    fn label_from_block_call(&mut self, bcall: BlockCall) -> Label {
        // 将SSA块转换为虚拟块ID，然后生成标签
        let virt_block = VirtBlockId::BasicBlock { block: bcall.block };
        virt_block.to_label(self.func_ref)
    }

    fn unpack_block_call_2(&mut self, bcalls: &BlockCall2) -> (BlockCall, BlockCall) {
        (bcalls[0], bcalls[1])
    }

    fn pack_block_call_2(&mut self, bcall0: BlockCall, bcall1: BlockCall) -> BlockCall2 {
        [bcall0, bcall1]
    }

    fn emit(&mut self, inst: &MInst) -> Unit {
        // 块的每一条指令是倒序emit的,
        // 而每一条指令生成一系列指令却是正序生成的,
        // 因此需要用一个cur_minsts
        self.cur_minsts.push(inst.clone());
        Unit
    }

    fn single_target(&mut self, targets: &LabelVec) -> Option<Label> {
        if targets.len() == 1 {
            Some(targets[0].clone())
        } else {
            None
        }
    }

    fn two_targets(&mut self, targets: &LabelVec) -> Option<(Label, Label)> {
        if targets.len() == 2 {
            Some((targets[0].clone(), targets[1].clone()))
        } else {
            None
        }
    }

    fn jump_table_size(&mut self, size: u32) -> Option<LabelVec> {
        // 生成跳转表标签
        let mut labels = Vec::new();
        for i in 0..size {
            labels.push(format!(".Ljt_{}", i));
        }
        Some(labels)
    }

    /// 从值检查指令是否可以下沉
    fn peek_def_inst(&mut self, arg0: Value) -> Option<Inst> {
        // 1. 首先值必须是指令的结果
        if let Some(inst) = self.def_inst(arg0) {
            let cur_inst = self.cur_inst.unwrap();
            let inst_data = self.fctx.func.dfg.insts[inst];
            // eprintln!(
            //     "DEBUG: peek_def_inst 检查 {:?} from inst {:?}",
            //     arg0, inst_data
            // );

            //2. 检查 inst 和 cur_inst 的颜色是否相同
            let cur_inst_data = self.fctx.func.dfg.insts[cur_inst];
            // eprintln!("DEBUG: cur_inst是 {:?}: {:?}", cur_inst, cur_inst_data);
            let cur_color = self.inst_entry_color[cur_inst];
            let inst_color = self.inst_entry_color[inst];
            // eprintln!(
            //     "DEBUG: 颜色检查 - cur_inst({:?})颜色: {:?}, inst({:?})颜色: {:?}",
            //     cur_inst, cur_color, inst, inst_color
            // );
            if cur_color == inst_color {
                //3. 检查 arg0 的使用次数
                use ValueUseState::*;
                let use_state = self.value_ir_uses[arg0];
                // eprintln!("DEBUG: 值 {:?} 使用状态: {:?}", arg0, use_state);
                return match use_state {
                    Unused => unreachable!("值肯定被使用了"),
                    Once => Some(inst), // 只使用一次肯定能融合
                    Multiple => {
                        // 4. 检查是否是副作用指令 & sink 成本
                        let inst_data = &self.fctx.func.dfg.insts[inst];
                        if inst_data.has_expensive_cost() && inst_data.has_side_effect() {
                            None // 成本太高，不融合
                        } else {
                            Some(inst) // 成本低，可以融合
                        }
                    }
                };
            }
            return None;
        }
        None
    }

    fn sink_inst(&mut self, inst: Inst) -> Unit {
        self.inst_sunk.insert(inst);
        // 已经被融合, 因此不可能出现 ValueData::Inst {} 被 put_in_reg 来增加计数的情况
        for result in self.fctx.func.dfg.inst_results(inst) {
            let _ = &mut self.value_lowered_uses[*result];
            assert!(self.value_lowered_uses[*result] == 0);
        }
        Unit
    }

    fn lower_br_table(&mut self, idx: XReg, targets: &LabelVec) -> Unit {
        // 生成跳转表实现
        // 实现方式：生成一系列条件跳转指令
        // 更高效的实现可以使用实际的跳转表，但需要更复杂的处理

        let temp_reg = self.vregs.alloc_x();

        for (i, target) in targets.iter().enumerate() {
            // li temp_reg, i
            self.emit(&MInst::Li {
                rd: temp_reg,
                imm: i as i64,
            });

            // beq idx, temp_reg, target
            self.emit(&MInst::Beq {
                rs1: idx,
                rs2: temp_reg,
                label: target.clone(),
            });
        }

        // 如果没有匹配，跳转到最后一个目标（默认分支）
        if let Some(default) = targets.last() {
            self.emit(&MInst::J {
                label: default.clone(),
            });
        }

        Unit
    }

    /// 生成CallPseudo/RetCallPseudo指令并处理ABI约束
    fn emit_call_pseudo(
        &mut self,
        label: &Label,
        ret: OptionReg,
        args: &RegsVec,
        is_retcall: bool,
    ) -> Unit {
        // ⚠️TODO: 应该cctx查询调用规范，来处理printf之类的函数

        // 预处理栈参数冲突：检测并解决传出栈参数使用传入栈参数的情况
        let processed_args = self.preprocess_stack_arg_conflicts_v2(args, is_retcall);

        let mut int_count = 0;
        let mut float_count = 0;
        let mut stack_count = 0;
        let mut arg_pairs = Vec::new();

        // 按照原始参数顺序处理
        for arg in processed_args {
            let arg_pair = if arg.is_freg() {
                // 浮点参数
                if float_count < CALL_FARGS_LEN {
                    // 寄存器传递
                    let preg = CALL_FARGS[float_count].into();
                    float_count += 1;
                    ArgPair::RegArg { vreg: arg, preg }
                } else {
                    // 栈传递
                    let slot_id = if is_retcall {
                        self.stack
                            .get_tail_arg(stack_count)
                            .expect("必须已经预分配")
                    } else {
                        self.stack
                            .get_outgoing_arg(stack_count)
                            .expect("必须已经预分配")
                    };
                    let kind = self.stack.get_disc(slot_id);
                    stack_count += 1;

                    // 插入Spill指令将参数存到outgoing_args栈上
                    self.emit(&MInst::Spill {
                        reg: arg,
                        slot: slot_id,
                        kind,
                    });

                    ArgPair::StackArg {
                        vreg: arg,
                        slot: slot_id,
                        kind,
                    }
                }
            } else {
                // 整数参数
                if int_count < CALL_XARGS_LEN {
                    // 寄存器传递
                    let preg = CALL_XARGS[int_count].into();
                    int_count += 1;
                    ArgPair::RegArg { vreg: arg, preg }
                } else {
                    // 栈传递
                    let slot_id = if is_retcall {
                        self.stack
                            .get_tail_arg(stack_count)
                            .expect("必须已经预分配")
                    } else {
                        self.stack
                            .get_outgoing_arg(stack_count)
                            .expect("必须已经预分配")
                    };
                    let kind = self.stack.get_disc(slot_id);
                    stack_count += 1;

                    // 插入Spill指令将参数存到outgoing_args栈上
                    self.emit(&MInst::Spill {
                        reg: arg,
                        slot: slot_id,
                        kind,
                    });

                    ArgPair::StackArg {
                        vreg: arg,
                        slot: slot_id,
                        kind,
                    }
                }
            };

            arg_pairs.push(arg_pair);
        }

        // 确定返回值
        let ret = match ret {
            Some(vreg) if vreg.is_xreg() => ArgPair::RegArg {
                vreg,
                preg: CALL_RET_XREG.into(),
            },
            Some(vreg) => ArgPair::RegArg {
                vreg,
                preg: CALL_RET_FREG.into(),
            },
            _ => ArgPair::None,
        };

        // 生成CallPseudo指令
        if is_retcall {
            assert_eq!(ret, ArgPair::None);
            self.emit(&MInst::RetCallPseudo {
                label: label.clone(),
                args: arg_pairs,
            });
        } else {
            self.emit(&MInst::CallPseudo {
                label: label.clone(),
                ret,
                args: arg_pairs,
            });
        }

        Unit
    }

    fn emit_ret_pseudo(&mut self, arg0: OptionReg) -> Unit {
        if let Some(arg0) = arg0 {
            if arg0.is_xreg() {
                self.emit(&MInst::RetPseudo {
                    ret: ArgPair::RegArg {
                        vreg: arg0.try_into().unwrap(),
                        preg: CALL_RET_XREG.into(),
                    },
                });
            } else {
                self.emit(&MInst::RetPseudo {
                    ret: ArgPair::RegArg {
                        vreg: arg0.try_into().unwrap(),
                        preg: CALL_RET_FREG.into(),
                    },
                });
            }
        } else {
            self.emit(&MInst::RetPseudo { ret: ArgPair::None });
        }
        Unit
    }
}

// ===================================================================
// 辅助方法
// ===================================================================
impl Lower<'_> {
    /// 检查类型是否为内存令牌
    #[inline]
    fn is_memory_token(&self, ty: &Type) -> bool {
        matches!(ty, Type::MemToken)
    }

    /// 过滤掉内存令牌，返回第一个非内存令牌的值
    #[inline]
    fn first_valid_value(&self, values: &[Value]) -> Option<Value> {
        let dfg = &self.fctx.func.dfg;
        values.iter().copied().find(|&v| {
            let ty = dfg.value_type(v);
            !matches!(ty, Type::MemToken)
        })
    }

    /// 检查指令是否应该被过滤（高级内存指令）
    fn should_skip_inst(&self, inst_data: &InstructionData) -> bool {
        if matches!(
            inst_data,
            InstructionData::ArrayAlloc { .. }
                | InstructionData::ArrayGet { .. }
                | InstructionData::ArrayPut { .. }
                | InstructionData::GlobalScalarGet { .. }
                | InstructionData::GlobalScalarSet { .. }
        ) {
            panic!(
                "居然有高级内存指令，说明低级地址展开没有奏效: {:?}",
                inst_data
            );
        } else {
            false
        }
    }

    /// 收集常量数据的字节表示
    fn collect_const_bytes(&self, vals: &ValueVec) -> Vec<u8> {
        let mut data = Vec::new();
        let dfg = &self.fctx.func.dfg;

        for &val in dfg.valuevec(*vals) {
            if let ValueData::Const { c, .. } = dfg.valuedata(val) {
                data.extend(c.to_abi_bytes());
            }
        }
        data
    }

    /// 解析常量值为 (立即数, 是否浮点, 字节大小)
    fn parse_const_value(c: &ConstValue) -> (i64, bool, usize) {
        match c {
            ConstValue::Int32 { val } => (*val as i64, false, 4),
            ConstValue::Int64 { val } => (*val, false, 8),
            ConstValue::Bool { val } => (if *val { 1 } else { 0 }, false, 4),
            ConstValue::Float32 { val } => (val.to_bits() as i64, true, 4),
            ConstValue::Float64 { val } => (val.to_bits() as i64, true, 8),
            ConstValue::Unit => (0, false, 4),
        }
    }

    /// 判断是否是根指令: NEVER
    /// 所谓根指令就是指令结果数量大于1, 但是在RISCV中, 大部分指令全是单结果的, 因此不存在root inst
    #[inline]
    fn is_root_inst(&self, inst: Inst) -> bool {
        false
    }

    /// 预处理栈参数冲突：检测并解决使用传入栈参数的情况
    ///
    /// 问题场景：

    /// 预处理栈参数冲突（保持原始顺序）
    /// 在函数调用时，栈上传出参数的位置可能会覆盖传入栈参数。
    /// 如果传出参数使用了传入栈参数的值，需要先加载到临时寄存器。
    fn preprocess_stack_arg_conflicts_v2(&mut self, args: &[Reg], _is_retcall: bool) -> Vec<Reg> {
        let mut resolved_args = Vec::new();

        // 按原始顺序处理每个参数
        for &arg in args {
            if let Some(resolved_arg) = self.resolve_incoming_stack_arg(arg) {
                resolved_args.push(resolved_arg);
            } else {
                resolved_args.push(arg);
            }
        }

        resolved_args
    }

    /// 检查并解决传入栈参数的使用
    /// 如果 vreg 来源于传入栈参数，创建新寄存器并插入 Reload
    /// 返回 Some(new_vreg) 如果需要替换，否则返回 None
    fn resolve_incoming_stack_arg(&mut self, vreg: Reg) -> Option<Reg> {
        // 检查这个 vreg 是否来源于传入栈参数
        // 通过查看 self.args 中是否有 StackArg 包含这个 vreg
        let mut found_incoming = None;
        for arg_pair in &self.args {
            if let ArgPair::StackArg {
                vreg: incoming_vreg,
                slot: incoming_slot,
                kind: incoming_kind,
            } = arg_pair
            {
                assert_eq!(incoming_kind, &SlotKindDisc::IncomingArg);
                if *incoming_vreg == vreg {
                    found_incoming = Some(*incoming_slot);
                    break;
                }
            }
        }

        if let Some(incoming_slot) = found_incoming {
            // 发现这个参数来源于传入栈参数，需要先加载
            let kind = self.stack.get_disc(incoming_slot);

            // 1. 创建新的临时虚拟寄存器
            let new_vreg = if vreg.is_xreg() {
                self.vregs.alloc_x().to_reg()
            } else {
                self.vregs.alloc_f().to_reg()
            };

            // 2. 立即插入 Reload 指令，从传入栈槽加载到新寄存器
            self.emit(&MInst::Reload {
                reg: new_vreg,
                slot: incoming_slot,
                kind,
            });

            // 3. 返回新寄存器，用于替换原参数
            return Some(new_vreg);
        }

        None
    }

    /// 创建栈槽并返回ArgPair
    fn create_stack_arg(&mut self, vreg: Reg, ty: &Type, kind: SlotKind) -> (ArgPair, SlotId) {
        // 对于IncomingArg和OutgoingArg，RISC-V ABI要求栈槽总是8字节
        // 即使实际类型（如int）只有4字节
        let size = match kind {
            SlotKind::IncomingArg { .. } | SlotKind::OutgoingArg => 8,
            _ => ty.abi_size(),
        };

        let slot_data = SlotData {
            id: SlotId::invalid(),
            kind,
            size,
            align: 3, // 8字节对齐 (2^3 = 8)
            fp_offset: None,
            sp_offset: None,
        };
        let slot_id = self.stack.add_slot(slot_data);
        let kind = self.stack.get_disc(slot_id);

        (
            ArgPair::StackArg {
                vreg,
                slot: slot_id,
                kind,
            },
            slot_id,
        )
    }

    // 字节大小方法在SSA IR Type: Type::abi_size -> u32, Type::abi_align -> u8
    // 转换为二进制字节: ConstValue::to_abi_bytes -> Vec<u8>
}

// ===================================================================
// 核心降级方法
// ===================================================================

impl Lower<'_> {
    pub fn lower_func(mut self) -> CEResult<VirtFunc> {
        let lowered_order = self.cfg.clone_block_order();

        // 1. 处理块的降级
        // 使用 rev() 确保反向处理
        for &bidx in lowered_order.iter().rev() {
            self.cur_block = Some(bidx);
            // PartA ---- 专门处理分支指令 ----
            match bidx {
                VirtBlockId::BasicBlock { block } => {
                    let term_inst = self
                        .cfg
                        .terminator(bidx)
                        .expect("BasicBlock绝不可能会出没有last_inst的情况");
                    let succ_labels: Vec<_> = self
                        .cfg
                        .successors(bidx)
                        .map(|succ| succ.to_label(self.func_ref))
                        .collect();
                    // 1. 从分支指令开始倒序处理
                    // eprintln!(
                    //     "DEBUG: 处理块 {:?} 的分支指令 {:?}",
                    //     block, self.fctx.func.dfg.insts[term_inst]
                    // );
                    self.cur_inst = Some(term_inst);
                    let branch_result = super::generated::selection::constructor_lower_branch(
                        &mut self,
                        term_inst,
                        &succ_labels,
                    );
                    self.emit_cur_minsts();
                    // 2. 分支处理后, BlockCall 的参数就已经被固定了
                    self.record_block_call(bidx, term_inst);
                    // 3. 降级基本块
                    self.lower_block(block).expect("降级失败");
                    // 4. 确保块内所有指令都被提交
                    self.emit_cur_minsts();
                }
                VirtBlockId::CriticalEdge {
                    succ_idx,
                    pred,
                    succ,
                } => {
                    // 唯一的作用就是插入一条jump: bidx(当前块)=>succ
                    let target_label =
                        VirtBlockId::BasicBlock { block: succ }.to_label(self.func_ref);
                    self.emit(&MInst::J {
                        label: target_label,
                    });
                    self.emit_cur_minsts();
                }
            }
        }

        // 2. 栈槽初始化
        self.cur_block = Some(self.cfg.entry());
        self.lower_stack_slot_init();
        self.emit_cur_minsts();

        // 3. 把所有指令反转 (因为是反向插入的)
        self.cfg.clone_block_order().iter().for_each(|&bidx| {
            let block_data = self.ensure_block_data(bidx);
            block_data.reverse_insts();
        });

        // 4. BlockParam/Phi 消除
        self.eliminate_phi_nodes();

        // 5. 解析所有虚拟寄存器的别名
        self.resolve_all_vreg_aliases();

        // 6. 在入口块插入参数映射注释
        self.add_entry_param_comment();

        Ok(VirtFunc {
            func_ref: self.func_ref,
            func_name: self.func_name,
            exported: self.exported,
            block_datas: self.block_datas,
            cfg: self.cfg,
            stack: self.stack,
            pst_slot_map: self.pst_slot_map,
            args: self.args,
        })
    }

    fn lower_block(&mut self, ir_bb: Block) -> CEResult<()> {
        let func = &self.fctx.func;
        // Demand-Driven Lowering
        // 从后往前遍历, 先遇到使用者，再遇到定义者
        for inst in func.layout.block_insts(ir_bb).rev() {
            let inst_data = func.dfg.insts[inst];
            let has_side_effect = inst_data.has_side_effect();
            if self.is_inst_sunk(inst) {
                // eprintln!("DEBUG: 指令 {:?} 在块 {:?} 被 sink", inst_data, ir_bb);
                continue;
            }
            let value_needed = self.is_any_inst_result_need(inst);
            // eprintln!(
            //     "DEBUG: 处理指令 {:?}, has_side_effect={}, value_needed={}",
            //     inst_data, has_side_effect, value_needed
            // );
            self.cur_inst = Some(inst);

            // FIX: 如下的 has_side_effect 已经废弃
            // 现在会记录每一个颜色的color, 而不仅仅是副作用指令
            // 因此使用一个 assert_eq
            if has_side_effect {
                let entry_color = *self
                    .inst_entry_color
                    .get(inst)
                    .expect("副作用指令必须有color");
                self.cur_inst_entry_color = Some(entry_color)
            }
            assert_eq!(
                self.cur_inst_entry_color.unwrap(),
                *self.inst_entry_color.get(inst).unwrap()
            );
            // 跳过分支指令: 分支指令已经在 selection::constructor_lower_branch 中处理
            // 注意: return / return_call 均不属于分支指令
            if inst_data.is_branch() {
                continue;
            }

            // 按需降低: 仅仅降低有副作用/被使用的指令
            if has_side_effect || value_needed {
                // 调用ISLE生成的lowering函数
                if let Some(lower_output) =
                    super::generated::selection::constructor_lower(self, inst)
                {
                    // eprintln!("  -> ISLE生成输出: {:?}", lower_output);
                    match lower_output {
                        InstOutput::OutputNone => {
                            // FIX: 不能 continue，因为需要调用 emit_cur_minsts
                        }
                        InstOutput::OutputReg { reg } => {
                            let inst_result = func.dfg.first_result(inst);
                            let pre_vreg = self.value_regs[inst_result];
                            // 建立别名映射：预分配寄存器 → ISLE 生成的寄存器
                            // - 第一阶段：在 Lower::new 中预分配虚拟寄存器给每个 SSA value
                            // - 第二阶段：ISLE 规则生成指令时可能使用不同的寄存器
                            // 通过别名机制，后续使用这个 SSA value 的指令会自动使用正确的寄存器
                            self.vregs.set_vreg_alias(pre_vreg, reg);
                        }
                    }
                }
            }
            self.emit_cur_minsts();
        }

        Ok(())
    }

    /// 解析所有虚拟寄存器的别名
    fn resolve_all_vreg_aliases(&mut self) {
        // 创建别名解析器
        struct AliasResolver<'a> {
            vregs: &'a VRegAllocator,
        }

        impl RegMapper for AliasResolver<'_> {
            fn map_reg(&mut self, reg: Reg) -> Reg {
                self.vregs
                    .resolve_vreg_alias(reg)
                    .unwrap_or_else(|| panic!("Failed to resolve alias for reg {:?}", reg))
            }
        }

        let mut resolver = AliasResolver { vregs: &self.vregs };

        // 遍历所有块的所有指令
        for block_data in self.block_datas.values_mut() {
            // 使用 map_regs 方法替换指令中的寄存器
            for inst in &mut block_data.insts {
                inst.map_regs(&mut resolver);
            }
        }
    }

    /// 消除Phi节点 - 插入必要的mv/li指令
    fn eliminate_phi_nodes(&mut self) {
        let edge_params = std::mem::take(&mut self.edge_params);

        for edge_param in edge_params {
            let insts = self.generate_transfer_instructions(&edge_param.transfers);
            self.insert_transfer_instructions(edge_param.move_loc, insts);
        }
    }

    /// 生成参数传递指令
    fn generate_transfer_instructions(&mut self, transfers: &[ParamTransfer]) -> Vec<MInst> {
        let mut insts = Vec::new();
        let mut float_resources = self.prepare_float_resources(transfers);
        let mut float_idx = 0;

        for transfer in transfers {
            match transfer {
                ParamTransfer::Move(src, dst) if src != dst => {
                    if let Some(inst) = self.generate_move_instruction(*src, *dst) {
                        insts.push(inst);
                    }
                }
                ParamTransfer::LoadConst(c, dst) => {
                    self.generate_const_load_instructions(
                        c,
                        *dst,
                        &mut insts,
                        &mut float_resources,
                        &mut float_idx,
                    );
                }
                _ => {} // src == dst 的情况忽略
            }
        }

        insts
    }

    /// 准备浮点数资源
    fn prepare_float_resources(
        &mut self,
        transfers: &[ParamTransfer],
    ) -> Vec<(bool, XReg, String)> {
        let mut resources = Vec::new();

        for transfer in transfers {
            if let ParamTransfer::LoadConst(c, _) = transfer {
                match c {
                    ConstValue::Float32 { val } => {
                        let temp_x = self.vregs.alloc_x();
                        let label = self
                            .vprog
                            .ensure_rodata(val.to_bits().to_le_bytes().to_vec());
                        resources.push((true, temp_x, label));
                    }
                    ConstValue::Float64 { val } => {
                        let temp_x = self.vregs.alloc_x();
                        let label = self
                            .vprog
                            .ensure_rodata(val.to_bits().to_le_bytes().to_vec());
                        resources.push((false, temp_x, label));
                    }
                    _ => {}
                }
            }
        }

        resources
    }

    /// 生成mov指令
    fn generate_move_instruction(&self, src: Reg, dst: Reg) -> Option<MInst> {
        match (src.is_xreg(), dst.is_xreg()) {
            (true, true) => Some(MInst::Mv {
                rd: dst.try_into().unwrap(),
                rs: src.try_into().unwrap(),
            }),
            (false, false) => Some(MInst::FmvD {
                fd: dst.try_into().unwrap(),
                fs: src.try_into().unwrap(),
            }),
            _ => None, // 跨类型移动不应该发生
        }
    }

    /// 生成常量加载指令
    fn generate_const_load_instructions(
        &mut self,
        c: &ConstValue,
        dst: Reg,
        insts: &mut Vec<MInst>,
        float_resources: &mut Vec<(bool, XReg, String)>,
        float_idx: &mut usize,
    ) {
        match c {
            ConstValue::Int32 { val } => {
                insts.push(MInst::Li {
                    rd: dst.try_into().unwrap(),
                    imm: *val as i64,
                });
            }
            ConstValue::Bool { val } => {
                insts.push(MInst::Li {
                    rd: dst.try_into().unwrap(),
                    imm: if *val { 1 } else { 0 },
                });
            }
            ConstValue::Int64 { val } => {
                insts.push(MInst::Li {
                    rd: dst.try_into().unwrap(),
                    imm: *val,
                });
            }
            ConstValue::Float32 { .. } => {
                let (_, temp_x, label) = &float_resources[*float_idx];
                *float_idx += 1;
                insts.push(MInst::La {
                    rd: *temp_x,
                    label: label.clone(),
                });
                insts.push(MInst::Flw {
                    fd: dst.try_into().unwrap(),
                    rs: *temp_x,
                    offset: 0,
                });
            }
            ConstValue::Float64 { .. } => {
                let (_, temp_x, label) = &float_resources[*float_idx];
                *float_idx += 1;
                insts.push(MInst::La {
                    rd: *temp_x,
                    label: label.clone(),
                });
                insts.push(MInst::Fld {
                    fd: dst.try_into().unwrap(),
                    rs: *temp_x,
                    offset: 0,
                });
            }
            _ => {} // Unit 等其他类型忽略
        }
    }

    /// 插入传递指令到正确位置
    fn insert_transfer_instructions(&mut self, move_loc: VirtBlockId, insts: Vec<MInst>) {
        if insts.is_empty() {
            return;
        }

        let block_data = self.ensure_block_data(move_loc);
        let insert_pos = block_data.find_terminator_pos();

        for (i, inst) in insts.into_iter().enumerate() {
            block_data.insert_inst(insert_pos + i, inst);
        }
    }

    /// 在入口块添加参数映射注释
    fn add_entry_param_comment(&mut self) {
        let func = &self.fctx.func;
        let entry_block = func.layout.entry_block().unwrap();

        // 获取函数签名参数（而不是块参数）
        let sig = &func.signature;
        if sig.params.is_empty() {
            // 如果没有函数参数，添加一个简单的注释表明这是入口
            let entry_virt_block = VirtBlockId::BasicBlock { block: entry_block };
            let block_data = self.ensure_block_data(entry_virt_block);
            block_data.insert_inst(
                0,
                MInst::Comment {
                    label: "Function entry: no arguments".to_string(),
                },
            );
            return;
        }

        // 构建函数参数映射注释
        let mut comment = String::from("Function Args: ");
        let mut param_count = 0;

        for arg in &self.args {
            if param_count > 0 {
                comment.push_str(", ");
            }
            comment.push_str(&format!("[{param_count}]:{}", arg));
            param_count += 1;
        }

        // 在入口块开头插入注释
        let entry_virt_block = VirtBlockId::BasicBlock { block: entry_block };
        let block_data = self.ensure_block_data(entry_virt_block);
        block_data.insert_inst(0, MInst::Comment { label: comment });
    }
}
