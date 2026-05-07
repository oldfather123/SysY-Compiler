//! 函数内联

use super::{ctx::*, cursor::*, dfg::*, function::*, ir::*};
use crate::prelude::*;
use crate::ssa::ir_impl::InstructionMapper;
use std::collections::{HashMap, HashSet};

/// 执行函数内联
///
/// 在specific_callees非空时，只内联指定的函数
/// 不负责内联策略，只负责直接内联
pub fn inline_func(
    caller_ref: FuncRef,
    specific_callees: &HashSet<FuncRef>,
    core_ctx: &mut CoreContext,
    func_ctxs: &mut FuncContexts,
) -> CEResult<bool> {
    let mut if_inlined = false;
    let mut inline_map = InlineMap::new();

    // 暂时取出函数上下文
    let mut func_ctx = func_ctxs.ctxs.remove(caller_ref).unwrap();
    let mut cursor = FuncCursor::new(&mut func_ctx.func, core_ctx);

    while let Some(block) = cursor.next_block() {
        let mut prev_pos;
        while let Some(inst) = {
            prev_pos = cursor.position();
            cursor.next_inst()
        } {
            match cursor.func.dfg.insts[inst] {
                InstructionData::Call {
                    func: callee_ref,
                    args,
                }
                | InstructionData::ReturnCall {
                    func: callee_ref,
                    args,
                } => {
                    // 指定了特定的callees，只内联这些函数
                    if !specific_callees.is_empty() && !specific_callees.contains(&callee_ref) {
                        continue;
                    }

                    // 跳过外部函数
                    let callee = match func_ctxs.ctxs.get(callee_ref) {
                        Some(ctx) => &ctx.func,
                        None => {
                            continue;
                        }
                    };

                    // 执行内联
                    inline_one(
                        &mut inline_map,
                        &mut cursor,
                        callee_ref,
                        block,
                        inst,
                        callee,
                        func_ctxs,
                    )?;

                    if_inlined = true;
                    cursor.set_position(prev_pos);
                }
                _ => continue,
            }
        }
    }

    // 放回函数上下文
    func_ctxs.ctxs.insert(caller_ref, func_ctx);

    Ok(if_inlined)
}

/// 内联映射 - 管理从被调用函数到调用函数的实体映射
struct InlineMap {
    /// 值映射：callee值 -> caller值
    values: HashMap<Value, Value>,
    /// 块映射：callee块 -> caller块
    blocks: HashMap<Block, Block>,
}

impl InlineMap {
    pub fn new() -> Self {
        Self {
            values: HashMap::new(),
            blocks: HashMap::new(),
        }
    }

    pub fn reset(&mut self, _caller: &mut Function, _callee: &Function) {
        self.values.clear();
        self.blocks.clear();
    }

    /// 设置值映射
    pub fn set_inlined_value(&mut self, callee_val: Value, inlined_val: Value) {
        // eprintln!(
        //     "[INLINE DEBUG] 值映射: {:?} -> {:?}",
        //     callee_val, inlined_vaGl
        // );
        self.values.insert(callee_val, inlined_val);
    }

    /// 获取内联后的值
    pub fn get_inlined_value(&self, callee_val: Value) -> Option<Value> {
        self.values.get(&callee_val).copied()
    }

    /// 设置块映射
    pub fn set_inlined_block(&mut self, callee_block: Block, inlined_block: Block) {
        self.blocks.insert(callee_block, inlined_block);
    }

    /// 获取内联后的块
    pub fn inlined_block(&self, callee_block: Block) -> Block {
        self.blocks
            .get(&callee_block)
            .copied()
            .expect("块应该已被映射")
    }
}

/// 指令重映射
struct InliningInstRemapper<'a> {
    inline_map: &'a InlineMap,
    callee: &'a Function,
}

