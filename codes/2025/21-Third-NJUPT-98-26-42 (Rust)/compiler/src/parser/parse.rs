//! 暴力的LL -- 等成为瓶颈再说 :)

use super::ast::*;
use crate::{
    lexer::{Token, TokenType},
    prelude::*,
};

pub struct Parser {
    tokens: Vec<Token>,
    current: usize,
    ctx: AstContext,
    /// 常量符号表 - 存储常量名到常量值的映射
    const_table: std::collections::HashMap<String, i32>,
}

impl Parser {
    pub fn new(tokens: Vec<Token>) -> Self {
        Self {
            tokens,
            current: 0,
            ctx: AstContext::new(),
            const_table: std::collections::HashMap::new(),
        }
    }

    pub fn parse(mut self) -> Result<(AstContext, CompUnitRef), CErr> {
        let comp_unit = self.parse_comp_unit()?;
        let comp_unit_ref = self.ctx.add_comp_unit(comp_unit);
        Ok((self.ctx, comp_unit_ref))
    }

    // ========================================================================
    // Utils
    // ========================================================================

    /// TODO: 计算常量表达式的值
    fn eval_const_expr(&self, expr_ref: ExprRef) -> Option<i32> {
        let expr = self.ctx.exprs.get(expr_ref)?;
        match &expr.kind {
            ExprKind::Int(value) => Some(*value),
            ExprKind::Ident(name) => {
                // 在常量符号表中查找常量值
                self.const_table.get(name).copied()
            }
            ExprKind::Binary { lhs, op, rhs } => {
                let lhs_val = self.eval_const_expr(*lhs)?;
                let rhs_val = self.eval_const_expr(*rhs)?;
                match op {
                    BinOp::Add => Some(lhs_val + rhs_val),
                    BinOp::Sub => Some(lhs_val - rhs_val),
                    BinOp::Mul => Some(lhs_val * rhs_val),
                    BinOp::Div => {
                        if rhs_val != 0 {
                            Some(lhs_val / rhs_val)
                        } else {
                            None
                        }
                    }
                    BinOp::Mod => {
                        if rhs_val != 0 {
                            Some(lhs_val % rhs_val)
                        } else {
                            None
                        }
                    }
                    _ => None,
                }
            }
            ExprKind::Unary { op, expr } => {
                let val = self.eval_const_expr(*expr)?;
                match op {
                    UnaryOp::Plus => Some(val),
                    UnaryOp::Minus => Some(-val),
                    UnaryOp::Not => Some(if val == 0 { 1 } else { 0 }),
                }
            }
            ExprKind::Paren(inner) => self.eval_const_expr(*inner),
            _ => None, // TODO 常量表达式
        }
    }

    /// 当前token
    fn current_token(&self) -> &Token {
        static EOF_TOKEN: std::sync::LazyLock<Token> = std::sync::LazyLock::new(|| {
            Token::simple(TokenType::Eof, crate::lexer::TokenPos::new(0, 0, 0, 0))
        });
        self.tokens.get(self.current).unwrap_or(&EOF_TOKEN)
    }

    /// 检查当前token类型
    fn check(&self, expected: TokenType) -> bool {
        self.current_token().kind == expected
    }

    /// 前进到下一个token
    fn advance(&mut self) -> &Token {
        if !self.is_at_end() {
            self.current += 1;
        }
        self.previous()
    }

    /// 是否到达末尾
    fn is_at_end(&self) -> bool {
        self.current_token().kind == TokenType::Eof
    }

    /// 前一个token
    fn previous(&self) -> &Token {
        &self.tokens[self.current - 1]
    }

    /// 消费token
    fn eat(&mut self, expected: TokenType, msg: &str) -> Result<&Token, CErr> {
        if self.check(expected) {
            Ok(self.advance())
        } else {
            let token = self.current_token();
            Err(CErr::parse_err(
                token.pos.line,
                token.pos.column,
                format!("{}, got {:?}", msg, token.kind),
            ))
        }
    }

    /// 匹配token
    fn match_token(&mut self, token_type: TokenType) -> bool {
        if self.check(token_type) {
            self.advance();
            true
        } else {
            false
        }
    }

    // ========================================================================
    // 语法解析
    // ========================================================================

