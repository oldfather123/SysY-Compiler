use crate::ast::{
    AddExpr, ArrayInit, ArrayRef, BoolCondition, BoolExprs, Ident, Invoke, Literal, MathExpr,
    MathOpe, RelExpr, RelOpe, UnaryExp, UnaryOp,
};
use crate::lexer::token::{Token, TokenKind};
use crate::parser::parse_core::{ok_or_err, ReturnState, StateMachine};


// 右值处理


enum InvokeOrRef {
    Invoke(Invoke),
    VarRef(Token),
    ArrayRef(ArrayRef),
}


pub(crate) struct ArrayAfter {
    pub dimensions: Vec<AddExpr>,
    pub init: Option<ArrayInit>,
}

impl<'a, 'b> StateMachine<'a, 'b> {
    /// 该函数不处理最后的停止符号

    pub(crate) fn must_math_expr(&mut self) -> ReturnState<AddExpr> {
        let expr = self.must_add_expr();
        return match expr {
            Ok(expr) => {
                // println!("new expr = {}", expr.to_string());
                ReturnState::Accept(expr)
            }
            Err(_) => ReturnState::Panic,
        };
    }


    pub(crate) fn must_bool_expr(&mut self) -> ReturnState<BoolCondition> {
        let bool_expr = self.must_or_expr();

        return match bool_expr {
            Ok(expr) => {
                // println!("new bool expr = {}", expr.to_string());
                ReturnState::Accept(expr)
            }
            Err(_) => ReturnState::Panic,
        };
    }

    /// a function or a reference to a var or array element
    /// 已支持数组