impl InstructionMapper for InliningInstRemapper<'_> {
    fn map_value(&mut self, value: Value) -> Value {
        // 首先解析别名，获取真实的值
        let real_value = self.callee.dfg.resolve_aliases(value);

        self.inline_map
            .get_inlined_value(real_value)
            .unwrap_or_else(|| {
                panic!(
                    "定义应该在使用之前：未找到值 {:?} (original: {:?}) 的映射",
                    real_value, value
                )
            })
    }

    fn map_value_vec(&mut self, value_vec: ValueVec, pool: &mut ValueVecPool) -> ValueVec {
        let old_values = self
            .callee
            .dfg
            .value_vecs
            .as_slice(value_vec)
            .unwrap_or(&[]);
        let mapped_values: Vec<Value> = old_values.iter().map(|v| self.map_value(*v)).collect();
        pool.insert(mapped_values)
    }

    fn map_block_call(&mut self, block_call: BlockCall, pool: &mut ValueVecPool) -> BlockCall {
        let mapped_block = self.inline_map.inlined_block(block_call.block);
        let mapped_args = self.map_value_vec(block_call.args, pool);
        BlockCall {
            block: mapped_block,
            args: mapped_args,
        }
    }

    fn map_func_ref(&mut self, func_ref: FuncRef) -> FuncRef {
        // 内联时不改变函数引用
        func_ref
    }

    fn map_jump_table(&mut self, table: JumpTable, pool: &mut ValueVecPool) -> JumpTable {
        // TODO: 实现跳转表映射
        unimplemented!("跳转表映射尚未实现")
    }

    fn map_global(&mut self, global: Global) -> Global {
        // 全局变量不需要映射
        global
    }

    fn map_array_init(&mut self, init: ArrayInit, pool: &mut ValueVecPool) -> ArrayInit {
        match init {
            ArrayInit::List { vals } => ArrayInit::List {
                vals: self.map_value_vec(vals, pool),
            },
            // 其他初始化类型直接复制
            _ => init,
        }
    }
}

