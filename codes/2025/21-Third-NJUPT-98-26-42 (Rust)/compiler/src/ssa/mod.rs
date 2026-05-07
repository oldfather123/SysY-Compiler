pub mod ast_to_ssa;
pub mod builder;
pub mod call_graph;
pub mod cfg;
pub mod ctx;
pub mod cursor;
pub mod dfg;
pub mod domtree;
pub mod function;
pub mod globals;
pub mod inst_builder;
pub mod ir;
pub mod ir_fmt;
pub mod ir_impl;
pub mod loop_analysis;
// pub mod mem_analysis;
pub mod egraph;
pub mod layout;
pub mod opts1;
pub mod opts2;
pub mod runtime;
pub mod scc;
pub mod ssa_builder;
pub mod stratify;
pub mod ty_promoter;

pub use self::cfg::{BlockPredecessor, ControlFlowGraph};
pub use self::ctx::*;
pub use self::cursor::{Cursor, CursorPosition, FuncCursor};
pub use self::domtree::{DominatorTree, DominatorTreePreorder};
pub use self::ir::{StackSlotData, StackSlots};
pub use self::loop_analysis::{LoopAnalysis, LoopLevel};

use crate::prelude::*;
use std::io::Write;

/// SSA Pass - 将 AST 转换为 SSA IR
#[derive(derive_new::new, Debug)]
pub struct SSAPass;

impl CompilerPass for SSAPass {
    fn name(&self) -> &'static str {
        "ssa"
    }

    fn run(&mut self, ctx: &mut CompilerContext) -> CEResult<()> {
        // 获取 AST
        let ast_ctx = ctx
            .ast_context
            .as_ref()
            .ok_or_else(|| CErr::ssa_err("No AST context"))?;
        let ast_root = ctx.ast_root.ok_or_else(|| CErr::ssa_err("No AST root"))?;

        // 获取模块名
        let module_name = ctx
            .settings
            .sources
            .file_stem()
            .and_then(|s| s.to_str())
            .unwrap_or("main")
            .to_string();

        // 执行 AST 到 SSA 转换
        let (mut core_ctx, functions) =
            ast_to_ssa::convert_ast_to_ssa(ast_ctx, ast_root, module_name)
                .map_err(|e| CErr::ssa_err(&e))?;

        // 函数上下文
        let mut func_ctxs = FuncContexts::new();
        for function in functions {
            let func_ctx = FuncContext::from_function(function);
            func_ctxs.with_func_ctx(func_ctx, &mut core_ctx);
        }

        // 执行优化
        func_ctxs
            .optimize(&mut core_ctx)
            .map_err(|e| CErr::ssa_err(&e))?;

        // 存储结果
        ctx.cur_stage = CompilerStage::SSA;
        ctx.core_ctx = Some(core_ctx);
        ctx.func_ctxs = Some(func_ctxs);

        Ok(())
    }

    fn stage(&self) -> CompilerStage {
        CompilerStage::SSA
    }

    fn write_output(&self, ctx: &CompilerContext, writer: &mut dyn Write) -> CEResult<()> {
        let core_ctx = ctx
            .core_ctx
            .as_ref()
            .ok_or_else(|| CErr::ssa_err("No SSA core context"))?;
        let functions = ctx
            .func_ctxs
            .as_ref()
            .ok_or_else(|| CErr::ssa_err("No SSA functions"))?;

        // 打印全局变量和外部函数声明
        let ctx_formatter = ir_fmt::BuilderContextFormatter::new(core_ctx);
        write!(writer, "{}", ctx_formatter)
            .map_err(|e| CErr::ssa_err(format!("Write error: {}", e)))?;

        let mut printed_funcs = std::collections::HashSet::new();
        for (func_ref, _ext_func_data) in core_ctx.ext_funcs.iter() {
            if let Some(func_ctx) = functions.ctxs.get(func_ref) {
                let func_formatter = ir_fmt::FunctionFormatter::new(&func_ctx.func, core_ctx);
                writeln!(writer, "{}", func_formatter)
                    .map_err(|e| CErr::ssa_err(format!("Write error: {}", e)))?;
                printed_funcs.insert(func_ref);
            }
        }

        Ok(())
    }
}
