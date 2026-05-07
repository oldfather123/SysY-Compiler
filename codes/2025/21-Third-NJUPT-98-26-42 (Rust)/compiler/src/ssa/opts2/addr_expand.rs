//! 将高级数组操作展开为低级内存操作
//!
//! ArrayAlloc/Get/Put/Slice和全局操作转换为低级指令：
//! - ArrayAlloc -> StackSlot + StackAddr
//! - ArrayGet/Put -> 地址计算 + Load/Store
//! - ArraySlice -> 地址计算
//! - GlobalScalarGet/Set -> GlobalAddr + Load/Store
//!
//! 内存语义仍然被维护，并且几乎和ArrayXXX的方法逻辑完全一致:
//! - (ptr,token) = StackAddr ...
//! - (ptr, token) = GlobalAddr
//! - val = Load .. token (使用token)
//! - token = Store .. token (消费一个token，并产生一个新token)

use crate::prelude::*;
use crate::ssa::{
    CoreContext,
    cursor::{Cursor, CursorPosition, FuncCursor},
    domtree::DominatorTree,
    function::Function,
    inst_builder::InstBuilder,
    ir::{Global, *},
};
use std::collections::{HashMap, HashSet};

pub struct AddressExpander<'a, 'c> {
    func: &'a mut Function,
    core_ctx: &'c mut CoreContext,
    domtree: &'a DominatorTree,
    /// 是否启用延迟插入优化：如果支配块内有使用，则在首次使用前插入而非块顶端
    precise_insertion: bool,
}

/// 全局变量的插入策略
#[derive(Debug, Clone)]
struct GlobalInsertionStrategy {
    /// 插入块
    block: Block,
    /// 如果为Some，表示应在该指令之前插入；如果为None，表示在块顶端插入
    before_inst: Option<Inst>,
}

/// 过程中状态
struct ExpanderState {
    /// ArrayAlloc指令到StackSlot的映射
    alloc_to_slot: HashMap<Inst, StackSlot>,
    /// 值的重映射表
    value_remap: HashMap<Value, Value>,
    /// 指针类型信息缓存
    ptr_info: HashMap<Value, PtrInfo>,
    /// 原始的别名关系  -- 别名 -> 原始值
    original_aliases: HashMap<Value, Value>,
    /// 全局地址缓存 - 避免重复生成global.addr
    /// 映射: Global -> (地址值, 对应的token值)
    global_addr_cache: HashMap<Global, (Value, Value)>,
    /// 预计算的全局变量地址指令的最佳插入策略
    /// 映射: Global -> 插入策略
    global_insertion_strategies: HashMap<Global, GlobalInsertionStrategy>,
}

/// 指针的类型信息
#[derive(Debug, Clone)]
struct PtrInfo {
    elem_ty: ArrayElemType,
    dims: U32OptVec,
}

impl<'a, 'c> AddressExpander<'a, 'c> {
    pub fn new(
        func: &'a mut Function,
        core_ctx: &'c mut CoreContext,
        domtree: &'a DominatorTree,
    ) -> Self {
        Self {
            func,
            core_ctx,
            domtree,
            precise_insertion: true, // 默认启用精确插入
        }
    }

    /// 运行地址展开pass
    pub fn run(mut self) {
        let mut state = ExpanderState {
            alloc_to_slot: HashMap::default(),
            value_remap: HashMap::default(),
            ptr_info: HashMap::default(),
            original_aliases: HashMap::default(),
            global_addr_cache: HashMap::default(),
            global_insertion_strategies: HashMap::default(),
        };

        // 0. 收集原始的别名关系
        self.collect_original_aliases(&mut state);

        // 1. 预计算所有全局变量的最佳插入点
        self.precompute_global_insertion_points(&mut state);

        // 2. 收集所有ArrayAlloc并创建StackSlot
        self.collect_allocations(&mut state);

        // 3. 收集所有指针类型信息
        self.collect_pointer_info(&mut state);

        // 4. 更新所有ArrayPtr类型为Int64
        self.update_types_to_i64(&state);

        // 5. 展开所有指令
        self.expand_instructions(&mut state);

        // 6. 更新所有分支指令中的参数
        self.update_branch_arguments(&state);

        // 7. 应用别名映射
        self.apply_aliases(&state);
    }

    /// 收集原始的别名关系
    fn collect_original_aliases(&self, state: &mut ExpanderState) {
        for (value, value_data) in self.func.dfg.all_values().iter() {
            if let ValueData::Alias { original, .. } = value_data {
                state.original_aliases.insert(value, *original);
            }
        }
    }

