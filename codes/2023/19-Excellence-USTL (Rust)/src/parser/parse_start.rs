use crate::ast::{
    ArrayDefine, ArrayType, FnDefine, FnParams, Ident, IdentWithInit, RowDefines, Type, VarDefine,
};
use crate::echo::make_advise;
use crate::lexer::token::TokenKind;
use crate::parser::parse_core::{acc_or_pac, DefineOrNone, FnAfter, ReturnState, StateMachine};


impl<'a, 'b> StateMachine<'a, 'b> {
    // 这里所识别到的define必须是在函数体外部的,而且对于值定义，一定不是const的
    // 这个函数过于臃肿
    // 解决办法
    // 1.先读取一对 <type> <ident1>
    // 2.如果下一个Token为 [ 或者 = ，那么可以确定是声明表达式，这种情况，手动处理第一对值，后面的其他对采用 return here
    // 3.如果下一个Token为 ( ，可以确定是函数调用 ，return here。
    // 4.否则肯定是单变量定义，检查下一个Token 是不是 ; 如果不是则报错

    pub(crate) fn must_fn_or_define(&mut self) -> ReturnState<DefineOrNone> {
        // 虽然void关键字开头的肯定是函数，可以特殊处理，但是，void作为类型，
        // 理论上可以作为变量类型，那么 TODO void类型变量的识别任务应该交给语义分析
        let state_ret = self.must_type_ident_pair();
        let def_or_ret_type;
        let ident_tok;
        match state_ret {
            ReturnState::Accept(twi) => {
                def_or_ret_type = twi.type_tok.get_type();
                ident_tok = twi.ident;
                // 可以继续识别
            }
            ReturnState::Panic => {
                return ReturnState::Panic;
            }
            ReturnState::Error(_) => {
                return ReturnState::Panic;
            }
        }
        self.maybe_ignorable(true);
        let first = self.token_parser.first_token();
        if first.kind == TokenKind::OpenParen {
            let fn_after = self.must_fn_after();
            return match fn_after {
                ReturnState::Accept(acc) => {
                    let mut fn_def = FnDefine::new(Ident::new(
                        ident_tok.to_ident_or_lit_symbol(),
                        ident_tok.span,
                    ));
                    fn_def.ret_type = def_or_ret_type;
                    fn_def.params = acc.params;
                    fn_def.stmts = acc.stmts;
                    ReturnState::Accept(DefineOrNone::Fn(fn_def))
                }
                ReturnState::Error(_) => ReturnState::Panic,
                ReturnState::Panic => ReturnState::Panic,
            };
        } else if first.kind == TokenKind::Eq {
            self.token_parser.next_usable_token(self.session);
            let first = self.token_parser.first_usable_token(self.session);
            return if first.is_in_expr_first() {
                let expr = self.must_math_expr();
                let init_expr = match expr {
                    ReturnState::Accept(expr) => expr,
                    ReturnState::Error(_) => {
                        return ReturnState::Panic;
                    }
                    ReturnState::Panic => {
                        return ReturnState::Panic;
                    }
                };
                let first = self.token_parser.first_usable_token(self.session);
                if first.is_semi() {
                    self.token_parser.next_usable_token(self.session); // ;
                    return ReturnState::Accept(DefineOrNone::Var(VarDefine {
                        is_const: false,
                        define_type: def_or_ret_type,
                        ident: Ident::new(ident_tok.to_ident_or_lit_symbol(), ident_tok.span),
                        with_value: Some(init_expr),
                    }));
                }
                if !first.is_comma() {
                    let error = first
                        .span
                        .make_error("unknown expression , expected ',' or ';'", None);
                    self.session.report_error(error);
                    return ReturnState::Panic;
                }
                self.token_parser.next_usable_token(self.session); // ,
                let idents_with_init = self.must_idents_with_init();
                let mut idents_with_init_all = vec![];
                idents_with_init_all.push(IdentWithInit::Var {
                    ident: Ident::new(ident_tok.to_ident_or_lit_symbol(), ident_tok.span),
                    init: Some(init_expr),
                });

                let mut idents_with_init = match idents_with_init {
                    ReturnState::Accept(acc) => acc,
                    ReturnState::Error(_) => {
                        return ReturnState::Panic;
                    }
                    ReturnState::Panic => {
                        return ReturnState::Panic;
                    }
                };
                // 这里可以优化
                idents_with_init_all.append(&mut idents_with_init);
                let row_defines = DefineOrNone::RowDefines(RowDefines {
                    is_const: false,
                    define_type: def_or_ret_type,
                    defs: idents_with_init_all,
                });
                ReturnState::Accept(row_defines)
            } else {
                let error = first.span.make_error("unknown expression", None);
                self.session.report_error(error);
                ReturnState::Panic
            };
        } else if first.kind == TokenKind::Comma {
            self.token_parser.next_usable_token(self.session);
            let first_var = IdentWithInit::Var {
                ident: Ident::new(ident_tok.to_ident_or_lit_symbol(), ident_tok.span),
                init: None,
            };
            let idents_with_init = self.must_idents_with_init();
            let mut idents_with_init_all = vec![];
            idents_with_init_all.push(first_var);
            let mut idents_with_init = match idents_with_init {
                ReturnState::Accept(acc) => acc,
                ReturnState::Error(_) => {
                    return ReturnState::Panic;
                }
                ReturnState::Panic => {
                    return ReturnState::Panic;
                }
            };
            // 这里可以优化
            idents_with_init_all.append(&mut idents_with_init);
            let row_defines = DefineOrNone::RowDefines(RowDefines {
                is_const: false,
                define_type: def_or_ret_type,
                defs: idents_with_init_all,
            });
            return ReturnState::Accept(row_defines);
        } else if first.kind == TokenKind::OpenBracket {
            let array_after = self.must_array_after();
            let array = match array_after {
                ReturnState::Accept(arr) => arr,
                ReturnState::Error(_) => {
                    return ReturnState::Panic;
                }
                ReturnState::Panic => {
                    return ReturnState::Panic;
                }
            };
            let first = self.token_parser.first_usable_token(self.session);
            if first.is_semi() {
                self.token_parser.next_usable_token(self.session); // ;
                return ReturnState::Accept(DefineOrNone::Array(ArrayDefine {
                    is_const: false,
                    define_type: def_or_ret_type,
                    ident: Ident::new(ident_tok.to_ident_or_lit_symbol(), ident_tok.span),
                    dimensions: array.dimensions,
                    init: array.init,
                }));
            }
            if !first.is_comma() {
                // the token is neither semi nor comma , so we return panic.
                let error = first.span.make_error(
                    "expected ';' or ',' ",
                    make_advise("try to check whether a chinese char is in here."),
                );
                self.session.report_error(error);
                return ReturnState::Panic;
            }
            // RowDefines
            self.token_parser.next_usable_token(self.session);
            let idents_with_init = self.must_idents_with_init();
            let mut idents_with_init = match idents_with_init {
                ReturnState::Accept(acc) => acc,
                ReturnState::Error(_) => {
                    return ReturnState::Panic;
                }
                ReturnState::Panic => {
                    return ReturnState::Panic;
                }
            };
            let mut idents_with_init_all = vec![];
            idents_with_init_all.push(IdentWithInit::Array {
                ident: Ident::new(ident_tok.to_ident_or_lit_symbol(), ident_tok.span),
                dimensions: array.dimensions,
                init: array.init,
            });
            idents_with_init_all.append(&mut idents_with_init);
            return ReturnState::Accept(DefineOrNone::RowDefines(RowDefines {
                is_const: false,
                define_type: def_or_ret_type,
                defs: idents_with_init_all,
            }));
            // todo ????? 脑子里一团浆糊， 2023年5月13日
        } else if first.kind == TokenKind::Semi {
            self.token_parser.next_token(self.session);
            let val_def = VarDefine {
                is_const: false,
                define_type: def_or_ret_type,
                ident: Ident::new(ident_tok.to_ident_or_lit_symbol(), ident_tok.span),
                with_value: None,
            };
            // 全局变量完成识别
            return ReturnState::Accept(DefineOrNone::Var(val_def));
        } else {
            // 不是函数也不是全局变量 既不是 = , [ 也不是 (
            // { ; =
            let error = first.span.make_error(
                "An expression or definition that is not recognized",
                make_advise("Try replacing it with a function or variable definition"),
            );
            self.session.report_error(error);
            return ReturnState::Panic;
        }
    }

