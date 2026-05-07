use std::fmt::Display;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum CErr {
    #[error("Parsing error at {line}:{col} -- {msg}")]
    ParseError {
        line: usize,
        col: usize,
        msg: String,
    },
    #[error("Lexer error at {line}:{col} -- {msg}")]
    LexerError {
        line: usize,
        col: usize,
        msg: String,
    },
    #[error("Argument error: {msg}")]
    ArgumentError { msg: String },
    #[error("Missing input: {0}")]
    MissingInput(String),
    #[error("Internal error: {0}")]
    Internal(String),
    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),
    #[error("SSA error: {0}")]
    SSAError(String),
}

impl CErr {
    pub fn parse_err(line: usize, col: usize, msg: impl Display) -> Self {
        Self::ParseError {
            line,
            col,
            msg: msg.to_string(),
        }
    }

    pub fn lexer_err(line: usize, col: usize, msg: impl Display) -> Self {
        Self::LexerError {
            line,
            col,
            msg: msg.to_string(),
        }
    }

    pub fn argument_err(msg: impl Display) -> Self {
        Self::ArgumentError {
            msg: msg.to_string(),
        }
    }

    /// 通用错误构造函数
    pub fn new(msg: impl Display) -> Self {
        Self::Internal(msg.to_string())
    }

    /// 运行时错误
    pub fn runtime_err(msg: impl Display) -> Self {
        Self::Internal(format!("Runtime error: {}", msg))
    }

    /// IO错误包装
    pub fn io_err(msg: impl Display) -> Self {
        Self::Internal(format!("IO error: {}", msg))
    }

    /// 阶段错误
    pub fn stage_err(msg: impl Display) -> Self {
        Self::Internal(format!("Stage error: {}", msg))
    }

    /// SSA 错误
    pub fn ssa_err(msg: impl Display) -> Self {
        Self::SSAError(msg.to_string())
    }

    /// ASM 错误
    pub fn asm_err(msg: impl Display) -> Self {
        Self::SSAError(msg.to_string())
    }
}

pub type CEResult<V> = Result<V, CErr>;