/// 内联一个函数调用
fn inline_one(
    inline_map: &mut InlineMap,
    cursor: &mut FuncCursor,
    callee_ref: FuncRef,
    call_block: Block,
    call_inst: Inst,
    callee: &Function,
    func_ctxs: &FuncContexts,
) -> CEResult<()> {
    // 重置内联映射
    inline_map.reset(cursor.func_mut(), callee);

    // 第一步：创建所有需要的实体（块、非入口块参数）
    create_entities(inline_map, cursor.func_mut(), callee);

    // 第二步：预映射所有常量和全局符号
    // 这一步至关重要，确保所有在指令中使用之前就存在的、需要映射的值都已处理
    for (val, val_data) in callee.dfg.all_values().iter() {
        // 跳过已经映射过的值（例如块参数）
        if inline_map.get_inlined_value(val).is_some() {
            continue;
        }

        let mapped_val = match val_data {
            ValueData::Const { ty, c } => {
                // 在调用者函数中创建一个新的常量值
                Some(
                    cursor
                        .func_mut()
                        .dfg
                        .make_value(ValueData::Const { ty: *ty, c: *c }),
                )
            }
            ValueData::GlobalSymbol { ty, sym } => {
                // 在调用者函数中创建一个新的全局符号值
                Some(
                    cursor
                        .func_mut()
                        .dfg
                        .make_value(ValueData::GlobalSymbol { ty: *ty, sym: *sym }),
                )
            }
            // 其他类型的值（如指令结果）将在其定义指令被复制时进行映射
            _ => None,
        };

        if let Some(inlined_val) = mapped_val {
            inline_map.set_inlined_value(val, inlined_val);
        }
    }

    // 第三步：分割调用点，为返回值创建新的块
    let return_block = split_off_return_block(cursor.func_mut(), call_inst, callee);

    // 第四步：用跳转指令替换原来的调用指令
    let callee_entry = callee.layout.entry_block().expect("callee应该有入口块");
    replace_call_with_jump(
        inline_map,
        cursor.func_mut(),
        call_inst,
        callee,
        callee_entry,
    );

    // 第五步：将被内联函数的块布局插入到调用者中
    inline_block_layout(cursor.func_mut(), call_block, callee, inline_map);

    // 第六步：遍历并复制所有指令
    for callee_block in callee.layout.blocks() {
        let inlined_block = inline_map.inlined_block(callee_block);
        let mut next_callee_inst = callee.layout.first_inst(callee_block);
        while let Some(callee_inst) = next_callee_inst {
            let is_return_call_in_normal_call = matches!(
                callee.dfg.insts[callee_inst],
                InstructionData::ReturnCall { .. }
            ) && return_block.is_some();

            if is_return_call_in_normal_call {
                // 特殊情况：在一个通过常规 call 调用的被调用函数中出现 return_call。
                // 这必须被转换为一个 call，后跟一个到返回块的 jump。

                // 1. 创建 call 指令
                let mut remapper = InliningInstRemapper { inline_map, callee };
                let (rcall_func_ref, rcall_args_vec) = match &callee.dfg.insts[callee_inst] {
                    InstructionData::ReturnCall { func, args } => (func, args),
                    _ => unreachable!(),
                };
                let mapped_args =
                    remapper.map_value_vec(*rcall_args_vec, &mut cursor.func_mut().dfg.value_vecs);
                let new_call_data = InstructionData::Call {
                    func: *rcall_func_ref,
                    args: mapped_args,
                };
                let new_call_inst = cursor.func_mut().dfg.make_inst(new_call_data);
                cursor
                    .func_mut()
                    .layout
                    .append_inst(new_call_inst, inlined_block);

                // 2. 为新的调用附加结果
                let rcall_callee_sig = &func_ctxs.ctxs[*rcall_func_ref].func.signature;
                let result_types = rcall_callee_sig.returns.clone();
                cursor
                    .func_mut()
                    .dfg
                    .make_inst_results(new_call_inst, result_types);

                // 3. 创建到返回块的跳转
                let call_results = cursor.func_mut().dfg.inst_results(new_call_inst).to_vec(); // to_vec to satisfy borrow checker

                // FIXED: 只传递return_block期待的参数数量
                // return_call在被内联时，应该只返回return_block期待的参数数量
                let caller_expected_returns = cursor
                    .func_mut()
                    .dfg
                    .block_params(return_block.unwrap())
                    .len();
                let filtered_results: Vec<Value> = call_results
                    .into_iter()
                    .take(caller_expected_returns)
                    .collect();

                let return_args = cursor.func_mut().dfg.make_valuevec(&filtered_results);
                let jump_data = InstructionData::Jump {
                    target: BlockCall {
                        block: return_block.unwrap(),
                        args: return_args,
                    },
                };
                let jump_inst = cursor.func_mut().dfg.make_inst(jump_data);
                cursor
                    .func_mut()
                    .layout
                    .append_inst(jump_inst, inlined_block);
            } else {
                // 适用于所有其他指令的原始逻辑
                // 确定指令结果的类型
                let result_types = if !callee.dfg.insts[callee_inst].is_return() {
                    infer_inst_result_types(&callee.dfg, cursor.ctx, callee_inst)
                } else {
                    vec![]
                };

                // 创建一个重映射器，用于转换指令中的值和块
                let mut remapper = InliningInstRemapper { inline_map, callee };

                // 映射指令并将其插入到调用者函数中
                let inlined_inst_data = callee.dfg.insts[callee_inst]
                    .map(&mut remapper, &mut cursor.func_mut().dfg.value_vecs);
                let inlined_inst = cursor.func_mut().dfg.make_inst(inlined_inst_data);
                cursor
                    .func_mut()
                    .layout
                    .append_inst(inlined_inst, inlined_block);

                // 处理指令的结果
                if callee.dfg.insts[callee_inst].is_return() {
                    // 如果是返回指令，需要特殊处理以跳转到返回块
                    if let Some(return_block) = return_block {
                        let func = cursor.func_mut();
                        fixup_inst_that_returns(
                            inline_map,
                            func,
                            callee,
                            inlined_inst,
                            callee_inst,
                            return_block,
                        )?;
                    }
                } else {
                    // 如果是普通指令，为其创建结果值，并建立映射
                    if !result_types.is_empty() {
                        cursor
                            .func_mut()
                            .dfg
                            .make_inst_results(inlined_inst, result_types);

                        let callee_results = callee.dfg.inst_results(callee_inst);
                        let inlined_results = cursor.func_mut().dfg.inst_results(inlined_inst);
                        for (callee_val, inlined_val) in callee_results.iter().zip(inlined_results)
                        {
                            inline_map.set_inlined_value(*callee_val, *inlined_val);
                        }
                    }
                }
            }

            next_callee_inst = callee.layout.next_inst(callee_inst);
        }
    }

    Ok(())
}

