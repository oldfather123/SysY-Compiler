//! Data Flow Graph
use super::{ctx::CoreContext, ir::*, ir_impl::InstructionMapper};
use crate::prelude::*;
use std::collections::HashMap;

#[derive(Clone, Debug)]
pub struct DataFlowGraph {
    pub insts: Insts,
    results: RefSecMap<Inst, ValueVec>, // 一条指令对应多个结果值
    pub blocks: Blocks,
    pub value_vecs: ValueVecPool,
    values: RefMap<Value, ValueData>,
    pub jump_tables: JumpTables,
    pub stack_slots: StackSlots, // 栈槽管理
}

impl Default for DataFlowGraph {
    fn default() -> Self {
        Self::new()
    }
}

impl DataFlowGraph {
    pub fn new() -> Self {
        DataFlowGraph {
            insts: Insts(RefMap::new()),
            results: RefSecMap::new(),
            blocks: Blocks(RefMap::new()),
            value_vecs: ValueVecPool::new(),
            values: RefMap::new(),
            jump_tables: RefMap::new(),
            stack_slots: StackSlots(RefMap::new()),
        }
    }

    pub fn clear(&mut self) {
        self.insts.0.clear();
        self.results.clear();
        self.blocks.0.clear();
        self.value_vecs.clear();
        self.values.clear();
        self.jump_tables.clear();
        self.stack_slots.0.clear();
    }

    pub fn block_call<'a>(
        &mut self,
        block: Block,
        args: impl IntoIterator<Item = &'a Value>,
    ) -> BlockCall {
        BlockCall::new(block, args.into_iter().copied(), &mut self.value_vecs)
    }

    // 注意：U32OptVec和函数签名相关的方法已移至SharedContext中，通过Function访问
}

//==============================================================================
// 别名解析
//==============================================================================

fn resolve_aliases(values: &RefMap<Value, ValueData>, value: Value) -> Value {
    if let Some(v) = maybe_resolve_aliases(values, value) {
        v
    } else {
        panic!("Value alias loop detected for {value}");
    }
}

fn maybe_resolve_aliases(values: &RefMap<Value, ValueData>, value: Value) -> Option<Value> {
    let mut v = value;
    let mut visited = 0;
    let max_iterations = values.len() + 1; // 防止无限循环

    while visited < max_iterations {
        match values.get(v) {
            Some(ValueData::Alias { original, .. }) => {
                if v == *original {
                    return None; // 检测到自引用循环
                }
                v = *original;
                visited += 1;
            }
            _ => return Some(v), // 找到非别名值，返回成功
        }
    }

    None // 超过最大迭代次数，存在间接循环
}

//==============================================================================
// 别名解析
//==============================================================================

/// 别名映射器 -- 相当于 InstData::map_values
struct InstAliasMapper<'a> {
    alias_map: &'a HashMap<Value, Value>,
}

impl InstructionMapper for InstAliasMapper<'_> {
    fn map_value(&mut self, value: Value) -> Value {
        self.alias_map.get(&value).copied().unwrap_or(value)
    }

    fn map_value_vec(&mut self, value_vec: ValueVec, pool: &mut ValueVecPool) -> ValueVec {
        let values: Vec<Value> = pool[value_vec].iter().map(|&v| self.map_value(v)).collect();
        pool.insert(values)
    }

    fn map_block_call(&mut self, mut block_call: BlockCall, pool: &mut ValueVecPool) -> BlockCall {
        block_call.args = self.map_value_vec(block_call.args, pool);
        block_call
    }

    fn map_func_ref(&mut self, func_ref: FuncRef) -> FuncRef {
        func_ref
    }

    fn map_jump_table(&mut self, table: JumpTable, _pool: &mut ValueVecPool) -> JumpTable {
        table
    }

    fn map_array_init(&mut self, init: ArrayInit, pool: &mut ValueVecPool) -> ArrayInit {
        if let ArrayInit::List { vals } = init {
            ArrayInit::List {
                vals: self.map_value_vec(vals, pool),
            }
        } else {
            init
        }
    }
}

//==============================================================================
// 值操作
//==============================================================================

impl DataFlowGraph {
    // 警告: 该intern方法会进行去重，意味着可能会导致多个索引指向同一个Value
    // 通常只能用于明确为常量的地方
    // TODO: 应该分析源码考虑哪些地方能够用intern_value进行优化!
    // 除非你非常明确地知道自己在做什么，否则请使用 make_value 方法
    // pub fn intern_value(&mut self, data: ValueData) -> Value {
    //     self.values.intern_owned(data)
    // }

