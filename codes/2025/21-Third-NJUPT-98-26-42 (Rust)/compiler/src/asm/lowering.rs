use itertools::Itertools;

use crate::{
    asm::{
        ctx::Lower,
        inst::*,
        prog::*,
        reg::*,
        reg_alloc,
        stack::{SlotData, SlotId, SlotKind, StackLayout},
    },
    prelude::*,
    ssa::{ctx::*, ir::FuncRef},
};
use std::collections::{HashMap, HashSet};

// ===================================================================
// 核心降级逻辑
// ===================================================================

pub fn lower_module(
    core_ctx: &CoreContext,
    func_ctxs: &FuncContexts,
    debug_opts: &crate::RegAllocDebugOptions,
) -> CEResult<(RvProg, bool)> {
    // 处理全局声明
    let mut vprog = VirtProg::new();
    vprog.init_globals(core_ctx);

    // 降级所有函数
    let mut vfuncs = Vec::new();
    let mut irc_failed = false;

    for (func_ref, func_ctx) in func_ctxs.ctxs.iter() {
        let (vfunc, run_finalize) =
            lower_function(core_ctx, func_ref, func_ctx, &mut vprog, debug_opts)?;

        // 检查是否是IRC失败（run_finalize为false但函数仍然返回）
        if !run_finalize {
            irc_failed = true;
            // IRC失败，只输出这个失败的函数
            vfuncs.push(vfunc);
            eprintln!("IRC失败，仅输出失败函数的虚拟汇编");
            break; // 立即停止处理其他函数
        }

        vfuncs.push(vfunc);
    }

    // 生成最终的 RvProg
    let rv_prog = emit_rv_prog(vprog, vfuncs);
    Ok((rv_prog, irc_failed))
}

/// 降级函数 + 寄存器分配
pub fn lower_function(
    core_ctx: &CoreContext,
    func_ref: FuncRef,
    func_ctx: &FuncContext,
    vprog: &mut VirtProg,
    debug_opts: &crate::RegAllocDebugOptions,
) -> CEResult<(VirtFunc, bool)> {
    // 1. ISLE 指令选择
    let lower = Lower::new(core_ctx, func_ref, func_ctx, vprog);
    let vfunc = lower.lower_func()?;

    // 2. 寄存器分配
    let (vfunc, run_finalize) =
        reg_alloc::allocate_registers(core_ctx, func_ctx, vprog, vfunc, debug_opts)?;
    if !run_finalize {
        eprintln!("根据 run_finalize={run_finalize},跳过后处理（可能是IRC失败）");
        return Ok((vfunc, run_finalize));
    }

    // 3. 伪指令展开 & 栈帧处理 & 函数序言
    let vfunc = finalize_function(vfunc);

    Ok((vfunc, run_finalize))
}

// ===================================================================
// VirtProg 到 RvProg 的代码发射
// ===================================================================

/// 将虚拟程序转换为实际RISC-V程序
pub fn emit_rv_prog(vprog: VirtProg, vfuncs: Vec<VirtFunc>) -> RvProg {
    let mut rv_prog = RvProg::default();

    // 1. 复制数据段
    rv_prog.data = vprog.data;
    rv_prog.rodata = vprog.rodata;
    rv_prog.bss = vprog.bss;

    // 2. 转换函数
    for vfunc in vfuncs {
        let rv_func = emit_rv_func(vfunc);
        rv_prog.text.push(rv_func);
    }

    rv_prog
}

