//! Lexer

use crate::prelude::*;
use std::io::Write;

pub mod token;
pub mod token_impl;
#[cfg(test)]
pub mod token_test;

pub use token::{Token, TokenPos, TokenType};
pub use token_impl::Lexer;

#[derive(derive_new::new, Debug)]
pub struct LexerPass;

impl CompilerPass for LexerPass {
    fn name(&self) -> &'static str {
        "lexer"
    }

    fn run(&mut self, ctx: &mut CompilerContext) -> CEResult<()> {
        // 检查输入文件
        let input_file = &ctx.settings.sources;
        if !input_file.exists() {
            return Err(CErr::lexer_err(0, 0, "Input file does not exist"));
        }

        // 读取源代码
        let source_code = std::fs::read_to_string(input_file)
            .map_err(|e| CErr::lexer_err(0, 0, format!("Failed to read file: {}", e)))?;

        // 创建词法分析器并分析
        let mut lexer = Lexer::new(source_code);
        let tokens = lexer.tokenize()?;

        // 更新当前阶段
        ctx.cur_stage = CompilerStage::Lexer;

        // 将 tokens 存储到 CompilerContext 中
        ctx.tokens = Some(tokens);

        Ok(())
    }

    fn stage(&self) -> CompilerStage {
        CompilerStage::Lexer
    }

    fn write_output(&self, ctx: &CompilerContext, writer: &mut dyn Write) -> CEResult<()> {
        let tokens = ctx
            .tokens
            .as_ref()
            .ok_or_else(|| CErr::lexer_err(0, 0, "没有可用的tokens"))?;

        for token in tokens {
            if token.is_eof() {
                break;
            }
            writeln!(writer, "{:?}", token)
                .map_err(|e| CErr::lexer_err(0, 0, format!("写入输出失败: {}", e)))?;
        }

        Ok(())
    }
}
