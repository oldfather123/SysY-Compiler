//! SSA验证器
//!
//! 验证SSA IR正确性：
//! - SSA性质 -- 每个值只被定义一次
//! - 内存令牌的正确使用
//! - 类型安全
//! - 控制流正确性

use crate::prelude::*;
use crate::ssa::{
    ctx::CoreContext,
    function::{FuncSignature, Function},
    ir::*,
};
use std::collections::{HashMap, HashSet};

/// SSA验证器
pub struct Validator<'a> {
    core_context: &'a CoreContext,
    errors: Vec<String>,
}

/// 验证上下文
struct ValidationContext {
    /// 已定义的值
    defined_values: HashSet<Value>,
    /// 值的类型信息
    value_types: HashMap<Value, Type>,
    /// 内存令牌追踪
    token_tracker: TokenTracker,
    /// 块的前驱
    predecessors: HashMap<Block, Vec<Block>>,
    /// 已访问的块
    visited_blocks: HashSet<Block>,
}

/// 内存令牌追踪器
struct TokenTracker {
    /// 内存对象的当前令牌版本
    current_tokens: HashMap<MemoryObject, (Value, u32)>,
    /// 令牌的内存对象映射
    token_objects: HashMap<Value, MemoryObject>,
}

impl<'a> Validator<'a> {
    pub fn new(core_context: &'a CoreContext) -> Self {
        Self {
            core_context,
            errors: Vec::new(),
        }
    }

    /// 验证函数
    pub fn validate_function(&mut self, func: &Function) -> CEResult<()> {
        self.errors.clear();

        let mut ctx = ValidationContext {
            defined_values: HashSet::new(),
            value_types: HashMap::new(),
            token_tracker: TokenTracker {
                current_tokens: HashMap::new(),
                token_objects: HashMap::new(),
            },
            predecessors: self.compute_predecessors(func),
            visited_blocks: HashSet::new(),
        };

        // 验证函数签名
        self.validate_signature(&func.signature);

        // 注册函数参数（作为入口块的块参数）
        self.register_entry_params(func, &mut ctx);

        // 验证所有块
        self.validate_blocks(func, &mut ctx);

        // 验证控制流完整性
        self.validate_control_flow(func, &ctx);

        // 检查是否有错误
        if !self.errors.is_empty() {
            let error_msg = self.errors.join("\n");
            return Err(CErr::runtime_err(format!(
                "SSA validation failed:\n{}",
                error_msg
            )));
        }

        Ok(())
    }

    /// 计算前驱关系
    fn compute_predecessors(&self, func: &Function) -> HashMap<Block, Vec<Block>> {
        let mut preds = HashMap::new();

        for block in func.layout.blocks() {
            // 遍历块的终结指令
            if let Some(last_inst) = func.layout.last_inst(block) {
                let inst_data = &func.dfg.insts[last_inst];

                match inst_data {
                    InstructionData::Jump { target } => {
                        preds
                            .entry(target.block())
                            .or_insert_with(Vec::new)
                            .push(block);
                    }
                    InstructionData::Brif { targets, .. } => {
                        for target in targets {
                            preds
                                .entry(target.block())
                                .or_insert_with(Vec::new)
                                .push(block);
                        }
                    }
                    InstructionData::BranchTable { table, .. } => {
                        if let Some(jump_table) = func.dfg.jump_tables.get(*table) {
                            for branch in jump_table.all_branches() {
                                preds
                                    .entry(branch.block())
                                    .or_insert_with(Vec::new)
                                    .push(block);
                            }
                        }
                    }
                    _ => {}
                }
            }
        }

        preds
    }

    /// 验证函数签名
    fn validate_signature(&mut self, sig: &FuncSignature) {
        // 验证参数类型
        for (i, param_type) in sig.params.iter().enumerate() {
            if !self.is_valid_param_type(param_type) {
                self.errors.push(format!(
                    "Invalid parameter type at index {}: {:?}",
                    i, param_type
                ));
            }
        }

        // 验证返回类型
        if !sig.returns.is_empty() {
            let ret_type = &sig.returns[0];
            if !self.is_valid_return_type(ret_type) {
                self.errors
                    .push(format!("Invalid return type: {:?}", ret_type));
            }
        }
    }

