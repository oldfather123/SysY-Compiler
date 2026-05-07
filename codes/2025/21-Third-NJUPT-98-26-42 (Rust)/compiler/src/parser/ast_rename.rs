//! AST 阶段重命名
//!
//! 用于处理变量作用域遮蔽问题，只产生唯一变量名

use crate::parser::ast::*;
use crate::prelude::*;
use std::collections::HashMap;

pub struct VarRenamer {
    /// 词法作用域栈: 每个作用域包含 原始名称 => 重命名 映射
    /// scope_stack[0] 是全局作用域
    scope_stack: Vec<HashMap<String, String>>,
    var_counter: u32,
}

impl Default for VarRenamer {
    fn default() -> Self {
        Self::new()
    }
}

impl VarRenamer {
    pub fn new() -> Self {
        Self {
            scope_stack: vec![HashMap::new()], // 全局作用域
            var_counter: 0,
        }
    }

    /// 入口函数
    pub fn rename_variables(
        &mut self,
        ast_ctx: &mut AstContext,
        root: CompUnitRef,
    ) -> CEResult<()> {
        self.visit_comp_unit(ast_ctx, root)
    }

    fn gen_unique_name(&mut self, original_name: &str) -> String {
        self.var_counter += 1;
        format!("{}_{}", original_name, self.var_counter)
    }

    /// 当前作用域添加变量绑定
    fn add_binding(&mut self, original_name: String, renamed_name: String) {
        if let Some(current_scope) = self.scope_stack.last_mut() {
            current_scope.insert(original_name, renamed_name);
        }
    }

    /// 在作用域栈中查找重命名的变量名
    fn lookup_variable(&self, name: &str) -> Option<String> {
        for scope in self.scope_stack.iter().rev() {
            if let Some(renamed_name) = scope.get(name) {
                return Some(renamed_name.clone());
            }
        }
        None
    }

    /// 进入新作用域
    fn enter_scope(&mut self) {
        self.scope_stack.push(HashMap::new());
    }

    /// 退出当前作用域
    fn exit_scope(&mut self) {
        self.scope_stack.pop();
    }

    fn visit_comp_unit(
        &mut self,
        ast_ctx: &mut AstContext,
        comp_unit_ref: CompUnitRef,
    ) -> CEResult<()> {
        let comp_unit = ast_ctx
            .get_comp_unit(comp_unit_ref)
            .ok_or_else(|| CErr::parse_err(0, 0, "Invalid CompUnit reference"))?
            .clone();

        for decl_ref in &comp_unit.items {
            self.visit_decl(ast_ctx, *decl_ref)?;
        }

        Ok(())
    }

    fn visit_decl(&mut self, ast_ctx: &mut AstContext, decl_ref: DeclRef) -> CEResult<()> {
        let decl = ast_ctx
            .get_decl(decl_ref)
            .ok_or_else(|| CErr::parse_err(0, 0, "Invalid Decl reference"))?
            .clone();

        match &decl.kind {
            DeclKind::Const(const_decl) => {
                self.visit_const_decl(ast_ctx, const_decl, decl_ref)?;
            }
            DeclKind::Var(var_decl) => {
                self.visit_var_decl(ast_ctx, var_decl, decl_ref)?;
            }
            DeclKind::Function(func_decl) => {
                self.visit_func_decl(ast_ctx, func_decl, decl_ref)?;
            }
        }

        Ok(())
    }

    /// 常量声明
    fn visit_const_decl(
        &mut self,
        ast_ctx: &mut AstContext,
        const_decl: &ConstDecl,
        decl_ref: DeclRef,
    ) -> CEResult<()> {
        let mut new_items = Vec::new();

        for ConstDefItem { name, ty, init } in &const_decl.items {
            let new_name = self.gen_unique_name(name);
            self.add_binding(name.clone(), new_name.clone());

            // 初始化表达式
            self.visit_expr(ast_ctx, *init)?;

            let new_item = ConstDefItem::new(new_name, ty.clone(), *init);
            new_items.push(new_item);
        }

        // AST上下文更新
        let new_const_decl = ConstDecl::new(const_decl.base_type, new_items);
        let new_decl = Decl::new(DeclKind::Const(new_const_decl));

        if let Some(decl_mut) = ast_ctx.decls.get_mut(decl_ref) {
            *decl_mut = new_decl;
        }

        Ok(())
    }

    /// 变量声明
    fn visit_var_decl(
        &mut self,
        ast_ctx: &mut AstContext,
        var_decl: &VarDecl,
        decl_ref: DeclRef,
    ) -> CEResult<()> {
        let mut new_items = Vec::new();

        for VarDefItem { name, ty, init } in &var_decl.items {
            let new_name = self.gen_unique_name(name);
            self.add_binding(name.clone(), new_name.clone());

            // 初始化表达式
            if let Some(init_expr) = init {
                self.visit_expr(ast_ctx, *init_expr)?;
            }

            let new_item = VarDefItem::new(new_name, ty.clone(), *init);
            new_items.push(new_item);
        }

        // AST上下文更新
        let new_var_decl = VarDecl::new(var_decl.base_type, new_items);
        let new_decl = Decl::new(DeclKind::Var(new_var_decl));

        if let Some(decl_mut) = ast_ctx.decls.get_mut(decl_ref) {
            *decl_mut = new_decl;
        }

        Ok(())
    }