    /// CompUnit → { Decl | FuncDef }
    fn parse_comp_unit(&mut self) -> Result<CompUnit, CErr> {
        let mut items = Vec::new();

        while !self.is_at_end() {
            let decl = self.parse_decl()?;
            let decl_ref = self.ctx.add_decl(decl);
            items.push(decl_ref);
        }

        Ok(CompUnit::new(items))
    }

    /// Decl → ConstDecl | VarDecl | FuncDef
    fn parse_decl(&mut self) -> Result<Decl, CErr> {
        if self.check(TokenType::Const) {
            self.parse_const_decl()
        } else if self.check(TokenType::Int)
            || self.check(TokenType::Float)
            || self.check(TokenType::Void)
        {
            self.parse_var_or_func_decl()
        } else {
            let token = self.current_token();
            Err(CErr::parse_err(
                token.pos.line,
                token.pos.column,
                "Expected declaration",
            ))
        }
    }

    /// ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
    fn parse_const_decl(&mut self) -> Result<Decl, CErr> {
        self.eat(TokenType::Const, "Expected 'const'")?;
        let base_type = self.parse_base_type()?;

        let mut items = Vec::new();
        items.push(self.parse_const_def_item(base_type)?);

        while self.match_token(TokenType::Comma) {
            items.push(self.parse_const_def_item(base_type)?);
        }

        self.eat(TokenType::Semi, "Expected ';' after const declaration")?;
        Ok(Decl::new(DeclKind::Const(ConstDecl::new(base_type, items))))
    }

    /// ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
    fn parse_const_def_item(&mut self, base_type: BaseType) -> Result<ConstDefItem, CErr> {
        let name_token = self.eat(TokenType::Ident, "Expected identifier")?;
        let name = name_token.text.clone();

        let mut dimensions = Vec::new();
        while self.match_token(TokenType::LBrack) {
            let size_expr = self.parse_expression()?;
            let size_expr_ref = self.ctx.add_expr(size_expr);
            let size_value = self.eval_const_expr(size_expr_ref);
            dimensions.push(size_value);
            self.eat(TokenType::RBrack, "Expected ']'")?;
        }

        let ty = TypeNode::new(base_type, dimensions);
        self.eat(TokenType::Assign, "Expected '=' in const definition")?;
        let init_expr = self.parse_expression()?;
        let init_ref = self.ctx.add_expr(init_expr);

        // 如果是标量常量，计算其值并存入常量表
        if ty.dimensions.is_empty() {
            if let Some(value) = self.eval_const_expr(init_ref) {
                self.const_table.insert(name.clone(), value);
            }
        }

        Ok(ConstDefItem::new(name, ty, init_ref))
    }

    /// 解析变量声明 / 函数定义
    fn parse_var_or_func_decl(&mut self) -> Result<Decl, CErr> {
        let base_type = self.parse_base_type()?;
        let name_token = self.eat(TokenType::Ident, "Expected identifier")?;
        let name = name_token.text.clone();

        // 检查是否函数定义 -- 后面跟着 '('
        if self.check(TokenType::LParen) {
            self.parse_func_decl(base_type, name)
        } else {
            self.parse_var_decl(base_type, name)
        }
    }

    /// VarDecl → BType VarDef { ',' VarDef } ';'
    fn parse_var_decl(&mut self, base_type: BaseType, first_name: String) -> Result<Decl, CErr> {
        let mut items = Vec::new();

        // 解析第一变量
        let first_item = self.parse_var_def_item(base_type, first_name)?;
        items.push(first_item);

        // 解析其他变量
        while self.match_token(TokenType::Comma) {
            let name_token = self.eat(TokenType::Ident, "Expected identifier")?;
            let name = name_token.text.clone();
            let item = self.parse_var_def_item(base_type, name)?;
            items.push(item);
        }

        self.eat(TokenType::Semi, "Expected ';' after variable declaration")?;
        Ok(Decl::new(DeclKind::Var(VarDecl::new(base_type, items))))
    }

