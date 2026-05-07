//! AST -- Visitor

use super::ast::*;
use crate::prelude::*;
use std::fmt::{self, Display, Formatter};

impl Display for TypeNode {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        write!(f, "{}", self.base)?;
        for dim in &self.dimensions {
            match dim {
                Some(size) => write!(f, "[{}]", size)?,
                None => write!(f, "[]")?,
            }
        }
        Ok(())
    }
}

impl AstContext {
    /// 打印字符串
    pub fn format_ast(&self, root: CompUnitRef) -> String {
        let mut output = String::new();
        if let Some(comp_unit) = self.get_comp_unit(root) {
            output.push_str("CompUnit {\n");
            for (i, &decl_ref) in comp_unit.items.iter().enumerate() {
                output.push_str(&format!("    [{}] {}\n", i, self.format_decl(decl_ref, 1)));
            }
            output.push_str("}\n");
        }
        output
    }

    pub fn format_decl(&self, decl_ref: DeclRef, indent: usize) -> String {
        let indent_str = "    ".repeat(indent);
        if let Some(decl) = self.get_decl(decl_ref) {
            match &decl.kind {
                DeclKind::Const(const_decl) => {
                    let mut result = format!("ConstDecl({:?}) {{\n", const_decl.base_type);
                    for (i, item) in const_decl.items.iter().enumerate() {
                        result.push_str(&format!(
                            "{}    [{}] {}: {} = {}\n",
                            indent_str,
                            i,
                            item.name,
                            item.ty,
                            self.format_expr(item.init, indent + 2)
                        ));
                    }
                    result.push_str(&format!("{}}}", indent_str));
                    result
                }
                DeclKind::Var(var_decl) => {
                    let mut result = format!("VarDecl({:?}) {{\n", var_decl.base_type);
                    for (i, item) in var_decl.items.iter().enumerate() {
                        let init_str = if let Some(init) = item.init {
                            format!(" = {}", self.format_expr(init, indent + 2))
                        } else {
                            String::new()
                        };
                        result.push_str(&format!(
                            "{}    [{}] {}: {}{}\n",
                            indent_str, i, item.name, item.ty, init_str
                        ));
                    }
                    result.push_str(&format!("{}}}", indent_str));
                    result
                }
                DeclKind::Function(func_decl) => {
                    let mut result =
                        format!("FuncDecl({:?} {}(", func_decl.return_type, func_decl.name);
                    for (i, param) in func_decl.params.iter().enumerate() {
                        if i > 0 {
                            result.push_str(", ");
                        }
                        result.push_str(&format!("{}: {}", param.name, param.ty));
                    }
                    result.push(')');

                    if let Some(body) = func_decl.body {
                        result.push_str(" {\n");
                        result.push_str(&format!(
                            "{}    body: {}\n",
                            indent_str,
                            self.format_stmt(body, indent + 1)
                        ));
                        result.push_str(&format!("{}}}", indent_str));
                    } else {
                        result.push(';');
                    }
                    result.push(')'); // 闭合 FuncDecl(
                    result
                }
            }
        } else {
            "InvalidDecl".to_string()
        }
    }