/// 将虚拟函数转换为RISC-V函数
pub fn emit_rv_func(vfunc: VirtFunc) -> RvFunc {
    let mut rv_blocks = Vec::new();

    // 按CFG顺序处理块
    let block_order = vfunc
        .cfg
        .block_order()
        .iter()
        .filter(|block_id| vfunc.block_datas.contains_key(block_id))
        .cloned()
        .collect_vec();
    let labels = block_order
        .iter()
        .map(|block_id| block_id.to_label(vfunc.func_ref))
        .collect_vec();
    for (i, &block_id) in block_order.iter().enumerate() {
        let mut instructions: Vec<RvInst> = vfunc.block_datas.get(&block_id).unwrap().insts.clone();
        // 跳过相邻的块的 j
        if let Some(MInst::J { label }) = instructions.last() {
            if i + 1 < labels.len() && *label == labels[i + 1] {
                instructions.pop();
            }
        }
        rv_blocks.push(RvBlock {
            label: labels[i].clone(),
            instructions,
        });
    }

    RvFunc {
        name: vfunc.func_name,
        exported: vfunc.exported,
        blocks: rv_blocks,
    }
}

// ===================================================================
// 展开伪指令 / 生成序言与尾声
// ===================================================================

/// 主函数：完成所有后处理步骤
pub fn finalize_function(mut vfunc: VirtFunc) -> VirtFunc {
    // 阶段一：收集信息并创建 callee-save 栈槽
    let callee_save_slots = collect_and_create_callee_save_slots(&mut vfunc);

    // 阶段二：最终化栈布局，计算所有偏移量
    // (假设已使用上一轮修复的、正确的 finalize 函数)
    vfunc.stack.finalize();

    // 阶段三：进行统一的指令转换
    // 这个阶段会原子化地处理序言、尾声和所有伪指令的展开
    expand_instructions_and_generate_frame(&mut vfunc, &callee_save_slots);

    vfunc
}

/// 阶段一：收集函数中实际使用的callee-saved寄存器并创建栈槽
fn collect_and_create_callee_save_slots(vfunc: &mut VirtFunc) -> Vec<(Reg, SlotId)> {
    let mut used_regs = HashSet::new();
    let mut has_call = false;

    for block_data in vfunc.block_datas.values() {
        for inst in &block_data.insts {
            if inst.is_call() {
                has_call = true;
            }
            for reg in inst.get_defs().iter().chain(inst.get_uses().iter()) {
                if reg.is_physical() {
                    used_regs.insert(*reg);
                }
            }
        }
    }

    let mut callee_saved_to_save = Vec::new();
    // 如果有函数调用，必须保存 ra
    if has_call {
        callee_saved_to_save.push(Reg::X(RvXReg::Ra));
    }
    // 我们总是使用 FP
    callee_saved_to_save.push(Reg::X(RvXReg::S0));

    for &xreg in &CALLEE_SAVED_XREGS {
        let reg = xreg.to_reg();
        // S0 已经被添加，跳过
        if used_regs.contains(&reg) && reg != Reg::X(RvXReg::S0) {
            callee_saved_to_save.push(reg);
        }
    }
    for &freg in &CALLEE_SAVED_FREGS {
        let reg = freg.to_reg();
        if used_regs.contains(&reg) {
            callee_saved_to_save.push(reg);
        }
    }

    let mut slots = Vec::new();
    for reg in callee_saved_to_save {
        let (kind, size, align) = match reg {
            Reg::X(RvXReg::Ra) => (SlotKind::ReturnAddress, 8, 3),
            Reg::X(RvXReg::S0) => (SlotKind::FpAddress, 8, 3),
            // 其他 callee-saved 寄存器
            _ => (SlotKind::CalleeSaved { preg: reg }, 8, 3),
        };

        let slot_data = SlotData {
            id: SlotId::new(0), // id 会被 add_slot 覆盖
            kind,
            size,
            align,
            fp_offset: None,
            sp_offset: None,
        };
        let slot_id = vfunc.stack.add_slot(slot_data);

        match kind {
            SlotKind::ReturnAddress => vfunc.stack.set_return_address(slot_id),
            SlotKind::FpAddress => vfunc.stack.set_old_fp(slot_id),
            SlotKind::CalleeSaved { .. } => vfunc.stack.add_callee_save(slot_id),
            _ => unreachable!(),
        }
        slots.push((reg, slot_id));
    }

    slots
}

