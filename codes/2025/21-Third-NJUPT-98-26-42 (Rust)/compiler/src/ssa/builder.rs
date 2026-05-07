//! SSA 构建器模块
//!
//! - BuilderContext 带有所有函数签名/符号/全局符号信息
//! - FunctionBuilder 用于构建单个函数
//! - FuncInstBuilder 块级别的指令构建器

use super::ctx::*;
use super::dfg::*;
use super::function::*;
use super::inst_builder::*;
use super::ir::*;
use crate::prelude::*;
use core::panic;

//==============================================================================
// FunctionBuilder - 函数级构建器
//==============================================================================

/// 函数构建器 - 负责构建单个函数
pub struct FunctionBuilder<'a> {
    /// 正在构建的函数
    pub func: &'a mut Function,
    /// 构建上下文
    pub ctx: &'a mut CoreContext,
}

impl<'a> FunctionBuilder<'a> {
    /// 创建新的函数构建器 -- 自动从ctx中获取收集的函数签名
    pub fn new(func: &'a mut Function, ctx: &'a mut CoreContext) -> Self {
        ctx.clear(); // 清理之前的构建状态
        // func.signature = ctx.ext_funcs.
        let func_ref = ctx.name_ref(&func.name);
        let sig = ctx
            .ext_funcs
            .get(func_ref)
            .map(|f| f.signature.clone())
            .unwrap_or_else(|| panic!("Function {} not found in ext_funcs", func.name));
        func.signature = sig;
        Self { func, ctx }
    }

    /// 获取当前块
    pub fn current_block(&self) -> Option<Block> {
        self.ctx.current_block
    }

    /// 切换到指定块
    pub fn switch_to_block(&mut self, block: Block) {
        self.ctx.current_block = Some(block);
    }

    /// 创建新块
    pub fn create_block(&mut self) -> Block {
        let block = self.func.dfg.make_block();
        self.ctx.ssa_builder.declare_block(block);
        // 不需要初始化BlockNode，布局会在需要时自动管理
        block
    }

    /// 密封块
    pub fn seal_block(&mut self, block: Block) {
        self.ctx.ssa_builder.seal_block(block, self.func);
    }

    /// 密封所有块
    pub fn seal_all_blocks(&mut self) {
        let blocks: Vec<Block> = self.func.dfg.blocks.keys().collect();
        for block in blocks {
            self.seal_block(block);
        }
    }

    /// 设置入口块参数
    pub fn make_block_params_from_func_params(&mut self, block: Block) {
        // 收集 (param_name, value) 对
        let param_values: Vec<(Type, NameRef, Value)> = self
            .func
            .signature
            .params
            .iter()
            .zip(&self.func.signature.param_names)
            .map(|(param_ty, param_name)| {
                let param_name = param_name.unwrap();
                let value = self.func.dfg.append_block_param(block, *param_ty);
                (*param_ty, param_name, value)
            })
            .collect();
        // 定义变量
        for (param_ty, param_name, value) in param_values {
            self.declare_var(param_name, param_ty);
            self.def_var(param_name, value, block);
        }
    }

    /// 定义变量
    pub fn def_var(&mut self, var: NameRef, val: Value, block: Block) {
        // TODO: 肯定有什么地方在def_var之前忘记声明了
        // if !self.check_is_var_declared(var) {
        //     panic!("Variable {} not declared", self.ctx.ref_name(var));
        // }
        self.ctx.ssa_builder.def_var(var, val, block);
    }

    /// 使用变量
    pub fn use_var(&mut self, var: NameRef, ty: Type) -> Value {
        let block = self.ctx.current_block.expect("No current block");
        self.ctx.ssa_builder.use_var(self.func, block, var, ty)
    }

