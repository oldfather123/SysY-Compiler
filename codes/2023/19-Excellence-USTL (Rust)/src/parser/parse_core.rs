use crate::allow_nothing;
use crate::ast::{ArrayDefine, AstUnit, FnDefine, FnParams, Ident, IdentWithInit, RowDefines, Stmts, VarDefine, AST};
use crate::echo::make_advise;
use crate::lexer::token::{Keyword, Token, TokenKind};
use crate::parser::ParserReader;
use crate::session::CompileSession;

/// `Accept` 接受状态
/// `Error` 强制接受
/// `Panic` 无法接受
pub(crate) enum ReturnState<AcceptState> {
    /// 接受状态
    Accept(AcceptState),
    // 这里的Error(AcceptState,Option<Error>) 设计错误，Error本身就在自身函数中得到了汇报
    // 注意这个Error返回有问题，他会把错误的位置信息抹掉，如果在他的调用者中没有汇报错误，那么这个错误将无法定位
    /// 可忍受错误接受
    // 5月10日
    // 新设计方案， 返回 Option<AcceptState> , is_report , error_start , stop_at
    // 分别代表        强行识别的错误的状态    , 是否报错了  ， 无法识别的base ， 恐慌的恢复base(这个token被eat了)
    Error(AcceptState),
    /// 无法接受的状态
    Panic,
}

pub(crate) struct StateMachine<'a, 'b> {
    pub(crate) token_parser: &'b mut ParserReader<'a>,
    // 状态机报错机制可优化，
    // 当我们报错时候，我们基本上是从当前的token位置来报错的，所以位置可以忽略，StateMachine 可以优化.
    pub(crate) session: &'b mut CompileSession,
}

impl<'a, 'b> StateMachine<'a, 'b> {
    pub(crate) fn new(parser: &'b mut ParserReader<'a>, session: &'b mut CompileSession) -> Self {
        Self {
            token_parser: parser,
            session,
        }
    }
}


pub(crate) enum DefineOrNone {
    Fn(FnDefine),
    Var(VarDefine),
    Array(ArrayDefine),
    RowDefines(RowDefines),
    None,
}


pub(crate) struct TypeWithIdent {
    pub type_tok: Token,
    pub ident: Token,
}


pub(crate) struct FnAfter {
    pub params: Option<FnParams>,
    pub stmts: Stmts,
}

impl FnAfter {
    pub(crate) fn new() -> FnAfter {
        FnAfter { params: None, stmts: vec![] }
    }
    pub(crate) fn set_params(&mut self, params: FnParams) {
        self.params = Some(params);
    }
}

impl<'a, 'b> StateMachine<'a, 'b> {
    pub fn parse_unit(&mut self) -> Result<AST, ()> {
        let mut ast = AST::new();
        loop {
            let first = self.token_parser.first_token();
            if first.kind == TokenKind::Eof {
                return Ok(ast);
            }
            let ast_unit = self.maybe_start();
            match ast_unit {
                Ok(fn_or_var) => {
                    match fn_or_var {
                        DefineOrNone::Fn(fn_def) => {
                            ast.push(AstUnit::FnDefine(fn_def));
                        }
                        DefineOrNone::Var(var_def) => {
                            ast.push(AstUnit::VarDefine(var_def));
                        }
                        DefineOrNone::None => {
                            // This can only be the case if the last function is followed by a comment
                        }
                        DefineOrNone::Array(arr) => ast.push(AstUnit::ArrayDefine(arr)),
                        DefineOrNone::RowDefines(row) => ast.push(AstUnit::RowDefines(row)),
                    }
                }
                Err(_) => {
                    return Err(());
                }
            }
        }
    }

