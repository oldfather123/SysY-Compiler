use crate::echo::make_advise;
use crate::lexer::symbol::Symbol;
use crate::lexer::token::{Keyword, Token, TokenKind};
use crate::lexer::{Cursor, TokenKind as PrimTokenKind};
use crate::session::CompileSession;
use crate::span::Span;

pub struct TokenReader<'a> {
    codes: &'a str,
    cursor: Cursor<'a>,
    at: u32,
    current_token:Option<Token>,
}

impl<'a> TokenReader<'a> {
    pub fn new(codes: &'a str) -> Self {
        let cursor = Cursor::new(codes);
        Self {
            codes,
            cursor,
            at: 0,
            current_token: None,
        }
    }

    pub fn get_current_token(&self) -> &Token{
        match self.current_token.as_ref() {
            None => {
                panic!("NOT START")
            }
            Some(tok) => {
                tok
            }
        }
    }

    pub fn next_token(&mut self, session: &mut CompileSession) -> Token {
        let mut token = self.cursor.advance_token();
        let now_line_at = self.cursor.line_at();
        let kind = match token.kind {
            PrimTokenKind::LineComment => TokenKind::Comment,
            PrimTokenKind::BlockComment => TokenKind::Comment,
            PrimTokenKind::Whitespace => TokenKind::Whitespace,
            PrimTokenKind::Ident => self.keyword_or_ident(&token),
            PrimTokenKind::Literal { kind } => {
                let sym = &self.codes[self.at as usize..(self.at + token.len) as usize];
                TokenKind::Literal {
                    kind,
                    sym: Symbol {
                        string: String::from(sym),
                    },
                }
            }
            PrimTokenKind::Semi => TokenKind::Semi,
            PrimTokenKind::Comma => TokenKind::Comma,
            PrimTokenKind::Dot => TokenKind::Dot,
            PrimTokenKind::OpenParen => {
                // BUG! 2023 5 9 TokenKind::OpenBrace
                TokenKind::OpenParen
            }
            PrimTokenKind::CloseParen => TokenKind::CloseParen,
            PrimTokenKind::OpenBrace => TokenKind::OpenBrace,
            PrimTokenKind::CloseBrace => TokenKind::CloseBrace,
            PrimTokenKind::OpenBracket => TokenKind::OpenBracket,
            PrimTokenKind::CloseBracket => TokenKind::CloseBracket,
            PrimTokenKind::Eq => {
                let kind = self.cursor.first_kind();
                if kind == PrimTokenKind::Eq {
                    self.cursor.advance_token();
                    token.len += 1;
                    TokenKind::DouEq
                } else {
                    TokenKind::Eq
                }
            }
            PrimTokenKind::Bang => {
                let kind = self.cursor.first_kind();
                if kind == PrimTokenKind::Eq {
                    self.cursor.advance_token();
                    token.len += 1;
                    TokenKind::NotEq
                } else {
                    TokenKind::Bang
                }
            }
            PrimTokenKind::Lt => {
                let kind = self.cursor.first_kind();
                if kind == PrimTokenKind::Eq {
                    self.cursor.advance_token();
                    token.len += 1;
                    TokenKind::Le
                } else {
                    TokenKind::Lt
                }
            }
            PrimTokenKind::Gt => {
                let kind = self.cursor.first_kind();
                if kind == PrimTokenKind::Eq {
                    self.cursor.advance_token();
                    token.len += 1;
                    TokenKind::Ge
                } else {
                    TokenKind::Gt
                }
            }
            PrimTokenKind::Minus => TokenKind::Minus,
            PrimTokenKind::And => {
                let kind = self.cursor.first_kind();
                if kind == PrimTokenKind::And {
                    self.cursor.advance_token();
                    token.len += 1;
                    TokenKind::DouAnd
                } else {
                    TokenKind::And
                }
            }
            PrimTokenKind::Or => {
                let kind = self.cursor.first_kind();
                if kind == PrimTokenKind::Or {
                    self.cursor.advance_token();
                    token.len += 1;
                    TokenKind::DouOr
                } else {
                    TokenKind::Or
                }
            }
            PrimTokenKind::Plus => TokenKind::Plus,
            PrimTokenKind::Star => TokenKind::Star,
            PrimTokenKind::Slash => TokenKind::Slash,
            PrimTokenKind::Percent => TokenKind::Percent,
            PrimTokenKind::Unknown => {
                let span = Span::new(self.at, token.len as u16);
                let sym = &self.codes[self.at as usize..(self.at + token.len) as usize];

                let error = span.make_error(
                    &format!("Unknown character \"{}\"", sym),
                    make_advise("considering check if there is illegal char"),
                );
                session.report_error(error);
                TokenKind::Illegal {
                    sym: Symbol::new(sym),
                }
            }
            PrimTokenKind::UnknownPrefix => {
                let span = Span::new(self.at, token.len as u16);
                let sym = &self.codes[self.at as usize..(self.at + token.len) as usize];
                let error = span.make_error(
                    &format!("Unknown prefix \"{}\"", sym),
                    make_advise("considering remove illegal char"),
                );
                session.report_error(error);
                TokenKind::Illegal {
                    sym: Symbol::new(sym),
                }
            }
            PrimTokenKind::Eof => TokenKind::Eof,
        };
        let token_new = Token::new(kind, Span::new(self.at, token.len as u16),now_line_at);
        self.at += token.len;
        self.current_token = Some(token_new.clone());
        return token_new;
    }
    fn keyword_or_ident(&self, token: &crate::lexer::SimpleToken) -> TokenKind {
        match token.kind {
            crate::lexer::TokenKind::Ident => {
                let chars = &self.codes[self.at as usize..(self.at + token.len) as usize];
                let keyword = match chars {
                    "int" => TokenKind::Keyword(Keyword::Int),
                    "float" => TokenKind::Keyword(Keyword::Float),
                    "void" => TokenKind::Keyword(Keyword::Void),
                    "while" => TokenKind::Keyword(Keyword::While),
                    "if" => TokenKind::Keyword(Keyword::If),
                    "else" => TokenKind::Keyword(Keyword::Else),
                    "continue" => TokenKind::Keyword(Keyword::Continue),
                    "break" => TokenKind::Keyword(Keyword::Break),
                    "const" => TokenKind::Keyword(Keyword::Const),
                    "return" => TokenKind::Keyword(Keyword::Return),
                    _ => {
                        let sym = Symbol::new(chars);
                        TokenKind::Ident { sym }
                    }
                };
                return keyword;
            }
            _ => {
                panic!("NEVER REACH")
            }
        }
    }
}