/// 阶段三：在一个统一的 pass 中展开所有伪指令并生成序言/尾声
fn expand_instructions_and_generate_frame(
    vfunc: &mut VirtFunc,
    callee_save_slots: &[(Reg, SlotId)],
) {
    let stack = &vfunc.stack;
    let frame_size = stack.total_frame_size();
    let entry_block_id = vfunc.cfg.entry();

    // 在一个新 Vec 中构建所有块，避免在迭代时修改 vfunc.block_datas
    let mut new_block_datas = HashMap::new();

    for (block_id, block_data) in vfunc.block_datas.iter() {
        let mut new_insts = Vec::new();

        // 步骤 3.1: 在入口块的开头插入序言
        if *block_id == entry_block_id {
            generate_prologue(frame_size, stack, callee_save_slots, &mut new_insts);

            // 插入函数参数从ABI寄存器到虚拟寄存器的移动指令 (preg -> vreg)
            for arg in &vfunc.args {
                if let &ArgPair::RegArg { vreg, preg } = arg {
                    if vreg != preg {
                        emit_move(vreg, preg, &mut new_insts);
                    }
                }
            }
        }

        // 步骤 3.2: 遍历并展开块内的所有指令
        for inst in &block_data.insts {
            match inst {
                MInst::RetPseudo { ret } => {
                    // 展开为：返回值移动 (vreg -> preg) + 尾声 + ret
                    if let &ArgPair::RegArg { vreg, preg } = ret {
                        if vreg != preg {
                            emit_move(preg, vreg, &mut new_insts);
                        }
                    }
                    generate_epilogue(frame_size, stack, callee_save_slots, &mut new_insts);
                    new_insts.push(MInst::Ret);
                }
                MInst::RetCallPseudo { label, args } => {
                    // 展开为：参数移动 (vreg -> preg) + 尾声 + j
                    emit_parallel_moves(args, &mut new_insts);
                    generate_epilogue(frame_size, stack, callee_save_slots, &mut new_insts);
                    new_insts.push(MInst::J {
                        label: label.clone(),
                    });
                }
                MInst::CallPseudo { label, args, ret } => {
                    // 展开为：参数移动 (vreg -> preg) + call + 返回值移动 (preg -> vreg)
                    emit_parallel_moves(args, &mut new_insts);
                    new_insts.push(MInst::Call {
                        label: label.clone(),
                    });
                    if let &ArgPair::RegArg { vreg, preg } = ret {
                        if vreg != preg {
                            emit_move(vreg, preg, &mut new_insts);
                        }
                    }
                }
                // 删去 nop
                MInst::Mv { rd, rs } if *rd == *rs => {}
                MInst::FmvD { fd, fs } if *fd == *fs => {}
                MInst::FmvS { fd, fs } if *fd == *fs => {}
                MInst::Nop => {}
                _ => {
                    // 处理其他可展开的伪指令（如 Spill/Reload, Li, etc.）
                    if let Some(expanded) = inst
                        .expand_pseudo(|slot_id, insts| compute_slot_address(slot_id, stack, insts))
                    {
                        new_insts.extend(expanded);
                    } else {
                        new_insts.push(inst.clone());
                    }
                }
            }
        }
        // 插入新生成的指令列表
        new_block_datas.insert(
            *block_id,
            VirtBlockData {
                insts: new_insts,
                ..Default::default()
            },
        );
    }

    // 用新生成的块数据替换旧的
    vfunc.block_datas = new_block_datas;
}

// ===================================================================
// 辅助函数
// ===================================================================

