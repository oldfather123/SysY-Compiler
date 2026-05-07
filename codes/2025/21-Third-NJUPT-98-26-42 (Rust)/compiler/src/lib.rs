pub mod args;
pub mod asm;
pub mod interpreter;
pub mod lexer;
pub mod parser;
pub mod pipeline;
pub mod ssa;
pub mod utils;

/// 包含pipeline中的核心类型
pub mod prelude {
    pub use crate::pipeline::{
        CompilerContext, CompilerPass, CompilerPipeline, CompilerStage, MainPass, Optimization,
    };
    pub use crate::utils::prelude::*;
}

use crate::{args::CompilerArgs, prelude::*};
use std::{collections::HashSet, path::PathBuf};
use strum::IntoEnumIterator;
/// 编译阶段特定设置
#[derive(Debug, Default)]
pub enum StageSettings {
    /// 解释器阶段的特定设置
    Interpreter {
        /// 重定向标准输入
        target_stdin: Option<PathBuf>,
        /// 用于验证程序输出是否正确的.out文件
        target_stdout: Option<PathBuf>,
    },
    /// 其他阶段没有特定设置
    #[default]
    Other,
}

/// 全局设置项
#[derive(Debug)]
pub struct Settings {
    /// 日志等级 -- TODO: 似乎用env_logger还要反向设置环境变量?
    pub rust_log: String,

    /// 用于输出中间IR的目录
    pub inter_result_dir: Option<PathBuf>,

    /// 源代码文件
    pub sources: PathBuf,

    /// 目标文件
    pub target: Option<PathBuf>,

    /// 更详细信息输出 -- 如liveness analysis的详细输出
    pub verbose: bool,

    /// 目标编译阶段
    pub stage: CompilerStage,

    /// 优化选项
    pub optimizations: HashSet<Optimization>,

    /// 阶段特定设置
    pub stage_settings: StageSettings,

    /// 寄存器分配调试选项
    pub reg_alloc_debug: RegAllocDebugOptions,
}

/// 寄存器分配调试选项
#[derive(Debug, Clone)]
pub struct RegAllocDebugOptions {
    pub debug_comments: bool,
    pub skip_spill: bool,
    pub skip_irc: bool,
    pub debug_irc: bool,
    pub skip_finalize: bool,
}

fn default_rust_log() -> String {
    "info".into()
}

impl TryFrom<CompilerArgs> for Settings {
    type Error = CErr;

    fn try_from(args: CompilerArgs) -> Result<Self, Self::Error> {
        let stage = if args.stage_arg.assembly {
            CompilerStage::ASM
        } else {
            args.stage_arg.stage
        };

        let optimizations = if !args.optimize_arg.optimize.is_empty() {
            args.optimize_arg.optimize.iter().cloned().collect()
        } else {
            match args.optimize_arg.optlevel {
                0 => HashSet::new(),                 // -O0 -- 不进行任何优化
                1 => Optimization::iter().collect(), // -O1 -- 所有优化
                _ => return Err(CErr::argument_err("Invalid optimization level")),
            }
        };

        let stage_settings = match stage {
            CompilerStage::Interp => StageSettings::Interpreter {
                target_stdin: args.target_stdin,
                target_stdout: args.target_stdout,
            },
            _ => {
                // 目标非解释器阶段 -- target_stdin 和 target_stdout 应该为 None
                if args.target_stdin.is_some() || args.target_stdout.is_some() {
                    return Err(CErr::argument_err(
                        "target_stdin & target_stdout only valid in Interpreter stage",
                    ));
                }
                StageSettings::Other
            }
        };

        let reg_alloc_debug = RegAllocDebugOptions {
            debug_comments: args.debug_comments,
            skip_spill: args.skip_spill,
            skip_irc: args.skip_irc,
            debug_irc: args.debug_irc,
            skip_finalize: args.skip_finalize,
        };

        Ok(Settings {
            rust_log: default_rust_log(),
            inter_result_dir: None,
            sources: args.source.clone(),
            target: args.target.clone(),
            verbose: args.verbose,
            stage,
            optimizations,
            stage_settings,
            reg_alloc_debug,
        })
    }
}