    // 识别到 (
    pub(crate) fn must_const_define(&mut self) -> ReturnState<RowDefines> {
        let ret = self.must_local_const();
        return acc_or_pac(ret, |row_def| ReturnState::Accept(row_def));
    }

    /// 识别到(
    fn must_fn_after(&mut self) -> ReturnState<FnAfter> {
        self.token_parser.next_usable_token(self.session); // 弹出(
        let mut fn_after;
        fn_after = FnAfter::new();
        let first_tok = self.token_parser.first_usable_token(self.session);
        if first_tok.is_type()
        // todo 语义检查点
        {
            let params = self.must_params();
            match params {
                ReturnState::Accept(param) => fn_after.set_params(param),
                ReturnState::Panic => {
                    return ReturnState::Panic;
                }
                ReturnState::Error(_) => {
                    return ReturnState::Panic;
                }
            }
        } else if first_tok.kind == TokenKind::CloseParen {
            fn_after.params = None;
            self.token_parser.next_usable_token(self.session);
        } else {
            // ）符号缺失 , 或者关键字异常
            // { 处可恢复
            self.session
                .report_error(first_tok.span.make_error("missing ')'", None));
            return ReturnState::Panic;
        }

        let enter_semi = self.token_parser.first_usable_token(self.session);
        if enter_semi.kind != TokenKind::OpenBrace {
            // 缺少 {
            let error = enter_semi.span.make_error("missing '{' ", None);
            self.session.report_error(error);
            return ReturnState::Panic;
        }
        // 进入函数体
        let exprs = self.maybe_exprs();
        return match exprs {
            ReturnState::Accept(acc) => {
                fn_after.stmts = acc;
                ReturnState::Accept(fn_after)
            }
            ReturnState::Error(_) => ReturnState::Panic,
            ReturnState::Panic => ReturnState::Panic,
        };
    }