    /// 此处非常激进，直接使用 intern 进行去重
    /// 理由: SSA 单赋值，直接改值本身的情况可能没有
    ///
    /// 更新：由于BlockParam的idx会被更新，使用intern会导致值重复
    /// 必须使用insert来确保每个值都有唯一的引用
    ///
    /// 更新：全部使用insert, 否则会导致后端寄存器分配重叠!
    pub fn make_value(&mut self, data: ValueData) -> Value {
        // 对于BlockParam、Alias和Union，必须使用insert避免引用重复
        // match &data {
        //     ValueData::BlockParam { .. } | ValueData::Alias { .. } | ValueData::Union { .. } => {
        //         self.values.insert(data)
        //     }
        //     // 其他类型可以安全地使用intern进行去重
        //     _ => self.values.intern_owned(data),
        // }
        self.values.insert(data)
    }

    pub fn value_type(&self, v: Value) -> Type {
        self.values[v].ty()
    }

    pub fn valuedata(&self, value: Value) -> &ValueData {
        &self.values[value]
    }

    pub fn valuedata_mut(&mut self, value: Value) -> &mut ValueData {
        &mut self.values[value]
    }

    pub fn all_values(&self) -> &RefMap<Value, ValueData> {
        &self.values
    }

    pub fn make_valuevec<'a>(&mut self, values: impl IntoIterator<Item = &'a Value>) -> ValueVec {
        self.value_vecs
            .insert(values.into_iter().copied().collect())
    }

    #[inline]
    pub fn valuevec(&self, vec: ValueVec) -> &Vec<Value> {
        &self.value_vecs[vec]
    }

    #[inline]
    pub fn valuevec_mut(&mut self, vec: ValueVec) -> &mut Vec<Value> {
        &mut self.value_vecs[vec]
    }

    /// 解析值的别名，返回最终的非别名值
    pub fn resolve_aliases(&self, value: Value) -> Value {
        resolve_aliases(&self.values, value)
    }

    /// 获取值的真正定义来源
    pub fn value_def(&self, value: Value) -> &ValueData {
        let value_data = self.valuedata(value);
        match value_data {
            &ValueData::Alias { original, .. } => {
                // 递归解析别名，避免返回Alias
                self.value_def(self.resolve_aliases(original))
            }
            _ => value_data,
        }
    }

    /// 替换所有值别名的使用并删除别名
    pub fn resolve_all_aliases(&mut self) {
        // 1. 重写每个别名链
        // 从 valA -> valB -> valC -> valD 重写为:
        // valA -> valD
        // valB -> valD
        // valC -> valD
        // 这样可以确保每个别名都指向最终的非别名值.但仍然是别名
        for mut src in self.values.keys().collect::<Vec<_>>() {
            let value_data = self.values[src];
            if let ValueData::Alias { mut original, .. } = value_data {
                // 解析到最终非别名值
                let resolved = resolve_aliases(&self.values, original);
                let resolved_ty = self.value_type(resolved);

                // 沿着别名链，逐个改写 -- O(n)复杂度
                loop {
                    self.values[src] = ValueData::Alias {
                        ty: resolved_ty,
                        original: resolved,
                    };
                    src = original;
                    if let ValueData::Alias { original: next, .. } = self.values[src] {
                        original = next;
                    } else {
                        break;
                    }
                }
            }
        }

        // 2. 收集别名映射用于指令重写
        // alias_map: (valA, valD), (valB, valD), (valC, valD) ...
        let mut alias_map = HashMap::new();
        for (value, value_data) in self.values.iter() {
            if let ValueData::Alias { original, .. } = value_data {
                alias_map.insert(value, *original);
            }
        }

        // 3. 在指令中替换所有别名的使用 -- 已经包括了块参数的处理了
        let inst_keys: Vec<Inst> = self.insts.keys().collect();
        for inst in inst_keys {
            let old_data = self.insts[inst];
            let new_data = old_data.map(
                &mut InstAliasMapper {
                    alias_map: &alias_map,
                },
                &mut self.value_vecs,
            );
            self.insts[inst] = new_data;
        }
    }

    /// 将一个值dest设置为另一个值src的别名
    /// 执行后，任何使用 dest 的地方都会被视为使用 src
    pub fn change_to_alias(&mut self, dest: Value, src: Value) {
        let original = self.resolve_aliases(src);
        // 检查是否会创建循环
        debug_assert_ne!(
            dest, original,
            "Aliasing {dest} to {src} would create a loop"
        );
        let ty = self.value_type(original);
        self.values[dest] = ValueData::Alias { original, ty };
    }

    /// 用于 e-graph 优化, 创建一个 Union 节点，表示两个等价的值
    pub fn union(&mut self, x: Value, y: Value) -> Value {
        // 获取类型并确保类型一致
        let ty = self.value_type(x);
        debug_assert_eq!(
            ty,
            self.value_type(y),
            "Union values must have the same type: {x} has type {ty:?}, {y} has type {:?}",
            self.value_type(y)
        );
        self.make_value(ValueData::Union { ty, x, y })
    }

    /// 将一个指令的结果替换为另一个指令结果的别名
    /// 必须要返回数量完全一致
    pub fn replace_with_aliases(&mut self, dest_inst: Inst, original_inst: Inst) {
        debug_assert_ne!(
            dest_inst, original_inst,
            "Replacing {dest_inst} with itself would create a loop"
        );

        let dest_results = self.inst_results(dest_inst).to_vec();
        let original_results = self.inst_results(original_inst).to_vec();

        debug_assert_eq!(
            dest_results.len(),
            original_results.len(),
            "Replacing {dest_inst} with {original_inst} would produce a different number of results."
        );

        // 将每个dest结果设置为对应original结果的别名
        for (&dest, &original) in dest_results.iter().zip(original_results.iter()) {
            self.change_to_alias(dest, original);
        }

        // 清除dest指令的结果
        self.detach_inst_results(dest_inst);
    }

    /// 如果v是别名，返回其目标值
    pub fn value_alias_dest(&self, v: Value) -> Option<Value> {
        if let ValueData::Alias { original, .. } = self.values[v] {
            Some(original)
        } else {
            None
        }
    }

    ///DEBUG: 验证别名映射的完整性
    pub fn debug_validate_aliases(&self) -> Vec<String> {
        let mut issues = Vec::new();

        for (value, value_data) in self.values.iter() {
            if let ValueData::Alias { original, .. } = value_data {
                // 检查是否存在别名循环
                if maybe_resolve_aliases(&self.values, value).is_none() {
                    issues.push(format!("Alias cycle detected for value {:?}", value));
                }

                // 检查目标值是否存在
                if !self.values.contains_key(*original) {
                    issues.push(format!(
                        "Alias {:?} points to non-existent value {:?}",
                        value, original
                    ));
                }
            }
        }

        issues
    }
}