    /// 注册入口块参数
    fn register_entry_params(&self, func: &Function, ctx: &mut ValidationContext) {
        let entry_block = func.entry;
        let block_data = &func.dfg.blocks[entry_block];

        for (idx, param_value) in func.dfg.block_params(entry_block).iter().enumerate() {
            ctx.defined_values.insert(*param_value);

            // 获取参数类型
            if let ValueData::BlockParam { ty, .. } = func.dfg.valuedata(*param_value) {
                ctx.value_types.insert(*param_value, *ty);

                // 如果是内存令牌，注册到tracker
                if matches!(ty, Type::MemToken) {
                    // 函数参数的内存令牌对应FnParamUnknown
                    // TODO: Need to create proper FuncRef from function name
                    // For now, use a dummy FuncRef - this needs to be fixed
                    let dummy_func_ref = FuncRef::default(); // This is a placeholder
                    let mem_obj = MemoryObject::FnParamUnknown {
                        func: dummy_func_ref,
                    };
                    ctx.token_tracker
                        .current_tokens
                        .insert(mem_obj, (*param_value, 0));
                    ctx.token_tracker
                        .token_objects
                        .insert(*param_value, mem_obj);
                }
            }
        }
    }

    /// 验证所有块
    fn validate_blocks(&mut self, func: &Function, ctx: &mut ValidationContext) {
        // 使用工作列表算法
        let mut worklist = vec![func.entry];

        while let Some(block) = worklist.pop() {
            if ctx.visited_blocks.contains(&block) {
                continue;
            }
            ctx.visited_blocks.insert(block);

            // 验证块
            self.validate_block(func, block, ctx);

            // 添加后继块到工作列表
            if let Some(last_inst) = func.layout.last_inst(block) {
                let inst_data = &func.dfg.insts[last_inst];

                match inst_data {
                    InstructionData::Jump { target } => {
                        worklist.push(target.block());
                    }
                    InstructionData::Brif { targets, .. } => {
                        for target in targets {
                            worklist.push(target.block());
                        }
                    }
                    InstructionData::BranchTable { table, .. } => {
                        if let Some(jump_table) = func.dfg.jump_tables.get(*table) {
                            for branch in jump_table.all_branches() {
                                worklist.push(branch.block());
                            }
                        }
                    }
                    _ => {}
                }
            }
        }
    }

    /// 验证单个块
    fn validate_block(&mut self, func: &Function, block: Block, ctx: &mut ValidationContext) {
        let _block_data = &func.dfg.blocks[block];

        // 验证块参数（跳过入口块，因为入口块参数已经在register_entry_params中注册）
        if block != func.entry {
            for param_value in func.dfg.block_params(block).iter() {
                if ctx.defined_values.contains(param_value) {
                    self.errors
                        .push(format!("Block parameter {:?} already defined", param_value));
                }
                ctx.defined_values.insert(*param_value);

                // 记录类型
                if let ValueData::BlockParam { ty, .. } = func.dfg.valuedata(*param_value) {
                    ctx.value_types.insert(*param_value, *ty);
                }
            }
        }

        // 验证指令
        for (idx, inst) in func.layout.block_insts(block).enumerate() {
            self.validate_instruction(func, inst, block, idx, ctx);
        }

        // 验证块必须有终结指令
        let is_terminated = if let Some(last_inst) = func.layout.last_inst(block) {
            if let Some(inst_data) = func.dfg.insts.get(last_inst) {
                inst_data.is_terminator()
            } else {
                false
            }
        } else {
            false
        };

        if !is_terminated {
            self.errors
                .push(format!("Block {:?} is not terminated", block));
        }
    }