    /// VarDef → Ident { '[' ConstExp ']' } [ '=' InitVal ]
    fn parse_var_def_item(
        &mut self,
        base_type: BaseType,
        name: String,
    ) -> Result<VarDefItem, CErr> {
        let mut dimensions = Vec::new();
        while self.match_token(TokenType::LBrack) {
            let size_expr = self.parse_expression()?;
            let size_expr_ref = self.ctx.add_expr(size_expr);
            let size_value = self.eval_const_expr(size_expr_ref);
            dimensions.push(size_value);
            self.eat(TokenType::RBrack, "Expected ']'")?;
        }

        let ty = TypeNode::new(base_type, dimensions);
        let init = if self.match_token(TokenType::Assign) {
            let init_expr = self.parse_expression()?;
            let init_ref = self.ctx.add_expr(init_expr);
            Some(init_ref)
        } else {
            None
        };

        Ok(VarDefItem::new(name, ty, init))
    }

    /// FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
    fn parse_func_decl(&mut self, return_type: BaseType, name: String) -> Result<Decl, CErr> {
        self.eat(TokenType::LParen, "Expected '(' in function declaration")?;

        let mut params = Vec::new();
        if !self.check(TokenType::RParen) {
            params.push(self.parse_func_param()?);
            while self.match_token(TokenType::Comma) {
                params.push(self.parse_func_param()?);
            }
        }

        self.eat(TokenType::RParen, "Expected ')' after parameters")?;

        let body = if self.check(TokenType::LBrace) {
            let block = self.parse_block_stmt()?;
            let stmt = Stmt::new(StmtKind::Block(block));
            let stmt_ref = self.ctx.add_stmt(stmt);
            Some(stmt_ref)
        } else {
            self.eat(TokenType::Semi, "Expected ';' after function declaration")?;
            None
        };

        Ok(Decl::new(DeclKind::Function(FuncDecl::new(
            return_type,
            name,
            params,
            body,
        ))))
    }

    /// FuncFParam → BType Ident ['[' ']' { '[' Exp ']' }]
    fn parse_func_param(&mut self) -> Result<FuncParam, CErr> {
        let base_type = self.parse_base_type()?;
        let name_token = self.eat(TokenType::Ident, "Expected parameter name")?;
        let name = name_token.text.clone();

        let mut dimensions = Vec::new();
        if self.match_token(TokenType::LBrack) {
            self.eat(TokenType::RBrack, "Expected ']'")?;
            dimensions.push(None); // FIX: 第一维为未指定大小?

            while self.match_token(TokenType::LBrack) {
                let size_expr = self.parse_expression()?;
                let size_expr_ref = self.ctx.add_expr(size_expr);
                let size_value = self.eval_const_expr(size_expr_ref);
                dimensions.push(size_value);
                self.eat(TokenType::RBrack, "Expected ']'")?;
            }
        }

        let ty = TypeNode::new(base_type, dimensions);
        Ok(FuncParam::new(name, ty))
    }

    /// BType → 'int' | 'float' | 'void'
    fn parse_base_type(&mut self) -> Result<BaseType, CErr> {
        if self.match_token(TokenType::Int) {
            Ok(BaseType::Int)
        } else if self.match_token(TokenType::Float) {
            Ok(BaseType::Float)
        } else if self.match_token(TokenType::Void) {
            Ok(BaseType::Void)
        } else {
            let token = self.current_token();
            Err(CErr::parse_err(
                token.pos.line,
                token.pos.column,
                "Expected type specifier",
            ))
        }
    }

    /// Block → '{' { BlockItem } '}'
    fn parse_block_stmt(&mut self) -> Result<BlockStmt, CErr> {
        self.eat(TokenType::LBrace, "Expected '{'")?;

        let mut stmts = Vec::new();
        while !self.check(TokenType::RBrace) && !self.is_at_end() {
            let stmt = self.parse_stmt()?;
            let stmt_ref = self.ctx.add_stmt(stmt);
            stmts.push(stmt_ref);
        }

        self.eat(TokenType::RBrace, "Expected '}'")?;
        Ok(BlockStmt::new(stmts))
    }