    // 注意我们不标记这个函数，因为他作为入口函数，返回的状态供给外部使用
    fn maybe_start(&mut self) -> Result<DefineOrNone, ()> {
        self.maybe_ignorable(true);

        let token = self.token_parser.first_token();
        match &token.kind {
            TokenKind::Comment | TokenKind::Whitespace => {
                // just a whitespace or comment in file and the comment must is not in function block or expression interval
                panic!("never here")
            }
            TokenKind::Keyword(sym) => {
                return match sym {
                    Keyword::Int | Keyword::Float => {
                        //  支持将RowDefine简化为SingleDefine
                        let def = self.must_fn_or_define();
                        ok_or_err(def, |def| {
                            let a = match def {
                                DefineOrNone::Fn(fn_def) => DefineOrNone::Fn(fn_def),
                                DefineOrNone::Var(var) => DefineOrNone::Var(var),
                                DefineOrNone::Array(array) => DefineOrNone::Array(array),
                                DefineOrNone::RowDefines(mut def) => {
                                    if def.defs.len() == 1 {
                                        let first = def.defs.pop().unwrap();
                                        match first {
                                            IdentWithInit::Var { ident, init } => {
                                                let var_define = VarDefine {
                                                    is_const: false,
                                                    define_type: def.define_type,
                                                    ident,
                                                    with_value: init,
                                                };
                                                DefineOrNone::Var(var_define)
                                            }
                                            IdentWithInit::Array { ident, dimensions, init } => {
                                                let array_define = ArrayDefine {
                                                    is_const: false,
                                                    define_type: def.define_type,
                                                    ident,
                                                    dimensions,
                                                    init,
                                                };
                                                DefineOrNone::Array(array_define)
                                            }
                                        }
                                    } else {
                                        DefineOrNone::RowDefines(def)
                                    }
                                }
                                DefineOrNone::None => DefineOrNone::None,
                            };
                            Ok(a)
                        })
                    }
                    Keyword::Void => {
                        //todo 语义检查
                        let def = self.must_fn_or_define();
                        ok_or_err(def, |def| Ok(def))
                    }
                    Keyword::Const => {
                        // 全局常量
                        let def = self.must_const_define();
                        match def {
                            ReturnState::Accept(mut def) => {
                                if def.defs.len() == 1 {
                                    let first = def.defs.pop().unwrap();
                                    return match first {
                                        IdentWithInit::Var { ident, init } => {
                                            let var_define = VarDefine {
                                                is_const: true,
                                                define_type: def.define_type,
                                                ident,
                                                with_value: init,
                                            };
                                            Ok(DefineOrNone::Var(var_define))
                                        }
                                        IdentWithInit::Array { ident, dimensions, init } => {
                                            let array_define = ArrayDefine {
                                                is_const: true,
                                                define_type: def.define_type,
                                                ident,
                                                dimensions,
                                                init,
                                            };
                                            Ok(DefineOrNone::Array(array_define))
                                        }
                                    };
                                }
                                Ok(DefineOrNone::RowDefines(def))
                            }
                            ReturnState::Error(_) => Err(()),
                            ReturnState::Panic => Err(()),
                        }
                    }
                    Keyword::If | Keyword::Else | Keyword::While | Keyword::Break | Keyword::Return | Keyword::Continue => {
                        let error = token.span.make_error("syntax error 1", None);
                        self.session.report_error(error);
                        Err(())
                    }
                };
            }
            TokenKind::Ident { .. } => {
                let error = token.span.make_error("syntax error 2", None);
                self.session.report_error(error);
                return Err(());
            }
            TokenKind::Literal { .. } => {
                let error = token.span.make_error("syntax error 3", None);
                self.session.report_error(error);
                return Err(());
            }
            TokenKind::Semi => {
                // 没有包含在 {} 里面的 ;
                let error = token.span.make_error("syntax error 4", None);
                self.session.report_error(error);
                return Err(());
            }
            TokenKind::Comma
            | TokenKind::Dot
            | TokenKind::OpenParen
            | TokenKind::CloseParen
            | TokenKind::OpenBrace
            | TokenKind::CloseBrace
            | TokenKind::OpenBracket
            | TokenKind::CloseBracket
            | TokenKind::Eq
            | TokenKind::DouEq
            | TokenKind::NotEq
            | TokenKind::Bang
            | TokenKind::Lt
            | TokenKind::Le
            | TokenKind::Gt
            | TokenKind::Ge
            | TokenKind::Minus
            | TokenKind::And
            | TokenKind::DouAnd
            | TokenKind::Or
            | TokenKind::DouOr
            | TokenKind::Plus
            | TokenKind::Star
            | TokenKind::Slash
            | TokenKind::Percent
            | TokenKind::Illegal { .. } => {
                // 非法字符不需要报错，因为上一阶段已经处理过了，
                // 启用恐慌模式,
                // ident,{
                // self.enter_panic_mode(vec![TokenKind::CloseBrace], true);
                let error = token.span.make_error("syntax error 5", None);
                self.session.report_error(error);
                return Err(());
            }
            TokenKind::Eof => {
                return Ok(DefineOrNone::None);
            }
        }
    }
    ///
    ///
    /// # 参数
    ///
    /// returns: ReturnState<bool> 只返回Panic和Accept
    /// 如果返回Accept(true) 代表确实是有多个ignorable字符存在
    /// 如果返回Accept(false) 代表只有一个ignorable字符存在
    /// 如果返回Panic 代表没有遇到ignorable字符
    /// # 说明
    /// 至少处理一个可分隔字符
    /// 不压栈
    