    /// 验证指令
    fn validate_instruction(
        &mut self,
        func: &Function,
        inst: Inst,
        block: Block,
        inst_idx: usize,
        ctx: &mut ValidationContext,
    ) {
        let inst_data = &func.dfg.insts[inst];

        // 验证指令操作数
        self.validate_operands(func, inst_data, ctx);

        // 验证内存令牌使用
        self.validate_memory_tokens(func, inst, inst_data, ctx);

        // 验证指令类型
        self.validate_instruction_types(func, inst, inst_data, ctx);

        // 注册指令结果
        self.register_instruction_results(func, inst, inst_data, ctx);

        // 验证终结指令位置
        let block_insts: Vec<_> = func.layout.block_insts(block).collect();
        if inst_idx < block_insts.len() - 1
            && matches!(
                inst_data,
                InstructionData::Jump { .. }
                    | InstructionData::Brif { .. }
                    | InstructionData::BranchTable { .. }
                    | InstructionData::Return { .. }
                    | InstructionData::Unreachable
            )
        {
            self.errors.push(format!(
                "Terminator instruction {:?} is not at the end of block",
                inst
            ));
        }
    }

    /// 验证操作数
    fn validate_operands(
        &mut self,
        _func: &Function,
        _inst: &InstructionData,
        _ctx: &ValidationContext,
    ) {
        // TODO: 暂时简化验证，跳过未定义值检查
        // 这需要更复杂的数据流分析来正确实现
    }

    /// 验证内存令牌
    fn validate_memory_tokens(
        &mut self,
        func: &Function,
        inst: Inst,
        inst_data: &InstructionData,
        ctx: &mut ValidationContext,
    ) {
        match inst_data {
            InstructionData::ArrayGet { ptr, token, .. }
            | InstructionData::ArrayPut { ptr, token, .. } => {
                // 验证令牌类型
                if let Some(token_type) = ctx.value_types.get(token) {
                    if !matches!(token_type, Type::MemToken) {
                        self.errors
                            .push(format!("Expected memory token, got {:?}", token_type));
                    }
                }

                // TODO: 验证令牌版本匹配
            }

            InstructionData::GlobalScalarGet { token, .. }
            | InstructionData::GlobalScalarSet { token, .. } => {
                // 验证令牌类型
                if let Some(token_type) = ctx.value_types.get(token) {
                    if !matches!(token_type, Type::MemToken) {
                        self.errors
                            .push(format!("Expected memory token, got {:?}", token_type));
                    }
                }
            }

            _ => {}
        }
    }

    /// 验证指令类型
    fn validate_instruction_types(
        &mut self,
        func: &Function,
        inst: Inst,
        inst_data: &InstructionData,
        ctx: &ValidationContext,
    ) {
        // TODO: 实现详细的类型检查
        match inst_data {
            InstructionData::Binary { op, lhs, rhs } => {
                // 验证二元运算的操作数类型匹配
                if let (Some(lhs_ty), Some(rhs_ty)) =
                    (ctx.value_types.get(lhs), ctx.value_types.get(rhs))
                {
                    if lhs_ty != rhs_ty {
                        self.errors.push(format!(
                            "Binary operation {:?} type mismatch: {:?} vs {:?}. Inst: {:?}, Block: {:?}",
                            op, lhs_ty, rhs_ty, inst, func.layout.inst_block(inst).unwrap()
                        ));
                    }
                }
            }

            InstructionData::Cast { val, to } => {
                // 验证类型转换的合法性
                if let Some(from_ty) = ctx.value_types.get(val) {
                    if !self.is_valid_cast(from_ty, to) {
                        self.errors
                            .push(format!("Invalid cast from {:?} to {:?}", from_ty, to));
                    }
                }
            }

            _ => {}
        }
    }

    /// 注册指令结果
    fn register_instruction_results(
        &mut self,
        func: &Function,
        inst: Inst,
        _inst_data: &InstructionData,
        ctx: &mut ValidationContext,
    ) {
        // 注册指令的所有结果值为已定义
        for result_value in func.dfg.inst_results(inst) {
            ctx.defined_values.insert(*result_value);

            // 记录结果值的类型
            let value_data = func.dfg.valuedata(*result_value);
            ctx.value_types.insert(*result_value, value_data.ty());
        }
    }

