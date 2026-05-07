//! 栈结构 & 栈槽
//!
//! plain
//!   (high address)
//!                              |          ...              |
//!                              | caller frames             |
//!                              |          ...              |
//!                              +===========================+
//!                              |          ...              |
//!                              | stack args         (+ FP) |
//! SP at function entry ------> +---------------------------+ <-- FP after prologue also points here (Canonical Frame Address)
//!                              | return address     (- FP) |
//!                              +---------------------------+
//!                              | old FP             (- FP) |
//!                              +---------------------------+           -----
//!                              |          ...              |             |
//!                              | clobbered callee-saves    |             |
//! unwind-frame base -------->  | (pushed by prologue)      |             |
//!                              +---------------------------+   -----     |
//!                              |          ...              |     |       |
//!                              | spill slots               |     |       |
//!                              | (accessed via SP)         |   fixed   active
//!                              |          ...              |   frame    size
//!                              | stack slots               |  storage    |
//!                              | (alloc'd by prologue)     |    size     |
//!                              |          ...              |     |       |
//!                              +---------------------------+   -----     |
//!                              | [alignment as needed]     |             |
//!                              |          ...              |             |
//!                              | args for largest call     |             |
//! SP after prologue -------->  | (alloc'd by prologue)     |             |
//!                              +===========================+           -----
//!
//!   (low address)
//!
use super::reg::*;
use crate::{define_index, prelude::*};
use std::{collections::HashSet, fmt::Debug};

// 定义Slot为StackSlot
define_index!(SlotId, SlotStorage, SlotData);

/// 语义类别
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, strum::Display, strum::EnumDiscriminants)]
#[strum_discriminants(vis(pub))]
#[strum_discriminants(name(SlotKindDisc))]
#[strum_discriminants(derive(strum::Display))]
pub enum SlotKind {
    /// 来自SSA IR的固定栈对象
    FixedObject { ir_slot: StackSlot },
    /// 为保存被调用者寄存器预留 -- 应该是物理寄存器
    CalleeSaved { preg: Reg },
    /// 为寄存器分配器溢出创建 -- 虚拟寄存器
    Spill { vreg: Reg },
    /// 传出参数
    OutgoingArg,
    /// 传入参数
    IncomingArg { vreg: Reg }, // 参数的虚拟寄存器
    /// 为TailCall重用栈的占位符
    ExtendTailArg,
    /// 返回地址槽
    ReturnAddress,
    /// FP地址槽
    FpAddress,
}

impl SlotKindDisc {
    pub fn is_persistent_slot(&self) -> bool {
        use SlotKindDisc::*;
        !matches!(self, OutgoingArg | ExtendTailArg)
    }
}

// 无需 pub fn disc(&self) -> SlotKindDisc
// 自动为 SlotKindDisc 生成into()

/// 单个栈槽信息
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct SlotData {
    pub id: SlotId,
    pub kind: SlotKind,
    pub size: usize,
    pub align: u8, // 2^align
    // 注意: 偏移量在“最终化”阶段之前都是 None
    pub fp_offset: Option<i64>, // 相对于fp位置
    pub sp_offset: Option<i64>, // 相对于sp位置
}

// impl SlotData {
//     pub fn is_persistent_slot(&self) -> bool {
//         !matches!(self.kind, SlotKind::OutgoingArg | SlotKind::ExtendTailArg)
//     }
// }

/// 函数栈帧布局 -- 包含每一个栈槽的偏移量
#[derive(Debug, Default, Clone)]
pub struct StackLayout {
    slots: SlotStorage, // 等价于 Vec<SlotData>
    // --------- 高地址
    /// 栈上传入参数 低地址->高地址: 按照函数签名的原始顺序排列
    incoming_args: Vec<SlotId>,
    // 调整SP以预留 tail_args_size 给TailCall -- 这就是一个纯粹的占位符
    extend_tail_args: Vec<SlotId>,
    // -------------------------------------- SP at function entry
    /// 返回地址
    return_address: Option<SlotId>,
    /// 旧的FP
    old_fp: Option<SlotId>,
    // -------------------------------------- FP after prologue
    /// 被调用者保存寄存器
    callee_saves: Vec<SlotId>,
    /// 固定的StackSlotData结构 -- 栈上数组
    fixed_objects: Vec<SlotId>,
    /// 溢出寄存器
    spill_slots: Vec<SlotId>,
    /// 栈上传出参数
    outgoing_args: Vec<SlotId>,
    // --------- 低地址