    pub fn format_stmt(&self, stmt_ref: StmtRef, indent: usize) -> String {
        let indent_str = "    ".repeat(indent);
        if let Some(stmt) = self.get_stmt(stmt_ref) {
            match &stmt.kind {
                StmtKind::Empty => "Empty".to_string(),
                StmtKind::Expr(expr_ref) => {
                    format!("ExprStmt({})", self.format_expr(*expr_ref, indent))
                }
                StmtKind::Assign { lval, rval } => {
                    format!(
                        "Assign({} = {})",
                        self.format_expr(*lval, indent),
                        self.format_expr(*rval, indent)
                    )
                }
                StmtKind::Block(block) => {
                    let mut result = format!("Block({} stmts) {{\n", block.stmts.len());
                    for (i, &stmt_ref) in block.stmts.iter().enumerate() {
                        result.push_str(&format!(
                            "{}    [{}] {}\n",
                            indent_str,
                            i,
                            self.format_stmt(stmt_ref, indent + 1)
                        ));
                    }
                    result.push_str(&format!("{}}}", indent_str));
                    result
                }
                StmtKind::If {
                    cond,
                    then_stmt,
                    else_stmt,
                } => {
                    let mut result = format!("If({}) {{\n", self.format_expr(*cond, indent));
                    result.push_str(&format!(
                        "{}    then: {}\n",
                        indent_str,
                        self.format_stmt(*then_stmt, indent + 1)
                    ));
                    if let Some(else_ref) = else_stmt {
                        result.push_str(&format!(
                            "{}    else: {}\n",
                            indent_str,
                            self.format_stmt(*else_ref, indent + 1)
                        ));
                    }
                    result.push_str(&format!("{}}}", indent_str));
                    result
                }
                StmtKind::While { cond, body } => {
                    format!(
                        "While({}) {{\n{}    body: {}\n{}}}",
                        self.format_expr(*cond, indent),
                        indent_str,
                        self.format_stmt(*body, indent + 1),
                        indent_str
                    )
                }
                StmtKind::Break => "Break".to_string(),
                StmtKind::Continue => "Continue".to_string(),
                StmtKind::Return(expr_opt) => {
                    if let Some(expr_ref) = expr_opt {
                        format!("Return({})", self.format_expr(*expr_ref, indent))
                    } else {
                        "Return(void)".to_string()
                    }
                }
                StmtKind::Decl(decl_ref) => {
                    format!("DeclStmt({})", self.format_decl(*decl_ref, indent + 1))
                }
            }
        } else {
            "InvalidStmt".to_string()
        }
    }

    pub fn format_expr(&self, expr_ref: ExprRef, indent: usize) -> String {
        if let Some(expr) = self.get_expr(expr_ref) {
            match &expr.kind {
                ExprKind::Int(n) => n.to_string(),
                ExprKind::Float(f) => f.to_string(),
                ExprKind::Ident(name) => name.clone(),
                ExprKind::Binary { lhs, op, rhs } => {
                    format!(
                        "({} {} {})",
                        self.format_expr(*lhs, indent),
                        op,
                        self.format_expr(*rhs, indent)
                    )
                }
                ExprKind::Unary { op, expr } => {
                    format!("({}{})", op, self.format_expr(*expr, indent))
                }
                ExprKind::Call { func, args } => {
                    let mut result = format!("{}(", func);
                    for (i, &arg_ref) in args.iter().enumerate() {
                        if i > 0 {
                            result.push_str(", ");
                        }
                        result.push_str(&self.format_expr(arg_ref, indent));
                    }
                    result.push(')');
                    result
                }
                ExprKind::Index { array, index } => {
                    format!(
                        "{}[{}]",
                        self.format_expr(*array, indent),
                        self.format_expr(*index, indent)
                    )
                }
                ExprKind::Paren(inner) => {
                    format!("({})", self.format_expr(*inner, indent))
                }
                ExprKind::InitList(exprs) => {
                    let mut result = String::from("{");
                    for (i, &expr_ref) in exprs.iter().enumerate() {
                        if i > 0 {
                            result.push_str(", ");
                        }
                        result.push_str(&self.format_expr(expr_ref, indent));
                    }
                    result.push('}');
                    result
                }
            }
        } else {
            "<invalid_expr>".to_string()
        }
    }
}

// ============================================================================
// Visitor
// ============================================================================

