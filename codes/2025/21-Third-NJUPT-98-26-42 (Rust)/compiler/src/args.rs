use super::{CompilerStage, Optimization};
use clap::{ArgAction, Args, Parser};
use std::path::PathBuf;

/// 命令行参数
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
#[command(propagate_version = true)]
pub struct CompilerArgs {
    /// Source code to compile
    pub source: PathBuf,

    /// Target file
    #[arg(short = 'o', long)]
    pub target: Option<PathBuf>,

    /// Turn debugging information on
    #[arg(short, long)]
    pub verbose: bool,

    #[command(flatten)]
    pub optimize_arg: OptimizeArg,

    #[command(flatten)]
    pub stage_arg: CompilerStageArg,

    /// Redirect standard target input for interpreter
    #[arg(long)]
    pub target_stdin: Option<PathBuf>,

    /// Used to validate whether the program's output is correct
    #[arg(long)]
    pub target_stdout: Option<PathBuf>,

    /// Debug: Enable debug comments in generated assembly
    #[arg(long = "debug-comments", default_value = "false")]
    pub debug_comments: bool,

    /// Debug: Skip register spilling phase
    #[arg(long = "skip-spill", default_value = "false")]
    pub skip_spill: bool,

    /// Debug: Skip IRC graph coloring phase
    #[arg(long = "skip-irc", default_value = "false")]
    pub skip_irc: bool,

    /// Debug: Enable IRC debug output
    #[arg(long = "debug-irc", default_value = "false")]
    pub debug_irc: bool,

    /// Debug: Skip assembly finalization (prologue/epilogue)
    #[arg(long = "skip-finalize", default_value = "false")]
    pub skip_finalize: bool,
}

// 为了生成 <--stage <STAGE>|--assembly> 而使用的关联技巧
#[derive(Args, Debug)]
#[group(required = true, multiple = false)]
pub struct CompilerStageArg {
    /// Which stage to compile
    #[arg(long, default_value_t=CompilerStage::None)]
    pub stage: CompilerStage,

    /// ASM stage
    #[arg(short = 'S', long)]
    pub assembly: bool,
}

// 为了生成 <--optlevel <OPTLEVEL>|--optimize <OPTIMIZE>> 而使用的关联技巧
#[derive(Args, Debug)]
#[group(required = false, multiple = false)]
pub struct OptimizeArg {
    /// Optimization 0-1
    #[arg(short = 'O', long, default_value_t=0, value_parser = clap::value_parser!(u8).range(0..2))]
    pub optlevel: u8,

    /// Manually specify the required optimizations
    #[arg(long, action = ArgAction::Append, value_delimiter = ',')]
    pub optimize: Vec<Optimization>,
}