    // === 布局计算结果 ===
    /// 传入参数大小
    incoming_args_size: u32,
    /// 预留给尾调用栈上参数的大小
    /// 注意不是叠加关系, 而应该是附加关系, 用于在incoming_args_size不够TailCall重用栈帧时扩大
    /// extend_tail_args_size = 0.(incoming_args_size.max(tail_args_size) - incoming_args_size)
    tail_args_size: u32,
    /// Callee-save区域的总大小
    callee_saves_size: u32,
    /// 固定对象区域的总大小
    fixed_objects_size: u32,
    /// 溢出区域的总大小
    spill_slots_size: u32,
    /// 为传出调用准备的最大参数区大小
    outgoing_args_size: u32,
    /// 整个栈帧的总大小 从FP到SP的总大小 (不含 incoming_args 和 setup_area)
    /// ⚠️RISC-V ABI强制要求栈指针在函数调用边界处必须保持16字节对齐
    total_frame_size: u32,
    // 布局是否已最终化 -- 防止在错误时间点访问偏移量
    finalized: bool,
    // === 用于查询的映射 ===
    // 注意: 映射应该由外部进行管理, 这不是StackLayout的职责
}

impl StackLayout {
    pub fn new() -> Self {
        Self::default()
    }

    /// 添加栈槽
    pub fn add_slot(&mut self, mut slot_data: SlotData) -> SlotId {
        // 计算将要分配的索引
        let slot_id = SlotId::new(self.slots.len());
        // 设置正确的 id
        slot_data.id = slot_id;
        // slots.push 会返回相同的索引
        let pushed_id = self.slots.push(slot_data);
        debug_assert_eq!(slot_id, pushed_id);
        slot_id
    }

    /// 添加 incoming_args 槽
    pub fn add_incoming_arg(&mut self, slot_id: SlotId) {
        self.incoming_args.push(slot_id);
    }

    /// 添加 outgoing_args 槽
    pub fn add_outgoing_arg(&mut self, slot_id: SlotId) {
        self.outgoing_args.push(slot_id);
    }

    /// 添加 extend_tail_args 槽
    pub fn add_extend_tail_arg(&mut self, slot_id: SlotId) {
        self.extend_tail_args.push(slot_id);
    }

    /// 添加 fixed_objects 槽
    pub fn add_fixed_object(&mut self, slot_id: SlotId) {
        self.fixed_objects.push(slot_id);
    }

    /// 添加 spill_slots 槽
    pub fn add_spill_slot(&mut self, slot_id: SlotId) {
        self.spill_slots.push(slot_id);
    }

    /// 设置 outgoing_args_size
    pub fn set_outgoing_args_size(&mut self, size: u32) {
        self.outgoing_args_size = size;
    }

    /// 设置 tail_args_size
    pub fn set_tail_args_size(&mut self, size: u32) {
        self.tail_args_size = size;
    }

    /// 获取栈槽数量
    pub fn slot_count(&self) -> usize {
        self.slots.len()
    }

    /// 获取 incoming_args 数量
    pub fn incoming_args_count(&self) -> usize {
        self.incoming_args.len()
    }

    /// 获取栈槽数据
    pub fn get_slotdata(&self, slot_id: SlotId) -> Option<&SlotData> {
        self.slots.get(slot_id)
    }

    pub fn get_disc(&self, slot_id: SlotId) -> SlotKindDisc {
        self.get_slotdata(slot_id)
            .expect("试图获取未分配的栈槽的kind")
            .kind
            .into()
    }

    /// 获取outgoing_args栈
    pub fn get_outgoing_arg(&self, idx: usize) -> Option<SlotId> {
        if idx >= self.outgoing_args.len() {
            return None;
        }
        Some(self.outgoing_args[idx])
    }

    /// 获取tail_args栈
    /// ⚠️TailCall会复用incoming_arg的栈
    pub fn get_tail_arg(&self, idx: usize) -> Option<SlotId> {
        let in_len = self.incoming_args.len();
        let ex_len = self.extend_tail_args.len();
        if idx >= in_len + ex_len {
            None
        // 尾调用参数首先填充扩展槽，然后才使用原始的传入参数槽
        // 匹配 finalize 中将 extend_tail_args 布局在低地址的逻辑
        } else if idx < ex_len {
            // 参数索引落在为尾调用扩展的范围内
            Some(self.extend_tail_args[idx])
        } else {
            // 参数索引落在原始 incoming_args 的范围内
            Some(self.incoming_args[idx - ex_len])
        }
    }
}

