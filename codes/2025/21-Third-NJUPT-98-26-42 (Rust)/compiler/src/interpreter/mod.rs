//! SSA IR 解释器
//!
//! 提供SSA IR的解释执行、严格验证和调试支持

mod debugger;
mod environment;
mod executor;
mod memory;
mod validator;

use crate::StageSettings;
use crate::prelude::*;
use crate::ssa::function::Function;
use std::io::Write;

pub use debugger::{DebugMode, Debugger};
pub use environment::{Environment, RuntimeValue};
pub use executor::Executor;
pub use memory::{MemoryId, MemoryManager, RuntimeElement};
pub use validator::Validator;

/// 解释器主Pass
#[derive(Debug)]
pub struct InterpPass {
    debug_mode: DebugMode,
}

impl Default for InterpPass {
    fn default() -> Self {
        Self::new()
    }
}

impl InterpPass {
    pub fn new() -> Self {
        Self {
            debug_mode: DebugMode::None,
        }
    }

    pub fn with_debug_mode(debug_mode: DebugMode) -> Self {
        Self { debug_mode }
    }
}

impl CompilerPass for InterpPass {
    fn name(&self) -> &'static str {
        "interpreter"
    }

    fn stage(&self) -> CompilerStage {
        CompilerStage::Interp
    }

    fn run(&mut self, ctx: &mut CompilerContext) -> CEResult<()> {
        // 只有目标阶段是解释器时才执行
        if ctx.settings.stage != CompilerStage::Interp {
            return Ok(());
        }

        let core_context = ctx
            .core_ctx
            .as_ref()
            .ok_or_else(|| CErr::runtime_err("SSA构建上下文未找到"))?;
        let func_contexts = ctx
            .func_ctxs
            .as_ref()
            .ok_or_else(|| CErr::runtime_err("SSA函数未找到"))?;

        // 获取解释器设置
        let (target_stdin, target_stdout) = if let StageSettings::Interpreter {
            target_stdin,
            target_stdout,
        } = &ctx.settings.stage_settings
        {
            (target_stdin, target_stdout)
        } else {
            return Err(CErr::runtime_err("解释器设置未找到"));
        };

        // 创建解释器环境
        let mut env = Environment::new(core_context)?;

        // 配置输入缓冲区
        if let Some(stdin_path) = target_stdin {
            let content = std::fs::read_to_string(stdin_path)
                .map_err(|e| CErr::runtime_err(format!("Failed to read .in file: {}", e)))?;
            let lines: Vec<String> = content.lines().map(|s| s.to_string()).collect();
            env.set_input_buffer(lines);
        }

        // 创建验证器并验证所有函数
        let mut validator = Validator::new(core_context);
        for func_ctx in func_contexts {
            validator.validate_function(&func_ctx.func)?;
        }

        println!("✓ SSA验证完成，所有函数通过验证");

        // 查找main函数并执行
        let main_func_ctx = func_contexts
            .iter()
            .find(|f| f.func.name == "main")
            .ok_or_else(|| CErr::runtime_err("未找到main函数"))?;
        let main_func = &main_func_ctx.func;

        // 创建调试器
        let mut debugger = Debugger::new(self.debug_mode, core_context);

        // 创建执行器并执行main函数
        let functions: Vec<Function> = func_contexts.iter().map(|ctx| ctx.func.clone()).collect();
        let mut executor = Executor::new(core_context, &mut env, &functions);

        println!("开始执行SSA IR...");

        let result = match self.debug_mode {
            DebugMode::None => executor.execute(main_func)?,
            _ => executor.execute_with_debugger(main_func, &mut debugger)?,
        };

        // 保存执行结果到context（包括输出和返回值）
        let mut complete_output = env.get_captured_output().to_string();

        // 处理main函数返回值：模拟Unix进程退出码行为（模256）
        let exit_code = {
            // Unix进程退出码是0-255，负数按照模256处理
            ((result % 256 + 256) % 256) as u8 as i32
        };

        // 只在输出不为空且不以换行符结尾时才添加换行符
        if !complete_output.is_empty() && !complete_output.ends_with('\n') {
            complete_output.push('\n');
        }
        complete_output.push_str(&exit_code.to_string());
        ctx.interp_results = Some(complete_output);

        // 校验输出（如果有.out文件）
        if let StageSettings::Interpreter { target_stdout, .. } = &ctx.settings.stage_settings {
            if let Some(out_path) = target_stdout {
                let expected = std::fs::read_to_string(out_path)
                    .map_err(|e| CErr::runtime_err(format!("Failed to read .out file: {}", e)))?;
                let expected = expected.trim();
                let actual = ctx.interp_results.as_ref().unwrap().trim();

                if expected != actual {
                    return Err(CErr::runtime_err(format!(
                        "输出不匹配:\n期望:\n{}\n实际:\n{}",
                        expected, actual
                    )));
                }
                println!("✓ 输出校验通过");
            }
        }

        // 打印统计信息
        println!("\n执行统计:");
        println!("  指令执行次数: {}", env.stats.instruction_count);
        println!("  内存分配次数: {}", env.stats.alloc_count);
        println!("  内存读取次数: {}", env.stats.load_count);
        println!("  内存写入次数: {}", env.stats.store_count);

        Ok(())
    }

    fn write_output(&self, ctx: &CompilerContext, writer: &mut dyn Write) -> CEResult<()> {
        // 直接输出已保存的执行结果
        if let Some(results) = &ctx.interp_results {
            write!(writer, "{}", results)?;
        }
        Ok(())
    }
}
