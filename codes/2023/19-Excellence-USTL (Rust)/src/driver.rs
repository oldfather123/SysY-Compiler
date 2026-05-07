use crate::asm::RISCVBackend;
use crate::compiler::{Compiler, RegBackend};
use crate::optimizer::OptLevel;
use crate::util::Colorize;
#[cfg(debug_assertions)]
use crate::compiler::{DebugStop, DebugAction};

pub struct CommandParamParser;

pub enum CompilerBuildResult {
    NoParam,
    BuildError,
    Ok(Compiler),
}

#[derive(Eq, PartialEq)]
pub enum ArgState {
    /// 设置优化等级
    Opt,
    /// 设置输出位置
    Output,
    /// 保留以后使用
    Include,
    /// 等待
    Wait,
    /// 寄存器分配backend
    WaitRegBackend,
    #[cfg(debug_assertions)]
    /// 等待HIrEmit输出位置
    WaitHirEmitOutput,
    #[cfg(debug_assertions)]
    /// 等待MirEmit输出位置
    WaitMirEmitOutput,
    #[cfg(debug_assertions)]
    /// 等待MirEmit输出位置
    WaitMirOptEmitOutput,
    #[cfg(debug_assertions)]
    /// Debug参数设置
    Debug,
}

impl CommandParamParser {
    pub fn parse_commands(commands: Vec<String>) -> CompilerBuildResult {
        if commands.len() == 0 {
            return CompilerBuildResult::NoParam;
        }
        let args = commands;
        let mut builder = CompilerBuilder::new_default();
        let mut is_input_init = false;
        let mut is_output_init = false;

        let mut arg_state = ArgState::Wait;

        for arg in args {
            match arg_state {
                ArgState::Opt => {
                    if &arg == "0" {
                        builder.opt_level = OptLevel::O0;
                    } else if &arg == "1" {
                        builder.opt_level = OptLevel::O1;
                    } else if &arg == "2" {
                        builder.opt_level = OptLevel::O2;
                    } else {
                        builder.opt_level = OptLevel::O2;
                        println!("{}", "warning: not support opt flag , use O2".yellow())
                    }
                    arg_state = ArgState::Wait
                }
                ArgState::Output => {
                    if !is_output_init {
                        builder.output = arg;
                        is_output_init = true;
                    } else {
                        println!("{}", "warning: output number is more than one".yellow())
                    }
                    arg_state = ArgState::Wait
                }
                ArgState::Include => {
                    println!("{}", "warning: -I not support".yellow())
                }
                #[cfg(debug_assertions)]
                ArgState::Debug => {
                    // -D emit_hir -D stop_ast
                    #[cfg(debug_assertions)]
                    {
                        if arg == "hir_emit" {
                            arg_state = ArgState::WaitHirEmitOutput;
                            continue;
                        } else if arg == "mir_emit" {
                            arg_state = ArgState::WaitMirEmitOutput;
                            continue;
                        } else if arg == "mir_opt_emit" {
                            arg_state = ArgState::WaitMirOptEmitOutput;
                            continue;
                        } else if arg == "ast_detect" {
                            builder.debug_actions.push(DebugAction::AstFeatureDetect);
                        } else if arg == "stop_ast" {
                            builder.debug_stop = DebugStop::Ast
                        } else if arg == "stop_hir" {
                            builder.debug_stop = DebugStop::Hir;
                        } else if arg == "stop_mir" {
                            builder.debug_stop = DebugStop::Mir;
                        } else if arg == "stop_lir" {
                            builder.debug_stop = DebugStop::Lir;
                        } else if arg == "stop_asm" {
                            builder.debug_stop = DebugStop::Asm;
                        } else {
                            println!("{}", "Unsupported Debug Param".red());
                            return CompilerBuildResult::BuildError;
                        }
                        arg_state = ArgState::Wait;
                    }
                }
                ArgState::Wait => {
                    if arg.starts_with("-o") && arg.len() == 2 {
                        arg_state = ArgState::Output;
                    } else if arg.starts_with("-O") {
                        // -o(012)
                        if arg.len() == 2 {
                            arg_state = ArgState::Opt;
                            continue;
                        } else if arg.len() == 3 {
                            for (index, arg) in arg.chars().enumerate() {
                                if index == 2 {
                                    if arg == '0' {
                                        builder.opt_level = OptLevel::O0;
                                    } else if arg == '1' {
                                        builder.opt_level = OptLevel::O1;
                                    } else if arg == '2' {
                                        builder.opt_level = OptLevel::O2;
                                    } else {
                                        println!("{}", "Not support opt level".red());
                                        return CompilerBuildResult::BuildError;
                                    }
                                }
                            }
                            continue;
                        } else {
                            print!("{}", "error: unknown param ".yellow());
                            println!("'{}'", arg);
                            return CompilerBuildResult::BuildError;
                        }
                    } else if arg.starts_with("-I") && arg.len() == 2 {
                        // -I
                        arg_state = ArgState::Include;
                    } else if arg.starts_with("-S") && arg.len() == 2 {
                        // -S
                        arg_state = ArgState::Wait;
                    } else if arg.starts_with("-D") && arg.len() == 2 {
                        #[cfg(not(debug_assertions))]
                        panic!("Release not support Debug");
                        #[cfg(debug_assertions)]
                        {
                            arg_state = ArgState::Debug;
                        }
                    } else if arg == "-reg_backend" {
                        arg_state = ArgState::WaitRegBackend;
                    } else {
                        if !is_input_init {
                            builder.input = Some(arg);
                            is_input_init = true;
                        } else {
                            print!("{}", "error: unknown param ".red());
                            println!("{}", arg);
                            return CompilerBuildResult::BuildError;
                        }
                        arg_state = ArgState::Wait;
                    }
                }
                ArgState::WaitRegBackend => {
                    if  arg == "T2" {
                        builder.reg_backend = RegBackend::T2;
                    }else if arg == "T3"{
                        builder.reg_backend = RegBackend::T3;
                    }else {
                        println!("{}", "Unsupported Translate Backend".red());
                        return CompilerBuildResult::BuildError;
                    }
                    arg_state = ArgState::Wait;
                },
                #[cfg(debug_assertions)]
                ArgState::WaitHirEmitOutput => {
                    builder.debug_actions.push(DebugAction::HirEmit(arg));
                    arg_state = ArgState::Wait;
                }
                #[cfg(debug_assertions)]
                ArgState::WaitMirEmitOutput => {
                    builder.debug_actions.push(DebugAction::MIREmit(arg));
                    arg_state = ArgState::Wait;
                }
                #[cfg(debug_assertions)]
                ArgState::WaitMirOptEmitOutput => {
                    builder.debug_actions.push(DebugAction::MIROptEmit(arg));
                    arg_state = ArgState::Wait;
                }
            }
        }
        if arg_state != ArgState::Wait {
            return CompilerBuildResult::BuildError;
        }
        builder.build_riscv()
    }
}