    pub(crate) fn must_ignore_more(&mut self) -> ReturnState<bool> {
        return match self.must_ignoreable() {
            ReturnState::Accept(_) => {
                let is_more = self.maybe_ignorable(true);
                match is_more {
                    ReturnState::Accept(is) => ReturnState::Accept(is),
                    _ => {
                        panic!("never here")
                    }
                }
            }
            ReturnState::Panic => ReturnState::Panic,
            _ => {
                panic!("never here")
            }
        };
    }
    ///
    ///
    /// # 参数
    ///
    /// returns: ReturnState<()> 只返回Panic和Accept
    ///
    /// # 说明
    /// 处理一个可分隔字符
    /// 不压栈
    
    fn must_ignoreable(&mut self) -> ReturnState<()> {
        if let Some(tok) = &self.token_parser.first {
            if tok.is_ignoreable() {
                return ReturnState::Accept(());
            }
        }
        return ReturnState::Panic;
    }
    ///
    ///
    /// # 参数
    ///
    /// returns: ReturnState<()> 只返回Accept
    ///
    /// # 说明
    /// 尽可能吃掉可忽略字符
    /// 不压栈
    // 这里返回值不明确，maybe函数任何情况下都应该返回Accept
    
    pub(crate) fn maybe_ignorable(&mut self, need_more: bool) -> ReturnState<bool> {
        let mut eat = false;
        loop {
            if self.token_parser.first_token().is_ignoreable() {
                if need_more {
                    self.token_parser.next_token(self.session);
                    if !eat {
                        eat = true;
                    }
                } else {
                    self.token_parser.next_token(self.session);
                    return ReturnState::Accept(true);
                }
            } else {
                break;
            }
        }
        ReturnState::Accept(eat)
    }

    ///
    ///
    /// # Arguments
    ///
    /// returns: ReturnState<TypeWithIdent>
    /// 三状态均返回
    ///
    /// # 说明
    /// 识别一个 （类型 标识符） 对
    /// 停止在type token前可进入该函数
    
    pub(crate) fn must_type_ident_pair(&mut self) -> ReturnState<TypeWithIdent> {
        let ret_or_type = self.token_parser.next_usable_token(self.session); // 这个token已经经过检查了
                                                                             // self.pushed_state.push_back(ret_or_type);
        match self.must_ignore_more() {
            ReturnState::Accept(_is_more) => {
                allow_nothing!();
            }
            ReturnState::Panic => {
                // 空格不出现
                let error = ret_or_type.span.make_error("error word after type define", None);
                self.session.report_error(error);
                return ReturnState::Panic;
            }
            _ => {
                panic!("never here")
            }
        }
        let first = self.token_parser.first_token();
        return if first.is_ident() {
            let name = self.token_parser.next_token(self.session);
            let ret = TypeWithIdent {
                type_tok: ret_or_type,
                ident: name,
            };
            ReturnState::Accept(ret)
        } else if first.is_unknown() {
            // 标识符没有出现在类型定义之后
            // 如果这里是illegal 的话可以接受
            // todo 语义检查
            let name = self.token_parser.next_token(self.session);
            let ret = TypeWithIdent {
                type_tok: ret_or_type,
                ident: name,
            };
            // 这里不汇报Error，因为上一阶段已经汇报过
            ReturnState::Error(ret)
        } else {
            let error = first.span.make_error("error word after type define", None);
            self.session.report_error(error);
            ReturnState::Panic
        };
    }

    /// 该函数会处理最后的 ; 而且只能是 ;
    
