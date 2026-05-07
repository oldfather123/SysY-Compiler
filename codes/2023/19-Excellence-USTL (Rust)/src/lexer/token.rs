use crate::lexer::symbol::Symbol;
use crate::lexer::LiteralKind;
use crate::span::Span;

#[derive(Debug, Clone)]
pub struct Token {
    pub kind: TokenKind,
    pub span: Span,
    pub line_at:usize
}

// at here , the token passed the lexical check.
impl Token {
    pub fn new(kind: TokenKind, span: Span,line_at:usize) -> Token {
        Token { kind, span,line_at }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Keyword {
    If,
    Else,

    While,

    Int,
    Float,
    Void,

    Const,

    Break,
    Return,
    Continue,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum TokenKind {
    // Multi-char tokens:
    Comment,
    Whitespace,
    Ident {
        sym: Symbol,
    },
    Keyword(Keyword),
    Literal {
        kind: LiteralKind,
        sym: Symbol,
    },
    // One-char tokens:
    /// ";"
    Semi,
    /// ","
    Comma,
    /// "."
    Dot,
    /// "("
    OpenParen,
    /// ")"
    CloseParen,
    /// "{"
    OpenBrace,
    /// "}"
    CloseBrace,
    /// "["
    OpenBracket,
    /// "]"
    CloseBracket,
    /// "="
    Eq,
    /// "=="
    DouEq,
    /// "!="
    NotEq,
    /// "!"
    Bang,
    /// "<"
    Lt,
    /// "<="
    Le,
    /// ">"
    Gt,
    /// ">="
    Ge,
    /// "-"
    Minus,
    /// "&"
    And,
    /// "&&"
    DouAnd,
    /// "|"
    Or,
    /// "||"
    DouOr,
    /// "+"
    Plus,
    /// "*"
    Star,
    /// "/"
    Slash,
    /// "%"
    Percent,
    /// 非法Token
    Illegal {
        sym: Symbol,
    },
    /// End of input.
    Eof,
}