    /// 预计算每个全局变量的最佳插入点 (LCA of all uses)
    fn precompute_global_insertion_points(&self, state: &mut ExpanderState) {
        // 记录每个全局变量在每个块中的使用情况
        // Global -> (Block -> Option<首次使用的指令>)
        let mut global_uses: HashMap<Global, HashMap<Block, Option<Inst>>> = HashMap::new();

        // Pass 1: 收集每个全局变量的所有使用块和首次使用位置
        for block in self.func.layout.blocks() {
            for inst in self.func.layout.block_insts(block) {
                let inst_data = &self.func.dfg.insts[inst];

                // 辅助闭包：记录全局变量使用
                let mut record_global_use = |sym: Global| {
                    let block_uses = global_uses.entry(sym).or_default();
                    // 只记录每个块中的第一次使用
                    block_uses.entry(block).or_insert(Some(inst));
                };

                match inst_data {
                    // ArrayGet/Put/Slice 可能使用全局符号作为指针
                    InstructionData::ArrayGet { ptr, .. }
                    | InstructionData::ArrayPut { ptr, .. }
                    | InstructionData::ArraySlice { ptr, .. } => {
                        if let ValueData::GlobalSymbol { sym, .. } = self.func.dfg.valuedata(*ptr) {
                            record_global_use(*sym);
                        }
                    }
                    // GlobalScalarGet/Set 直接使用全局符号
                    InstructionData::GlobalScalarGet { global, .. }
                    | InstructionData::GlobalScalarSet { global, .. } => {
                        if let ValueData::GlobalSymbol { sym, .. } =
                            self.func.dfg.valuedata(*global)
                        {
                            record_global_use(*sym);
                        }
                    }
                    // Call/ReturnCall 的参数可能包含全局符号
                    InstructionData::Call { args, .. }
                    | InstructionData::ReturnCall { args, .. } => {
                        for &arg in &self.func.dfg.value_vecs[*args] {
                            if let ValueData::GlobalSymbol { sym, .. } =
                                self.func.dfg.valuedata(arg)
                            {
                                record_global_use(*sym);
                            }
                        }
                    }
                    _ => {}
                }
            }
        }

        // Pass 2: 为每个全局变量计算其使用块集合的LCA和插入策略
        for (global, block_uses) in global_uses {
            if block_uses.is_empty() {
                continue;
            }

            let use_blocks: Vec<Block> = block_uses.keys().copied().collect();

            // 计算LCA
            let mut lca = use_blocks[0];
            for &other_block in &use_blocks[1..] {
                lca = self.find_lca(lca, other_block);
            }

            // 决定插入策略
            let strategy = if self.precise_insertion {
                // 如果LCA块本身有使用，则在首次使用前插入
                if let Some(first_use) = block_uses.get(&lca).and_then(|&opt| opt) {
                    GlobalInsertionStrategy {
                        block: lca,
                        before_inst: Some(first_use),
                    }
                } else {
                    // LCA块没有使用，在块顶端插入
                    GlobalInsertionStrategy {
                        block: lca,
                        before_inst: None,
                    }
                }
            } else {
                // 禁用精确插入时，总是在块顶端插入
                GlobalInsertionStrategy {
                    block: lca,
                    before_inst: None,
                }
            };

            // 存储插入策略
            state.global_insertion_strategies.insert(global, strategy);
        }
    }

    /// 辅助函数：在支配树中查找两个块的LCA
    fn find_lca(&self, block_a: Block, block_b: Block) -> Block {
        // 使用一个集合来记录block_a到根的路径
        let mut ancestors = HashSet::new();

        // 记录block_a到根的所有祖先
        let mut current = block_a;
        loop {
            ancestors.insert(current);
            match self.domtree.idom(current) {
                Some(idom) => current = idom,
                None => break, // 到达根节点
            }
        }

        // 从block_b向上查找，直到找到第一个在ancestors中的块
        current = block_b;
        loop {
            if ancestors.contains(&current) {
                return current;
            }
            match self.domtree.idom(current) {
                Some(idom) => current = idom,
                None => {
                    // 如果block_b的路径没有与block_a的路径相交，说明至少有一个不可达
                    // 这种情况下返回入口块作为fallback
                    return self
                        .func
                        .layout
                        .entry_block()
                        .expect("Function must have an entry block");
                }
            }
        }
    }

    /// 收集所有ArrayAlloc指令并创建对应的StackSlot
    fn collect_allocations(&mut self, state: &mut ExpanderState) {
        let mut allocs = Vec::new();

        // 收集所有ArrayAlloc指令
        for block in self.func.layout.blocks() {
            for inst in self.func.layout.block_insts(block) {
                if matches!(
                    self.func.dfg.insts[inst],
                    InstructionData::ArrayAlloc { .. }
                ) {
                    allocs.push(inst);
                }
            }
        }

        // 为每个ArrayAlloc创建StackSlot
        for inst in allocs {
            if let InstructionData::ArrayAlloc {
                elem,
                dims,
                is_const,
                init,
                ..
            } = self.func.dfg.insts[inst]
            {
                let _elem_ty =
                    ArrayElemType::from_type(elem).expect("Array element must be primitive type");
                let elem_size = elem.abi_size();

                // 计算数组大小
                let dims_vec = &self.core_ctx.type_dims[dims];
                let mut total_size = elem_size;
                for &dim in dims_vec.iter() {
                    match dim {
                        Some(d) => total_size *= d as usize,
                        None => panic!("Dynamic array allocation not supported"),
                    }
                }

                // 创建StackSlot
                let align = elem_size.next_power_of_two().trailing_zeros() as u8;
                let slot = self
                    .func
                    .dfg
                    .stack_slots
                    .add(elem, is_const, total_size, align, init);
                state.alloc_to_slot.insert(inst, slot);
            }
        }
    }