    /// Stmt → LVal '=' Exp ';' | [Exp] ';' | Block | IfStmt | WhileStmt | BreakStmt | ContinueStmt | ReturnStmt
    fn parse_stmt(&mut self) -> Result<Stmt, CErr> {
        match self.current_token().kind {
            TokenType::LBrace => {
                let block = self.parse_block_stmt()?;
                Ok(Stmt::new(StmtKind::Block(block)))
            }
            TokenType::If => self.parse_if_stmt(),
            TokenType::While => self.parse_while_stmt(),
            TokenType::Break => {
                self.advance();
                self.eat(TokenType::Semi, "Expected ';' after 'break'")?;
                Ok(Stmt::new(StmtKind::Break))
            }
            TokenType::Continue => {
                self.advance();
                self.eat(TokenType::Semi, "Expected ';' after 'continue'")?;
                Ok(Stmt::new(StmtKind::Continue))
            }
            TokenType::Return => self.parse_return_stmt(),
            TokenType::Semi => {
                self.advance();
                Ok(Stmt::new(StmtKind::Empty))
            }
            TokenType::Int | TokenType::Float | TokenType::Const => {
                let decl = self.parse_decl()?;
                let decl_ref = self.ctx.add_decl(decl);
                Ok(Stmt::new(StmtKind::Decl(decl_ref)))
            }
            _ => {
                // 表达式语句 / 赋值语句
                let expr = self.parse_expression()?;

                if self.match_token(TokenType::Assign) {
                    let rval = self.parse_expression()?;
                    let lval_ref = self.ctx.add_expr(expr);
                    let rval_ref = self.ctx.add_expr(rval);
                    self.eat(TokenType::Semi, "Expected ';' after assignment")?;
                    Ok(Stmt::new(StmtKind::Assign {
                        lval: lval_ref,
                        rval: rval_ref,
                    }))
                } else {
                    let expr_ref = self.ctx.add_expr(expr);
                    self.eat(TokenType::Semi, "Expected ';' after expression")?;
                    Ok(Stmt::new(StmtKind::Expr(expr_ref)))
                }
            }
        }
    }

    /// IfStmt → 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
    fn parse_if_stmt(&mut self) -> Result<Stmt, CErr> {
        self.eat(TokenType::If, "Expected 'if'")?;
        self.eat(TokenType::LParen, "Expected '(' after 'if'")?;
        let cond = self.parse_expression()?;
        let cond_ref = self.ctx.add_expr(cond);
        self.eat(TokenType::RParen, "Expected ')' after condition")?;

        let then_stmt = self.parse_stmt()?;
        let then_ref = self.ctx.add_stmt(then_stmt);

        let else_stmt = if self.match_token(TokenType::Else) {
            let else_s = self.parse_stmt()?;
            let else_ref = self.ctx.add_stmt(else_s);
            Some(else_ref)
        } else {
            None
        };

        Ok(Stmt::new(StmtKind::If {
            cond: cond_ref,
            then_stmt: then_ref,
            else_stmt,
        }))
    }

    /// WhileStmt → 'while' '(' Cond ')' Stmt
    fn parse_while_stmt(&mut self) -> Result<Stmt, CErr> {
        self.eat(TokenType::While, "Expected 'while'")?;
        self.eat(TokenType::LParen, "Expected '(' after 'while'")?;
        let cond = self.parse_expression()?;
        let cond_ref = self.ctx.add_expr(cond);
        self.eat(TokenType::RParen, "Expected ')' after condition")?;

        let body = self.parse_stmt()?;
        let body_ref = self.ctx.add_stmt(body);

        Ok(Stmt::new(StmtKind::While {
            cond: cond_ref,
            body: body_ref,
        }))
    }

    /// ReturnStmt → 'return' [Exp] ';'
    fn parse_return_stmt(&mut self) -> Result<Stmt, CErr> {
        self.eat(TokenType::Return, "Expected 'return'")?;

        let expr = if self.check(TokenType::Semi) {
            None
        } else {
            let e = self.parse_expression()?;
            let e_ref = self.ctx.add_expr(e);
            Some(e_ref)
        };

        self.eat(TokenType::Semi, "Expected ';' after return")?;
        Ok(Stmt::new(StmtKind::Return(expr)))
    }

    /// 表达式解析 -- 运算符优先级LL
    fn parse_expression(&mut self) -> Result<Expr, CErr> {
        self.parse_logical_or()
    }

    /// LogicalOr → LogicalAnd { '||' LogicalAnd }
    fn parse_logical_or(&mut self) -> Result<Expr, CErr> {
        let mut expr = self.parse_logical_and()?;

        while self.match_token(TokenType::Or) {
            let rhs = self.parse_logical_and()?;
            let lhs_ref = self.ctx.add_expr(expr);
            let rhs_ref = self.ctx.add_expr(rhs);
            expr = Expr::new(ExprKind::Binary {
                lhs: lhs_ref,
                op: BinOp::Or,
                rhs: rhs_ref,
            });
        }

        Ok(expr)
    }