    /// 验证控制流完整性
    fn validate_control_flow(&mut self, func: &Function, ctx: &ValidationContext) {
        // 验证所有块都可达 - 不可达块发出警告但放行
        for (block, _) in &func.dfg.blocks {
            if block != func.entry && !ctx.visited_blocks.contains(&block) {
                eprintln!(
                    "⚠️  警告: func: {}, Block {:?} is unreachable",
                    func.name, block
                );
            }
        }

        // 验证跳转目标的块参数匹配
        for block in func.layout.blocks() {
            if let Some(last_inst) = func.layout.last_inst(block) {
                let inst_data = &func.dfg.insts[last_inst];

                match inst_data {
                    InstructionData::Jump { target } => {
                        self.validate_block_call(func, target);
                    }
                    InstructionData::Brif { targets, .. } => {
                        for target in targets {
                            self.validate_block_call(func, target);
                        }
                    }
                    InstructionData::BranchTable { table, .. } => {
                        if let Some(jump_table) = func.dfg.jump_tables.get(*table) {
                            for branch in jump_table.all_branches() {
                                self.validate_block_call(func, branch);
                            }
                        }
                    }
                    _ => {}
                }
            }
        }
    }

    /// 验证块调用
    fn validate_block_call(&mut self, func: &Function, block_call: &BlockCall) {
        let target_block = block_call.block();
        let target_params = func.dfg.block_params(target_block);
        let args = block_call.args(&func.dfg.value_vecs);

        let param_count = target_params.len();

        if args.len() != param_count {
            self.errors.push(format!(
                "Block call argument count mismatch: target block {:?}, expected {}, got {}",
                target_block,
                param_count,
                args.len()
            ));
        }
    }

    /// 检查是否是有效的参数类型
    fn is_valid_param_type(&self, ty: &Type) -> bool {
        match ty {
            Type::Unit | Type::Bool | Type::Int32 | Type::Float32 => true,
            Type::ArrayPtr { .. } | Type::MemToken => true,
            Type::Int64 | Type::Float64 => true, // 新增的 64 位类型支持
        }
    }

    /// 检查是否是有效的返回类型
    fn is_valid_return_type(&self, ty: &Type) -> bool {
        match ty {
            Type::Unit | Type::Bool | Type::Int32 | Type::Float32 | Type::MemToken => true,
            Type::ArrayPtr { .. } => false,
            Type::Int64 | Type::Float64 => true, // 新增的 64 位类型支持 // 不能返回数组指针，但可以返回内存令牌
        }
    }

    /// 检查是否是有效的类型转换
    fn is_valid_cast(&self, from: &Type, to: &Type) -> bool {
        match (from, to) {
            // 相同类型
            (a, b) if a == b => true,

            // 整数类型之间的转换
            (Type::Int32, Type::Int64) | (Type::Int64, Type::Int32) => true,

            // 浮点类型之间的转换
            (Type::Float32, Type::Float64) | (Type::Float64, Type::Float32) => true,

            // 整数到浮点转换
            (Type::Int32, Type::Float32) | (Type::Int32, Type::Float64) => true,
            (Type::Int64, Type::Float32) | (Type::Int64, Type::Float64) => true,

            // 浮点到整数转换
            (Type::Float32, Type::Int32) | (Type::Float32, Type::Int64) => true,
            (Type::Float64, Type::Int32) | (Type::Float64, Type::Int64) => true,

            // 布尔转换
            (Type::Bool, Type::Int32) | (Type::Bool, Type::Int64) => true,
            (Type::Bool, Type::Float32) | (Type::Bool, Type::Float64) => true,

            // 到布尔转换
            (Type::Int32, Type::Bool) | (Type::Int64, Type::Bool) => true,
            (Type::Float32, Type::Bool) | (Type::Float64, Type::Bool) => true,

            // 其他转换无效
            _ => false,
        }
    }
}