//==============================================================================
// 块参数操作
//==============================================================================

impl DataFlowGraph {
    pub fn make_block(&mut self) -> Block {
        self.blocks.add(&mut self.value_vecs)
    }

    pub fn block_params(&self, block: Block) -> &Vec<Value> {
        let params_ref = self.blocks[block].params;
        self.valuevec(params_ref)
    }

    pub fn block_params_mut(&mut self, block: Block) -> &mut Vec<Value> {
        let params_ref = self.blocks[block].params;
        self.valuevec_mut(params_ref)
    }

    pub fn append_block_param(&mut self, block: Block, ty: Type) -> Value {
        let idx = self.block_params(block).len() as u32;
        let value = self.make_value(ValueData::BlockParam { ty, block, idx });
        self.block_params_mut(block).push(value);
        value
    }

    pub fn remove_block_param(&mut self, val: Value) {
        let (block, num) = if let ValueData::BlockParam { block, idx, .. } = self.values[val] {
            (block, idx as usize)
        } else {
            panic!("Value {val} is not a block parameter");
        };

        // Collect the values that need updating
        let params_to_update: Vec<Value> = {
            let params = self.block_params_mut(block);
            params.remove(num);
            params[num..].to_vec()
        };

        // Update the indices after releasing the borrow
        for (offset, param) in params_to_update.into_iter().enumerate() {
            match self.valuedata_mut(param) {
                ValueData::BlockParam { idx, .. } => *idx = (num + offset) as u32,
                _ => panic!("Expected BlockParam data for value"),
            }
        }
    }
}

//==============================================================================
// 指令操作
//==============================================================================

impl DataFlowGraph {
    pub fn make_inst(&mut self, data: InstructionData) -> Inst {
        // 禁止使用intern, 必须使用 insert !
        self.insts.0.insert(data)
    }

    /// 替换现有指令的数据
    pub fn replace_inst(&mut self, inst: Inst, new_data: InstructionData) {
        self.insts
            .0
            .get_mut(inst)
            .map(|inst_data| *inst_data = new_data)
            .expect("指令必须存在");
    }