/// 创建实体
fn create_entities(inline_map: &mut InlineMap, caller: &mut Function, callee: &Function) {
    // 为callee的每个块创建对应的caller块
    for callee_block in callee.dfg.blocks.keys() {
        let caller_block = caller.dfg.make_block();
        inline_map.set_inlined_block(callee_block, caller_block);

        // eprintln!(
        //     "[INLINE DEBUG] 创建块映射: callee {:?} -> caller {:?}",
        //     callee_block, caller_block
        // );

        // 注意：入口块不需要参数，会直接映射参数值
        if callee.layout.entry_block() != Some(callee_block) {
            // 为非入口块创建块参数
            for callee_param in callee.dfg.block_params(callee_block) {
                let ty = callee.dfg.value_type(*callee_param);
                let caller_param = caller.dfg.append_block_param(caller_block, ty);
                inline_map.set_inlined_value(*callee_param, caller_param);
                // eprintln!(
                //     "[INLINE DEBUG] 块参数映射: {:?} -> {:?}",
                //     callee_param, caller_param
                // );
            }
        }
    }
}

/// 分割调用点以创建返回块
fn split_off_return_block(
    func: &mut Function,
    call_inst: Inst,
    callee: &Function,
) -> Option<Block> {
    // 检查调用指令后是否有下一条指令
    func.layout.next_inst(call_inst).map(|next_inst| {
        let return_block = func.dfg.make_block();
        func.layout.split_block(return_block, next_inst);

        // 获取调用指令的所有返回值
        let old_values: Vec<_> = func.dfg.inst_results(call_inst).to_vec();

        // 分离指令结果
        func.dfg.detach_inst_results(call_inst);

        // 为每个返回值创建块参数，并建立别名
        for (i, old_val) in old_values.iter().enumerate() {
            // 使用callee的签名来获取正确的类型
            let ty = if i < callee.signature.returns.len() {
                callee.signature.returns[i]
            } else {
                // 如果是内存令牌，使用MemToken类型
                Type::MemToken
            };
            let block_param = func.dfg.append_block_param(return_block, ty);
            func.dfg.change_to_alias(*old_val, block_param);
        }

        // eprintln!(
        //     "[INLINE DEBUG] 创建返回块 {:?}，有 {} 个参数",
        //     return_block,
        //     old_values.len()
        // );

        return_block
    })
}

/// 替换调用指令为跳转
fn replace_call_with_jump(
    inline_map: &mut InlineMap,
    func: &mut Function,
    call_inst: Inst,
    callee: &Function,
    callee_entry: Block,
) {
    // eprintln!("[INLINE DEBUG] 替换调用指令 {:?} 为跳转", call_inst);

    let inlined_entry = inline_map.inlined_block(callee_entry);

    // 获取调用参数
    let call_args = func.dfg.inst_args(call_inst).to_vec();

    // 映射参数：调用参数对应到callee的入口块参数
    let callee_params = callee.dfg.block_params(callee_entry).to_vec();
    assert_eq!(
        call_args.len(),
        callee_params.len(),
        "调用参数数量与函数参数不匹配"
    );

    for (call_arg, callee_param) in call_args.iter().zip(callee_params.iter()) {
        inline_map.set_inlined_value(*callee_param, *call_arg);
        // eprintln!(
        //     "[INLINE DEBUG] 参数映射: {:?} -> {:?}",
        //     callee_param, call_arg
        // );
    }

    // 替换调用指令为跳转
    let empty_args = func.dfg.make_valuevec(&[]);
    func.dfg.replace_inst(
        call_inst,
        InstructionData::Jump {
            target: BlockCall {
                block: inlined_entry,
                args: empty_args,
            },
        },
    );
    // Jump指令不应该有返回值，清空原来的返回值
    func.dfg.detach_inst_results(call_inst);
    // eprintln!("[INLINE DEBUG] 已替换为跳转到入口块 {:?}", inlined_entry);
}