    fn must_invoke_or_ref(&mut self) -> ReturnState<InvokeOrRef> {
        let ident = self.token_parser.next_usable_token(self.session);
        let eq_paren_or_bracket = self.token_parser.first_usable_token(self.session);
        match eq_paren_or_bracket.kind {
            // 函数调用
            TokenKind::OpenParen => {
                self.token_parser.next_usable_token(self.session);
                let tok = self.token_parser.first_usable_token(self.session);
                let mut invoke = Invoke {
                    fun_name: Ident::new(ident.to_ident_or_lit_symbol(), ident.span),
                    params: vec![],
                    line_at:ident.line_at
                };
                if tok.kind == TokenKind::CloseParen {
                    self.token_parser.next_usable_token(self.session);
                    return ReturnState::Accept(InvokeOrRef::Invoke(invoke));
                }
                let mut last_is_comma = true;
                loop {
                    let tok = self.token_parser.first_usable_token(self.session);
                    match &tok.kind {
                        TokenKind::CloseParen => {
                            self.token_parser.next_usable_token(self.session);
                            if last_is_comma {
                                // invoke_name(int a,) 这种情况
                                return ReturnState::Panic;
                            }
                            return ReturnState::Accept(InvokeOrRef::Invoke(invoke));
                        }
                        //  这里是表达式的First集
                        TokenKind::Ident { .. }
                        | TokenKind::OpenParen
                        | TokenKind::Plus
                        | TokenKind::Minus
                        | TokenKind::Literal { .. } => {
                            if last_is_comma {
                                let ident = self.must_math_expr();
                                match ident {
                                    ReturnState::Accept(expr) => {
                                        invoke.params.push(expr);
                                    }
                                    ReturnState::Error(_) => {
                                        return ReturnState::Panic;
                                    }
                                    ReturnState::Panic => {
                                        return ReturnState::Panic;
                                    }
                                }

                                let tok = self.token_parser.first_usable_token(self.session);
                                if tok.kind == TokenKind::Comma {
                                    self.token_parser.next_usable_token(self.session);
                                    last_is_comma = true;
                                } else {
                                    last_is_comma = false;
                                }
                            } else {
                                // invoke_name(int a<>int b)
                                return ReturnState::Panic;
                            }
                        }
                        _ => {
                            // invoke_name(int a<unknown>)
                            return ReturnState::Panic;
                        }
                    }
                }
            }
            // 引用数组情况
            TokenKind::OpenBracket => {
                // 为了不失去普遍性，这里不弹出 [
                let mut array_ref = ArrayRef {
                    array_name: Ident::new(ident.to_ident_or_lit_symbol(), ident.span),
                    pos: vec![],
                };
                loop {
                    let first = self.token_parser.first_usable_token(self.session);
                    match first.kind {
                        TokenKind::OpenBracket => {
                            self.token_parser.next_usable_token(self.session);
                            let ret = self.must_math_expr();
                            match ret {
                                ReturnState::Accept(expr) => {
                                    array_ref.pos.push(expr);
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
                                let error = ret.span.make_error("reference to array error", None);
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
                return ReturnState::Accept(InvokeOrRef::ArrayRef(array_ref));
            }
            // 单独的左值
            _ => {
                return ReturnState::Accept(InvokeOrRef::VarRef(ident));
            }
        }
    }

    fn must_or_expr(&mut self) -> Result<BoolExprs, ()> {
        let mul_expr = self.must_and_expr();
        let mut left = match mul_expr {
            Ok(expr) => expr,
            Err(_) => {
                return Err(());
            }
        };
        loop {
            let first = self.token_parser.first_usable_token(self.session);
            match first.kind {
                TokenKind::DouOr => {
                    self.token_parser.next_usable_token(self.session);
                }
                _ => {
                    return Ok(left);
                }
            };
            let mul_expr = self.must_and_expr();
            let right = match mul_expr {
                Ok(unary) => unary,
                Err(_) => {
                    return Err(());
                }
            };
            left = BoolExprs::Or(Box::new(left), Box::new(right));
        }
    }

    fn must_and_expr(&mut self) -> Result<BoolExprs, ()> {
        let mul_expr = self.must_eq_expr();
        let mut left = match mul_expr {
            Ok(expr) => BoolExprs::RelExpr(expr),
            Err(_) => {
                return Err(());
            }
        };
        loop {
            let first = self.token_parser.first_usable_token(self.session);
            match first.kind {
                TokenKind::DouAnd => {
                    self.token_parser.next_usable_token(self.session);
                }
                _ => {
                    return Ok(left);
                }
            };
            let mul_expr = self.must_eq_expr();
            let right = match mul_expr {
                Ok(unary) => BoolExprs::RelExpr(unary),
                Err(_) => {
                    return Err(());
                }
            };
            left = BoolExprs::And(Box::new(left), Box::new(right));
        }
    }

    fn must_eq_expr(&mut self) -> Result<RelExpr, ()> {
        let mul_expr = self.must_rel_exp();
        let mut left = match mul_expr {
            Ok(expr) => expr,
            Err(_) => {
                return Err(());
            }
        };
        loop {
            let first = self.token_parser.first_usable_token(self.session);
            let op = match first.kind {
                TokenKind::DouEq => {
                    self.token_parser.next_usable_token(self.session);
                    RelOpe::IsEqual
                }
                TokenKind::NotEq => {
                    self.token_parser.next_usable_token(self.session);
                    RelOpe::NotEqual
                }
                _ => {
                    return Ok(left);
                }
            };
            let mul_expr = self.must_rel_exp();
            let right = match mul_expr {
                Ok(unary) => unary,
                Err(_) => {
                    return Err(());
                }
            };
            left = RelExpr::Rel(Box::new(left), op, Box::new(right));
        }
    }
    fn must_rel_exp(&mut self) -> Result<RelExpr, ()> {
        let mul_expr = self.must_add_expr();
        let mut left = match mul_expr {
            Ok(expr) => RelExpr::Math(expr),
            Err(_) => {
                return Err(());
            }
        };
        loop {
            let first = self.token_parser.first_usable_token(self.session);
            let op = match first.kind {
                TokenKind::Lt => {
                    self.token_parser.next_usable_token(self.session);
                    RelOpe::Less
                }
                TokenKind::Le => {
                    self.token_parser.next_usable_token(self.session);
                    RelOpe::LessOrEqual
                }
                TokenKind::Gt => {
                    self.token_parser.next_usable_token(self.session);
                    RelOpe::Greater
                }
                TokenKind::Ge => {
                    self.token_parser.next_usable_token(self.session);
                    RelOpe::GreaterOrEqual
                }
                _ => {
                    return Ok(left);
                }
            };
            let mul_expr = self.must_add_expr();
            let right = match mul_expr {
                Ok(unary) => RelExpr::Math(unary),
                Err(_) => {
                    return Err(());
                }
            };
            left = RelExpr::Rel(Box::new(left), op, Box::new(right));
        }
    }
    /// 识别到 [
    /// 经过查看，官方给出用例之中没有那种变态的数组定义

    pub(crate) fn must_array_after(&mut self) -> ReturnState<ArrayAfter> {
        // self.token_parser.next_usable_token(self.session); 这里不弹出 [
        let mut arr_def = ArrayAfter {
            dimensions: vec![],
            init: None,
        };
        loop {
            let first = self.token_parser.first_usable_token(self.session);
            match first.kind {
                TokenKind::OpenBracket => {
                    self.token_parser.next_usable_token(self.session);
                    let ret = self.must_math_expr();
                    match ret {
                        ReturnState::Accept(expr) => {
                            arr_def.dimensions.push(expr);
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
                        let error = ret.span.make_error("reference to array error", None);
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
        // 完成解析 dimensions
        let first = self.token_parser.first_usable_token(self.session);
        if first.kind != TokenKind::Eq {
            return ReturnState::Accept(arr_def);
        }
        self.token_parser.next_usable_token(self.session);
        // 解析数组定义右边
        let right = self.must_array_right();
        return match right {
            Ok(array_init) => {
                arr_def.init = Some(array_init);
                ReturnState::Accept(arr_def)
            }
            Err(_) => ReturnState::Panic,
        };
    }
    // 数组右边识别同样采用完全自动机
    fn must_array_right(&mut self) -> Result<ArrayInit, ()> {
        let first = self.token_parser.first_usable_token(self.session);
        return match first.kind {
            TokenKind::OpenBrace => {
                let mut init = vec![];
                self.token_parser.next_usable_token(self.session);
                loop {
                    if self.token_parser.first_usable_token(self.session).kind
                        == TokenKind::CloseBrace
                    {
                        self.token_parser.next_usable_token(self.session);
                        break;
                    }
                    let expr = self.must_array_right();
                    match expr {
                        Ok(array_init) => {
                            init.push(array_init);
                        }
                        Err(_) => {
                            return Err(());
                        }
                    }
                    // todo 元素合不合法的问题抛给下面处理
                    let first = self.token_parser.first_usable_token(self.session);
                    if first.kind == TokenKind::Comma {
                        self.token_parser.next_usable_token(self.session);
                    } else if first.kind != TokenKind::CloseBrace {
                        let error = first.span.make_error("unknown char in array define", None);
                        self.session.report_error(error);
                    }
                }
                Ok(ArrayInit::Brace(init))
            }
            _ => {
                let is_expr = first.is_in_expr_first();
                if is_expr {
                    let math = self.must_math_expr();
                    return match math {
                        ReturnState::Accept(expr) => Ok(ArrayInit::new(expr)),
                        ReturnState::Error(_) => Err(()),
                        ReturnState::Panic => Err(()),
                    };
                };
                if first.kind == TokenKind::CloseParen {
                    Ok(ArrayInit::None)
                } else {
                    let error = first
                        .span
                        .make_error("unknown expression in array define", None);
                    self.session.report_error(error);
                    Err(())
                }
            }
        };
    }

    // 由于AddExpr相当复杂，这里采用完全自动机来完成
    fn must_add_expr(&mut self) -> Result<AddExpr, ()> {
        let mul_expr = self.must_mul_expr();
        let mut left = match mul_expr {
            Ok(expr) => expr.to_add_expr(),
            Err(_) => {
                return Err(());
            }
        };
        loop {
            let first = self.token_parser.first_usable_token(self.session);
            let op = match first.kind {
                TokenKind::Plus => {
                    self.token_parser.next_usable_token(self.session);
                    MathOpe::Add
                }
                TokenKind::Minus => {
                    self.token_parser.next_usable_token(self.session);
                    MathOpe::Sub
                }
                _ => {
                    return Ok(left);
                }
            };
            let mul_expr = self.must_mul_expr();
            let right = match mul_expr {
                Ok(unary) => unary.to_add_expr(),
                Err(_) => {
                    return Err(());
                }
            };
            left = MathExpr::Expr(Box::new(left), op, Box::new(right));
        }
    }

    fn must_mul_expr(&mut self) -> Result<MulExpr, ()> {
        let unary_expr = self.must_unary_expr();
        let mut left = MulExpr::UnaryExp(match unary_expr {
            Ok(unary) => unary,
            Err(_) => {
                return Err(());
            }
        });
        loop {
            let first = self.token_parser.first_usable_token(self.session);
            let ope = match first.kind {
                TokenKind::Star => {
                    self.token_parser.next_usable_token(self.session);
                    MulOpe::Mul
                }
                TokenKind::Slash => {
                    self.token_parser.next_usable_token(self.session);
                    MulOpe::Div
                }
                TokenKind::Percent => {
                    self.token_parser.next_usable_token(self.session);
                    MulOpe::Ram
                }
                _ => {
                    return Ok(left);
                }
            };
            let unary_expr = self.must_unary_expr();
            let right = MulExpr::UnaryExp(match unary_expr {
                Ok(unary) => unary,
                Err(_) => {
                    return Err(());
                }
            });
            left = MulExpr::Expr(Box::new(left), ope, Box::new(right))
        }
    }

    fn must_unary_expr(&mut self) -> Result<UnaryExp, ()> {
        let mut first = self.token_parser.first_usable_token(self.session);
        let mut unary_ops = vec![];
        loop {
            // todo 语义检查点
            // 见 functional/21_if_test2.sy , 不同于c语言，这里的表达式支持 ---++---++ 这样的语法。
            match &first.kind {
                // 具有UnaryOper
                TokenKind::Plus => {
                    self.token_parser.next_usable_token(self.session);
                    let is_ignore = self.maybe_ignorable(true);
                    match is_ignore {
                        ReturnState::Accept(is) => {
                            if is {
                                unary_ops.push(UnaryOp::Space)
                            }
                        }
                        ReturnState::Error(_) | ReturnState::Panic => {
                            panic!("never here")
                        }
                    }
                    first = self.token_parser.first_usable_token(self.session);
                    unary_ops.push(UnaryOp::Positive)
                }
                TokenKind::Minus => {
                    self.token_parser.next_usable_token(self.session);
                    let is_ignore = self.maybe_ignorable(true);
                    match is_ignore {
                        ReturnState::Accept(is) => {
                            if is {
                                unary_ops.push(UnaryOp::Space)
                            }
                        }
                        ReturnState::Error(_) | ReturnState::Panic => {
                            panic!("never here")
                        }
                    }
                    first = self.token_parser.first_usable_token(self.session);
                    unary_ops.push(UnaryOp::Negative)
                }
                TokenKind::Bang => {
                    self.token_parser.next_usable_token(self.session);
                    let is_ignore = self.maybe_ignorable(true);
                    match is_ignore {
                        ReturnState::Accept(is) => {
                            if is {
                                unary_ops.push(UnaryOp::Space)
                            }
                        }
                        ReturnState::Error(_) | ReturnState::Panic => {
                            panic!("NEVER HERE")
                        }
                    }
                    first = self.token_parser.first_usable_token(self.session);
                    unary_ops.push(UnaryOp::Bang)
                }
                _ => {
                    break;
                }
            }
        }
        return match &first.kind {
            // PrimaryExp::(Exp)
            TokenKind::OpenParen => {
                self.token_parser.next_usable_token(self.session);
                let ret = self.must_add_expr();
                let close_paren = self.token_parser.first_usable_token(self.session);
                match close_paren.kind == TokenKind::CloseParen {
                    true => {
                        self.token_parser.next_usable_token(self.session);
                    }
                    false => {
                        let first = close_paren
                            .span
                            .make_error("expected ')' or others math expression", None);
                        self.session.report_error(first);
                        return Err(());
                    }
                }
                match ret {
                    Ok(add_expr) => {
                        return Ok(UnaryExp::Paren(unary_ops, Box::new(add_expr)));
                    }
                    Err(_) => Err(()),
                }
            }
            // PrimaryExp::Number
            TokenKind::Literal { kind, sym: _ } => {
                let kind = kind.clone();
                let token = self.token_parser.next_usable_token(self.session);
                Ok(UnaryExp::Lit(
                    unary_ops,
                    Literal {
                        sym: token.to_ident_or_lit_symbol(),
                        kind,
                        span: token.span,
                    },
                ))
            }
            // Invoke = UnaryExp::Ident([params])
            // PrimaryExp::LVal or Invoke
            TokenKind::Ident { .. } => {
                let ret = self.must_invoke_or_ref();
                ok_or_err(ret, |acc| match acc {
                    InvokeOrRef::Invoke(invoke) => Ok(UnaryExp::Invoke(unary_ops, invoke)),
                    InvokeOrRef::VarRef(var) => Ok(UnaryExp::VarRef(
                        unary_ops,
                        Ident::new(var.to_ident_or_lit_symbol(), var.span),
                    )),
                    InvokeOrRef::ArrayRef(array_ref) => {
                        Ok(UnaryExp::ArrayRef(unary_ops, array_ref))
                    }
                })
            }
            TokenKind::CloseBracket => Ok(UnaryExp::None),
            _ => {
                let error = first
                    .span
                    .make_error("Expression can not be recognized", None);
                self.session.report_error(error);
                Err(())
            }
        };
    }
}

/// It does not appear in the syntax tree because the syntax tree comes with its own priority
#[derive(Debug)]
enum MulExpr {
    UnaryExp(UnaryExp),
    Expr(Box<MulExpr>, MulOpe, Box<MulExpr>),
}

#[derive(Debug)]
enum MulOpe {
    Mul,
    Div,
    Ram,
}

impl MulExpr {
    fn to_add_expr(self) -> AddExpr {
        match self {
            MulExpr::UnaryExp(exp) => AddExpr::Unary(exp),
            MulExpr::Expr(left, ope, right) => {
                let left = left.to_add_expr();
                let ope = match ope {
                    MulOpe::Mul => MathOpe::Mul,
                    MulOpe::Div => MathOpe::Div,
                    MulOpe::Ram => MathOpe::Ram,
                };
                let right = right.to_add_expr();
                MathExpr::Expr(Box::new(left), ope, Box::new(right))
            }
        }
    }
}