    /// 收集所有指针的类型信息
    fn collect_pointer_info(&self, state: &mut ExpanderState) {
        for (value, value_data) in self.func.dfg.all_values().iter() {
            if let Type::ArrayPtr { elem, dims } = value_data.ty() {
                state.ptr_info.insert(
                    value,
                    PtrInfo {
                        elem_ty: elem,
                        dims,
                    },
                );
            }
        }
    }

    /// 将所有ArrayPtr类型更新为Int64
    fn update_types_to_i64(&mut self, state: &ExpanderState) {
        // 更新值的类型
        for (value, _) in state.ptr_info.iter() {
            let value_data = self.func.dfg.valuedata_mut(*value);
            match value_data {
                ValueData::Inst { ty, .. } => *ty = Type::Int64,
                ValueData::BlockParam { ty, .. } => *ty = Type::Int64,
                ValueData::Alias { ty, .. } => *ty = Type::Int64,
                _ => {}
            }
        }

        // 更新函数签名
        for param_ty in &mut self.func.signature.params {
            if matches!(param_ty, Type::ArrayPtr { .. }) {
                *param_ty = Type::Int64;
            }
        }
        for ret_ty in &mut self.func.signature.returns {
            if matches!(ret_ty, Type::ArrayPtr { .. }) {
                *ret_ty = Type::Int64;
            }
        }
    }

    /// 展开所有高级数组操作为低级操作
    fn expand_instructions(&mut self, state: &mut ExpanderState) {
        // 收集所有需要处理的指令
        let mut instructions_to_process = Vec::new();

        for block in self.func.layout.blocks() {
            for inst in self.func.layout.block_insts(block) {
                let inst_data = &self.func.dfg.insts[inst];
                match inst_data {
                    InstructionData::ArrayAlloc { .. }
                    | InstructionData::ArrayGet { .. }
                    | InstructionData::ArrayPut { .. }
                    | InstructionData::ArraySlice { .. }
                    | InstructionData::GlobalScalarGet { .. }
                    | InstructionData::GlobalScalarSet { .. }
                    | InstructionData::Call { .. }
                    | InstructionData::ReturnCall { .. } => {
                        instructions_to_process.push((block, inst, *inst_data));
                    }
                    _ => {}
                }
            }
        }

        // 处理收集到的指令
        for (block, inst, inst_data) in instructions_to_process {
            // 为每条指令创建新的cursor
            let mut cursor = FuncCursor::new(self.func, self.core_ctx);
            cursor.goto_first_insertion_point(block);
            cursor.set_position(CursorPosition::At(inst));

            match inst_data {
                InstructionData::ArrayAlloc { .. } => {
                    Self::expand_array_alloc(&mut cursor, inst, state);
                }
                InstructionData::ArrayGet {
                    ptr,
                    indices,
                    token,
                } => {
                    Self::expand_array_get(&mut cursor, inst, ptr, indices, token, state);
                }
                InstructionData::ArrayPut {
                    ptr,
                    indices,
                    value,
                    token,
                } => {
                    Self::expand_array_put(&mut cursor, inst, ptr, indices, value, token, state);
                }
                InstructionData::ArraySlice { ptr, sdims } => {
                    Self::expand_array_slice(&mut cursor, inst, ptr, sdims, state);
                }
                InstructionData::GlobalScalarGet { global, token } => {
                    Self::expand_global_scalar_get(&mut cursor, inst, global, token, state);
                }
                InstructionData::GlobalScalarSet {
                    global,
                    value,
                    token,
                } => {
                    Self::expand_global_scalar_set(&mut cursor, inst, global, value, token, state);
                }
                InstructionData::Call { func, args } => {
                    Self::expand_call(&mut cursor, inst, func, args, state);
                }
                InstructionData::ReturnCall { func, args } => {
                    Self::expand_return_call(&mut cursor, inst, func, args, state);
                }
                _ => {}
            }
        }
    }

