//! 语法解析模块

pub mod ast;
pub mod ast_impl;
pub mod ast_rename;
pub mod parse;

#[cfg(test)]
mod ast_test;
#[cfg(test)]
mod parse_test;

use crate::prelude::*;
use parse::Parser;
use std::io::Write;

#[derive(derive_new::new, Debug)]
pub struct ParserPass;

impl CompilerPass for ParserPass {
    fn name(&self) -> &'static str {
        "parser"
    }

    fn run(&mut self, ctx: &mut CompilerContext) -> CEResult<()> {
        let tokens = ctx
            .tokens
            .as_ref()
            .ok_or_else(|| CErr::parse_err(0, 0, "No tokens?"))?
            .clone();

        let parser = Parser::new(tokens);
        let (mut ast_context, ast_root) = parser.parse()?;

        // 变量重命名 -- 处理变量遮蔽
        let mut renamer = ast_rename::VarRenamer::new();
        renamer.rename_variables(&mut ast_context, ast_root)?;

        // 存储AST ctx & root
        ctx.cur_stage = CompilerStage::Parser;
        ctx.ast_context = Some(ast_context);
        ctx.ast_root = Some(ast_root);

        Ok(())
    }

    fn stage(&self) -> CompilerStage {
        CompilerStage::Parser
    }

    fn write_output(&self, ctx: &CompilerContext, writer: &mut dyn Write) -> CEResult<()> {
        let ast_ctx = ctx
            .ast_context
            .as_ref()
            .ok_or_else(|| CErr::parse_err(0, 0, "No AST Context"))?;
        let ast_root = ctx
            .ast_root
            .ok_or_else(|| CErr::parse_err(0, 0, "No AST Root"))?;

        writeln!(writer, "{}", ast_ctx.format_ast(ast_root))
            .map_err(|e| CErr::parse_err(0, 0, format!("Write error {}", e)))?;

        Ok(())
    }
}
