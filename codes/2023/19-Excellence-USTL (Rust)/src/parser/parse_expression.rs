use crate::allow_nothing;
use crate::ast::{
    ArrayDefine, Assign, BoolExprs, Branch, Branches, IdentWithInit, Invoke, LVarRef, MathExpr,
    RelExpr, Return, RowDefines, Stmt, Stmts, UnaryExp, VarDefine, While,
};
use crate::echo::make_advise;
use crate::lexer::token::{Keyword, TokenKind};
use crate::parser::parse_core::{acc_or_pac, ReturnState, StateMachine};


pub(crate) enum AssignOrExpr {
    Assign(Assign),
    Expr(BoolExprs),
}

impl<'a, 'b> StateMachine<'a, 'b> {
    /// # 介绍
    /// 开始确定符号{和结束确定符号}都会被吃掉，所以必须预留开始确定符号
    
    pub(crate) fn maybe_exprs(&mut self) -> ReturnState<Stmts> {
        // 这里可以加入maybe_expr函数
        self.token_parser.next_token(self.session); // {
        let mut stats = Stmts::new();
        loop {
            let tok = self.token_parser.first_usable_token(self.session);
            // ParserReader::print_token(tok, "now start tok");
            if tok.kind == TokenKind::CloseBrace {
                self.token_parser.next_usable_token(self.session);
                return ReturnState::Accept(stats);
            } else if tok.kind == TokenKind::CloseBrace {
                self.token_parser.next_usable_token(self.session);
                return ReturnState::Accept(stats);
            }
            let ret = self.must_expr();
            match ret {
                ReturnState::Accept(stat) => match stat {
                    Stmt::None => {
                        allow_nothing!();
                    }
                    _ => {
                        stats.push(stat);
                    }
                },
                ReturnState::Error(_) => {
                    return ReturnState::Panic;
                }
                ReturnState::Panic => {
                    return ReturnState::Panic;
                }
            }
        }
    }

    /// # 支持空语句识别
    /// 会吃掉最后的控制符 ;
    