    /// 更新所有分支指令中的参数
    fn update_branch_arguments(&mut self, state: &ExpanderState) {
        // 遍历所有块
        for block in self.func.layout.blocks() {
            // 获取块的终结指令
            if let Some(terminator) = self.func.layout.last_inst(block) {
                let inst_data = &self.func.dfg.insts[terminator];

                // 检查是否是分支指令
                match inst_data {
                    InstructionData::Jump { target } => {
                        let target_block = target.block;
                        let args = target.args;
                        let mut updated_args = Vec::new();
                        let mut needs_update = false;

                        // 检查并更新每个参数
                        for &arg in &self.func.dfg.value_vecs[args] {
                            let resolved_arg = Self::resolve_value(arg, state);

                            updated_args.push(resolved_arg);
                            if resolved_arg != arg {
                                needs_update = true;
                            }
                        }

                        // 如果有参数被重映射，更新跳转指令
                        if needs_update {
                            let new_args = self.func.dfg.value_vecs.insert(updated_args);
                            self.func.dfg.insts[terminator] = InstructionData::Jump {
                                target: BlockCall {
                                    block: target_block,
                                    args: new_args,
                                },
                            };
                        }
                    }
                    InstructionData::Brif { cond, targets } => {
                        let cond = *cond;
                        let resolved_cond = Self::resolve_value(cond, state);
                        let mut needs_update = resolved_cond != cond;

                        // 处理true分支 (targets[0])
                        let true_block = targets[0].block;
                        let true_args = targets[0].args;
                        let mut updated_true_args = Vec::new();
                        for &arg in &self.func.dfg.value_vecs[true_args] {
                            let resolved_arg = Self::resolve_value(arg, state);
                            updated_true_args.push(resolved_arg);
                            if resolved_arg != arg {
                                needs_update = true;
                            }
                        }

                        // 处理false分支 (targets[1])
                        let false_block = targets[1].block;
                        let false_args = targets[1].args;
                        let mut updated_false_args = Vec::new();
                        for &arg in &self.func.dfg.value_vecs[false_args] {
                            let resolved_arg = Self::resolve_value(arg, state);
                            updated_false_args.push(resolved_arg);
                            if resolved_arg != arg {
                                needs_update = true;
                            }
                        }

                        // 如果有参数被重映射，更新分支指令
                        if needs_update {
                            let new_true_args = self.func.dfg.value_vecs.insert(updated_true_args);
                            let new_false_args =
                                self.func.dfg.value_vecs.insert(updated_false_args);
                            self.func.dfg.insts[terminator] = InstructionData::Brif {
                                cond: resolved_cond,
                                targets: [
                                    BlockCall {
                                        block: true_block,
                                        args: new_true_args,
                                    },
                                    BlockCall {
                                        block: false_block,
                                        args: new_false_args,
                                    },
                                ],
                            };
                        }
                    }
                    _ => {}
                }
            }
        }
    }

    /// 展开ArrayAlloc为StackAddr
    fn expand_array_alloc(cursor: &mut FuncCursor, inst: Inst, state: &mut ExpanderState) {
        let slot = state.alloc_to_slot[&inst];
        let old_ptr = cursor.func.dfg.inst_results(inst)[0];
        let old_token = cursor.func.dfg.inst_results(inst)[1];

        // 创建StackAddr指令
        let stack_addr_inst = cursor.ins().stack_addr(slot);
        let new_ptr = cursor.func.dfg.inst_results(stack_addr_inst)[0];
        let new_token = cursor.func.dfg.inst_results(stack_addr_inst)[1];

        // 记录重映射
        state.value_remap.insert(old_ptr, new_ptr);
        state.value_remap.insert(old_token, new_token);

        // 复制指针信息
        if let Some(info) = state.ptr_info.remove(&old_ptr) {
            state.ptr_info.insert(new_ptr, info);
        }

        // 删除原指令
        cursor.remove_inst_and_step_back();
    }

    /// 展开ArrayGet为地址计算+Load
    fn expand_array_get(
        cursor: &mut FuncCursor,
        inst: Inst,
        ptr: Value,
        indices: ValueVec,
        token: Value,
        state: &mut ExpanderState,
    ) {
        let (base_addr, final_token) = Self::get_base_address(cursor, ptr, token, state);
        let indices_vec = cursor.func.dfg.value_vecs[indices].clone();

        // 解析ptr的重映射以正确查找类型信息
        let resolved_ptr = Self::resolve_value(ptr, state);

        // 计算最终地址
        let final_addr = if let Some(info) = state.ptr_info.get(&resolved_ptr).cloned() {
            Self::calculate_address(cursor, base_addr, &indices_vec, &info)
        } else if let Some(info) = state.ptr_info.get(&ptr).cloned() {
            // 尝试使用原始ptr查找
            Self::calculate_address(cursor, base_addr, &indices_vec, &info)
        } else {
            // 没有类型信息时的fallback（如函数参数）
            Self::calculate_address_simple(cursor, base_addr, &indices_vec)
        };

        // 获取结果类型
        let result_ty = cursor.func.dfg.inst_result_type(inst, 0);

        // 创建Load指令
        let load_val = cursor.ins().load(final_addr, 0, result_ty, final_token);

        // 记录重映射
        let old_result = cursor.func.dfg.inst_results(inst)[0];
        state.value_remap.insert(old_result, load_val);

        // 删除原指令
        cursor.remove_inst_and_step_back();
    }