pub struct CompilerBuilder {
    input: Option<String>,
    output: String,
    #[cfg(debug_assertions)]
    debug_stop: DebugStop,
    #[cfg(debug_assertions)]
    debug_actions: Vec<DebugAction>,
    opt_level: OptLevel,
    reg_backend:RegBackend
}

impl CompilerBuilder {
    pub fn new_default() -> CompilerBuilder {
        CompilerBuilder {
            input: None,
            output: "out.S".to_string(),
            #[cfg(debug_assertions)]
            debug_stop: DebugStop::None,
            #[cfg(debug_assertions)]
            debug_actions: vec![],
            opt_level: OptLevel::O0,
            reg_backend: RegBackend::T2, // 默认 T2
        }
    }

    pub fn build_riscv(self) -> CompilerBuildResult {
        let mut compiler = Compiler::new(&self.output, &RISCVBackend);
        #[cfg(debug_assertions)]
        compiler.set_compile_stop(self.debug_stop);
        #[cfg(debug_assertions)]
        for action in self.debug_actions {
            compiler.add_debug_mode(action);
        }
        compiler.set_opt(self.opt_level);
        #[cfg(not(debug_assertions))]
        {
            // Sysyc O1 ->  Real O1 (half open)
            // Sysyc O2 ->  Real O1
            if self.opt_level == OptLevel::O1 || self.opt_level == OptLevel::O2  {
                compiler.set_opt(OptLevel::O2)
            }else {
                // 默认00优化
                compiler.set_opt(OptLevel::O1)
            }
        }
        compiler.set_reg_backend(self.reg_backend);

        if self.input.is_none() {
            println!("{}", "error: missing input".red());
            return CompilerBuildResult::BuildError;
        }
        compiler.set_single_input(self.input.unwrap());
        CompilerBuildResult::Ok(compiler)
    }
}