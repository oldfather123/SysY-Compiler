//! # 循環不變量外提
//!
//! ## 條件
//! 給定一個指令 I：
//! - 所有操作數都在循環外定義（或是循環不變的）
//! - 指令沒有副作用（memory write、IO、trap 等）
//! - 指令在循環內的所有執行路徑都會被執行（Dominate 所有 Exit）
use crate::ssa::{
    CoreContext, FuncContext, FuncCursor, Loop, LoopAnalysis, cursor::Cursor, dfg::DataFlowGraph,
    ir::InstructionData, ir::ValueData, layout::Layout,
};
use crate::utils::error::CEResult;
use crate::{HashSet, Inst, Value};
use std::collections::HashMap;

pub fn licm(fctx: &mut FuncContext, cctx: &mut CoreContext) -> CEResult<()> {
    println!("Running licm pass start");
    let loop_infos = &mut fctx.loop_analysis;
    // table records if the loop is processed
    let mut table: HashMap<_, _> = loop_infos.loops().map(|lp| (lp, false)).collect();
    let loops_to_process: Vec<Loop> = table.keys().cloned().collect();
    for lp in loops_to_process {
        println!("Running licm on loop: {}", lp);
        process_loop_if_needed(fctx, cctx, lp, &mut table);
    }
    println!("Running licm pass end");
    Ok(())
}

// for each loop in the table
// if has parent, find parrent
// if not processed then process it and its inner, mark as processed
// else skip
fn process_loop_if_needed(
    fctx: &mut FuncContext,
    cctx: &mut CoreContext,
    lp: Loop,
    table: &mut HashMap<Loop, bool>,
) {
    let la = &mut fctx.loop_analysis;
    if table[&lp] {
        return;
    }

    if let Some(parent) = la.loop_parent(lp) {
        // has parent, process it.
        println!("Proccess parent");
        process_loop_if_needed(fctx, cctx, parent, table);
    }

    // process this one
    run_licm_on_loop(lp, fctx, cctx);
    println!("Processed loop: {}", lp);

    // mark as processed
    table.insert(lp, true);
}

fn run_licm_on_loop(lp: Loop, fctx: &mut FuncContext, cctx: &mut CoreContext) {
    let la = &mut fctx.loop_analysis;
    let header = la.loop_header(lp).unwrap();
    let mut moved_insts = HashSet::<Inst>::new();

    // 遍历所有block并把指令收集起来
    let mut blocks_in_loop = HashSet::new();
    for block in fctx.func.layout.blocks() {
        if la.is_in_loop(block, lp) {
            blocks_in_loop.insert(block);
        }
    }

    // 使用工作清单处理依赖关系
    let mut worklist: Vec<Inst> = Vec::new();
    for block in &blocks_in_loop {
        let inst_cursor = fctx.func.layout.block_insts(*block);
        for inst in inst_cursor {
            if fctx.func.dfg.insts.contains_key(inst) {
                worklist.push(inst);
            }
        }
    }

    let mut moved_something = true;
    while moved_something {
        moved_something = false;
        let mut next_worklist = Vec::new();

        for inst in worklist {
            let idata = fctx.func.dfg.insts.get(inst).unwrap();

            // 检查是否可以移动
            if !moved_insts.contains(&inst)
                && !idata.is_terminator()
                && can_move_inst(
                    inst,
                    idata,
                    lp,
                    la,
                    &fctx.func.dfg,
                    &fctx.func.layout,
                    &moved_insts,
                )
            {
                moved_insts.insert(inst);
                moved_something = true;

                // 移动指令
                let mut cursor = FuncCursor::new(&mut fctx.func, cctx);
                let layout = cursor.layout_mut();
                layout.remove_inst(inst);
                if let Some(first_inst_in_header) = layout.first_inst(header) {
                    // 如果头部有指令, 就插入在第一条指令前
                    layout.insert_inst(inst, first_inst_in_header);
                } else {
                    // 不然就直接追加
                    layout.append_inst(inst, header);
                }
            } else {
                next_worklist.push(inst);
            }
        }
        worklist = next_worklist;
    }
}

fn can_move_inst(
    _inst: Inst,
    idata: &InstructionData,
    lp: Loop,
    info: &LoopAnalysis,
    dfg: &DataFlowGraph,
    layout: &Layout,
    moved_insts: &HashSet<Inst>,
) -> bool {
    use InstructionData::*;

    match idata {
        ArrayPut { .. }
        | Call { .. }
        | GlobalScalarSet { .. }
        | ReturnCall { .. }
        | Return { .. }
        | Brif { .. }
        | Jump { .. }
        | BranchTable { .. }
        | Unreachable => false,
        ArrayGet { .. }
        | GlobalScalarGet { .. }
        | ArrayAlloc { .. }
        | ArraySlice { .. }
        | Store { .. }
        | Load { .. } => false,
        Binary { lhs, rhs, .. } => {
            is_operand_or_moved(*lhs, lp, info, dfg, layout, moved_insts)
                && is_operand_or_moved(*rhs, lp, info, dfg, layout, moved_insts)
        }
        Unary { val, .. } | Cast { val, .. } => {
            is_operand_or_moved(*val, lp, info, dfg, layout, moved_insts)
        }
        _ => false,
    }
}

// 辅助函数, 检查操作数是不是循环不变量, 或者所属指令是不是已经被移动
fn is_operand_or_moved(
    val: Value,
    lp: Loop,
    info: &LoopAnalysis,
    dfg: &DataFlowGraph,
    layout: &Layout,
    moved_insts: &HashSet<Inst>,
) -> bool {
    let vdata = dfg.valuedata(val);
    if vdata.is_const() {
        return true;
    }

    if let ValueData::Inst { inst, .. } = *vdata {
        // 如果这个操作数来自的指令已经被移动, 那么它就是循环不变的
        if moved_insts.contains(&inst) {
            return true;
        }

        if let Some(block) = layout.inst_block(inst) {
            // 指令所在区域不再当前循环内, 它是不变的
            return !info.is_in_loop(block, lp);
        }
    }
    false
}