    /// 获取指令构建器
    pub fn ins(&mut self) -> FuncInstBuilder<'_> {
        let block = self.ctx.current_block.expect("No current block");
        // eprintln!("DEBUG: builder.ins() - current_block = {:?}", block);
        FuncInstBuilder::new(self.func, self.ctx, block)
    }

    /// 检查指定块是否已经有终结指令
    pub fn is_block_terminated(&self, block: Block) -> bool {
        // 检查块的最后一条指令是否为终结指令
        if let Some(last_inst) = self.func.layout.last_inst(block) {
            if let Some(inst_data) = self.func.dfg.insts.get(last_inst) {
                Self::is_terminator(inst_data)
            } else {
                false
            }
        } else {
            false
        }
    }

    /// 检查当前块是否已经有终结指令
    pub fn is_current_block_terminated(&self) -> bool {
        if let Some(block) = self.ctx.current_block {
            self.is_block_terminated(block)
        } else {
            false
        }
    }

    /// 进入循环上下文
    pub fn push_loop(&mut self, continue_block: Block, break_block: Block) {
        self.ctx.loop_stack.push((continue_block, break_block));
    }

    /// 退出循环上下文
    pub fn pop_loop(&mut self) {
        self.ctx.loop_stack.pop();
    }

    /// 获取当前循环的 continue 目标
    pub fn loop_continue_target(&self) -> Option<Block> {
        self.ctx.loop_stack.last().map(|(cont, _)| *cont)
    }

    /// 获取当前循环的 break 目标
    pub fn loop_break_target(&self) -> Option<Block> {
        self.ctx.loop_stack.last().map(|(_, brk)| *brk)
    }

    /// 完成函数构建
    pub fn finalize(self) {
        // 清理状态，准备下一次使用
        self.ctx.clear();
    }

    // === 辅助方法 ===

    /// 获取名称引用
    pub fn name_ref(&mut self, name: &str) -> NameRef {
        self.ctx.name_ref(name)
    }

    /// 设置名称类型
    pub fn declare_var(&mut self, name_ref: NameRef, ty: Type) {
        self.ctx.name_types.insert(name_ref, ty);
    }

    /// 检查变量是否已声明 -- 否则不声明就没有类型
    pub fn check_is_var_declared(&self, name_ref: NameRef) -> bool {
        self.ctx.name_types.contains_key(name_ref)
    }

    /// 获取名称类型
    pub fn name_type(&self, name_ref: NameRef) -> Option<Type> {
        self.ctx.name_types.get(name_ref).copied()
    }

    /// 创建类型维度
    pub fn make_u32optvec(&mut self, vec: U32OptVecData) -> U32OptVec {
        self.ctx.make_type_dims(vec)
    }

    /// 获取函数引用的签名
    pub fn get_signature(&self, func_ref: FuncRef) -> Option<&FuncSignature> {
        self.ctx.ext_funcs.get(func_ref).map(|f| &f.signature)
    }

    /// 获取或创建全局变量的符号值 -- 懒加载
    /// 这个方法封装了全局变量访问的逻辑，避免在多处重复if-else判断
    pub fn get_or_create_global_symbol(&mut self, name_ref: NameRef) -> Option<Value> {
        if let Some(global) = self.ctx.globals.get(name_ref) {
            let ty = global.ty;

            // 尝试使用已定义的符号
            let current_block = self.ctx.current_block.unwrap_or(self.func.entry);

            // 先检查是否已经在SSA中被定义过
            if let Some(_existing_val) = self.ctx.name_types.get(name_ref) {
                // 检查是否在entry block中已经有定义
                if let Some(existing_val) = self
                    .ctx
                    .ssa_builder
                    .var_defined_in_block(name_ref, self.func.entry)
                {
                    return Some(existing_val);
                }
            }

            // 创建新的GlobalSymbol值
            let global_sym = self
                .func
                .dfg
                .make_value(ValueData::GlobalSymbol { ty, sym: name_ref });

            // 在入口块定义这个符号
            self.ctx
                .ssa_builder
                .def_var(name_ref, global_sym, self.func.entry);

            // 如果是全局变量（标量或数组），还需要创建内存令牌
            let token_ref = self.ctx.name_ref_token_ref(name_ref);
            let token_val = self.func.dfg.make_value(ValueData::GlobalSymbol {
                ty: Type::MemToken,
                sym: name_ref,
            });
            self.declare_var(token_ref, Type::MemToken);
            self.ctx
                .ssa_builder
                .def_var(token_ref, token_val, self.func.entry);

            Some(global_sym)
        } else {
            None
        }
    }

    /// 获取全局标量的值（包含读取操作）
    pub fn get_global_scalar_value(&mut self, name_ref: NameRef) -> Result<Value, String> {
        if let Some(global_sym) = self.get_or_create_global_symbol(name_ref) {
            let token_ref = self.ctx.name_ref_token_ref(name_ref);
            let token_val = self.use_var(token_ref, Type::MemToken);
            Ok(self.ins().global_scalar_get(global_sym, token_val))
        } else {
            Err(format!(
                "Global scalar {} not found",
                self.ctx.ref_name(name_ref)
            ))
        }
    }

    /// 设置全局标量的值（包含写入操作）
    pub fn set_global_scalar_value(
        &mut self,
        name_ref: NameRef,
        value: Value,
    ) -> Result<(), String> {
        if let Some(global_sym) = self.get_or_create_global_symbol(name_ref) {
            let token_ref = self.ctx.name_ref_token_ref(name_ref);
            let token_val = self.use_var(token_ref, Type::MemToken);
            let new_token = self.ins().global_scalar_set(global_sym, value, token_val);

            let block = self.ctx.current_block.expect("No current block");
            self.def_var(token_ref, new_token, block);
            Ok(())
        } else {
            Err(format!(
                "Global scalar {} not found",
                self.ctx.ref_name(name_ref)
            ))
        }
    }

    /// 获取全局数组的指针值
    pub fn get_global_array_ptr(&mut self, name_ref: NameRef) -> Result<Value, String> {
        if let Some(global_sym) = self.get_or_create_global_symbol(name_ref) {
            Ok(global_sym)
        } else {
            Err(format!(
                "Global array {} not found",
                self.ctx.ref_name(name_ref)
            ))
        }
    }

    /// 计算并设置块布局
    pub fn compute_block_layout(&mut self) {
        let entry = self.func.entry;
        let reverse_postorder = Self::compute_reverse_postorder(entry, self.func);

        // 保存现有的指令信息
        let mut block_instructions: std::collections::HashMap<Block, Vec<Inst>> =
            std::collections::HashMap::new();

        // 收集每个块的指令
        for &block in &reverse_postorder {
            let mut insts = Vec::new();
            for inst in self.func.layout.block_insts(block) {
                insts.push(inst);
            }
            if !insts.is_empty() {
                block_instructions.insert(block, insts);
            }
        }

        // 清理并重新设置布局 -- 不会影响dfg中的数据
        self.func.layout.clear();

        // 按逆后序顺序重新构建布局
        for &block in &reverse_postorder {
            self.func.layout.append_block(block);

            // 重新添加该块的所有指令
            if let Some(insts) = block_instructions.get(&block) {
                for &inst in insts {
                    self.func.layout.append_inst(inst, block);
                }
            }
        }
    }

    /// 计算逆后序遍历顺序
    fn compute_reverse_postorder(entry: Block, func: &Function) -> Vec<Block> {
        use std::collections::HashSet;

        let mut visited = HashSet::new();
        let mut postorder = Vec::new();

        fn dfs(
            block: Block,
            func: &Function,
            visited: &mut HashSet<Block>,
            postorder: &mut Vec<Block>,
        ) {
            if !visited.insert(block) {
                return;
            }

            // 找到块的终结指令并遍历后继块
            if let Some(last_inst) = func.layout.last_inst(block) {
                let inst_data = &func.dfg.insts[last_inst];
                if FunctionBuilder::is_terminator(inst_data) {
                    let successors =
                        FunctionBuilder::get_successors(inst_data, &func.dfg.jump_tables);
                    for succ in successors {
                        dfs(succ, func, visited, postorder);
                    }
                }
            }

            postorder.push(block);
        }

        dfs(entry, func, &mut visited, &mut postorder);
        postorder.reverse(); // 逆后序
        postorder
    }

    /// 判断指令是否为终结指令
    fn is_terminator(inst_data: &InstructionData) -> bool {
        matches!(
            inst_data,
            InstructionData::Jump { .. }
                | InstructionData::Brif { .. }
                | InstructionData::BranchTable { .. }
                | InstructionData::Return { .. }
                | InstructionData::ReturnCall { .. }
                | InstructionData::Unreachable
        )
    }

    /// 从终结指令中提取后继块
    fn get_successors(inst_data: &InstructionData, jump_tables: &JumpTables) -> Vec<Block> {
        match inst_data {
            InstructionData::Jump { target } => vec![target.block()],
            InstructionData::Brif { targets, .. } => {
                let [then_block, else_block] = targets;
                if then_block.block() == else_block.block() {
                    vec![then_block.block()]
                } else {
                    vec![then_block.block(), else_block.block()]
                }
            }
            InstructionData::BranchTable { table, .. } => {
                use std::collections::HashSet;
                let table_data = &jump_tables[*table];
                let mut blocks = Vec::new();
                let mut seen = HashSet::new();
                for branch in table_data.all_branches() {
                    let block = branch.block();
                    if seen.insert(block) {
                        blocks.push(block);
                    }
                }
                blocks
            }
            _ => Vec::new(),
        }
    }
}