    fn must_expr(&mut self) -> ReturnState<Stmt> {
        loop {
            let first = self.token_parser.first_usable_token(self.session);
            match &first.kind {
                TokenKind::Comment => {
                    self.token_parser.next_token(self.session);
                    continue;
                }
                TokenKind::Whitespace => {
                    panic!("never here")
                }
                TokenKind::Ident { .. } => {
                    // 这里实际上可以把Invoke单独出来，因为Invoke的单独出现的场景较为常见
                    let ret = self.must_assign_or_expr();
                    return acc_or_pac(ret, |ass| {
                        match ass {
                            AssignOrExpr::Assign(ass) => ReturnState::Accept(Stmt::Assign(ass)),
                            AssignOrExpr::Expr(expr) => {
                                // 这里加入检查
                                let ret = expr.is_invoke();
                                match ret {
                                    Ok(invoke) => ReturnState::Accept(Stmt::Invoke(invoke)),
                                    Err(_) => ReturnState::Accept(Stmt::Expr(expr)),
                                }
                            }
                        }
                    });
                }
                TokenKind::Keyword(key) => {
                    return match key {
                        Keyword::If => {
                            let branches = self.must_branches();
                            acc_or_pac(branches, |acc| ReturnState::Accept(Stmt::IfElse(acc)))
                        }
                        Keyword::Else => {
                            // 错误情况,else 分支会由must_if识别，这里不负责
                            self.session.report_error(first.span.make_error(
                                "unknown else branch",
                                make_advise(
                                    "remove the else branch , symbols or add if before here",
                                ),
                            ));
                            ReturnState::Panic
                        }
                        Keyword::While => {
                            let while_loop = self.must_while();
                            acc_or_pac(while_loop, |acc| ReturnState::Accept(Stmt::While(acc)))
                        }
                        Keyword::Int | Keyword::Float => {
                            let define = self.must_defines();
                            // 已优化，单定义，直接解开
                            acc_or_pac(define, |mut def| {
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
                                            return ReturnState::Accept(Stmt::VarDefine(
                                                var_define,
                                            ));
                                        }
                                        IdentWithInit::Array {
                                            ident,
                                            dimensions,
                                            init,
                                        } => {
                                            let array_define = ArrayDefine {
                                                is_const: false,
                                                define_type: def.define_type,
                                                ident,
                                                dimensions,
                                                init,
                                            };
                                            return ReturnState::Accept(Stmt::ArrayDefine(
                                                array_define,
                                            ));
                                        }
                                    }
                                }

                                ReturnState::Accept(Stmt::RowDefines(def))
                            })
                        }
                        Keyword::Void => {
                            // todo miss error position message
                            self.session.report_error(first.span.make_error(
                                "void can be type declare",
                                make_advise("use int or float"),
                            ));
                            ReturnState::Panic
                        }
                        Keyword::Const => {
                            // 允许
                            let define = self.must_local_const();
                            acc_or_pac(define, |mut def| {
                                if def.defs.len() == 1 {
                                    let first = def.defs.pop().unwrap();
                                    match first {
                                        IdentWithInit::Var { ident, init } => {
                                            let var_define = VarDefine {
                                                is_const: true,
                                                define_type: def.define_type,
                                                ident,
                                                with_value: init,
                                            };
                                            return ReturnState::Accept(Stmt::VarDefine(
                                                var_define,
                                            ));
                                        }
                                        IdentWithInit::Array {
                                            ident,
                                            dimensions,
                                            init,
                                        } => {
                                            let array_define = ArrayDefine {
                                                is_const: true,
                                                define_type: def.define_type,
                                                ident,
                                                dimensions,
                                                init,
                                            };
                                            return ReturnState::Accept(Stmt::ArrayDefine(
                                                array_define,
                                            ));
                                        }
                                    }
                                }
                                def.is_const = true;
                                ReturnState::Accept(Stmt::RowDefines(def))
                            })
                        }
                        Keyword::Break => {
                            // todo break 语义检查点
                            // 允许，合法检查放在语义分析阶段
                            self.token_parser.next_token(self.session);
                            let may_be_semi = self.token_parser.next_usable_token(self.session);
                            if may_be_semi.kind == TokenKind::Semi {
                                ReturnState::Accept(Stmt::Break)
                            } else {
                                let error = may_be_semi.span.make_error(
                                    "missing ';' after break",
                                    make_advise("use break;"),
                                );
                                self.session.report_error(error);
                                ReturnState::Panic
                            }
                        }
                        Keyword::Return => {
                            // todo return 语义检查点
                            // 允许，合法检查放在语义分析阶段
                            self.token_parser.next_token(self.session);
                            let ignore = match self.maybe_ignorable(true) {
                                ReturnState::Accept(_) => true,
                                ReturnState::Error(_) => false,
                                ReturnState::Panic => false,
                            };
                            let may_be_semi = self.token_parser.first_usable_token(self.session);
                            if may_be_semi.kind == TokenKind::Semi {
                                self.token_parser.next_usable_token(self.session);
                                return ReturnState::Accept(Stmt::Return(Return {
                                    return_value: None,
                                }));
                            } else if may_be_semi.is_in_expr_first() {
                                let ret_value = self.must_math_expr();
                                let add_expr = match ret_value {
                                    ReturnState::Accept(expr) => expr,
                                    ReturnState::Error(_) => {
                                        return ReturnState::Panic;
                                    }
                                    ReturnState::Panic => {
                                        return ReturnState::Panic;
                                    }
                                };
                                let first = self.token_parser.first_usable_token(self.session);
                                if !first.is_semi() {
                                    let error = first.span.make_error(
                                        "missing ';' after return",
                                        make_advise("use return;"),
                                    );
                                    self.session.report_error(error);
                                    return ReturnState::Panic;
                                }
                                self.token_parser.next_usable_token(self.session); // ;
                                if !ignore {
                                    return ReturnState::Panic;
                                }
                                return ReturnState::Accept(Stmt::Return(Return {
                                    return_value: Some(add_expr),
                                }));
                            } else {
                                let error = may_be_semi.span.make_error(
                                    "missing ';' after return",
                                    make_advise("use return;"),
                                );
                                self.session.report_error(error);
                                ReturnState::Panic
                            }
                        }
                        Keyword::Continue => {
                            // todo 语义检查点
                            // 允许，合法检查放在语义分析阶段
                            self.token_parser.next_token(self.session);
                            let may_be_semi = self.token_parser.next_usable_token(self.session);
                            if may_be_semi.kind == TokenKind::Semi {
                                ReturnState::Accept(Stmt::Continue)
                            } else {
                                let error = may_be_semi.span.make_error(
                                    "missing ';' , after continue ",
                                    make_advise("use continue;"),
                                );
                                self.session.report_error(error);
                                ReturnState::Panic
                            }
                        }
                    };
                }
                // 这里是表达式的FIRST集子集,
                // 对于Ident，因为存在Assign的情况，特殊处理
                // todo Bang ???
                TokenKind::OpenParen
                | TokenKind::Plus
                | TokenKind::Minus
                | TokenKind::Literal { .. } => {
                    // 属于无赋值表达式，这个出现的情况只能是落单了
                    // 但是如果包含函数调用，那么就不能轻易忽略，如果函数调用不会影响全部变量，那么可以忽略
                    let expr = self.must_bool_expr();
                    return acc_or_pac(expr, |expr| {
                        // 注意must_math_expr不处理任何分界符号或者定位符号
                        let semi = self.token_parser.first_usable_token(self.session);
                        match semi.kind == TokenKind::Semi {
                            true => {
                                self.token_parser.next_usable_token(self.session);
                                ReturnState::Accept(Stmt::Expr(expr))
                            }
                            false => {
                                let error =
                                    semi.span.make_error("Expected ';' after expression", None);
                                self.session.report_error(error);
                                ReturnState::Panic
                            }
                        }
                    });
                }
                TokenKind::Semi => {
                    let tok = self.token_parser.next_usable_token(self.session);
                    let warning = tok.span.make_warning("empty Statement", None);
                    self.session.report_warning(warning);
                    return ReturnState::Accept(Stmt::None);
                }
                TokenKind::OpenBrace => {
                    let exprs = self.maybe_exprs();
                    return acc_or_pac(exprs, |stmts| {
                        let tok = self.token_parser.first_usable_token(self.session);
                        if tok.is_semi() {
                            self.token_parser.next_usable_token(self.session);
                        }
                        ReturnState::Accept(Stmt::Block(stmts))
                    });
                }
                TokenKind::CloseBrace => {
                    // 注意must_expr函数不允许遇到 } ，如果遇到那就是有额外的 }
                    let error = first.span.make_error("syntax error , extra '}'", None);
                    self.session.report_error(error);
                    return ReturnState::Panic;
                }
                TokenKind::CloseParen
                | TokenKind::OpenBracket
                | TokenKind::CloseBracket
                | TokenKind::Dot
                | TokenKind::Comma
                | TokenKind::Eq
                | TokenKind::DouEq
                | TokenKind::NotEq
                | TokenKind::Bang
                | TokenKind::Lt
                | TokenKind::Le
                | TokenKind::Gt
                | TokenKind::Ge
                | TokenKind::And
                | TokenKind::Or
                | TokenKind::Star
                | TokenKind::Slash
                | TokenKind::Percent
                | TokenKind::DouAnd
                | TokenKind::DouOr => {
                    let error = first.span.make_error("syntax error", None);
                    self.session.report_error(error);
                    return ReturnState::Panic;
                }
                TokenKind::Illegal { .. } => {
                    // 错误
                    let error = first.span.make_error("unknown stmt", None);
                    self.session.report_error(error);
                    return ReturnState::Panic;
                }
                TokenKind::Eof => {
                    // 注意这里出现这个符号很危险，意味着很可能，提前结束了函数体
                    let error = first
                        .span
                        .make_error("source code error ,compiler meets EOF in function", None);
                    self.session.report_error(error);
                    return ReturnState::Panic;
                }
            }
        }
    }

    // 这里的must_define仅仅来自block内部,所以不可能出现函数
    /// # 识别到类型，类型必须经过检查
    //  must_defines具有歧义，这里有可能是单变量定义或者行变量定义
    
    fn must_defines(&mut self) -> ReturnState<RowDefines> {
        let type_tok = self.token_parser.next_usable_token(self.session);
        let defs = self.must_idents_with_init();
        let defs = match defs {
            ReturnState::Accept(acc) => acc,
            ReturnState::Error(error_acc) => error_acc,
            ReturnState::Panic => {
                return ReturnState::Panic;
            }
        };
        let row_defs = RowDefines {
            is_const: false,
            define_type: type_tok.get_type(),
            defs,
        };
        ReturnState::Accept(row_defs)
    }

    
    pub(crate) fn must_local_const(&mut self) -> ReturnState<RowDefines> {
        self.token_parser.next_usable_token(self.session); // const

        let first = self.token_parser.first_usable_token(self.session);
        if !first.is_not_void_type() {
            if first.is_type() {
                let error = first.span.make_error(
                    "void type is not supported to be as variable or array",
                    None,
                );
                self.session.report_error(error);
            } else {
                let error = first.span.make_error("define stmt is error", None);
                self.session.report_error(error);
            }
            return ReturnState::Panic;
        }

        let local_define = self.must_defines();
        return match local_define {
            ReturnState::Accept(mut define) => {
                define.is_const = true;
                ReturnState::Accept(define)
            }
            ReturnState::Error(_) => ReturnState::Panic,
            ReturnState::Panic => ReturnState::Panic,
        };
    }

    
    fn must_while(&mut self) -> ReturnState<While> {
        let if_branch = self.must_if();
        return match if_branch {
            ReturnState::Accept(acc) => {
                let while_loop = While {
                    condition: acc.condition.unwrap(),
                    stats: acc.stats,
                };
                ReturnState::Accept(while_loop)
            }
            ReturnState::Error(_) => ReturnState::Panic,
            ReturnState::Panic => ReturnState::Panic,
        };
    }

    
    fn must_branches(&mut self) -> ReturnState<Branches> {
        let mut branches = Branches { ifs: vec![] };
        let first_if = self.must_if();
        match first_if {
            ReturnState::Accept(now_branch) => {
                branches.ifs.push(now_branch);
            }
            ReturnState::Error(_) => {
                return ReturnState::Panic;
            }
            ReturnState::Panic => {
                return ReturnState::Panic;
            }
        }

        loop {
            let maybe_else = self.token_parser.first_usable_token(self.session);
            match maybe_else.kind {
                TokenKind::Keyword(ke) => {
                    match ke == Keyword::Else {
                        true => {
                            self.token_parser.next_usable_token(self.session);
                            let maybe_else_if = self.token_parser.first_usable_token(self.session);

                            match maybe_else_if.kind {
                                TokenKind::OpenBrace => {
                                    let exprs = self.maybe_exprs();
                                    return acc_or_pac(exprs, |stats| {
                                        let branch = Branch {
                                            condition: None,
                                            stats,
                                        };
                                        branches.ifs.push(branch);
                                        ReturnState::Accept(branches)
                                    });
                                }
                                TokenKind::Keyword(key) => {
                                    match key {
                                        Keyword::If => {
                                            let new_branch = self.must_if();
                                            match new_branch {
                                                ReturnState::Accept(now_branch) => {
                                                    branches.ifs.push(now_branch);
                                                }
                                                ReturnState::Error(_) => {
                                                    return ReturnState::Panic;
                                                }
                                                ReturnState::Panic => {
                                                    return ReturnState::Panic;
                                                }
                                            }
                                            continue;
                                        }
                                        _ => {
                                            // keyword after else can be
                                            // else if ,else break ,else continue , else <expr> , else while() ,
                                            // TODO 这里可以缩小范围，else之后允许的字符是确定的
                                            let warn = maybe_else_if
                                                .span
                                                .make_warning("no { } found after else", None);
                                            self.session.report_warning(warn);
                                            let expr = self.must_expr();
                                            return acc_or_pac(expr, |expr| {
                                                branches.ifs.push(Branch {
                                                    condition: None,
                                                    stats: vec![expr],
                                                });
                                                ReturnState::Accept(branches)
                                            });
                                        }
                                    }
                                }
                                _ => {
                                    // TODO 这里可以缩小范围，else之后允许的字符是确定的
                                    // else 之后没出现关键字
                                    // 错误
                                    let warn = maybe_else_if
                                        .span
                                        .make_warning("no { } found after else", None);
                                    self.session.report_warning(warn);
                                    let expr = self.must_expr();
                                    return acc_or_pac(expr, |expr| {
                                        branches.ifs.push(Branch {
                                            condition: None,
                                            stats: vec![expr],
                                        });
                                        ReturnState::Accept(branches)
                                    });
                                }
                            } // 出现了else if 分支
                        }
                        false => {
                            // 没有else分支
                            return ReturnState::Accept(branches);
                            // 结束
                        }
                    }
                }
                _ => {
                    return ReturnState::Accept(branches);
                    // 没有else分支
                    // 结束
                }
            }
        }
    }

    /// 赋值或者空表达式语句
    // 解决办法
    // 1.先进行一遍表达式识别 令 a = <this expr>,
    // 2.如果next_token == TK::semi , 则是空表达式；
    // 3.如果next_token == TK::Eq , 则Assign，这种情况下需要检查 a 执行子过程 must_math_expr()，
    // Sysy不支持动态的数组大小，无需担心数组重新赋值
    
    fn must_assign_or_expr(&mut self) -> ReturnState<AssignOrExpr> {
        let left_expr = self.must_bool_expr();
        let eq_or_paren = self.token_parser.first_usable_token(self.session);
        match eq_or_paren.kind {
            TokenKind::Eq => {
                self.token_parser.next_usable_token(self.session);
                return match left_expr {
                    ReturnState::Accept(acc) => {
                        let lvar = acc.into_left_val_ref();
                        match lvar {
                            Ok(var) => match var {
                                LVarRef::VarRef(var) => {
                                    let right = self.must_math_expr();
                                    acc_or_pac(right, |expr| {
                                        let semi =
                                            self.token_parser.first_usable_token(self.session);
                                        if semi.kind == TokenKind::Semi {
                                            self.token_parser.next_usable_token(self.session);
                                        } else {
                                            let error = self.token_parser.get_current_token().span.make_error("missing ';'", None);
                                            self.session.report_error(error);
                                            return ReturnState::Panic;
                                        }
                                        ReturnState::Accept(AssignOrExpr::Assign(Assign {
                                            from: expr,
                                            to: LVarRef::VarRef(var),
                                        }))
                                    })
                                }
                                LVarRef::ArrayRef(array_ref) => {
                                    let right = self.must_math_expr();
                                    acc_or_pac(right, |expr| {
                                        let semi =
                                            self.token_parser.first_usable_token(self.session);
                                        if semi.kind == TokenKind::Semi {
                                            self.token_parser.next_usable_token(self.session);
                                        } else {
                                            let error = semi.span.make_error("missing ';'", None);
                                            self.session.report_error(error);
                                            return ReturnState::Panic;
                                        }
                                        ReturnState::Accept(AssignOrExpr::Assign(Assign {
                                            from: expr,
                                            to: LVarRef::ArrayRef(array_ref),
                                        }))
                                    })
                                }
                            },
                            Err(_) => {
                                // <unknown> =
                                ReturnState::Panic
                            }
                        }
                    }
                    ReturnState::Error(_) => {
                        // <unknown> =
                        ReturnState::Panic
                    }
                    ReturnState::Panic => {
                        // <unknown> =
                        ReturnState::Panic
                    }
                };
            }
            TokenKind::Semi => {
                // todo 语义检查点，空表达式检测
                self.token_parser.next_usable_token(self.session);
                // 空表达式
                return match left_expr {
                    ReturnState::Accept(expr) => ReturnState::Accept(AssignOrExpr::Expr(expr)),
                    ReturnState::Error(_) => ReturnState::Panic,
                    ReturnState::Panic => ReturnState::Panic,
                };
            }
            _ => {
                return ReturnState::Panic;
            }
        }
    }

    
    fn must_if(&mut self) -> ReturnState<Branch> {
        self.token_parser.next_usable_token(self.session); // if
        let tok = self.token_parser.first_usable_token(self.session);
        if tok.kind != TokenKind::OpenParen {
            return ReturnState::Panic;
        } else {
            self.token_parser.next_usable_token(self.session);
        }
        let expr = self.must_bool_expr();
        let mut now_branch = Branch {
            condition: None,
            stats: vec![],
        };

        match expr {
            ReturnState::Accept(cond) => {
                now_branch.condition = Some(cond);
            }
            ReturnState::Error(_) => {
                return ReturnState::Panic;
            }
            ReturnState::Panic => {
                return ReturnState::Panic;
            }
        };
        let paren = self.token_parser.first_usable_token(self.session);
        match paren.kind == TokenKind::CloseParen {
            true => {
                self.token_parser.next_usable_token(self.session);
            }
            false => {
                self.session.report_error(paren.span.make_error(
                    "if branch syntax error",
                    make_advise("Try adding ')' to correct the syntax"),
                ));
                return ReturnState::Panic;
            }
        }
        let paren_or_expr = self.token_parser.first_usable_token(self.session);
        return match paren_or_expr.kind {
            TokenKind::OpenBrace => {
                let exprs = self.maybe_exprs();
                acc_or_pac(exprs, |stats| {
                    now_branch.stats = stats;
                    ReturnState::Accept(now_branch)
                })
            }
            _ => {
                let expr = self.must_expr();
                acc_or_pac(expr, |stmt| {
                    now_branch.stats.push(stmt);
                    ReturnState::Accept(now_branch)
                })
            }
        };
    }
}

impl BoolExprs {
    fn is_invoke(&self) -> Result<Invoke, ()> {
        match self {
            BoolExprs::And(_, _) => {
                return Err(());
            }
            BoolExprs::Or(_, _) => {
                return Err(());
            }
            BoolExprs::RelExpr(math) => match math {
                RelExpr::Math(math) => match math {
                    MathExpr::Unary(expr) => match expr {
                        UnaryExp::Invoke(_, invoke) => {
                            let invoke = invoke.clone();
                            return Ok(invoke);
                        }
                        _ => {
                            return Err(());
                        }
                    },
                    MathExpr::Expr(_, _, _) => {
                        return Err(());
                    }
                },
                RelExpr::Rel(_, _, _) => {
                    return Err(());
                }
            },
        }
    }
}