    /// 展开ArrayPut为地址计算+Store
    fn expand_array_put(
        cursor: &mut FuncCursor,
        inst: Inst,
        ptr: Value,
        indices: ValueVec,
        value: Value,
        token: Value,
        state: &mut ExpanderState,
    ) {
        // 检查ptr是否是全局符号
        let _is_global = if let ValueData::GlobalSymbol { sym, .. } = cursor.func.dfg.valuedata(ptr)
        {
            Some(*sym)
        } else {
            None
        };

        let (base_addr, final_token) = Self::get_base_address(cursor, ptr, token, state);
        let indices_vec = cursor.func.dfg.value_vecs[indices].clone();

        // 解析ptr的重映射以正确查找类型信息
        let resolved_ptr = Self::resolve_value(ptr, state);

        // 计算最终地址
        let final_addr = if let Some(info) = state.ptr_info.get(&resolved_ptr).cloned() {
            Self::calculate_address(cursor, base_addr, &indices_vec, &info)
        } else if let Some(info) = state.ptr_info.get(&ptr).cloned() {
            // 尝试使用原始ptr查找
            Self::calculate_address(cursor, base_addr, &indices_vec, &info)
        } else {
            // 没有类型信息时的fallback（如函数参数）
            Self::calculate_address_simple(cursor, base_addr, &indices_vec)
        };

        // 创建Store指令
        let value_ty = cursor.func.dfg.value_type(value);
        let new_token = cursor
            .ins()
            .store(final_addr, 0, value, value_ty, final_token);

        // 记录重映射
        let old_result = cursor.func.dfg.inst_results(inst)[0];
        state.value_remap.insert(old_result, new_token);

        // Debug: Track token updates

        // 删除原指令
        cursor.remove_inst_and_step_back();
    }

    /// 展开ArraySlice为地址计算
    fn expand_array_slice(
        cursor: &mut FuncCursor,
        inst: Inst,
        ptr: Value,
        sdims: ValueVec,
        state: &mut ExpanderState,
    ) {
        let dummy_token = cursor.ins().iconst32(0); // slice不需要token
        let (base_addr, _) = Self::get_base_address(cursor, ptr, dummy_token, state);
        let sdims_vec = cursor.func.dfg.value_vecs[sdims].clone();

        // 解析ptr的重映射以正确查找类型信息
        let resolved_ptr = Self::resolve_value(ptr, state);

        // 计算切片地址
        let slice_addr = if let Some(info) = state.ptr_info.get(&resolved_ptr).cloned() {
            let offset_addr = Self::calculate_address(cursor, base_addr, &sdims_vec, &info);

            // 即使偏移为0，也创建一个新的值来携带正确的类型信息
            // 这避免了多个切片共享同一个地址值导致的类型信息覆盖问题
            let new_addr = if offset_addr == base_addr {
                // 创建一个 iadd 0 来产生新的值
                let zero = cursor.ins().iconst64(0);
                cursor.ins().iadd(base_addr, zero)
            } else {
                offset_addr
            };

            // 创建新的指针信息（去掉前面的维度）
            let original_dims = &cursor.type_dims()[info.dims];
            let new_dims_vec: Vec<_> = original_dims
                .iter()
                .skip(sdims_vec.len())
                .cloned()
                .collect();
            let new_dims = cursor.core_ctx_mut().type_dims.insert(new_dims_vec);
            state.ptr_info.insert(
                new_addr,
                PtrInfo {
                    elem_ty: info.elem_ty,
                    dims: new_dims,
                },
            );

            new_addr
        } else if let Some(info) = state.ptr_info.get(&ptr).cloned() {
            let offset_addr = Self::calculate_address(cursor, base_addr, &sdims_vec, &info);

            // 即使偏移为0，也创建一个新的值来携带正确的类型信息
            let new_addr = if offset_addr == base_addr {
                let zero = cursor.ins().iconst64(0);
                cursor.ins().iadd(base_addr, zero)
            } else {
                offset_addr
            };

            // 创建新的指针信息（去掉前面的维度）
            let original_dims = &cursor.type_dims()[info.dims];
            let new_dims_vec: Vec<_> = original_dims
                .iter()
                .skip(sdims_vec.len())
                .cloned()
                .collect();
            let new_dims = cursor.core_ctx_mut().type_dims.insert(new_dims_vec);
            state.ptr_info.insert(
                new_addr,
                PtrInfo {
                    elem_ty: info.elem_ty,
                    dims: new_dims,
                },
            );

            new_addr
        } else {
            // 没有类型信息时的fallback
            Self::calculate_address_simple(cursor, base_addr, &sdims_vec)
        };

        // 记录重映射
        let old_result = cursor.func.dfg.inst_results(inst)[0];
        state.value_remap.insert(old_result, slice_addr);

        // 删除原指令
        cursor.remove_inst_and_step_back();
    }