    /// LogicalAnd → Equality { '&&' Equality }
    fn parse_logical_and(&mut self) -> Result<Expr, CErr> {
        let mut expr = self.parse_equality()?;

        while self.match_token(TokenType::And) {
            let rhs = self.parse_equality()?;
            let lhs_ref = self.ctx.add_expr(expr);
            let rhs_ref = self.ctx.add_expr(rhs);
            expr = Expr::new(ExprKind::Binary {
                lhs: lhs_ref,
                op: BinOp::And,
                rhs: rhs_ref,
            });
        }

        Ok(expr)
    }

    /// Equality → Relational { ('==' | '!=') Relational }
    fn parse_equality(&mut self) -> Result<Expr, CErr> {
        let mut expr = self.parse_relational()?;

        while let Some(op) = self.match_equality_op() {
            let rhs = self.parse_relational()?;
            let lhs_ref = self.ctx.add_expr(expr);
            let rhs_ref = self.ctx.add_expr(rhs);
            expr = Expr::new(ExprKind::Binary {
                lhs: lhs_ref,
                op,
                rhs: rhs_ref,
            });
        }

        Ok(expr)
    }

    fn match_equality_op(&mut self) -> Option<BinOp> {
        if self.match_token(TokenType::Eq) {
            Some(BinOp::Eq)
        } else if self.match_token(TokenType::Ne) {
            Some(BinOp::Ne)
        } else {
            None
        }
    }

    /// Relational → Additive { ('<' | '<=' | '>' | '>=') Additive }
    fn parse_relational(&mut self) -> Result<Expr, CErr> {
        let mut expr = self.parse_additive()?;

        while let Some(op) = self.match_relational_op() {
            let rhs = self.parse_additive()?;
            let lhs_ref = self.ctx.add_expr(expr);
            let rhs_ref = self.ctx.add_expr(rhs);
            expr = Expr::new(ExprKind::Binary {
                lhs: lhs_ref,
                op,
                rhs: rhs_ref,
            });
        }

        Ok(expr)
    }

    fn match_relational_op(&mut self) -> Option<BinOp> {
        if self.match_token(TokenType::Lt) {
            Some(BinOp::Lt)
        } else if self.match_token(TokenType::Le) {
            Some(BinOp::Le)
        } else if self.match_token(TokenType::Gt) {
            Some(BinOp::Gt)
        } else if self.match_token(TokenType::Ge) {
            Some(BinOp::Ge)
        } else {
            None
        }
    }

    /// Additive → Multiplicative { ('+' | '-') Multiplicative }
    fn parse_additive(&mut self) -> Result<Expr, CErr> {
        let mut expr = self.parse_multiplicative()?;

        while let Some(op) = self.match_additive_op() {
            let rhs = self.parse_multiplicative()?;
            let lhs_ref = self.ctx.add_expr(expr);
            let rhs_ref = self.ctx.add_expr(rhs);
            expr = Expr::new(ExprKind::Binary {
                lhs: lhs_ref,
                op,
                rhs: rhs_ref,
            });
        }

        Ok(expr)
    }

    fn match_additive_op(&mut self) -> Option<BinOp> {
        if self.match_token(TokenType::Plus) {
            Some(BinOp::Add)
        } else if self.match_token(TokenType::Minus) {
            Some(BinOp::Sub)
        } else {
            None
        }
    }

    /// Multiplicative → Unary { ('*' | '/' | '%') Unary }
    fn parse_multiplicative(&mut self) -> Result<Expr, CErr> {
        let mut expr = self.parse_unary()?;

        while let Some(op) = self.match_multiplicative_op() {
            let rhs = self.parse_unary()?;
            let lhs_ref = self.ctx.add_expr(expr);
            let rhs_ref = self.ctx.add_expr(rhs);
            expr = Expr::new(ExprKind::Binary {
                lhs: lhs_ref,
                op,
                rhs: rhs_ref,
            });
        }

        Ok(expr)
    }

    fn match_multiplicative_op(&mut self) -> Option<BinOp> {
        if self.match_token(TokenType::Star) {
            Some(BinOp::Mul)
        } else if self.match_token(TokenType::Slash) {
            Some(BinOp::Div)
        } else if self.match_token(TokenType::Percent) {
            Some(BinOp::Mod)
        } else {
            None
        }
    }