/// 生成函数序言
fn generate_prologue(
    frame_size: u32,
    stack: &StackLayout,
    callee_save_slots: &[(Reg, SlotId)],
    insts: &mut Vec<MInst>,
) {
    if frame_size == 0 {
        return;
    }
    insts.push(MInst::Comment {
        label: "-------- prologue start --------".into(),
    });
    // 1. 分配栈帧
    emit_add_imm(XReg::Sp, XReg::Sp, -(frame_size as i64), insts);
    // 2. 保存 callee-saved 寄存器
    for (reg, slot_id) in callee_save_slots {
        let offset = stack.get_slotdata(*slot_id).unwrap().sp_offset.unwrap();
        emit_store(*reg, XReg::Sp, offset, insts);
    }
    // 3. 设置新的 FP
    emit_add_imm(XReg::S0, XReg::Sp, frame_size as i64, insts);
    insts.push(MInst::Comment {
        label: "-------- prologue end --------".into(),
    });
}

/// 生成函数尾声
fn generate_epilogue(
    frame_size: u32,
    stack: &StackLayout,
    callee_save_slots: &[(Reg, SlotId)],
    insts: &mut Vec<MInst>,
) {
    if frame_size == 0 {
        return;
    }
    insts.push(MInst::Comment {
        label: "-------- epilogue start --------".into(),
    });
    // 1. 恢复 callee-saved 寄存器 -- 逆序恢复
    for (reg, slot_id) in callee_save_slots.iter().rev() {
        let offset = stack.get_slotdata(*slot_id).unwrap().sp_offset.unwrap();
        emit_load(*reg, XReg::Sp, offset, insts);
    }
    // 2. 恢复 SP
    emit_add_imm(XReg::Sp, XReg::Sp, frame_size as i64, insts);
    insts.push(MInst::Comment {
        label: "-------- epilogue end --------".into(),
    });
}

/// 发射寄存器移动指令
fn emit_move(dest: Reg, src: Reg, insts: &mut Vec<MInst>) {
    // 避免 `mv a0, a0` 这样的无效移动
    if dest == src {
        return;
    }
    match (dest, src) {
        (dest, src) if dest.is_xreg() && src.is_xreg() => insts.push(MInst::Mv {
            rd: dest.try_into().unwrap(),
            rs: src.try_into().unwrap(),
        }),
        (dest, src) if dest.is_freg() && src.is_freg() => insts.push(MInst::FmvS {
            fd: dest.try_into().unwrap(),
            fs: src.try_into().unwrap(),
        }),
        _ => panic!("Mismatched register types in move"),
    }
}

/// 处理函数调用的并行移动（参数准备）
fn emit_parallel_moves(args: &[ArgPair], insts: &mut Vec<MInst>) {
    let mut pending_moves: Vec<(Reg, Reg)> = args
        .iter()
        .filter_map(|arg| {
            if let &ArgPair::RegArg { vreg, preg } = arg {
                (vreg != preg).then_some((preg, vreg)) // dest=preg, src=vreg
            } else {
                None
            }
        })
        .collect();

    while !pending_moves.is_empty() {
        // 寻找一个可以安全执行的移动 (其目标寄存器不是其他任何移动的源寄存器)
        if let Some(ready_idx) = pending_moves
            .iter()
            .position(|(dest, _)| !pending_moves.iter().any(|(_, src)| src == dest))
        {
            let (dest, src) = pending_moves.remove(ready_idx);
            emit_move(dest, src, insts);
        } else {
            // 如果没有安全的移动，说明存在循环依赖 (e.g., a0->a1, a1->a0)
            // 使用临时寄存器打破循环
            let (dest, src) = pending_moves.pop().unwrap();
            let temp_reg = if dest.is_xreg() {
                SPILL_TEMP_REG.to_reg()
            } else {
                FReg::Ft11.to_reg()
            };

            emit_move(temp_reg, src, insts); // temp <- src

            // 将所有 `... <- src` 的移动改为 `... <- temp`
            for m in pending_moves.iter_mut() {
                if m.1 == src {
                    m.1 = temp_reg;
                }
            }
            // 将当前移动加入待处理列表
            pending_moves.push((dest, temp_reg));
        }
    }
}