    /// 展开GlobalScalarGet
    fn expand_global_scalar_get(
        cursor: &mut FuncCursor,
        inst: Inst,
        global: Value,
        token: Value,
        state: &mut ExpanderState,
    ) {
        let value_data = *cursor.func.dfg.valuedata(global);
        let _global_sym = if let ValueData::GlobalSymbol { sym, .. } = value_data {
            sym
        } else {
            panic!("GlobalScalarGet expects a global symbol");
        };

        // 使用统一的get_base_address获取地址（使用缓存）
        let (addr, new_token) = Self::get_base_address(cursor, global, token, state);

        // 获取结果类型
        let result_ty = cursor.func.dfg.inst_result_type(inst, 0);

        // 创建Load指令
        let load_val = cursor.ins().load(addr, 0, result_ty, new_token);

        // 记录重映射
        let old_result = cursor.func.dfg.inst_results(inst)[0];
        state.value_remap.insert(old_result, load_val);

        // 删除原指令
        cursor.remove_inst_and_step_back();
    }

    /// 展开GlobalScalarSet
    fn expand_global_scalar_set(
        cursor: &mut FuncCursor,
        inst: Inst,
        global: Value,
        value: Value,
        token: Value,
        state: &mut ExpanderState,
    ) {
        let value_data = *cursor.func.dfg.valuedata(global);
        let _global_sym = if let ValueData::GlobalSymbol { sym, .. } = value_data {
            sym
        } else {
            panic!("GlobalScalarSet expects a global symbol");
        };

        // 使用统一的get_base_address获取地址（使用缓存）
        let (addr, new_token) = Self::get_base_address(cursor, global, token, state);

        // 创建Store指令
        let value_ty = cursor.func.dfg.value_type(value);
        let store_token = cursor.ins().store(addr, 0, value, value_ty, new_token);

        // 记录重映射
        let old_result = cursor.func.dfg.inst_results(inst)[0];
        state.value_remap.insert(old_result, store_token);

        // 删除原指令
        cursor.remove_inst_and_step_back();
    }

    /// 获取基地址，处理全局符号（按需插入）
    fn get_base_address(
        cursor: &mut FuncCursor,
        ptr: Value,
        token: Value,
        state: &mut ExpanderState,
    ) -> (Value, Value) {
        // 应用重映射
        let ptr = Self::resolve_value(ptr, state);
        let token = Self::resolve_value(token, state);

        // 处理全局符号
        let value_data = *cursor.func.dfg.valuedata(ptr);
        if let ValueData::GlobalSymbol { sym, .. } = value_data {
            // 检查缓存
            if let Some(&(cached_addr, _cached_token)) = state.global_addr_cache.get(&sym) {
                // 缓存命中，直接返回
                // 注意：这里的token逻辑需要小心，我们应该使用传入的token
                return (cached_addr, token);
            } else {
                // 缓存未命中，这是第一次遇到这个全局变量
                // 1. 获取预计算好的插入策略
                let strategy = state
                    .global_insertion_strategies
                    .get(&sym)
                    .expect("Insertion strategy should have been precomputed for all used globals");

                // 2. 保存当前cursor的位置
                let current_block = cursor
                    .current_block()
                    .expect("Cursor must be at a valid position");
                let current_inst = cursor.current_inst();

                // 3. 根据策略移动cursor到插入位置
                if let Some(before_inst) = strategy.before_inst {
                    // 精确插入：在指定指令之前插入
                    cursor.goto_first_insertion_point(strategy.block);
                    cursor.set_position(CursorPosition::At(before_inst));
                } else {
                    // 块顶端插入
                    cursor.goto_first_insertion_point(strategy.block);
                }

                // 4. 插入 global_addr 指令
                let global_addr_inst = cursor.ins().global_addr(sym);
                let new_addr = cursor.func.dfg.inst_results(global_addr_inst)[0];
                let new_token = cursor.func.dfg.inst_results(global_addr_inst)[1];

                // 5. 恢复cursor的位置
                cursor.goto_first_insertion_point(current_block);
                if let Some(inst) = current_inst {
                    cursor.set_position(CursorPosition::At(inst));
                }

                // 6. 更新缓存
                state.global_addr_cache.insert(sym, (new_addr, new_token));

                // 7. 返回新生成的地址和传入的token
                // 为什么是传入的token？因为global_addr自身虽然产生token，但那个token的逻辑位置
                // 是在插入点。在当前使用点，我们必须遵守当前的数据流依赖，使用传入的token。
                return (new_addr, token);
            }
        }

        // 确保地址是Int64类型
        let addr = if cursor.func.dfg.value_type(ptr) == Type::Int64 {
            ptr
        } else {
            cursor.ins().cast(ptr, Type::Int64)
        };

        (addr, token)
    }