/// 内联块布局
fn inline_block_layout(
    caller: &mut Function,
    call_block: Block,
    callee: &Function,
    inline_map: &InlineMap,
) {
    // eprintln!("[INLINE DEBUG] 开始内联块布局");

    // 按照callee的布局顺序插入块
    let mut prev_block = call_block;
    for callee_block in callee.layout.blocks() {
        let inlined_block = inline_map.inlined_block(callee_block);
        caller.layout.insert_block_after(inlined_block, prev_block);
        prev_block = inlined_block;
        // eprintln!(
        //     "[INLINE DEBUG] 插入块 {:?} 在 {:?} 之后",
        //     inlined_block, prev_block
        // );
    }
}

/// 修复返回指令
fn fixup_inst_that_returns(
    inline_map: &InlineMap,
    func: &mut Function,
    callee: &Function,
    inlined_inst: Inst,
    callee_inst: Inst,
    return_block: Block,
) -> CEResult<()> {
    // 使用 callee_inst 作为真值来源，因为它包含原始的、未映射的返回值
    let return_value_vec = match &callee.dfg.insts[callee_inst] {
        InstructionData::Return { returns } => *returns,
        InstructionData::ReturnCall { args, .. } => *args,
        _ => unreachable!("fixup_inst_that_returns应该只处理返回指令"),
    };

    // 从 callee 的池中获取返回值的 Value 列表
    let callee_return_values = callee
        .dfg
        .value_vecs
        .as_slice(return_value_vec)
        .unwrap_or(&[]);

    // 创建一个重映射器，将 callee 的值映射到 caller 的值
    let mut remapper = InliningInstRemapper { inline_map, callee };

    // 映射所有返回值
    let caller_return_values: Vec<Value> = callee_return_values
        .iter()
        .map(|&v| remapper.map_value(v))
        .collect();

    // **修复**: 对于所有返回指令，只传递return_block期待的参数数量
    // 需要查看return_block的参数数量，而不是caller函数签名的返回值数量
    let expected_return_count = func.dfg.block_params(return_block).len();
    let filtered_return_values: Vec<Value> = caller_return_values
        .into_iter()
        .take(expected_return_count)
        .collect();

    // 替换 caller 中的 inlined_inst (它只是一个占位符) 为一个跳转
    let return_args = func.dfg.make_valuevec(&filtered_return_values);
    func.dfg.replace_inst(
        inlined_inst,
        InstructionData::Jump {
            target: BlockCall {
                block: return_block,
                args: return_args,
            },
        },
    );

    Ok(())
}

//==============================================================================
// 内联策略 -- 内联所有非SCC
//==============================================================================

/// 内联器，用于执行整个模块的内联操作
pub struct Inliner<'a> {
    core_ctx: &'a mut CoreContext,
    func_ctxs: &'a mut FuncContexts,
}

impl<'a> Inliner<'a> {
    pub fn new(core_ctx: &'a mut CoreContext, func_ctxs: &'a mut FuncContexts) -> Self {
        Self {
            core_ctx,
            func_ctxs,
        }
    }