    fn visit_func_decl(
        &mut self,
        ast_ctx: &mut AstContext,
        func_decl: &FuncDecl,
        decl_ref: DeclRef,
    ) -> CEResult<()> {
        // 进入函数作用域
        self.enter_scope();

        // 重命名函数参数
        let mut new_params = Vec::new();
        for FuncParam { name, ty } in &func_decl.params {
            let new_name = self.gen_unique_name(name);
            self.add_binding(name.clone(), new_name.clone());

            let new_param = FuncParam::new(new_name, ty.clone());
            new_params.push(new_param);
        }

        let new_body = if let Some(body_ref) = func_decl.body {
            self.visit_stmt(ast_ctx, body_ref)?;
            Some(body_ref)
        } else {
            None
        };

        // 函数声明
        let new_func_decl = FuncDecl::new(
            func_decl.return_type,
            func_decl.name.clone(), // 函数名不变
            new_params,
            new_body,
        );
        let new_decl = Decl::new(DeclKind::Function(new_func_decl));

        if let Some(decl_mut) = ast_ctx.decls.get_mut(decl_ref) {
            *decl_mut = new_decl;
        }

        self.exit_scope();

        Ok(())
    }

    fn visit_stmt(&mut self, ast_ctx: &mut AstContext, stmt_ref: StmtRef) -> CEResult<()> {
        let stmt = ast_ctx
            .get_stmt(stmt_ref)
            .ok_or_else(|| CErr::parse_err(0, 0, "Invalid Stmt reference"))?
            .clone();

        match &stmt.kind {
            StmtKind::Empty => {}
            StmtKind::Expr(expr_ref) => {
                self.visit_expr(ast_ctx, *expr_ref)?;
            }
            StmtKind::Assign { lval, rval } => {
                self.visit_expr(ast_ctx, *lval)?;
                self.visit_expr(ast_ctx, *rval)?;
            }
            StmtKind::Block(block) => {
                self.visit_block_stmt(ast_ctx, block, stmt_ref)?;
            }
            StmtKind::If {
                cond,
                then_stmt,
                else_stmt,
            } => {
                self.visit_expr(ast_ctx, *cond)?;
                self.visit_stmt(ast_ctx, *then_stmt)?;
                if let Some(else_ref) = else_stmt {
                    self.visit_stmt(ast_ctx, *else_ref)?;
                }
            }
            StmtKind::While { cond, body } => {
                self.visit_expr(ast_ctx, *cond)?;
                self.visit_stmt(ast_ctx, *body)?;
            }
            StmtKind::Break | StmtKind::Continue => {}
            StmtKind::Return(expr_opt) => {
                if let Some(expr_ref) = expr_opt {
                    self.visit_expr(ast_ctx, *expr_ref)?;
                }
            }
            StmtKind::Decl(decl_ref) => {
                self.visit_decl(ast_ctx, *decl_ref)?;
            }
        }

        Ok(())
    }

    /// 大括号/块语句 {}
    fn visit_block_stmt(
        &mut self,
        ast_ctx: &mut AstContext,
        block: &BlockStmt,
        _stmt_ref: StmtRef,
    ) -> CEResult<()> {
        self.enter_scope();
        for stmt_ref in &block.stmts {
            self.visit_stmt(ast_ctx, *stmt_ref)?;
        }
        self.exit_scope();

        Ok(())
    }

    fn visit_expr(&mut self, ast_ctx: &mut AstContext, expr_ref: ExprRef) -> CEResult<()> {
        let expr = ast_ctx
            .get_expr(expr_ref)
            .ok_or_else(|| CErr::parse_err(0, 0, "Invalid Expr reference"))?
            .clone();

        match &expr.kind {
            ExprKind::Int(_) | ExprKind::Float(_) => {}
            ExprKind::Ident(name) => {
                // 重命名变量引用
                if let Some(renamed_name) = self.lookup_variable(name) {
                    let new_expr = Expr::new(ExprKind::Ident(renamed_name));
                    if let Some(expr_mut) = ast_ctx.exprs.get_mut(expr_ref) {
                        *expr_mut = new_expr;
                    }
                }
                // 如果在作用域中找不到变量 -- 可能是函数调用 -- 保持原样
            }
            ExprKind::Binary { lhs, rhs, .. } => {
                self.visit_expr(ast_ctx, *lhs)?;
                self.visit_expr(ast_ctx, *rhs)?;
            }
            ExprKind::Unary { expr, .. } => {
                self.visit_expr(ast_ctx, *expr)?;
            }
            ExprKind::Call { func: _, args } => {
                for arg_ref in args {
                    self.visit_expr(ast_ctx, *arg_ref)?;
                }
            }
            ExprKind::Index { array, index } => {
                self.visit_expr(ast_ctx, *array)?;
                self.visit_expr(ast_ctx, *index)?;
            }
            ExprKind::InitList(exprs) => {
                for expr_ref in exprs {
                    self.visit_expr(ast_ctx, *expr_ref)?;
                }
            }
            ExprKind::Paren(expr) => {
                self.visit_expr(ast_ctx, *expr)?;
            }
        }

        Ok(())
    }
}
