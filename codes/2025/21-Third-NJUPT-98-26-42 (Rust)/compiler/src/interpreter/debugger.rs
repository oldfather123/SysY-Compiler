//! 调试器模块

use super::{Environment, environment::RuntimeValue};
use crate::prelude::*;
use crate::ssa::{CoreContext, function::Function};
use std::collections::{HashMap, HashSet};

/// 调试模式
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DebugMode {
    /// 不启用调试
    None,
    /// 单步执行
    Step,
    /// 断点调试
    Breakpoint,
    /// 跟踪执行
    Trace,
}

/// 调试器
pub struct Debugger<'a> {
    mode: DebugMode,
    core_context: &'a CoreContext,
    /// 断点
    breakpoints: HashSet<(String, Block, usize)>,
    /// 执行历史
    execution_history: Vec<ExecutionRecord>,
    /// 观察点
    watchpoints: HashMap<Value, WatchPoint>,
}

/// 执行记录
#[derive(Debug, Clone)]
struct ExecutionRecord {
    function: String,
    block: Block,
    inst_idx: usize,
    inst: Inst,
    timestamp: std::time::Instant,
}

/// 观察点
#[derive(Debug, Clone)]
struct WatchPoint {
    value: Value,
    old_value: Option<RuntimeValue>,
    hit_count: usize,
}

impl<'a> Debugger<'a> {
    pub fn new(mode: DebugMode, core_context: &'a CoreContext) -> Self {
        Self {
            mode,
            core_context,
            breakpoints: HashSet::new(),
            execution_history: Vec::new(),
            watchpoints: HashMap::new(),
        }
    }

    /// 添加断点
    pub fn add_breakpoint(&mut self, func: String, block: Block, inst_idx: usize) {
        self.breakpoints.insert((func, block, inst_idx));
    }

    /// 移除断点
    pub fn remove_breakpoint(&mut self, func: String, block: Block, inst_idx: usize) {
        self.breakpoints.remove(&(func, block, inst_idx));
    }

    /// 添加观察点
    pub fn add_watchpoint(&mut self, value: Value) {
        self.watchpoints.insert(
            value,
            WatchPoint {
                value,
                old_value: None,
                hit_count: 0,
            },
        );
    }

    /// 检查断点
    pub fn check_breakpoint(&self, func: String, block: Block, inst_idx: usize) -> bool {
        self.breakpoints.contains(&(func, block, inst_idx))
    }

    /// 记录执行
    pub fn record_execution(&mut self, func: String, block: Block, inst_idx: usize, inst: Inst) {
        self.execution_history.push(ExecutionRecord {
            function: func,
            block,
            inst_idx,
            inst,
            timestamp: std::time::Instant::now(),
        });
    }

    /// 检查观察点
    pub fn check_watchpoints(&mut self, env: &Environment, func: &Function) {
        for (value, watchpoint) in self.watchpoints.iter_mut() {
            if let Ok(current_value) = env.get_value(*value, &func.dfg) {
                let changed = match &watchpoint.old_value {
                    Some(old) => !values_equal(old, &current_value),
                    None => true,
                };

                if changed {
                    watchpoint.hit_count += 1;
                    println!("Watchpoint hit: {:?} changed to {:?}", value, current_value);
                    watchpoint.old_value = Some(current_value);
                }
            }
        }
    }

    /// 调试器开始
    pub fn on_start(&mut self) {
        match self.mode {
            DebugMode::Trace => {
                println!("=== Starting SSA Interpretation (Trace Mode) ===");
            }
            DebugMode::Step => {
                println!("=== Starting SSA Interpretation (Step Mode) ===");
                println!("Press Enter to step through each instruction...");
            }
            _ => {}
        }
    }

    /// 调试器结束
    pub fn on_finish(&self) {
        if self.mode != DebugMode::None {
            println!("\n=== Interpretation Finished ===");
            self.print_summary();
        }
    }

    /// 在指令执行前调用
    pub fn on_before_instruction(
        &mut self,
        env: &Environment,
        func: &Function,
        block: Block,
        inst_idx: usize,
        inst: Inst,
    ) -> bool {
        let func_ref = func.name.clone();

        // 记录执行
        self.record_execution(func_ref.clone(), block, inst_idx, inst);

        // 检查断点
        if self.check_breakpoint(func_ref.clone(), block, inst_idx) {
            println!("Breakpoint hit at {}:{}:{}", func_ref, block, inst_idx);
            return self.interactive_debug(env, func, block, inst_idx);
        }

        // 根据模式处理
        match self.mode {
            DebugMode::Step => {
                self.print_current_instruction(func, block, inst_idx, inst);
                return self.wait_for_step();
            }
            DebugMode::Trace => {
                self.print_current_instruction(func, block, inst_idx, inst);
            }
            _ => {}
        }

        false
    }

    /// 在指令执行后调用
    pub fn on_after_instruction(&mut self, env: &Environment, func: &Function) {
        // 检查观察点
        self.check_watchpoints(env, func);
    }

