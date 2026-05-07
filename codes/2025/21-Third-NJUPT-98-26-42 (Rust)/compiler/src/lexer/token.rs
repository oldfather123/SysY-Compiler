//! SysY2022 Token 定义
//!
//! 专为 SysY2022 语法设计的轻量级 Token 系统，支持：
//! - 关键字：int, float, void, const, if, else, while, break, continue, return
//! - 运算符：算术、关系、逻辑等
//! - 字面量：整数、浮点数
//! - 标识符和符号

use strum::{Display, EnumIter, EnumString, VariantNames};

/// Token 类型枚举
///
/// 使用 strum 宏自动生成字符串转换、显示和迭代功能
#[derive(Debug, Clone, PartialEq, Eq, Hash, Display, EnumString, EnumIter, VariantNames)]
pub enum TokenType {
    // === 关键字 ===
    #[strum(serialize = "int")]
    Int,
    #[strum(serialize = "float")]
    Float,
    #[strum(serialize = "void")]
    Void,
    #[strum(serialize = "const")]
    Const,
    #[strum(serialize = "if")]
    If,
    #[strum(serialize = "else")]
    Else,
    #[strum(serialize = "while")]
    While,
    #[strum(serialize = "break")]
    Break,
    #[strum(serialize = "continue")]
    Continue,
    #[strum(serialize = "return")]
    Return,

    // === 运算符 ===
    // 算术运算符
    #[strum(serialize = "+")]
    Plus,
    #[strum(serialize = "-")]
    Minus,
    #[strum(serialize = "*")]
    Star,
    #[strum(serialize = "/")]
    Slash,
    #[strum(serialize = "%")]
    Percent,

    // 关系运算符
    #[strum(serialize = "<")]
    Lt,
    #[strum(serialize = "<=")]
    Le,
    #[strum(serialize = ">")]
    Gt,
    #[strum(serialize = ">=")]
    Ge,
    #[strum(serialize = "==")]
    Eq,
    #[strum(serialize = "!=")]
    Ne,

    // 逻辑运算符
    #[strum(serialize = "&&")]
    And,
    #[strum(serialize = "||")]
    Or,
    #[strum(serialize = "!")]
    Not,

    // 赋值运算符
    #[strum(serialize = "=")]
    Assign,

    // === 符号 ===
    LParen,
    RParen,
    LBrack,
    RBrack,
    LBrace,
    RBrace,
    Semi,
    Comma,

    // === 字面量 ===
    /// 整数字面量
    IntLit,
    /// 浮点数字面量
    FloatLit,
    /// 标识符
    Ident,

    // === 特殊 ===
    /// 文件结束
    Eof,
    /// 词法错误
    Error,
}

/// Token 位置信息
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct TokenPos {
    /// 行号 (1-based)
    pub line: usize,
    /// 列号 (1-based)
    pub column: usize,
    /// 字节偏移 (0-based)
    pub offset: usize,
    /// Token 长度
    pub len: usize,
}

impl TokenPos {
    pub fn new(line: usize, column: usize, offset: usize, len: usize) -> Self {
        Self {
            line,
            column,
            offset,
            len,
        }
    }

    /// 获取结束位置
    pub fn end_offset(&self) -> usize {
        self.offset + self.len
    }

    /// 获取结束列号
    pub fn end_column(&self) -> usize {
        self.column + self.len
    }
}

/// Token 结构体
#[derive(Debug, Clone, PartialEq)]
pub struct Token {
    /// Token 类型
    pub kind: TokenType,
    /// 原始文本内容 -- 字面量 / 标识符
    pub text: String,
    /// 位置信息
    pub pos: TokenPos,
}

impl Token {
    pub fn new(kind: TokenType, text: String, pos: TokenPos) -> Self {
        Self { kind, text, pos }
    }

    /// 空
    pub fn simple(kind: TokenType, pos: TokenPos) -> Self {
        Self {
            kind,
            text: String::new(),
            pos,
        }
    }
    pub fn is_keyword(&self) -> bool {
        matches!(
            self.kind,
            TokenType::Int
                | TokenType::Float
                | TokenType::Void
                | TokenType::Const
                | TokenType::If
                | TokenType::Else
                | TokenType::While
                | TokenType::Break
                | TokenType::Continue
                | TokenType::Return
        )
    }

    pub fn is_operator(&self) -> bool {
        matches!(
            self.kind,
            TokenType::Plus
                | TokenType::Minus
                | TokenType::Star
                | TokenType::Slash
                | TokenType::Percent
                | TokenType::Lt
                | TokenType::Le
                | TokenType::Gt
                | TokenType::Ge
                | TokenType::Eq
                | TokenType::Ne
                | TokenType::And
                | TokenType::Or
                | TokenType::Not
                | TokenType::Assign
        )
    }

    pub fn is_literal(&self) -> bool {
        matches!(self.kind, TokenType::IntLit | TokenType::FloatLit)
    }

    pub fn is_ident(&self) -> bool {
        matches!(self.kind, TokenType::Ident)
    }

    pub fn is_eof(&self) -> bool {
        matches!(self.kind, TokenType::Eof)
    }

    pub fn is_error(&self) -> bool {
        matches!(self.kind, TokenType::Error)
    }
}