    pub(crate) fn must_idents_with_init(&mut self) -> ReturnState<Vec<IdentWithInit>> {
        let mut idents_init = vec![];
        let mut is_last_comma = false;
        loop {
            let ident = self.token_parser.first_usable_token(self.session);
            match ident.kind {
                TokenKind::Ident { .. } => {
                    let ident = self.token_parser.next_usable_token(self.session);
                    let after_ident = self.token_parser.first_usable_token(self.session);
                    match after_ident.kind {
                        TokenKind::OpenBracket => {
                            let array = self.must_array_after();
                            let array = match array {
                                ReturnState::Accept(array) => array,
                                ReturnState::Error(_) => {
                                    return ReturnState::Panic;
                                }
                                ReturnState::Panic => {
                                    return ReturnState::Panic;
                                }
                            };
                            idents_init.push(IdentWithInit::Array {
                                ident: Ident::new(ident.to_ident_or_lit_symbol(), ident.span),
                                dimensions: array.dimensions,
                                init: array.init,
                            });
                            let first = self.token_parser.first_usable_token(self.session);
                            if first.is_comma() {
                                is_last_comma = true;
                                self.token_parser.next_usable_token(self.session);
                            } else if !first.is_semi() {
                                // 如果这里不是 Semi,说明肯定出问题
                                let error = first.span.make_error("unknown char", None);
                                self.session.report_error(error);
                                return ReturnState::Panic;
                            } else {
                                is_last_comma = false; // 这里的逻辑混乱
                            } // 数组声明识别
                        } // 数组识别
                        TokenKind::Eq => {
                            self.token_parser.next_token(self.session);
                            let expr = self.must_math_expr();
                            let expr = match expr {
                                ReturnState::Accept(acc) => acc,
                                ReturnState::Error(_) => {
                                    return ReturnState::Panic;
                                }
                                ReturnState::Panic => {
                                    return ReturnState::Panic;
                                }
                            };
                            idents_init.push(IdentWithInit::Var {
                                ident: Ident::new(ident.to_ident_or_lit_symbol(), ident.span),
                                init: Some(expr),
                            });
                            let first = self.token_parser.first_usable_token(self.session);
                            if first.kind == TokenKind::Comma {
                                is_last_comma = true;
                                self.token_parser.next_usable_token(self.session);
                            } else if first.is_semi() {
                                is_last_comma = false;
                            } else {
                                if first.is_any_start_keyword()||first.kind == TokenKind::CloseBrace {
                                    let error = self
                                        .token_parser
                                        .get_current_token()
                                        .span
                                        .make_error("missing ; after statement.", make_advise("add ';' after statement."));
                                    self.session.report_error(error);
                                    return ReturnState::Error(idents_init); // 强制接受
                                } else {
                                    let error = first
                                        .span
                                        .make_error("expression can't be parsed",None);
                                    self.session.report_error(error);
                                    return ReturnState::Panic;
                                }
                            }
                        } // 变量声明识别
                        TokenKind::Comma => {
                            is_last_comma = true;
                            self.token_parser.next_token(self.session);
                            let var = IdentWithInit::Var {
                                ident: Ident::new(ident.to_ident_or_lit_symbol(), ident.span),
                                init: None,
                            };
                            idents_init.push(var);
                        } // 变量定义识别
                        TokenKind::Semi => {
                            is_last_comma = false;
                            let var = IdentWithInit::Var {
                                ident: Ident::new(ident.to_ident_or_lit_symbol(), ident.span),
                                init: None,
                            };
                            idents_init.push(var);
                        } // 结束识别
                        _ => {
                            let error = after_ident.span.make_error("expect ';' , ',' ,'=' or '[' ", None);
                            self.session.report_error(error);
                            return ReturnState::Panic;
                        }
                    }
                }
                TokenKind::Semi => {
                    if is_last_comma {
                        let error = ident.span.make_error("extra comma before ';'", None);
                        self.session.report_error(error);
                        return ReturnState::Panic;
                    }
                    self.token_parser.next_usable_token(self.session);
                    return ReturnState::Accept(idents_init);
                }
                _ => {
                    let error = ident.span.make_error("only ident allowed in here", None);
                    self.session.report_error(error);
                    return ReturnState::Panic;
                }
            }
        }
    }
}

pub(crate) fn ok_or_err<T, F>(rs: ReturnState<T>, closure: impl FnOnce(T) -> Result<F, ()>) -> Result<F, ()> {
    match rs {
        ReturnState::Accept(def) => {
            closure(def)
            // 注意这里设计成
            // closure(def) 而不是 Ok(closure(def))，
            // 因为有时候就算返回Accept,也有可能需要返回Error或者Panic
        }
        ReturnState::Error(_) => Err(()),
        ReturnState::Panic => Err(()),
    }
}

pub(crate) fn acc_or_pac<T, F>(t: ReturnState<T>, closure: impl FnOnce(T) -> ReturnState<F>) -> ReturnState<F> {
    match t {
        ReturnState::Accept(acc) => closure(acc),
        ReturnState::Error(_) => ReturnState::Panic,
        ReturnState::Panic => ReturnState::Panic,
    }
}