// ===================================================================
// 最终栈布局计算
// ===================================================================
impl StackLayout {
    /// 添加 callee_saves 槽
    pub fn add_callee_save(&mut self, slot_id: SlotId) {
        self.callee_saves.push(slot_id);
    }

    /// 设置返回地址槽
    pub fn set_return_address(&mut self, slot_id: SlotId) {
        self.return_address = Some(slot_id);
    }

    /// 设置旧FP槽
    pub fn set_old_fp(&mut self, slot_id: SlotId) {
        self.old_fp = Some(slot_id);
    }

    /// 获取总栈帧大小（必须在finalize后调用）
    pub fn total_frame_size(&self) -> u32 {
        assert!(self.finalized, "栈布局尚未最终化");
        self.total_frame_size
    }

    /// 是否已经最终化
    pub fn is_finalized(&self) -> bool {
        self.finalized
    }

    /// 获取返回地址槽ID
    pub fn return_address(&self) -> Option<SlotId> {
        self.return_address
    }

    /// 获取旧FP槽ID
    pub fn old_fp(&self) -> Option<SlotId> {
        self.old_fp
    }
    /// 最终化栈布局 - 计算所有偏移量和大小
    pub fn finalize(&mut self) {
        if self.finalized {
            return;
        }

        // =================================================================================
        // 定义FP模型
        // 采用标准的 FP 模型: FP (s0) 在 prologue 后指向它自己在栈上的保存位置。
        // 这意味着 FP 下方是 callee-saves 等局部数据 (负偏移)，上方是返回地址和传入参数 (正偏移)。
        //
        //   (高地址)
        //                              +---------------------------+
        //   ...                        | incoming_arg[N]           | NFP + 24 + 8*N
        //                              +---------------------------+
        //                              | incoming_arg[0]           | NFP + 16
        //  NFP (s0) after prologue --> +---------------------------+
        //   ...                        | extend_tail_arg[1]        | NFP + 8 (假设因为复用只增加两个)
        //                              +---------------------------+
        //                              | extend_tail_arg[0]        | NFP
        //                              +---------------------------+------------ <-- Old SP at function entry (CFA) & New FP after prologue
        //                              | return address            | NFP - 8
        //                              +---------------------------+
        //                              | saved old_fp (s0)         | NFP - 16
        //                              +---------------------------+
        //                              | callee_saves, locals...   |
        //                              | ...                       |
        //   NSP after prologue ------> +---------------------------+---------------- NSP: New SP
        //   (低地址)
        // =================================================================================
        // =================================================================================
        // 步骤 1: 计算 FP 正向偏移量 (高地址区域 - incoming_args 和为尾调用准备的扩展槽)
        // 根据RISC-V ABI，栈上传入参数从 FP+16 开始。
        // 尾调用会复用这片区域。如果尾调用需要的参数空间比当前函数的传入参数空间大，
        // 就需要通过 extend_tail_args 来扩展。
        // 布局顺序 (从 FP+16 开始向上，即地址增大的方向): incoming_args, 然后是 extend_tail_args.
        // =================================================================================
        let mut current_positive_fp_offset = 0i64;

        // 辅助函数，用于为槽分配正向偏移量并处理对齐
        let assign_positive_offset = |offset: &mut i64, slot: &mut SlotData| {
            // 栈上传参至少8字节对齐
            let align = (1u64 << slot.align).max(8);
            let align_mask = !(align - 1);
            // 向上对齐当前偏移量以满足槽的对齐要求
            let aligned_offset = (*offset + (align - 1) as i64) & align_mask as i64;
            slot.fp_offset = Some(aligned_offset);
            // 更新下一个可用偏移量
            *offset = aligned_offset + slot.size as i64;
        };

        // 1.1 首先为常规传入参数分配空间
        for &slot_id in &self.incoming_args {
            assign_positive_offset(&mut current_positive_fp_offset, &mut self.slots[slot_id]);
        }

        // 1.2 为 extend_tail_args 分配空间
        for &slot_id in &self.extend_tail_args {
            assign_positive_offset(&mut current_positive_fp_offset, &mut self.slots[slot_id]);
        }

        // 1.2 计算总的传入参数区域大小
        let total_arg_area_size = if current_positive_fp_offset > 16 {
            (current_positive_fp_offset - 16) as u32
        } else {
            0
        };

        // incoming_args_size 应该反映整个可重用区域的大小。
        // 它必须至少是 tail_args_size (外部传入的最大尾调用需求) 和刚刚计算的大小的最大值。
        self.incoming_args_size = total_arg_area_size.max(self.tail_args_size);

        // =================================================================================
        // 步骤 2: 计算 FP 负向偏移量 (低地址区域)
        // 所有保存在当前函数栈帧内的数据都在这里分配
        // =================================================================================
        let mut current_negative_fp_offset = 0i64;

        let assign_negative_offset = |offset: &mut i64, slot: &mut SlotData| {
            let align = 1u64 << slot.align.max(3); // RISC-V 栈对象至少8字节对齐
            let align_mask = !(align - 1);
            // 先减去大小，再将结果向下对齐
            *offset -= slot.size as i64;
            *offset &= align_mask as i64;
            slot.fp_offset = Some(*offset);
        };

        // 2.1 收集所有需要保存在当前栈帧的槽，使用 HashSet 去重
        let mut slots_to_layout_in_frame = Vec::new();
        let mut laid_out = HashSet::new();

        // RA
        if let Some(slot_id) = self.return_address {
            if laid_out.insert(slot_id) {
                slots_to_layout_in_frame.push(slot_id);
            }
        }
        // FP
        if let Some(slot_id) = self.old_fp {
            if laid_out.insert(slot_id) {
                slots_to_layout_in_frame.push(slot_id);
            }
        }

        // 然后是其他 callee-saved 寄存器
        for &slot_id in &self.callee_saves {
            if laid_out.insert(slot_id) {
                slots_to_layout_in_frame.push(slot_id);
            }
        }

        // 2.2 分配 callee_saves
        let callee_saves_start = current_negative_fp_offset;
        for &slot_id in &slots_to_layout_in_frame {
            assign_negative_offset(&mut current_negative_fp_offset, &mut self.slots[slot_id]);
        }
        self.callee_saves_size = (callee_saves_start - current_negative_fp_offset) as u32;

        // 2.3 分配 fixed_objects
        let fixed_objects_start = current_negative_fp_offset;
        for &slot_id in &self.fixed_objects {
            assign_negative_offset(&mut current_negative_fp_offset, &mut self.slots[slot_id]);
        }
        self.fixed_objects_size = (fixed_objects_start - current_negative_fp_offset) as u32;

        // 2.4 分配 spill_slots
        let spill_slots_start = current_negative_fp_offset;
        for &slot_id in &self.spill_slots {
            assign_negative_offset(&mut current_negative_fp_offset, &mut self.slots[slot_id]);
        }
        self.spill_slots_size = (spill_slots_start - current_negative_fp_offset) as u32;

        // =================================================================================
        // 步骤 3: 计算总栈帧大小并对齐到 16 字节
        // =================================================================================
        let persistent_frame_size = -current_negative_fp_offset as u32;
        let unaligned_size = persistent_frame_size + self.outgoing_args_size;
        self.total_frame_size = (unaligned_size + 15) & !15;

        // =================================================================================
        // 步骤 4: 计算所有槽相对于 SP 的偏移量
        // address = FP + fp_offset = SP + sp_offset
        // FP = SP + total_frame_size
        // SP + total_frame_size + fp_offset = SP + sp_offset
        // ====> sp_offset = fp_offset + total_frame_size
        // =================================================================================
        // outgoing_args 的偏移量是基于 SP 的，从 0 开始
        let mut current_sp_offset = 0i64;
        for &slot_id in &self.outgoing_args {
            let slot = &mut self.slots[slot_id];
            slot.sp_offset = Some(current_sp_offset);
            // 同时计算 fp_offset
            slot.fp_offset = Some(current_sp_offset - self.total_frame_size as i64);
            current_sp_offset += slot.size as i64;
        }

        // 为所有其他槽计算 sp_offset
        for slot in self.slots.iter_mut() {
            // 跳过已经处理过的 outgoing_args
            if matches!(slot.kind, SlotKind::OutgoingArg) {
                continue;
            }

            if let Some(fp_offset) = slot.fp_offset {
                slot.sp_offset = Some(fp_offset + self.total_frame_size as i64);
            }
        }

        self.finalized = true;
    }
}