//==============================================================================
// FuncInstBuilder - 块级别的指令构建器
//==============================================================================

/// 函数指令构建器 - 负责在特定块中构建指令
pub struct FuncInstBuilder<'a> {
    /// 引用的函数
    pub func: &'a mut Function,
    /// 核心上下文 -- 用于类型推导
    pub ctx: &'a mut CoreContext,
    // builder: &'short mut FunctionBuilder<'long>,
    block: Block,
}

// impl<'short> FuncInstBuilder<'short> {
//     pub fn new<'long: 'short>(builder: &'short mut FunctionBuilder<'long>, block: Block) -> Self {
//         eprintln!("DEBUG: FuncInstBuilder::new - block = {:?}", block);
//         Self {
//             func: builder.func,
//             ctx: builder.ctx,
//             block,
//         }
//     }
// }

impl<'a> FuncInstBuilder<'a> {
    pub fn new(func: &'a mut Function, ctx: &'a mut CoreContext, block: Block) -> Self {
        // eprintln!("DEBUG: FuncInstBuilder::new - block = {:?}", block);
        Self { func, ctx, block }
    }
}

impl<'a> InstBuilderBase<'a> for FuncInstBuilder<'a> {
    fn dfg(&self) -> &DataFlowGraph {
        &self.func.dfg
    }

    fn dfg_mut(&mut self) -> &mut DataFlowGraph {
        &mut self.func.dfg
    }

    fn core_ctx(&self) -> &CoreContext {
        self.ctx
    }

    fn core_ctx_mut(&mut self) -> &mut CoreContext {
        self.ctx
    }

    fn dfg_ctx_mut(&mut self) -> (&mut DataFlowGraph, &mut CoreContext) {
        (&mut self.func.dfg, &mut self.ctx)
    }

    fn build(self, data: InstructionData) -> (Inst, &'a mut DataFlowGraph) {
        // eprintln!(
        //     "DEBUG: FuncInstBuilder::build - block = {:?}, data = {:?}",
        //     self.block,
        //     std::mem::discriminant(&data)
        // );
        // 检查块是否已经终结
        // if self.is_block_terminated(self.block) {
        //     panic!(
        //         "Attempted to add instruction to already terminated block {:?}",
        //         self.block
        //     );
        // }

        // 创建指令
        let inst = self.func.dfg.insts.insert(data);
        // eprintln!("DEBUG: 创建了指令 {:?}", inst);

        // 将指令添加到布局中（这应该由布局管理器自动处理）
        // eprintln!("DEBUG: 将指令添加到布局中");
        self.func.layout.append_inst(inst, self.block);
        // eprintln!("DEBUG: 指令已添加到布局");

        // 推导类型并创建结果
        let types = infer_inst_result_types(&self.func.dfg, self.ctx, inst);
        self.func.dfg.make_inst_results(inst, types);

        // 处理控制流指令
        match &self.func.dfg.insts[inst] {
            InstructionData::Jump { target } => {
                let block = target.block();
                self.ctx.ssa_builder.declare_block_predecessor(block, inst);
            }
            InstructionData::Brif { targets, .. } => {
                let [then_block, else_block] = targets;
                self.ctx
                    .ssa_builder
                    .declare_block_predecessor(then_block.block(), inst);
                if then_block.block() != else_block.block() {
                    self.ctx
                        .ssa_builder
                        .declare_block_predecessor(else_block.block(), inst);
                }
            }
            InstructionData::BranchTable { table: _table, .. } => {
                // TODO: 处理跳转表
            }
            _ => {}
        }

        (inst, &mut self.func.dfg)
    }
}

impl<'short> InstBuilder<'short> for FuncInstBuilder<'short> {}