/// 统一大数值立即数加法
fn emit_add_imm(rd: XReg, rs: XReg, imm: i64, insts: &mut Vec<MInst>) {
    if imm >= -2048 && imm <= 2047 && imm != 0 {
        insts.push(MInst::Addi {
            rd,
            rs,
            imm: imm as i16,
        });
    } else if imm != 0 {
        // 对于大立即数或 imm=0 但 rd!=rs 的情况，使用 li+add
        // 总是使用 SPILL_TEMP_REG 来加载立即数，以避免潜在的寄存器冲突
        insts.push(MInst::Li {
            rd: SPILL_TEMP_REG,
            imm,
        });
        insts.push(MInst::Add {
            rd,
            rs1: rs,
            rs2: SPILL_TEMP_REG,
        });
    } else if rd != rs {
        // imm = 0 且 rd != rs, 相当于 mv
        insts.push(MInst::Mv { rd, rs });
    }
    // imm = 0 且 rd == rs, 无操作
}

/// 统一的存指令 (使用修复后的 emit_add_imm)
fn emit_store(src: Reg, base: XReg, offset: i64, insts: &mut Vec<MInst>) {
    if offset >= -2048 && offset <= 2047 {
        match src {
            Reg::X(_rs2) => insts.push(MInst::Sd {
                rs2: src.try_into().unwrap(),
                rs1: base,
                offset: offset as i16,
            }),
            Reg::F(_fs) => insts.push(MInst::Fsd {
                fs: src.try_into().unwrap(),
                rs: base,
                offset: offset as i16,
            }),
            _ => unreachable!(),
        }
    } else {
        emit_add_imm(SPILL_TEMP_REG, base, offset, insts);
        match src {
            Reg::X(_rs2) => insts.push(MInst::Sd {
                rs2: src.try_into().unwrap(),
                rs1: SPILL_TEMP_REG,
                offset: 0,
            }),
            Reg::F(_fs) => insts.push(MInst::Fsd {
                fs: src.try_into().unwrap(),
                rs: SPILL_TEMP_REG,
                offset: 0,
            }),
            _ => unreachable!(),
        }
    }
}

/// 统一的取指令 (使用修复后的 emit_add_imm)
fn emit_load(dest: Reg, base: XReg, offset: i64, insts: &mut Vec<MInst>) {
    if offset >= -2048 && offset <= 2047 {
        match dest {
            Reg::X(_rd) => insts.push(MInst::Ld {
                rd: dest.try_into().unwrap(),
                rs: base,
                offset: offset as i16,
            }),
            Reg::F(_fd) => insts.push(MInst::Fld {
                fd: dest.try_into().unwrap(),
                rs: base,
                offset: offset as i16,
            }),
            _ => unreachable!(),
        }
    } else {
        emit_add_imm(SPILL_TEMP_REG, base, offset, insts);
        match dest {
            Reg::X(_rd) => insts.push(MInst::Ld {
                rd: dest.try_into().unwrap(),
                rs: SPILL_TEMP_REG,
                offset: 0,
            }),
            Reg::F(_fd) => insts.push(MInst::Fld {
                fd: dest.try_into().unwrap(),
                rs: SPILL_TEMP_REG,
                offset: 0,
            }),
            _ => unreachable!(),
        }
    }
}

/// 计算栈槽地址（用于Spill/Reload等）
fn compute_slot_address(
    slot_id: SlotId,
    stack: &StackLayout,
    insts: &mut Vec<MInst>,
) -> (XReg, Imm12) {
    let slot = stack.get_slotdata(slot_id).expect("Slot does not exist");
    // 在最终化阶段，FP (s0) 是最稳定的基址
    let offset = slot
        .fp_offset
        .expect("FP offset must be valid in finalization");

    if offset >= -2048 && offset <= 2047 {
        (XReg::S0, offset as Imm12)
    } else {
        // 大偏移量，使用临时寄存器计算地址
        emit_add_imm(SPILL_TEMP_REG, XReg::S0, offset, insts);
        (SPILL_TEMP_REG, 0)
    }
}
