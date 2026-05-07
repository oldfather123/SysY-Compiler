//! SSA 函数定义
use super::ctx::*;
use super::dfg::*;
use super::ir::*;
use super::layout::*;
use crate::parser::ast;
use crate::prelude::*;

/// 外部函数
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ExtFuncData {
    pub name: String,
    pub signature: FuncSignature,
}

impl ExtFuncData {
    pub fn declare_extfunc(func_decl: &ast::FuncDecl, ctx: &mut CoreContext) {
        let signature = FuncSignature::make_signature(func_decl, ctx);
        let extfunc = Self {
            name: func_decl.name.clone(),
            signature,
        };
        let extname = ctx.name_ref(func_decl.name.clone());
        ctx.ext_funcs.insert(extname, extfunc);
    }
}

/// 函数调用规范
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum CallConv {
    /// 标准 C 调用规范
    C,
    /// 快速调用规范 -- 寄存器优先
    Fast,
    /// C 可变参数调用规范 -- 如printf
    CVarArgs,
}

/// 函数签名
/// Call: 是潜在的状态修改操作, 消费其ptr参数对应的所有 MemToken, 并(在保守情况下)为它们各自产生一个新的 MemToken 作为返回值
///         函数调用带有约定: 所有传入的指针必须紧随其后传入内存令牌，形成: ret_val, token1', token2', ..., tokenX' = Call @func(..., ptr1, token1, ptr2, token2, ...., ptrX, tokenX)
///         而且在细化的内存分析中, Call 可能还会返回全局变量的内存令牌
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct FuncSignature {
    /// 参数类型 -- 传入指针严格按照参数布局: ptr1, token1, ptr2, token2, ..., ptrN, tokenN 内存令牌紧随ptr
    pub params: Vec<Type>,
    /// 返回值 -- 允许有多个返回值, 使用 Value::Inst{InstRef -> Call, ids: N } 来表示第N个返回值
    /// 返回值会出现内存令牌，代表 ret_val, token1', token2', ..., tokenX' 内存的使用
    pub returns: Vec<Type>,

    /// 参数名/返回值名 -- 需要用 xx_token 且与returns一一对应
    pub param_names: Vec<Option<NameRef>>,
    pub return_names: Vec<Option<NameRef>>,

    /// 原始参数和返回值名
    pub param_origins: Vec<Type>,
    pub return_origins: Option<Type>,

    /// 调用规范
    pub call_conv: CallConv,
}

impl Default for FuncSignature {
    fn default() -> Self {
        Self::new()
    }
}

impl FuncSignature {
    pub fn new() -> Self {
        Self {
            params: Vec::new(),
            returns: Vec::new(),
            param_names: Vec::new(),
            return_names: Vec::new(),
            param_origins: Vec::new(),
            return_origins: None,
            call_conv: CallConv::C,
        }
    }

    /// 制作函数签名
    pub fn make_signature(func_decl: &ast::FuncDecl, ctx: &mut CoreContext) -> Self {
        use super::ast_to_ssa;
        let mut sig = FuncSignature::new();
        if func_decl.return_type != ast::BaseType::Void {
            let ty = ast_to_ssa::convert_base_type(func_decl.return_type);
            sig.returns.push(ty);
            sig.return_names.push(None); // origins是用来处理ptr参数的token
            sig.return_origins = Some(ty);
        }
        for param in &func_decl.params {
            let ty = ast_to_ssa::convert_type(&param.ty, ctx).unwrap();
            sig.params.push(ty);
            sig.param_names.push(Some(ctx.name_ref(param.name.clone())));
            sig.param_origins.push(ty);

            // 数组参数需要额外的内存令牌
            if !param.ty.dimensions.is_empty() {
                sig.params.push(Type::MemToken);
                sig.returns.push(Type::MemToken);
                let token_ref = ctx.make_token_ref(param.name.clone());
                sig.param_names.push(Some(token_ref));
                sig.return_names.push(Some(token_ref));
            }
        }
        sig
    }
}

/// 函数定义
#[derive(Debug, Clone)]
pub struct Function {
    /// 函数名
    pub name: String,
    /// 函数签名
    pub signature: FuncSignature,
    /// 入口块
    pub entry: Block,
    /// 数据流图
    pub dfg: DataFlowGraph,
    /// 布局
    pub layout: Layout,
}

impl Default for Function {
    fn default() -> Self {
        Self::new()
    }
}

impl Function {
    pub fn new() -> Self {
        Self {
            name: String::new(),
            signature: FuncSignature::new(),
            entry: Block::default(),
            dfg: DataFlowGraph::new(),
            layout: Layout::new(),
        }
    }

    pub fn inst_block(&self, inst: Inst) -> Block {
        self.layout.inst_block(inst).unwrap()
    }

    /// 推断指令的结果类型
    pub fn infer_inst_result_types(&self, ctx: &mut CoreContext, inst: Inst) -> Vec<Type> {
        super::dfg::infer_inst_result_types(&self.dfg, ctx, inst)
    }

    /// 获取块的后继迭代器
    pub fn successors(&self, block: Block) -> impl Iterator<Item = Block> + '_ {
        // 获取块的最后一条指令
        let last_inst = self.layout.last_inst(block);

        // 创建一个收集所有后继的向量
        let mut successors = Vec::new();

        if let Some(inst) = last_inst {
            if let Some(inst_data) = self.dfg.insts.get(inst) {
                use crate::ssa::ir::InstructionData;
                match inst_data {
                    InstructionData::Jump { target } => {
                        successors.push(target.block);
                    }
                    InstructionData::Brif { targets, .. } => {
                        successors.push(targets[0].block);
                        successors.push(targets[1].block);
                    }
                    InstructionData::BranchTable { table, .. } => {
                        if let Some(table_data) = self.dfg.jump_tables.get(*table) {
                            for entry in &table_data.table {
                                successors.push(entry.block);
                            }
                        }
                    }
                    _ => {}
                }
            }
        }

        successors.into_iter()
    }
}