    /// 计算数组地址 -- 有类型信息
    fn calculate_address(
        cursor: &mut FuncCursor,
        base_addr: Value,
        indices: &[Value],
        info: &PtrInfo,
    ) -> Value {
        if indices.is_empty() {
            return base_addr;
        }

        let dims = &cursor.type_dims()[info.dims];
        let mut strides = vec![0usize; dims.len()];

        // 计算步长
        if !dims.is_empty() {
            strides[dims.len() - 1] = info.elem_ty.to_type().abi_size();
            for i in (0..dims.len() - 1).rev() {
                let dim = dims[i + 1].expect("Dynamic dims not supported") as usize;
                strides[i] = strides[i + 1] * dim;
            }
        }

        // 计算偏移
        let mut final_addr = base_addr;
        for (i, &index) in indices.iter().enumerate() {
            if i >= strides.len() || strides[i] == 0 {
                continue;
            }

            // 优化：如果索引是常量0，跳过整个偏移计算
            let value_data = *cursor.func.dfg.valuedata(index);
            match value_data {
                ValueData::Const {
                    c: ConstValue::Int32 { val: 0 },
                    ..
                } => {
                    // 索引为0，不需要任何偏移
                    continue;
                }
                ValueData::Const {
                    c: ConstValue::Int32 { val },
                    ..
                } => {
                    // 索引是非0常量，直接计算偏移
                    let offset_val = cursor.ins().iconst64((strides[i] as i64) * (val as i64));
                    final_addr = cursor.ins().iadd(final_addr, offset_val);
                }
                _ => {
                    // 索引不是常量，需要运行时计算
                    let stride_val = cursor.ins().iconst64(strides[i] as i64);
                    let index_i64 = cursor.ins().cast(index, Type::Int64);
                    let offset = cursor.ins().imul(index_i64, stride_val);
                    final_addr = cursor.ins().iadd(final_addr, offset);
                }
            }
        }

        final_addr
    }

    /// 计算数组地址（无类型信息，假设4字节元素）
    fn calculate_address_simple(
        cursor: &mut FuncCursor,
        base_addr: Value,
        indices: &[Value],
    ) -> Value {
        let mut final_addr = base_addr;

        for (i, &index) in indices.iter().enumerate() {
            // 优化：如果索引是常量0，跳过整个偏移计算
            let value_data = *cursor.func.dfg.valuedata(index);
            match value_data {
                ValueData::Const {
                    c: ConstValue::Int32 { val: 0 },
                    ..
                } => {
                    // 索引为0，不需要任何偏移
                    continue;
                }
                ValueData::Const {
                    c: ConstValue::Int32 { val },
                    ..
                } => {
                    // 索引是非0常量，直接计算偏移（假设4字节元素）
                    let offset_val = cursor.ins().iconst64(4 * (val as i64));
                    final_addr = cursor.ins().iadd(final_addr, offset_val);
                }
                _ => {
                    // 索引不是常量，需要运行时计算
                    let stride_val = cursor.ins().iconst64(4); // 假设4字节元素
                    let index_i64 = cursor.ins().cast(index, Type::Int64);
                    let offset = cursor.ins().imul(index_i64, stride_val);
                    final_addr = cursor.ins().iadd(final_addr, offset);
                }
            }
        }

        final_addr
    }