    /// 识别到(
    fn must_params(&mut self) -> ReturnState<FnParams> {
        // let first = self.token_parser.next_token(self.session); // (
        // println!("{:?}",first);
        let mut fn_params = FnParams::new();
        loop {
            let type_tok = self.token_parser.next_usable_token(self.session);
            if !type_tok.is_type() {
                // todo 语意检查点
                // 不是type token出现在这里
                //  ）type_token ,
                let error = type_tok.span.make_error("only type keyword allowed", None);
                self.session.report_error(error);
                return ReturnState::Panic;
            }
            match self.must_ignore_more() {
                ReturnState::Accept(_is_more) => {}
                ReturnState::Panic => {
                    // 间隔token没有出现
                    //  ） type_token ,
                    let tok = self.token_parser.first_token();
                    let error = tok.span.make_error("missing whitespace", None);
                    self.session.report_error(error);
                    return ReturnState::Panic;
                }
                _ => {
                    panic!("never here!")
                }
            }
            let ident_tok = self.token_parser.next_usable_token(self.session);
            if !ident_tok.is_ident() {
                // 形参名称没有出现
                //  ） type_token ,
                let error = ident_tok.span.make_error("ident expected", None);
                self.session.report_error(error);
                return ReturnState::Panic;
            }
            let basic_type = type_tok.get_type();

            self.maybe_ignorable(true);
            let mut first = self.token_parser.first_usable_token(self.session);
            if first.kind == TokenKind::OpenBracket {
                // 传递了数组
                let mut dimensions = vec![];
                loop {
                    let first = self.token_parser.first_usable_token(self.session);
                    match first.kind {
                        TokenKind::OpenBracket => {
                            self.token_parser.next_usable_token(self.session);
                            let ret = self.must_math_expr();
                            match ret {
                                ReturnState::Accept(expr) => {
                                    dimensions.push(expr);
                                }
                                ReturnState::Error(_) => {
                                    return ReturnState::Panic;
                                }
                                ReturnState::Panic => {
                                    return ReturnState::Panic;
                                }
                            }
                            let ret = self.token_parser.first_usable_token(self.session);
                            if ret.kind != TokenKind::CloseBracket {
                                let error = ret.span.make_error("array type error", None);
                                self.session.report_error(error);
                                return ReturnState::Panic;
                            }
                            self.token_parser.next_usable_token(self.session);
                        }
                        _ => {
                            break;
                        }
                    }
                }
                let var_type = Type::Array(ArrayType {
                    basic_type,
                    dimensions,
                });
                let ident = Ident::new(ident_tok.to_ident_or_lit_symbol(), ident_tok.span);
                fn_params.add(var_type, ident);
                first = self.token_parser.first_usable_token(self.session);
            } else {
                // 单变量
                let ident = Ident::new(ident_tok.to_ident_or_lit_symbol(), ident_tok.span);
                fn_params.add(Type::Basic(basic_type), ident);
            }
            match first.kind {
                TokenKind::Comma => {
                    self.token_parser.next_usable_token(self.session);
                }
                TokenKind::CloseParen => {
                    self.token_parser.next_usable_token(self.session);
                    break;
                }
                _ => {
                    // 既不是 ， 也不是 ）
                    //  ） type_token ,
                    let error = first.span.make_error("error syntax", None);
                    self.session.report_error(error);
                    return ReturnState::Panic;
                }
            }
        }
        ReturnState::Accept(fn_params)
    }
}
