use crate::ast::{BasicType, AST};
use crate::lexer::symbol::Symbol;
use crate::lexer::token::{Keyword, TokenKind};
use crate::lexer::{token::Token, TokenReader};
use crate::parser::parse_core::StateMachine;
use crate::session::CompileSession;

mod parse_core;
mod parse_expression;
mod parse_right;
mod parse_start;
mod tests;

pub type ParseResult = Result<AST, ()>;

pub struct TokenParser<'a, 'b> {
    lexer: ParserReader<'a>,
    session: &'b mut CompileSession,
}

pub(crate) struct ParserReader<'a> {
    lexer: TokenReader<'a>,
    first: Option<Token>,
    current: Option<Token>,
}

impl<'a> ParserReader<'a> {
    fn next_token(&mut self, session: &mut CompileSession) -> Token {
        if self.first.is_some() {
            let new_tok = Some(self.lexer.next_token(session));
            let ret_tok = std::mem::replace(&mut self.first, new_tok);
            self.current = ret_tok.clone();
            return ret_tok.unwrap();
        } else {
            panic!("never reach")
        }
    }
    ///This function ignores Comment, WhiteSpace
    fn next_usable_token(&mut self, session: &mut CompileSession) -> Token {
        if self.first.is_some() {
            let first = self.first.as_ref().unwrap();
            if first.kind == TokenKind::Comment || first.kind == TokenKind::Whitespace {
                loop {
                    let ret_tok = self.lexer.next_token(session);
                    if ret_tok.kind == TokenKind::Comment || ret_tok.kind == TokenKind::Whitespace {
                        continue;
                    }
                    let new_tok = self.lexer.next_token(session);
                    self.first = Some(new_tok);
                    self.current = Some(ret_tok.clone());
                    return ret_tok;
                }
            } else {
                let new_tok = Some(self.lexer.next_token(session));
                let ret_tok = std::mem::replace(&mut self.first, new_tok);
                self.current = ret_tok.clone();
                return ret_tok.unwrap();
            }
        } else {
            panic!("never reach")
        }
    }

    fn get_current_token(&self) -> &Token {
        match &self.current {
            None => {
                panic!("NEVER HERE")
            }
            Some(tok) => tok,
        }
    }

    fn first_token(&self) -> &Token {
        if self.first.is_none() {
            panic!("ERROR")
        }
        return self.first.as_ref().unwrap();
    }

    fn first_usable_token(&mut self, session: &mut CompileSession) -> &Token {
        if self.first.is_some() {
            let first = self.first.as_ref().unwrap();
            if first.kind == TokenKind::Comment || first.kind == TokenKind::Whitespace {
                loop {
                    let ret_tok = self.lexer.next_token(session);
                    if ret_tok.kind == TokenKind::Comment || ret_tok.kind == TokenKind::Whitespace {
                        continue;
                    }
                    self.first = Some(ret_tok);
                    return self.first.as_ref().unwrap();
                }
            } else {
                return self.first.as_ref().unwrap();
            }
        } else {
            panic!("never reach")
        }
    }
}

impl<'a, 'b> TokenParser<'a, 'b> {
    pub fn new(mut lexer: TokenReader<'a>, session: &'b mut CompileSession) -> Self {
        let first = lexer.next_token(session);
        Self {
            lexer: ParserReader {
                lexer,
                first: Some(first),
                current: None,
            },
            session,
        }
    }

    pub fn parse_ast(&mut self) -> ParseResult {
        let mut state_machine = StateMachine::new(&mut self.lexer, self.session);
        let ast = state_machine.parse_unit();
        return match ast {
            Ok(err) => Ok(err),
            Err(_) => Err(()),
        };
    }
}

impl Token {
    #[inline]
    fn to_ident_or_lit_symbol(&self) -> Symbol {
        if let TokenKind::Ident { sym } = &self.kind {
            return Symbol::new(&sym.string);
        } else if let TokenKind::Literal { kind: _, sym } = &self.kind {
            return Symbol::new(&sym.string);
        } else {
            panic!("NEVER HERE")
        }
    }

    #[inline]
    fn is_any_start_keyword(&self) -> bool {
        if let TokenKind::Keyword(key) = self.kind {
            return key == Keyword::Void
                || key == Keyword::Int
                || key == Keyword::Float
                || key == Keyword::Break
                || key == Keyword::Continue
                || key == Keyword::Return
                || key == Keyword::If
                || key == Keyword::While;
        }
        false
    }
    #[inline]
    fn is_ignoreable(&self) -> bool {
        self.kind == TokenKind::Whitespace || self.kind == TokenKind::Comment
    }
    #[inline]
    fn is_ident(&self) -> bool {
        match self.kind {
            TokenKind::Ident { .. } => true,
            _ => false,
        }
    }
    #[inline]
    fn is_unknown(&self) -> bool {
        return match self.kind {
            TokenKind::Illegal { .. } => true,
            _ => false,
        };
    }
    #[inline]
    fn is_type(&self) -> bool {
        if let TokenKind::Keyword(key) = self.kind {
            return key == Keyword::Void || key == Keyword::Int || key == Keyword::Float;
        }
        false
    }
    #[inline]
    fn is_not_void_type(&self) -> bool {
        if let TokenKind::Keyword(key) = self.kind {
            return key == Keyword::Int || key == Keyword::Float;
        }
        false
    }
    fn get_type(&self) -> BasicType {
        match self.kind {
            TokenKind::Keyword(key) => match key {
                Keyword::Int => BasicType::Int,
                Keyword::Float => BasicType::Float,
                Keyword::Void => BasicType::Void,
                _ => {
                    panic!("never here")
                }
            },
            _ => {
                panic!("never here")
            }
        }
    }

    /// <ident> , ( , + , - , <Literal> , !
    pub(crate) fn is_in_expr_first(&self) -> bool {
        return match self.kind {
            TokenKind::Ident { .. } | TokenKind::OpenParen | TokenKind::Plus | TokenKind::Minus | TokenKind::Literal { .. } | TokenKind::Bang => true,
            _ => false,
        };
    }
    /// ;
    #[inline]
    pub(crate) fn is_semi(&self) -> bool {
        return match self.kind {
            TokenKind::Semi => true,
            _ => false,
        };
    }

    /// ,
    #[inline]
    pub(crate) fn is_comma(&self) -> bool {
        return match self.kind {
            TokenKind::Comma => true,
            _ => false,
        };
    }
}