    /// 创建一个替换现有指令的构建器
    // pub fn replace(&mut self, inst: Inst) -> DfgReplaceBuilder<'_> {
    //     DfgReplaceBuilder::new(self, inst)
    // }

    /// 指令参数 -- 只包括指令参数，不包括块参数
    pub fn inst_args(&self, inst: Inst) -> Vec<Value> {
        self.insts[inst].arguments(&self.value_vecs)
    }

    /// 指令所用的所有值 -- 包括指令参数和块参数
    pub fn inst_values(&self, inst: Inst) -> Vec<Value> {
        self.inst_args(inst)
            .iter()
            .copied()
            .chain(
                self.insts[inst]
                    .branch_destination(&self.jump_tables)
                    .iter()
                    .flat_map(|branch| branch.args(&self.value_vecs).iter().copied()),
            )
            .collect()
    }

    /// 获取指令的第一个结果值
    pub fn first_result(&self, inst: Inst) -> Value {
        self.inst_result(inst, 0)
    }

    /// 安全获取指令的结果向量引用
    fn results(&self, inst: Inst) -> &Vec<Value> {
        static EMPTY_VEC: Vec<Value> = Vec::new();
        if let Some(value_vec) = self.results.get(inst) {
            self.valuevec(*value_vec)
        } else {
            &EMPTY_VEC
        }
    }

    pub fn inst_destination(&self, inst: Inst) -> &[BlockCall] {
        self.insts[inst].branch_destination(&self.jump_tables)
    }

    /// 获取指令的特定索引的结果值
    pub fn inst_result(&self, inst: Inst, idx: u32) -> Value {
        *self
            .results(inst)
            .get(idx as usize)
            .unwrap_or_else(|| panic!("No result found for instruction {inst} at index {idx}."))
    }

    pub fn has_results(&self, inst: Inst) -> bool {
        !self.results(inst).is_empty()
    }

    /// 获取指令的所有结果值
    pub fn inst_results(&self, inst: Inst) -> &Vec<Value> {
        self.results(inst)
    }

    /// 获取指令结果的类型 --注意是直接获取: 必须要先infer才有结果
    pub fn inst_result_type(&self, inst: Inst, idx: u32) -> Type {
        let value = self.inst_result(inst, idx);
        self.value_type(value)
    }

    pub fn inst_result_types(&self, inst: Inst) -> Vec<Type> {
        self.results(inst)
            .iter()
            .map(|v| self.value_type(*v))
            .collect()
    }

    /// 获取或创建指令的结果向量引用
    fn results_mut(&mut self, inst: Inst) -> &mut Vec<Value> {
        if !self.results.contains_key(inst) {
            let value_vec = self.value_vecs.insert_empty();
            self.results.insert(inst, value_vec);
        }
        let value_vec = self.results[inst];
        self.valuevec_mut(value_vec)
    }

    /// 直接设置指令结果类型 & 并设置results数据
    pub fn make_inst_results(&mut self, inst: Inst, types: Vec<Type>) -> usize {
        self.detach_inst_results(inst);
        for (idx, ty) in types.iter().enumerate() {
            let value_data = ValueData::Inst {
                ty: *ty,
                inst,
                idx: idx as u32,
            };
            let value = self.make_value(value_data);
            self.results_mut(inst).push(value);
        }
        types.len()
    }

    /// 设置指令的结果值列表
    pub fn set_inst_results(&mut self, inst: Inst, results: Vec<Value>) {
        let value_vec = self.value_vecs.insert(results);
        self.results.insert(inst, value_vec);
    }

    /// 清空指令结果
    pub fn detach_inst_results(&mut self, inst: Inst) {
        self.results_mut(inst).clear();
    }
}

//==============================================================================
// 类型推断
//==============================================================================