    /// 打印当前指令
    fn print_current_instruction(
        &self,
        func: &Function,
        block: Block,
        inst_idx: usize,
        inst: Inst,
    ) {
        let inst_data = &func.dfg.insts[inst];
        println!("[{}:{}:{}] {:?}", &func.name, block, inst_idx, inst_data);
    }

    /// 等待单步
    fn wait_for_step(&self) -> bool {
        use std::io::{self, BufRead};
        let stdin = io::stdin();
        let mut line = String::new();
        stdin.lock().read_line(&mut line).unwrap();

        match line.trim() {
            "c" | "continue" => false, // 继续执行
            "q" | "quit" => std::process::exit(0),
            _ => true, // 继续单步
        }
    }

    /// 交互式调试
    fn interactive_debug(
        &mut self,
        env: &Environment,
        func: &Function,
        block: Block,
        inst_idx: usize,
    ) -> bool {
        use std::io::{self, BufRead, Write};

        loop {
            print!("(debug) ");
            io::stdout().flush().unwrap();

            let stdin = io::stdin();
            let mut line = String::new();
            stdin.lock().read_line(&mut line).unwrap();

            let parts: Vec<&str> = line.split_whitespace().collect();
            if parts.is_empty() {
                continue;
            }

            match parts[0] {
                "c" | "continue" => return false,
                "s" | "step" => return true,
                "q" | "quit" => std::process::exit(0),
                "p" | "print" => {
                    if parts.len() > 1 {
                        self.print_value(env, func, parts[1]);
                    }
                }
                "bt" | "backtrace" => {
                    self.print_backtrace(env);
                }
                "l" | "list" => {
                    self.print_context(func, block, inst_idx);
                }
                "h" | "help" => {
                    self.print_help();
                }
                _ => {
                    println!("Unknown command: {}", parts[0]);
                }
            }
        }
    }

    /// 打印值
    fn print_value(&self, env: &Environment, func: &Function, value_str: &str) {
        // TODO: 解析值引用并打印
        println!("Value printing not implemented");
    }

    /// 打印调用栈
    fn print_backtrace(&self, env: &Environment) {
        println!("Call stack:");
        for (i, frame) in env.call_stack.iter().enumerate().rev() {
            println!(
                "  #{}: {} @ {}:{}",
                i, &frame.function.name, frame.current_block, frame.next_inst
            );
        }
    }

    /// 打印上下文
    fn print_context(&self, func: &Function, block: Block, inst_idx: usize) {
        let insts: Vec<_> = func.layout.block_insts(block).collect();
        let start = inst_idx.saturating_sub(2);
        let end = (inst_idx + 3).min(insts.len());

        println!("Instructions in block {}:", block);
        for i in start..end {
            let inst = insts[i];
            let prefix = if i == inst_idx { "=>" } else { "  " };
            println!("{} [{}] {:?}", prefix, i, func.dfg.insts[inst]);
        }
    }

    /// 打印帮助
    fn print_help(&self) {
        println!("Debugger commands:");
        println!("  c, continue  - Continue execution");
        println!("  s, step      - Step to next instruction");
        println!("  q, quit      - Quit the program");
        println!("  p <value>    - Print value");
        println!("  bt           - Print backtrace");
        println!("  l, list      - List instructions");
        println!("  h, help      - Show this help");
    }

    /// 打印执行摘要
    fn print_summary(&self) {
        println!("\nExecution Summary:");
        println!(
            "Total instructions executed: {}",
            self.execution_history.len()
        );

        // 统计每个函数的执行次数
        let mut func_counts: HashMap<String, usize> = HashMap::new();
        for record in &self.execution_history {
            *func_counts.entry(record.function.clone()).or_insert(0) += 1;
        }

        println!("\nInstructions per function:");
        for (func, count) in func_counts {
            println!("  {}: {}", func, count);
        }

        // 打印观察点统计
        if !self.watchpoints.is_empty() {
            println!("\nWatchpoint hits:");
            for (value, wp) in &self.watchpoints {
                println!("  {:?}: {} hits", value, wp.hit_count);
            }
        }
    }
}

/// 比较两个运行时值是否相等
fn values_equal(a: &RuntimeValue, b: &RuntimeValue) -> bool {
    match (a, b) {
        (RuntimeValue::Unit, RuntimeValue::Unit) => true,
        (RuntimeValue::Bool(a), RuntimeValue::Bool(b)) => a == b,
        (RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => a == b,
        (RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => (a - b).abs() < f32::EPSILON,
        (
            RuntimeValue::ArrayPtr { memory_id: a, .. },
            RuntimeValue::ArrayPtr { memory_id: b, .. },
        ) => a == b,
        (
            RuntimeValue::MemToken {
                object: a,
                version: av,
            },
            RuntimeValue::MemToken {
                object: b,
                version: bv,
            },
        ) => a == b && av == bv,
        _ => false,
    }
}
