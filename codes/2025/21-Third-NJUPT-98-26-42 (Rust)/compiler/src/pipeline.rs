use crate::utils::metadata::MetadataPass;
use crate::{Settings, asm, interpreter, lexer, parser, ssa, utils::prelude::*};
use clap::ValueEnum;
use enum_dispatch::enum_dispatch;
use std::{fs::File, path::PathBuf};
use strum::{Display, EnumIter, EnumString};

/// 编译阶段
#[derive(
    Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord, Hash, EnumIter, ValueEnum, Display, Default,
)]
pub enum CompilerStage {
    /// None stage -- Just start
    #[default]
    #[strum(serialize = "none")]
    None,
    /// Lexer stage -- Outputs Tokens
    #[strum(serialize = "lexer")]
    Lexer,
    /// Parser stage -- Outputs AST
    #[strum(serialize = "parser")]
    Parser,
    /// SSA stage -- Outputs SSA IR
    #[strum(serialize = "ssa")]
    SSA,
    /// Interpreter stage -- Executes using an internal interpreter
    #[strum(serialize = "interp")]
    Interp,
    /// ASM stage -- Outputs RISCV assembly
    #[strum(serialize = "asm")]
    ASM,
}

/// 所有优化项 -- 会在不同的IR阶段都有相应优化
#[derive(Debug, PartialEq, Eq, Hash, Clone, ValueEnum, Display, EnumIter, EnumString)]
pub enum Optimization {
    /// Dead code elimination
    #[strum(serialize = "dead-code-elimination")]
    DeadCodeElimination,
    /// Function inlining
    #[strum(serialize = "function-inline")]
    FunctionInline,
}

/// 中心上下文 -- 保存所有中间IR & 设置
#[derive(Debug)]
pub struct CompilerContext {
    pub settings: Settings, // 不再用OnceLock
    pub tokens: Option<Vec<lexer::Token>>,
    pub ast_context: Option<parser::ast::AstContext>,
    pub ast_root: Option<parser::ast::CompUnitRef>,
    // SSA IR 相关
    pub core_ctx: Option<ssa::CoreContext>,
    pub func_ctxs: Option<ssa::FuncContexts>,
    // 解释器执行结果（包含所有输出和返回值的完整字符串）
    pub interp_results: Option<String>,
    pub cur_stage: CompilerStage,
    pub metadata: CompilerMetadata,
    // 汇编程序
    pub rvprog: Option<asm::RvProg>,
    // IRC失败标志（用于延迟panic）
    pub irc_failed: bool,
}
impl CompilerContext {
    pub fn new(settings: Settings) -> Self {
        Self {
            settings,
            tokens: None,
            ast_context: None,
            ast_root: None,
            core_ctx: None,
            func_ctxs: None,
            interp_results: None,
            cur_stage: CompilerStage::None,
            metadata: CompilerMetadata::new(),
            rvprog: None,
            irc_failed: false,
        }
    }
}

/// Pass trait -- 所有管线中的Pass应该使用
#[enum_dispatch]
pub trait CompilerPass {
    fn name(&self) -> &'static str;
    fn run(&mut self, ctx: &mut CompilerContext) -> CEResult<()>;
    fn stage(&self) -> CompilerStage;
    fn write_output(&self, ctx: &CompilerContext, writer: &mut dyn std::io::Write) -> CEResult<()>;
}

/// 枚举静态分发 -- 替代动态分发
/// 这里的设计是让Pipeline不直接管理优化，而是从Pipeline视角来看是划分几个固定IR Pass，
/// 例如 SSAOptimizerPass 会根据settings进行一系列的优化
/// 然后这些IR Pass分别根据settings执行对应一系列优化
#[derive(Debug)]
#[enum_dispatch(CompilerPass)]
pub enum MainPass {
    Metadata(MetadataPass),
    Lexer(lexer::LexerPass),
    Parser(parser::ParserPass),
    SSA(ssa::SSAPass),
    Interpreter(interpreter::InterpPass),
    ASM(asm::AsmPass),
    // SSAOptimizer(SSAOptimizerPass),
    // ASMOptimizer(ASMOptimizerPass) -- 例如窥孔优化，超优化 ...
}

/// 编译管线
#[derive(Debug)]
pub struct CompilerPipeline {
    pub passes: Vec<MainPass>,
    pub ctx: CompilerContext,
}

impl CompilerPipeline {
    pub fn new(settings: Settings) -> Self {
        // 初始化日志
        init_logger();

        let passes = vec![
            // 在这个地方添加管线
            MainPass::Lexer(lexer::LexerPass::new()),
            MainPass::Parser(parser::ParserPass::new()),
            MainPass::SSA(ssa::SSAPass::new()),
            MainPass::Interpreter(interpreter::InterpPass::new()),
            MainPass::ASM(asm::AsmPass::new()),
        ];
        Self {
            passes,
            ctx: CompilerContext::new(settings),
        }
    }

    pub fn run(&mut self) -> Result<&CompilerContext, CErr> {
        let target_stage = self.ctx.settings.stage;
        let output_file = self.ctx.settings.target.clone();

        for pass in &mut self.passes {
            let stage = pass.stage();
            let start_time = self.ctx.metadata.phase_start(stage);
            if stage > target_stage {
                break;
            }

            pass.run(&mut self.ctx)?;

            if stage == target_stage {
                Self::write_output(pass, &self.ctx, output_file.as_ref())?;
            }

            // 跳过不需要计时的阶段
            if stage == CompilerStage::None {
                continue;
            } else {
                self.ctx.metadata.phase_end(stage, start_time);
            }
        }

        // 输出最后的元数据
        MainPass::Metadata(MetadataPass::new()).run(&mut self.ctx)?;

        Ok(&self.ctx)
    }

    /// 根据output_file输出到文件/stdout
    fn write_output(
        pass: &MainPass,
        ctx: &CompilerContext,
        output_file: Option<&PathBuf>,
    ) -> CEResult<()> {
        use std::io::{BufWriter, stdout};

        if let Some(output_file) = output_file {
            let file = File::create(output_file)
                .map_err(|e| CErr::argument_err(format!("Error crate file {}", e)))?;
            let mut writer = BufWriter::new(file);
            pass.write_output(ctx, &mut writer)?;
        } else {
            let stdout = stdout();
            let mut writer = BufWriter::new(stdout.lock());
            pass.write_output(ctx, &mut writer)?;
        }

        Ok(())
    }
}