pub fn infer_inst_result_types(
    dfg: &DataFlowGraph,
    ctx: &mut CoreContext,
    inst: Inst,
) -> Vec<Type> {
    if let Some(inst_data) = dfg.insts.get(inst) {
        match inst_data {
            InstructionData::Binary { op, lhs, .. } => {
                use super::ir::BinOp;
                match op {
                    // 比较运算总是返回Bool类型
                    BinOp::Eq | BinOp::Ne | BinOp::Lt | BinOp::Le | BinOp::Gt | BinOp::Ge => {
                        vec![Type::Bool]
                    }
                    // 算术运算结果类型与操作数相同
                    _ => vec![dfg.value_type(*lhs)],
                }
            }
            InstructionData::Unary { op, val } => {
                use super::ir::UnaryOp;
                match op {
                    // 逻辑非总是返回Bool类型
                    UnaryOp::Not => vec![Type::Bool],
                    // 其他一元运算（INeg, BNot, FNeg）结果类型与操作数相同
                    UnaryOp::INeg | UnaryOp::BNot | UnaryOp::FNeg => vec![dfg.value_type(*val)],
                }
            }
            InstructionData::Call { func, .. } => {
                // 函数调用返回函数签名中的返回类型
                if let Some(ext_func) = ctx.ext_funcs.get(*func) {
                    // 外部函数调用
                    ext_func.signature.returns.clone()
                } else {
                    // 内部函数调用或未知函数
                    // 在单函数转换模式下，只能处理当前函数和外部函数
                    // 对于其他情况，返回空向量（可能需要在调用方进行修正）
                    vec![]
                }
            }
            InstructionData::ReturnCall { func, .. } => {
                // 尾调用返回被调用函数的返回类型
                if let Some(ext_func) = ctx.ext_funcs.get(*func) {
                    ext_func.signature.returns.clone()
                } else {
                    // 调试信息：记录哪个函数没有找到
                    let func_name = &ctx.names[*func];
                    eprintln!(
                        "Warning: ReturnCall function '{}' not found in ext_funcs during type inference",
                        func_name
                    );
                    vec![]
                }
            }
            InstructionData::Cast { to, .. } => {
                // 类型转换返回目标类型
                vec![*to]
            }
            InstructionData::ArrayAlloc { elem, dims, .. } => {
                // 数组分配返回指针和令牌
                vec![
                    Type::ArrayPtr {
                        elem: ArrayElemType::from_type(*elem).unwrap(),
                        dims: *dims,
                    },
                    Type::MemToken,
                ]
            }
            InstructionData::ArrayGet { ptr, .. } => {
                // 数组读取返回元素类型
                if let Some(elem_type) = dfg.value_type(*ptr).elem_type() {
                    vec![elem_type]
                } else {
                    vec![dfg.value_type(*ptr)] // 非数组类型，直接返回
                }
            }
            InstructionData::ArrayPut { .. } => {
                vec![Type::MemToken] // 数组写入返回新的令牌
            }
            InstructionData::ArraySlice { ptr, sdims } => {
                if let Type::ArrayPtr { elem, dims } = dfg.value_type(*ptr) {
                    // 数组切片返回新的数组指针
                    // 计算切片后的维度：如果原始是 N 维，sdims 长度为 M，则结果是 (N-M) 维
                    let sdims_vec = dfg.valuevec(*sdims);
                    let sdims_count = sdims_vec.len();

                    // 获取原始维度数据
                    if let Some(original_dims) = ctx.type_dims.get(dims) {
                        // 跳过前 sdims_count 个维度
                        let new_dims_vec: Vec<Option<u32>> =
                            original_dims.iter().skip(sdims_count).cloned().collect();

                        // 创建新的维度
                        let new_dims = ctx.make_type_dims(new_dims_vec);

                        vec![Type::ArrayPtr {
                            elem,
                            dims: new_dims,
                        }]
                    } else {
                        // 无法获取原始维度，返回原始类型
                        vec![Type::ArrayPtr { elem, dims }]
                    }
                } else {
                    panic!("Expected ArrayPtr type for array slice");
                }
            }
            InstructionData::GlobalScalarGet { global, .. } => {
                // 全局标量读取返回其类型
                if let Some(elem_type) = dfg.value_type(*global).elem_type() {
                    vec![elem_type]
                } else {
                    vec![dfg.value_type(*global)]
                }
            }
            InstructionData::GlobalScalarSet { .. } => {
                vec![Type::MemToken] // 全局标量写入返回新的令牌
            }
            // 低级内存操作指令的类型推导
            InstructionData::StackAddr { .. } => {
                vec![Type::Int64, Type::MemToken] // 栈地址返回地址和令牌
            }
            InstructionData::GlobalAddr { .. } => {
                vec![Type::Int64, Type::MemToken] // 全局地址返回地址和令牌
            }
            InstructionData::Load { ty, .. } => {
                vec![*ty] // Load返回指定类型的值
            }
            InstructionData::Store { .. } => {
                vec![Type::MemToken] // Store返回新的内存令牌
            }
            _ => {
                // 其他指令不返回值
                vec![]
            }
        }
    } else {
        vec![]
    }
}