    /// 展开Call指令中的全局符号参数
    fn expand_call(
        cursor: &mut FuncCursor,
        inst: Inst,
        func: FuncRef,
        args: ValueVec,
        state: &mut ExpanderState,
    ) {
        let original_args = cursor.func.dfg.value_vecs[args].clone();
        let mut new_args = Vec::new();
        let mut needs_update = false;

        // 处理每个参数，需要成对处理指针和令牌
        let mut i = 0;
        while i < original_args.len() {
            let arg = original_args[i];
            let resolved_arg = Self::resolve_value(arg, state);

            // 检查是否是全局符号
            let value_data = *cursor.func.dfg.valuedata(resolved_arg);
            if let ValueData::GlobalSymbol { sym, ty } = value_data {
                if matches!(ty, Type::ArrayPtr { .. }) {
                    needs_update = true;

                    // 检查下一个参数是否是对应的内存令牌
                    if i + 1 < original_args.len() {
                        let next_arg = original_args[i + 1];
                        let resolved_next = Self::resolve_value(next_arg, state);
                        let next_data = *cursor.func.dfg.valuedata(resolved_next);

                        if let ValueData::GlobalSymbol {
                            sym: next_sym,
                            ty: next_ty,
                        } = next_data
                        {
                            if next_sym == sym && matches!(next_ty, Type::MemToken) {
                                // 找到配对的令牌，使用get_base_address获取实际的地址和令牌
                                let (addr, token) = Self::get_base_address(
                                    cursor,
                                    resolved_arg,
                                    resolved_next,
                                    state,
                                );
                                new_args.push(addr);
                                new_args.push(token);
                                i += 2; // 跳过下一个参数
                                continue;
                            }
                        }
                    }

                    // 如果没有找到配对的令牌，尝试从缓存获取
                    if let Some(&(cached_addr, _)) = state.global_addr_cache.get(&sym) {
                        new_args.push(cached_addr);
                    } else {
                        // 没有配对令牌，创建一个虚拟令牌来调用get_base_address
                        let dummy_token = resolved_arg; // 使用自身作为虚拟令牌
                        let (addr, _) =
                            Self::get_base_address(cursor, resolved_arg, dummy_token, state);
                        new_args.push(addr);
                    }
                } else if matches!(ty, Type::MemToken) {
                    needs_update = true;

                    // 对于单独的内存令牌，从缓存获取
                    if let Some(&(_, cached_token)) = state.global_addr_cache.get(&sym) {
                        new_args.push(cached_token);
                    } else {
                        // 如果缓存中没有，创建地址和令牌
                        // 这种情况比较特殊，通常不应该发生
                        let dummy_ptr = resolved_arg; // 使用自身作为虚拟指针
                        let (_, token) =
                            Self::get_base_address(cursor, dummy_ptr, resolved_arg, state);
                        new_args.push(token);
                    }
                } else {
                    // 其他类型的全局符号保持不变
                    new_args.push(resolved_arg);
                }
            } else {
                // 非全局符号，直接使用解析后的值
                new_args.push(resolved_arg);
            }

            i += 1;
        }

        // 如果有参数被展开，更新Call指令
        if needs_update {
            let new_args_vec = cursor.func.dfg.value_vecs.insert(new_args);
            cursor.func.dfg.insts[inst] = InstructionData::Call {
                func,
                args: new_args_vec,
            };
        }
    }

    /// 展开ReturnCall指令中的全局符号参数
    fn expand_return_call(
        cursor: &mut FuncCursor,
        inst: Inst,
        func: FuncRef,
        args: ValueVec,
        state: &mut ExpanderState,
    ) {
        let original_args = cursor.func.dfg.value_vecs[args].clone();
        let mut new_args = Vec::new();
        let mut needs_update = false;

        // 处理每个参数（与expand_call相同的逻辑）
        for &arg in &original_args {
            let resolved_arg = Self::resolve_value(arg, state);

            // 检查是否是全局符号
            let value_data = *cursor.func.dfg.valuedata(resolved_arg);
            if let ValueData::GlobalSymbol { sym, ty } = value_data {
                if matches!(ty, Type::ArrayPtr { .. }) {
                    needs_update = true;

                    // 从缓存获取地址（应该在入口块已经生成）
                    if let Some(&(cached_addr, _)) = state.global_addr_cache.get(&sym) {
                        new_args.push(cached_addr);
                    } else {
                        panic!(
                            "Global symbol {:?} not found in cache during ReturnCall expansion!",
                            sym
                        );
                    }
                } else if matches!(ty, Type::MemToken) {
                    needs_update = true;

                    // 对于内存令牌，从缓存获取
                    if let Some(&(_, cached_token)) = state.global_addr_cache.get(&sym) {
                        new_args.push(cached_token);
                    } else {
                        panic!(
                            "Global symbol {:?} token not found in cache during ReturnCall expansion!",
                            sym
                        );
                    }
                } else {
                    // 其他类型的全局符号保持不变
                    new_args.push(resolved_arg);
                }
            } else {
                // 非全局符号，直接使用解析后的值
                new_args.push(resolved_arg);
            }
        }

        // 如果有参数被展开，更新ReturnCall指令
        if needs_update {
            let new_args_vec = cursor.func.dfg.value_vecs.insert(new_args);
            cursor.func.dfg.insts[inst] = InstructionData::ReturnCall {
                func,
                args: new_args_vec,
            };
        }
    }

    /// 应用所有值的别名映射
    fn apply_aliases(&mut self, state: &ExpanderState) {
        // 首先应用value_remap中的映射
        for (from, to) in &state.value_remap {
            self.func.dfg.change_to_alias(*from, *to);
        }

        // 然后恢复那些不在value_remap中的原始别名关系
        for (alias_value, original_value) in &state.original_aliases {
            // 如果这个别名值没有被value_remap处理
            if !state.value_remap.contains_key(alias_value) {
                // 解析原始值的重映射
                let remapped_original = Self::resolve_value(*original_value, state);
                // 重新创建别名关系
                self.func
                    .dfg
                    .change_to_alias(*alias_value, remapped_original);
            }
        }

        self.func.dfg.resolve_all_aliases();
    }

    /// 解析值的重映射
    fn resolve_value(value: Value, state: &ExpanderState) -> Value {
        let mut current = value;
        while let Some(&remapped) = state.value_remap.get(&current) {
            current = remapped;
        }
        current
    }
}