pub trait AstVisitor<T = ()>
where
    T: Default,
{
    fn visit_comp_unit(&mut self, ctx: &AstContext, comp_unit_ref: CompUnitRef) -> T {
        self.walk_comp_unit(ctx, comp_unit_ref)
    }

    fn visit_decl(&mut self, ctx: &AstContext, decl_ref: DeclRef) -> T {
        self.walk_decl(ctx, decl_ref)
    }

    fn visit_stmt(&mut self, ctx: &AstContext, stmt_ref: StmtRef) -> T {
        self.walk_stmt(ctx, stmt_ref)
    }

    fn visit_expr(&mut self, ctx: &AstContext, expr_ref: ExprRef) -> T {
        self.walk_expr(ctx, expr_ref)
    }

    // 默认遍历
    fn walk_comp_unit(&mut self, ctx: &AstContext, comp_unit_ref: CompUnitRef) -> T {
        if let Some(comp_unit) = ctx.get_comp_unit(comp_unit_ref) {
            for decl_ref in &comp_unit.items {
                self.visit_decl(ctx, *decl_ref);
            }
        }
        Default::default()
    }

    fn walk_decl(&mut self, ctx: &AstContext, decl_ref: DeclRef) -> T {
        if let Some(decl) = ctx.get_decl(decl_ref) {
            match &decl.kind {
                DeclKind::Const(const_decl) => {
                    for item in &const_decl.items {
                        self.visit_expr(ctx, item.init);
                    }
                }
                DeclKind::Var(var_decl) => {
                    for item in &var_decl.items {
                        if let Some(init) = item.init {
                            self.visit_expr(ctx, init);
                        }
                    }
                }
                DeclKind::Function(func_decl) => {
                    if let Some(body) = func_decl.body {
                        self.visit_stmt(ctx, body);
                    }
                }
            }
        }
        Default::default()
    }

    fn walk_stmt(&mut self, ctx: &AstContext, stmt_ref: StmtRef) -> T {
        if let Some(stmt) = ctx.get_stmt(stmt_ref) {
            match &stmt.kind {
                StmtKind::Expr(expr) => {
                    self.visit_expr(ctx, *expr);
                }
                StmtKind::Assign { lval, rval } => {
                    self.visit_expr(ctx, *lval);
                    self.visit_expr(ctx, *rval);
                }
                StmtKind::Block(block) => {
                    for stmt_ref in &block.stmts {
                        self.visit_stmt(ctx, *stmt_ref);
                    }
                }
                StmtKind::If {
                    cond,
                    then_stmt,
                    else_stmt,
                } => {
                    self.visit_expr(ctx, *cond);
                    self.visit_stmt(ctx, *then_stmt);
                    if let Some(else_ref) = else_stmt {
                        self.visit_stmt(ctx, *else_ref);
                    }
                }
                StmtKind::While { cond, body } => {
                    self.visit_expr(ctx, *cond);
                    self.visit_stmt(ctx, *body);
                }
                StmtKind::Return(expr_opt) => {
                    if let Some(expr) = expr_opt {
                        self.visit_expr(ctx, *expr);
                    }
                }
                StmtKind::Decl(decl_ref) => {
                    self.visit_decl(ctx, *decl_ref);
                }
                _ => {} // Empty, Break, Continue
            }
        }
        Default::default()
    }

    fn walk_expr(&mut self, ctx: &AstContext, expr_ref: ExprRef) -> T {
        if let Some(expr) = ctx.get_expr(expr_ref) {
            match &expr.kind {
                ExprKind::Binary { lhs, rhs, .. } => {
                    self.visit_expr(ctx, *lhs);
                    self.visit_expr(ctx, *rhs);
                }
                ExprKind::Unary { expr, .. } => {
                    self.visit_expr(ctx, *expr);
                }
                ExprKind::Call { args, .. } => {
                    for arg in args {
                        self.visit_expr(ctx, *arg);
                    }
                }
                ExprKind::Index { array, index } => {
                    self.visit_expr(ctx, *array);
                    self.visit_expr(ctx, *index);
                }
                ExprKind::InitList(exprs) => {
                    for expr in exprs {
                        self.visit_expr(ctx, *expr);
                    }
                }
                ExprKind::Paren(expr) => {
                    self.visit_expr(ctx, *expr);
                }
                _ => {} // Int, Float, Ident -- 叶子节点
            }
        }
        Default::default()
    }
}
