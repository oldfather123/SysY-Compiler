//! 统一日志接口

use crate::pipeline::CompilerStage;
use std::time::Instant;

pub use log::{debug, error, info, trace, warn};

/// 初始化日志
pub fn init_logger() {
    env_logger::Builder::from_default_env()
        .format_timestamp_millis()
        .init();
}

/// 日志宏
#[macro_export]
macro_rules! compiler_log {
    (trace, $phase:expr, $($arg:tt)*) => {
        log::trace!("[{}] {}", $phase, format!($($arg)*))
    };
    (debug, $phase:expr, $($arg:tt)*) => {
        log::debug!("[{}] {}", $phase, format!($($arg)*))
    };
    (info, $phase:expr, $($arg:tt)*) => {
        log::info!("[{}] {}", $phase, format!($($arg)*))
    };
    (warn, $phase:expr, $($arg:tt)*) => {
        log::warn!("[{}] {}", $phase, format!($($arg)*))
    };
    (error, $phase:expr, $($arg:tt)*) => {
        log::error!("[{}] {}", $phase, format!($($arg)*))
    };
}

/// 计时宏
#[macro_export]
macro_rules! time_phase {
    ($phase:expr, $name:expr, $block:block) => {{
        let _timer = $crate::utils::log::Timer::new($phase, $name);
        $block
    }};
}

#[macro_export]
macro_rules! lexer_log {
    ($level:ident, $($arg:tt)*) => {
        $crate::compiler_log!($level, $crate::pipeline::CompilerStage::Lexer, $($arg)*)
    };
}

#[macro_export]
macro_rules! parser_log {
    ($level:ident, $($arg:tt)*) => {
        $crate::compiler_log!($level, $crate::pipeline::CompilerStage::Parser, $($arg)*)
    };
}

#[macro_export]
macro_rules! ssa_log {
    ($level:ident, $($arg:tt)*) => {
        $crate::compiler_log!($level, $crate::pipeline::CompilerStage::SSA, $($arg)*)
    };
}

#[macro_export]
macro_rules! vasm_log {
    ($level:ident, $($arg:tt)*) => {
        $crate::compiler_log!($level, $crate::pipeline::CompilerStage::VirtualASM, $($arg)*)
    };
}

#[macro_export]
macro_rules! asm_log {
    ($level:ident, $($arg:tt)*) => {
        $crate::compiler_log!($level, $crate::pipeline::CompilerStage::ASM, $($arg)*)
    };
}

/// 计时器
pub struct Timer {
    phase: CompilerStage,
    start: Instant,
    name: String,
}

impl Timer {
    pub fn new(phase: CompilerStage, name: impl Into<String>) -> Self {
        let timer = Self {
            phase,
            start: Instant::now(),
            name: name.into(),
        };
        compiler_log!(debug, timer.phase, "Start {}", timer.name);
        timer
    }

    /// 手动停止计时器
    pub fn stop(self) {
        let duration = self.start.elapsed();
        compiler_log!(
            info,
            self.phase,
            "Total {} (Duration: {:?})",
            self.name,
            duration
        );
    }
}

impl Drop for Timer {
    fn drop(&mut self) {
        let duration = self.start.elapsed();
        compiler_log!(
            debug,
            self.phase,
            "End {} (Duration: {:?})",
            self.name,
            duration
        );
    }
}