    /// Unary → ('+' | '-' | '!') Unary | Postfix
    fn parse_unary(&mut self) -> Result<Expr, CErr> {
        if let Some(op) = self.match_unary_op() {
            let expr = self.parse_unary()?;
            let expr_ref = self.ctx.add_expr(expr);
            Ok(Expr::new(ExprKind::Unary { op, expr: expr_ref }))
        } else {
            self.parse_postfix()
        }
    }

    fn match_unary_op(&mut self) -> Option<UnaryOp> {
        if self.match_token(TokenType::Plus) {
            Some(UnaryOp::Plus)
        } else if self.match_token(TokenType::Minus) {
            Some(UnaryOp::Minus)
        } else if self.match_token(TokenType::Not) {
            Some(UnaryOp::Not)
        } else {
            None
        }
    }

    /// Postfix → Primary { '[' Exp ']' | '(' [ArgList] ')' }
    fn parse_postfix(&mut self) -> Result<Expr, CErr> {
        let mut expr = self.parse_primary()?;

        loop {
            if self.match_token(TokenType::LBrack) {
                let index = self.parse_expression()?;
                let array_ref = self.ctx.add_expr(expr);
                let index_ref = self.ctx.add_expr(index);
                self.eat(TokenType::RBrack, "Expected ']'")?;
                expr = Expr::new(ExprKind::Index {
                    array: array_ref,
                    index: index_ref,
                });
            } else if self.match_token(TokenType::LParen) {
                // Function Call:
                if let ExprKind::Ident(func_name) = &expr.kind {
                    let func_name = func_name.clone();
                    let mut args = Vec::new();

                    if !self.check(TokenType::RParen) {
                        let expr = self.parse_expression()?;
                        args.push(self.ctx.add_expr(expr));
                        while self.match_token(TokenType::Comma) {
                            let expr = self.parse_expression()?;
                            args.push(self.ctx.add_expr(expr));
                        }
                    }

                    self.eat(TokenType::RParen, "Expected ')'")?;
                    expr = Expr::new(ExprKind::Call {
                        func: func_name,
                        args,
                    });
                } else {
                    return Err(CErr::parse_err(
                        self.current_token().pos.line,
                        self.current_token().pos.column,
                        "Invalid function call",
                    ));
                }
            } else {
                break;
            }
        }

        Ok(expr)
    }

    /// Primary → Number | Ident | '(' Exp ')' | '{' [ExpList] '}'
    fn parse_primary(&mut self) -> Result<Expr, CErr> {
        match &self.current_token().kind {
            TokenType::IntLit => {
                let token = self.advance();
                let value = parse_int_literal(&token.text).unwrap_or(0);
                Ok(Expr::new(ExprKind::Int(value)))
            }
            TokenType::FloatLit => {
                let token = self.advance();
                let value = parse_float_literal(&token.text).unwrap_or(0.0);
                Ok(Expr::new(ExprKind::Float(value)))
            }
            TokenType::Ident => {
                let token = self.advance();
                Ok(Expr::new(ExprKind::Ident(token.text.clone())))
            }
            TokenType::LParen => {
                self.advance();
                let expr = self.parse_expression()?;
                self.eat(TokenType::RParen, "Expected ')'")?;
                let expr_ref = self.ctx.add_expr(expr);
                Ok(Expr::new(ExprKind::Paren(expr_ref)))
            }
            TokenType::LBrace => {
                self.advance();
                let mut exprs = Vec::new();

                if !self.check(TokenType::RBrace) {
                    let expr = self.parse_expression()?;
                    exprs.push(self.ctx.add_expr(expr));
                    while self.match_token(TokenType::Comma) {
                        let expr = self.parse_expression()?;
                        exprs.push(self.ctx.add_expr(expr));
                    }
                }

                self.eat(TokenType::RBrace, "Expected '}'")?;
                Ok(Expr::new(ExprKind::InitList(exprs)))
            }
            _ => {
                let token = self.current_token();
                Err(CErr::parse_err(
                    token.pos.line,
                    token.pos.column,
                    format!("Unexpected token {:?}", token.kind),
                ))
            }
        }
    }
}