    /// 执行激进的内联策略
    ///
    /// 该策略会内联所有非递归（非SCC）的函数。
    /// 内联顺序由SCC的拓扑排序确定，从调用图的叶子节点开始。
    pub fn run(&mut self) -> CEResult<bool> {
        let mut changed = false;

        // 1. 获取必要的上下文信息，并克隆以避免借用冲突
        let strata_layers: Vec<Vec<Scc>> = self
            .func_ctxs
            .strata
            .as_ref()
            .ok_or_else(|| CErr::new("执行内联前需要计算Strata"))?
            .layers()
            .map(|s| s.to_vec())
            .collect();

        let sccs_nodes: RefSecMap<Scc, Vec<FuncRef>> = self
            .func_ctxs
            .sccs
            .as_ref()
            .ok_or_else(|| CErr::new("执行内联前需要计算SCC"))?
            .all_nodes_cloned();

        // 2. 按照拓扑顺序 -- 从叶到根遍历所有SCC层
        for layer in strata_layers {
            for &scc_idx in &layer {
                let funcs_in_scc = &sccs_nodes[scc_idx];

                // 3. 只处理单节点SCC
                if funcs_in_scc.len() == 1 {
                    let callee_ref = funcs_in_scc[0];

                    // 4. 跳过main函数和自递归函数
                    if self.core_ctx.names[callee_ref] == "main"
                        || self.func_ctxs.call_graph.is_self_recursive(callee_ref)
                    {
                        continue;
                    }

                    // 5. 找到所有调用该函数的调用者
                    // 注意：这里需要构建反向调用图来找到正确的调用者
                    let reverse_call_graph = self.func_ctxs.call_graph.build_reverse_graph();
                    let callers: Vec<FuncRef> = reverse_call_graph.callees(callee_ref).to_vec();

                    if callers.is_empty() {
                        continue;
                    }

                    if std::env::var("RUST_LOG")
                        .unwrap_or_default()
                        .contains("debug")
                    {
                        // eprintln!(
                        //     "[INLINE STRATEGY] 准备内联函数: {:?} ({}) 到 {} 个调用点",
                        //     callee_ref,
                        //     self.core_ctx.names[callee_ref],
                        //     callers.len()
                        // );
                    }

                    // 6. 将该函数内联到所有调用点
                    for caller_ref in callers {
                        // 自己调用自己已经在SCC检查中排除了
                        if caller_ref == callee_ref {
                            continue;
                        }

                        if std::env::var("RUST_LOG")
                            .unwrap_or_default()
                            .contains("debug")
                        {
                            // eprintln!(
                            // "[INLINE STRATEGY] ==> 在 {:?} ({}) 中内联 {:?} ({})",
                            // caller_ref,
                            // self.core_ctx.names[caller_ref],
                            // callee_ref,
                            // self.core_ctx.names[callee_ref]
                            // );
                        }

                        let mut callees_to_inline = HashSet::new();
                        callees_to_inline.insert(callee_ref);
                        let inlined = inline_func(
                            caller_ref,
                            &callees_to_inline,
                            self.core_ctx,
                            self.func_ctxs,
                        )?;
                        if inlined {
                            changed = true;
                            // 内联后，调用者的调用关系可能发生变化，需要更新调用图
                            self.func_ctxs
                                .call_graph
                                .update_for_inlining(caller_ref, callee_ref);
                        }
                    }
                }
            }
        }
        // 3. 移除已内联的函数
        if changed {
            self.remove_inlined_func()?;
        }

        Ok(changed)
    }

    /// 移除已完全内联的函数
    fn remove_inlined_func(&mut self) -> CEResult<()> {
        // 构建反向调用图来检查哪些函数还有调用者
        let reverse_call_graph = self.func_ctxs.call_graph.build_reverse_graph();

        // 收集需要移除的函数
        let mut functions_to_remove = Vec::new();

        for func_ref in self.func_ctxs.ctxs.keys() {
            let func_name = &self.core_ctx.names[func_ref];

            if func_name == "main" {
                continue;
            }

            // 检查是否还有调用者
            let callers = reverse_call_graph.callees(func_ref); // 在反向图中 callees 实际上是调用者
            if callers.is_empty() {
                functions_to_remove.push(func_ref);

                // if std::env::var("RUST_LOG")
                //     .unwrap_or_default()
                //     .contains("debug")
                // {
                //     eprintln!(
                //         "[INLINE CLEANUP] 移除已完全内联的函数: {} ({})",
                //         func_name, func_ref.index()
                //     );
                // }
            }
        }

        // 从 func_ctxs 中移除这些函数
        for func_ref in functions_to_remove {
            self.func_ctxs.ctxs.remove(func_ref);
        }

        Ok(())
    }
}
