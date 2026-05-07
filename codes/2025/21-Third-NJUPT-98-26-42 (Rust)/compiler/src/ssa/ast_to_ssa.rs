//! AST 到 SSA IR 的转换
//!
//! 纯粹的 AST 遍历器，为了解决复杂的可变借用问题，AstToSsa上下文被干掉了
//! 现在异常方便使用的 FunctionBuilder 现已经加入肯德基豪华早餐

use core::panic;

use super::builder::*;
use super::ctx::*;
use super::dfg::DataFlowGraph;
use super::function::*;
use super::inst_builder::InstBuilder;
use super::ir::*;
use crate::parser::ast::{self, AstContext};
use crate::prelude::*;
use crate::ssa::ty_promoter::*;

/// 转换单个函数
pub fn convert_function(
    ast_ctx: &AstContext,
    func_decl: &ast::FuncDecl,
    ctx: &mut CoreContext,
) -> Result<Function, String> {
    let mut func = Function::new();
    func.name = func_decl.name.clone();

    // 使用 FunctionBuilder 构建函数体 -- 函数签名在 FunctionBuilder::new 中从ctx自动获取
    let mut builder = FunctionBuilder::new(&mut func, ctx);

    // 创建入口块（使用 FunctionBuilder 确保 BlockNode 被正确创建）
    let entry_block = builder.create_block();
    builder.func.entry = entry_block;

    // 将入口块添加到布局中
    builder.func.layout.append_block(entry_block);

    builder.switch_to_block(entry_block);

    // 设置函数参数 & 调用 def_var 定义块参数 (注意: 函数签名已经进行过预处理了直接包含token名)
    builder.make_block_params_from_func_params(entry_block);

    // 转换函数体
    if let Some(body_ref) = func_decl.body {
        if let Some(body_stmt) = ast_ctx.stmts.get(body_ref) {
            if let ast::StmtKind::Block(block) = &body_stmt.kind {
                convert_block(ast_ctx, block, &mut builder)?;
            }
        }
    }

    // 添加默认返回（如果当前块未终止）
    let current_block = builder.current_block().expect("No current block");
    if !builder.is_block_terminated(current_block) {
        let mut return_vals = Vec::new();

        // 如果函数有返回值但没有显式return，则返回默认值
        if !builder.func.signature.returns.is_empty() {
            let first_return_type = builder.func.signature.returns[0];
            if first_return_type != Type::MemToken {
                // 添加默认主返回值
                let default_val = create_default_value(first_return_type, &mut builder.func.dfg);
                return_vals.push(default_val);
            }
        }

        // 处理内存令牌返回（如果需要）
        let token_vals = collect_token_returns(&mut builder, return_vals.len())?;
        return_vals.extend(token_vals);

        builder.ins().return_(&return_vals);
    }

    // 密封所有块
    builder.seal_all_blocks();

    // 计算块布局
    builder.compute_block_layout();

    // 完成构建
    builder.finalize();

    Ok(func)
}

/// 转换语句块
fn convert_block(
    ast_ctx: &AstContext,
    block: &ast::BlockStmt,
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    // eprintln!("DEBUG: convert_block 开始，语句数量: {}", block.stmts.len());
    for (i, &stmt_ref) in block.stmts.iter().enumerate() {
        // eprintln!("DEBUG: 处理语句 {} / {}", i + 1, block.stmts.len());
        // 如果当前块已经终结，跳过后续语句（不可达代码）
        if builder.is_current_block_terminated() {
            // eprintln!("DEBUG: 当前块已终结，跳过剩余语句");
            break;
        }
        convert_stmt(ast_ctx, stmt_ref, builder)?;
    }
    // eprintln!("DEBUG: convert_block 完成");
    Ok(())
}

/// 转换语句
fn convert_stmt(
    ast_ctx: &AstContext,
    stmt_ref: ast::StmtRef,
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    let stmt = ast_ctx
        .stmts
        .get(stmt_ref)
        .ok_or("Invalid statement reference")?;

    // eprintln!(
    //     "DEBUG: convert_stmt - 语句类型: {:?}",
    //     std::mem::discriminant(&stmt.kind)
    // );
    match &stmt.kind {
        ast::StmtKind::Empty => {
            // 空语句
        }
        ast::StmtKind::Expr(expr) => {
            // 表达式语句
            convert_expr(ast_ctx, *expr, builder)?;
        }
        ast::StmtKind::Assign { lval, rval } => {
            // 赋值语句
            convert_assign(ast_ctx, *lval, *rval, builder)?;
        }
        ast::StmtKind::Block(block) => {
            // 嵌套块
            convert_block(ast_ctx, block, builder)?;
        }
        ast::StmtKind::If {
            cond,
            then_stmt,
            else_stmt,
        } => {
            // if 语句
            convert_if(ast_ctx, *cond, *then_stmt, *else_stmt, builder)?;
        }
        ast::StmtKind::While { cond, body } => {
            // while 循环
            convert_while(ast_ctx, *cond, *body, builder)?;
        }
        ast::StmtKind::Break => {
            // break 语句
            if let Some(break_target) = builder.loop_break_target() {
                builder.ins().jump(break_target, &[]);
            } else {
                return Err("break statement outside of loop".to_string());
            }
        }
        ast::StmtKind::Continue => {
            // continue 语句
            if let Some(continue_target) = builder.loop_continue_target() {
                builder.ins().jump(continue_target, &[]);
            } else {
                return Err("continue statement outside of loop".to_string());
            }
        }
        ast::StmtKind::Return(expr) => {
            // return 语句
            convert_return(ast_ctx, *expr, builder)?;
        }
        ast::StmtKind::Decl(decl_ref) => {
            // 局部声明
            convert_local_decl(ast_ctx, *decl_ref, builder)?;
        }
    }

    Ok(())
}

/// 转换表达式 - 主入口
fn convert_expr(
    ast_ctx: &AstContext,
    expr_ref: ast::ExprRef,
    builder: &mut FunctionBuilder,
) -> Result<Value, String> {
    // 使用迭代防止爆栈
    let depth = calculate_expr_depth(ast_ctx, expr_ref);
    if depth > 100 {
        convert_expr_iterative(ast_ctx, expr_ref, builder)
    } else {
        convert_expr_recursive(ast_ctx, expr_ref, builder)
    }
}

/// 迭代版本的表达式转换 - 避免栈溢出
fn convert_expr_iterative(
    ast_ctx: &AstContext,
    expr_ref: ast::ExprRef,
    builder: &mut FunctionBuilder,
) -> Result<Value, String> {
    #[derive(Debug)]
    enum WorkItem {
        Expression(ast::ExprRef),
        BinaryOp {
            op: ast::BinOp,
            rhs_expr: ast::ExprRef,
        },
    }

    let mut work_stack = vec![WorkItem::Expression(expr_ref)];
    let mut value_stack = Vec::new();

    while let Some(item) = work_stack.pop() {
        match item {
            WorkItem::Expression(expr_ref) => {
                if let Some(expr) = ast_ctx.exprs.get(expr_ref) {
                    match &expr.kind {
                        ast::ExprKind::Binary { lhs, op, rhs } => {
                            // 对于短路求值运算符，仍需要特殊处理
                            match op {
                                ast::BinOp::And | ast::BinOp::Or => {
                                    // 短路求值需要控制流，必须用递归处理
                                    let result =
                                        convert_expr_recursive(ast_ctx, expr_ref, builder)?;
                                    value_stack.push(result);
                                }
                                _ => {
                                    // 普通二元运算符，使用迭代处理
                                    // 按照后序遍历的顺序：右操作数，左操作数，操作符
                                    work_stack.push(WorkItem::BinaryOp {
                                        op: *op,
                                        rhs_expr: *rhs,
                                    });
                                    work_stack.push(WorkItem::Expression(*lhs));
                                }
                            }
                        }
                        _ => {
                            // 非二元表达式，使用递归处理
                            let result = convert_expr_recursive(ast_ctx, expr_ref, builder)?;
                            value_stack.push(result);
                        }
                    }
                } else {
                    return Err("Invalid expression reference".to_string());
                }
            }
            WorkItem::BinaryOp { op, rhs_expr } => {
                // 左操作数已经在value_stack顶部，现在需要计算右操作数
                if value_stack.is_empty() {
                    return Err("Missing left operand".to_string());
                }

                // 暂存左操作数，先计算右操作数
                let lhs_val = value_stack.pop().unwrap();

                // 计算右操作数
                let rhs_val = convert_expr_recursive(ast_ctx, rhs_expr, builder)?;

                // 执行二元运算（带类型提升）
                let result = perform_binary(op, lhs_val, rhs_val, builder)?;
                value_stack.push(result);
            }
        }
    }

    if value_stack.len() == 1 {
        Ok(value_stack.pop().unwrap())
    } else {
        Err(format!(
            "Expression evaluation error: {} values left on stack",
            value_stack.len()
        ))
    }
}

/// 递归版本的表达式转换（原有逻辑）
fn convert_expr_recursive(
    ast_ctx: &AstContext,
    expr_ref: ast::ExprRef,
    builder: &mut FunctionBuilder,
) -> Result<Value, String> {
    let expr = ast_ctx
        .exprs
        .get(expr_ref)
        .ok_or("Invalid expression reference")?;

    match &expr.kind {
        ast::ExprKind::Int(val) => {
            // 整数常量
            Ok(builder.ins().iconst32(*val))
        }
        ast::ExprKind::Float(val) => {
            // 浮点常量
            Ok(builder.ins().fconst32(*val))
        }
        ast::ExprKind::Ident(name) => {
            // 变量引用
            let name_ref = builder.name_ref(name);
            let ty = builder
                .name_type(name_ref)
                .ok_or_else(|| format!("Undefined variable: {}", name))?;

            // 检查是否是全局变量
            if builder.ctx.globals.contains_key(name_ref) {
                // 全局变量引用
                if matches!(ty, Type::ArrayPtr { .. }) {
                    // 全局数组 - 返回指针
                    builder.get_global_array_ptr(name_ref)
                } else {
                    // 全局标量 - 读取值
                    builder.get_global_scalar_value(name_ref)
                }
            } else {
                // 局部变量
                Ok(builder.use_var(name_ref, ty))
            }
        }
        ast::ExprKind::Binary { lhs, op, rhs } => {
            use ast::BinOp;

            // 对于短路求值运算符特殊处理
            match op {
                BinOp::And => {
                    // && 短路求值 - C语言语义：返回int类型（0或1）

                    // 1. 先计算左操作数并转换为bool
                    let lhs_val = convert_expr(ast_ctx, *lhs, builder)?;
                    let lhs_bool = TypePromoter::promote_to_bool(lhs_val, builder)?;

                    // 2. 创建基本块
                    let eval_rhs_block = builder.create_block();
                    let merge_block = builder.create_block();

                    // 3. 如果 lhs 为 false，直接跳到 merge 块并传递 0 (int)
                    // 如果 lhs 为 true，跳到 eval_rhs 块
                    let false_val = builder.ins().iconst32(0);
                    builder
                        .ins()
                        .brif(lhs_bool, eval_rhs_block, &[], merge_block, &[false_val]);

                    // 4. 在 eval_rhs 块中计算右操作数
                    builder.switch_to_block(eval_rhs_block);
                    let rhs_val = convert_expr(ast_ctx, *rhs, builder)?;
                    let rhs_bool = TypePromoter::promote_to_bool(rhs_val, builder)?;
                    // 将bool结果转换为int（0或1）
                    let rhs_int = builder.ins().cast(rhs_bool, Type::Int32);
                    builder.ins().jump(merge_block, &[rhs_int]);

                    // 5. 切换到 merge 块并添加块参数（int类型）
                    builder.switch_to_block(merge_block);
                    let result = builder
                        .func
                        .dfg
                        .append_block_param(merge_block, Type::Int32);

                    Ok(result)
                }
                BinOp::Or => {
                    // || 短路求值 - C语言语义：返回int类型（0或1）

                    // 1. 先计算左操作数并转换为bool
                    let lhs_val = convert_expr(ast_ctx, *lhs, builder)?;
                    let lhs_bool = TypePromoter::promote_to_bool(lhs_val, builder)?;

                    // 2. 创建基本块
                    let eval_rhs_block = builder.create_block();
                    let merge_block = builder.create_block();

                    // 3. 如果 lhs 为 true，直接跳到 merge 块并传递 1 (int)
                    // 如果 lhs 为 false，跳到 eval_rhs 块
                    let true_val = builder.ins().iconst32(1);
                    builder
                        .ins()
                        .brif(lhs_bool, merge_block, &[true_val], eval_rhs_block, &[]);

                    // 4. 在 eval_rhs 块中计算右操作数
                    builder.switch_to_block(eval_rhs_block);
                    let rhs_val = convert_expr(ast_ctx, *rhs, builder)?;
                    let rhs_bool = TypePromoter::promote_to_bool(rhs_val, builder)?;
                    // 将bool结果转换为int（0或1）
                    let rhs_int = builder.ins().cast(rhs_bool, Type::Int32);
                    builder.ins().jump(merge_block, &[rhs_int]);

                    // 5. 切换到 merge 块并添加块参数（int类型）
                    builder.switch_to_block(merge_block);
                    let result = builder
                        .func
                        .dfg
                        .append_block_param(merge_block, Type::Int32);

                    Ok(result)
                }
                _ => {
                    // 其他二元运算符正常处理
                    let lhs_val = convert_expr(ast_ctx, *lhs, builder)?;
                    let rhs_val = convert_expr(ast_ctx, *rhs, builder)?;

                    // 执行二元运算（带类型提升）
                    let result = perform_binary(*op, lhs_val, rhs_val, builder)?;
                    Ok(result)
                }
            }
        }
        ast::ExprKind::Unary { op, expr } => {
            // 一元运算
            let val = convert_expr(ast_ctx, *expr, builder)?;

            use ast::UnaryOp;
            let result = match op {
                UnaryOp::Plus => val,
                UnaryOp::Minus => {
                    let ty = builder.func.dfg.value_type(val);
                    match ty {
                        Type::Int32 => builder.ins().ineg(val),
                        Type::Float32 => builder.ins().fneg(val),
                        _ => return Err(format!("Invalid type for negation: {:?}", ty)),
                    }
                }
                UnaryOp::Not => {
                    // C语言语义：!操作符将任何标量类型转换为int类型（0或1）
                    TypePromoter::promote_unary_not(val, builder)?
                }
            };
            Ok(result)
        }
        ast::ExprKind::Call {
            func: func_name,
            args,
        } => {
            // 函数调用
            convert_call(ast_ctx, func_name, args, builder)
        }
        ast::ExprKind::Index { .. } => {
            // 数组索引（右值）
            let (base_expr, indices) = parse_array_indices(ast_ctx, expr_ref)?;

            // 获取数组指针
            let arr_ptr = convert_expr(ast_ctx, base_expr, builder)?;

            // 转换索引
            let mut index_values = Vec::new();
            for idx_expr in indices {
                index_values.push(convert_expr(ast_ctx, idx_expr, builder)?);
            }

            // 获取数组类型信息
            let arr_type = builder.func.dfg.value_type(arr_ptr);
            if let Type::ArrayPtr { elem: _, dims } = arr_type {
                // 获取数组的维度数
                let total_dims = if let Some(dims_vec) = builder.ctx.type_dims.get(dims) {
                    dims_vec.len()
                } else {
                    0
                };

                // 判断是否需要使用 array.slice
                if index_values.len() < total_dims {
                    // 索引数量少于维度数，返回子数组，使用 array.slice
                    Ok(builder.ins().array_slice(arr_ptr, &index_values))
                } else {
                    // 索引数量等于维度数，返回元素，使用 array.get
                    // 获取数组的内存令牌
                    if let Some(base) = ast_ctx.exprs.get(base_expr) {
                        if let ast::ExprKind::Ident(name) = &base.kind {
                            let ptr_ref = builder.name_ref(name);
                            let token_ref = builder.ctx.name_ref_token_ref(ptr_ref);
                            let token_val = builder.use_var(token_ref, Type::MemToken);

                            // 执行 array.get
                            Ok(builder.ins().array_get(arr_ptr, &index_values, token_val))
                        } else {
                            Err("Complex array expressions not yet supported".to_string())
                        }
                    } else {
                        Err("Invalid array base expression".to_string())
                    }
                }
            } else {
                Err("Expected array type for indexing".to_string())
            }
        }
        ast::ExprKind::InitList(_) => Err("初始化列表在变量声明时就已经被处理".to_string()),
        ast::ExprKind::Paren(expr) => convert_expr(ast_ctx, *expr, builder), // 括号表达式
    }
}

/// 转换赋值
fn convert_assign(
    ast_ctx: &AstContext,
    lval: ast::ExprRef,
    rval: ast::ExprRef,
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    let rval_value = convert_expr(ast_ctx, rval, builder)?;
    let lval_expr = &ast_ctx.exprs[lval];
    let block = builder.current_block().unwrap();

    match &lval_expr.kind {
        ast::ExprKind::Ident(name) => {
            // 简单变量赋值
            let name_ref = builder.name_ref(name);

            // 检查是否是全局变量
            if builder.ctx.globals.contains_key(name_ref) {
                // 全局标量赋值 - 使用统一的类型转换处理
                let expected_type = builder.ctx.globals[name_ref].ty;
                let converted_value = AssignmentHandler::convert_for_global_assignment(
                    rval_value,
                    &expected_type,
                    builder,
                )?;
                builder.set_global_scalar_value(name_ref, converted_value)
            } else {
                // 局部变量赋值 - 使用统一的类型转换处理
                if let Some(declared_type) = builder.name_type(name_ref) {
                    // 变量已声明，根据声明类型进行转换
                    let converted_value = AssignmentHandler::convert_for_variable_assignment(
                        rval_value,
                        &declared_type,
                        builder,
                    )?;
                    builder.def_var(name_ref, converted_value, block);
                    Ok(())
                } else {
                    // 变量未声明，这种情况不应该发生（除非是编译器前端的问题）
                    // 直接使用右值类型
                    builder.def_var(name_ref, rval_value, block);
                    Ok(())
                }
            }
        }
        ast::ExprKind::Index { .. } => {
            // 数组元素赋值 -- 数组索引 -- 左值
            let (base_expr, indices) = parse_array_indices(ast_ctx, lval)?;

            // 获取数组指针
            let arr_ptr = convert_expr(ast_ctx, base_expr, builder)?;

            // 转换索引
            let mut index_values = Vec::new();
            for idx_expr in indices {
                index_values.push(convert_expr(ast_ctx, idx_expr, builder)?);
            }

            // 获取数组的内存令牌
            if let Some(base) = ast_ctx.exprs.get(base_expr) {
                if let ast::ExprKind::Ident(name) = &base.kind {
                    let ptr_ref = builder.name_ref(name);
                    let token_ref = builder.ctx.name_ref_token_ref(ptr_ref);
                    let token_val = builder.use_var(token_ref, Type::MemToken);

                    // 使用统一的数组赋值类型转换处理
                    let converted_rval = if let Some(array_type) = builder.name_type(ptr_ref) {
                        AssignmentHandler::convert_for_array_assignment(
                            rval_value,
                            &array_type,
                            builder,
                        )?
                    } else {
                        rval_value // 如果无法获取类型信息，直接使用原值
                    };

                    // 执行 array.put
                    let new_token =
                        builder
                            .ins()
                            .array_put(arr_ptr, &index_values, converted_rval, token_val);

                    // 更新内存令牌
                    builder.def_var(token_ref, new_token, block);
                    Ok(())
                } else {
                    Err("Complex array expressions not yet supported".to_string())
                }
            } else {
                Err("Invalid array base expression".to_string())
            }
        }
        _ => Err("Invalid lvalue expression".to_string()),
    }
}

/// 转换 if 语句
fn convert_if(
    ast_ctx: &AstContext,
    cond: ast::ExprRef,
    then_stmt: ast::StmtRef,
    else_stmt: Option<ast::StmtRef>,
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    let cond_val = convert_expr(ast_ctx, cond, builder)?;

    // 显式转换条件为bool类型 - 对应RISC-V的整数分支指令语义
    let cond_bool = TypePromoter::promote_to_bool(cond_val, builder)?;

    // 创建基本块
    let then_block = builder.create_block();
    let else_block = builder.create_block();
    let merge_block = builder.create_block();

    // 条件分支
    builder
        .ins()
        .brif(cond_bool, then_block, &[], else_block, &[]);

    // then 分支
    builder.switch_to_block(then_block);
    convert_stmt(ast_ctx, then_stmt, builder)?;
    let then_terminated = builder.is_current_block_terminated();
    // 只有在当前块没有被终结时才添加jump
    if !then_terminated {
        builder.ins().jump(merge_block, &[]);
    }

    // else 分支
    builder.switch_to_block(else_block);
    if let Some(else_stmt) = else_stmt {
        convert_stmt(ast_ctx, else_stmt, builder)?;
    }
    let else_terminated = builder.is_current_block_terminated();
    // 只有在当前块没有被终结时才添加jump
    if !else_terminated {
        builder.ins().jump(merge_block, &[]);
    }

    // 只有当至少有一个分支没有终结时，才切换到合并块
    if !then_terminated || !else_terminated {
        // 合并块
        builder.switch_to_block(merge_block);
    }

    Ok(())
}

/// 转换 while 循环
fn convert_while(
    ast_ctx: &AstContext,
    cond: ast::ExprRef,
    body: ast::StmtRef,
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    // 创建基本块
    let preheader_block = builder.create_block();
    let header_block = builder.create_block();
    let body_block = builder.create_block();
    let exit_block = builder.create_block();

    // 跳转到循环头
    builder.ins().jump(preheader_block, &[]);
    builder.switch_to_block(preheader_block);
    builder.ins().jump(header_block, &[]);

    // 循环头
    builder.switch_to_block(header_block);
    let cond_val = convert_expr(ast_ctx, cond, builder)?;

    // 显式转换条件为bool类型 - 对应RISC-V的整数分支指令语义
    let cond_bool = TypePromoter::promote_to_bool(cond_val, builder)?;

    builder
        .ins()
        .brif(cond_bool, body_block, &[], exit_block, &[]);

    // 循环体
    builder.switch_to_block(body_block);
    builder.push_loop(header_block, exit_block);
    convert_stmt(ast_ctx, body, builder)?;
    builder.pop_loop();
    // 只有在当前块没有被终结时才添加jump
    if !builder.is_current_block_terminated() {
        builder.ins().jump(header_block, &[]);
    }

    // 退出块
    builder.switch_to_block(exit_block);

    Ok(())
}

/// 转换 return 语句
fn convert_return(
    ast_ctx: &AstContext,
    expr: Option<ast::ExprRef>,
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    // eprintln!("DEBUG: convert_return 开始，expr = {:?}", expr.is_some());
    // 检查是否是尾调用的情况
    if let Some(expr_ref) = expr {
        if let Some(expr_data) = ast_ctx.exprs.get(expr_ref) {
            if let ast::ExprKind::Call {
                func: func_name,
                args,
            } = &expr_data.kind
            {
                // 这是一个尾调用的情况：return func(args);
                // 直接生成 ReturnCall 指令
                return convert_return_call(ast_ctx, func_name, args, builder);
            }
        }
    }

    // 普通的 return 语句处理
    let mut return_vals = Vec::new();

    // 处理returns[0] - 主返回值
    if let Some(expr) = expr {
        let expr_val = convert_expr(ast_ctx, expr, builder)?;
        // 应用返回值类型提升
        if let Some(expected_return_type) = builder.func.signature.return_origins {
            let promoted_val =
                TypePromoter::promote_func_params(expr_val, &expected_return_type, builder)?;
            return_vals.push(promoted_val);
        } else {
            return_vals.push(expr_val);
        }
    }

    // 处理returns[i>0] - 内存令牌返回
    let token_vals = collect_token_returns(builder, return_vals.len())?;
    return_vals.extend(token_vals);

    // eprintln!(
    //     "DEBUG: 即将生成 return 指令，返回值数量: {}",
    //     return_vals.len()
    // );
    builder.ins().return_(&return_vals);
    // eprintln!("DEBUG: return 指令生成完成");
    Ok(())
}

/// 转换尾调用 - 生成 ReturnCall 指令
fn convert_return_call(
    ast_ctx: &AstContext,
    func_name: &str,
    args: &[ast::ExprRef],
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    // 获取函数引用和签名
    let func_ref = builder.name_ref(func_name);
    let func_sig = builder
        .get_signature(func_ref)
        .ok_or_else(|| format!("Function {} not found", func_name))?
        .clone();

    // 转换参数，并为数组参数紧随其后添加内存令牌
    let mut arg_vals = Vec::new();

    for (idx, &arg) in args.iter().enumerate() {
        let arg_val = convert_expr(ast_ctx, arg, builder)?;

        // 使用统一的类型转换系统进行参数类型转换
        let final_arg_val = if idx < func_sig.param_origins.len() {
            let expected_type = &func_sig.param_origins[idx];
            TypePromoter::promote_func_params(arg_val, expected_type, builder)?
        } else {
            arg_val
        };
        arg_vals.push(final_arg_val);

        // 检查是否是数组参数（根据param_origins）
        if idx < func_sig.param_origins.len() {
            if let Type::ArrayPtr { .. } = func_sig.param_origins[idx] {
                // 数组参数后需要紧跟内存令牌
                if let Some((token_val, _token_ref)) = get_array_token(ast_ctx, arg, builder) {
                    arg_vals.push(token_val);
                }
            }
        }
    }

    // 生成 ReturnCall 指令
    builder.ins().return_call(func_ref, &arg_vals);
    Ok(())
}

/// 转换局部声明
fn convert_local_decl(
    ast_ctx: &AstContext,
    decl_ref: ast::DeclRef,
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    let decl = ast_ctx
        .decls
        .get(decl_ref)
        .ok_or("Invalid declaration reference")?;

    match &decl.kind {
        ast::DeclKind::Var(var_decl) => {
            // 局部变量声明
            for item in &var_decl.items {
                convert_decl_item(
                    ast_ctx, &item.name, &item.ty, item.init, false, // is_const
                    builder,
                )?;
            }
            Ok(())
        }
        ast::DeclKind::Const(const_decl) => {
            // 常量声明
            for item in &const_decl.items {
                convert_decl_item(
                    ast_ctx,
                    &item.name,
                    &item.ty,
                    Some(item.init), // 常量必须有初始化
                    true,            // is_const
                    builder,
                )?;
            }
            Ok(())
        }
        _ => Err("Invalid local declaration".to_string()),
    }
}

/// 转换声明项 变量/常量
fn convert_decl_item(
    ast_ctx: &AstContext,
    name: &str,
    ty: &ast::TypeNode,
    init: Option<ast::ExprRef>,
    is_const: bool,
    builder: &mut FunctionBuilder,
) -> Result<(), String> {
    let name_ref = builder.name_ref(name);
    let ssa_ty = convert_type(ty, builder.ctx)?;
    builder.declare_var(name_ref, ssa_ty);

    if !ty.dimensions.is_empty() {
        // 数组初始化
        let block = builder.current_block().unwrap();

        // 创建数组分配指令
        let elem_ty = convert_base_type(ty.base);
        let dims_vec: Vec<Option<u32>> =
            ty.dimensions.iter().map(|&d| d.map(|v| v as u32)).collect();
        let dims = builder.make_u32optvec(dims_vec);

        // 解析初始化列表
        let (array_init, non_const_inits) = if let Some(init_expr) = init {
            parse_array_init_v2(ast_ctx, init_expr, ty, builder)?
        } else {
            (ArrayInit::Undef, Vec::new())
        };

        // 创建数组分配
        let alloc_inst =
            builder
                .ins()
                .array_alloc(elem_ty, dims, is_const, MemoryLocation::Stack, array_init);

        // 获取返回的指针和内存令牌
        let ptr_val = builder.func.dfg.inst_result(alloc_inst, 0);
        let mut token_val = builder.func.dfg.inst_result(alloc_inst, 1);

        // 定义数组指针变量
        builder.def_var(name_ref, ptr_val, block);

        // 定义内存令牌变量
        let token_ref = builder.ctx.name_ref_token_ref(name_ref);
        builder.declare_var(token_ref, Type::MemToken);

        // 如果有非常量初始化，生成 ArrayPut 指令
        for (indices, value) in non_const_inits {
            token_val = builder.ins().array_put(ptr_val, &indices, value, token_val);
        }

        builder.def_var(token_ref, token_val, block);
    } else {
        // 标量初始化
        let init_val = if let Some(init_expr) = init {
            let expr_val = convert_expr(ast_ctx, init_expr, builder)?;
            // 执行类型转换以匹配声明类型
            TypePromoter::promote_func_params(expr_val, &ssa_ty, builder)?
        } else {
            create_default_value(ssa_ty, &mut builder.func.dfg)
        };
        let block = builder.current_block().unwrap();
        builder.def_var(name_ref, init_val, block);
    }

    Ok(())
}

/// 转换函数调用
fn convert_call(
    ast_ctx: &AstContext,
    func_name: &str,
    args: &[ast::ExprRef],
    builder: &mut FunctionBuilder,
) -> Result<Value, String> {
    // 获取函数引用和签名
    let func_ref = builder.name_ref(func_name);
    let func_sig = builder
        .get_signature(func_ref)
        .ok_or_else(|| format!("Function {} not found", func_name))?
        .clone();

    // 转换参数，并为数组参数紧随其后添加内存令牌
    let mut arg_vals = Vec::new();
    let mut token_positions = Vec::new(); // 记录token在返回值中的位置

    for (idx, &arg) in args.iter().enumerate() {
        let arg_val = convert_expr(ast_ctx, arg, builder)?;

        // 使用统一的类型转换系统进行参数类型转换
        let final_arg_val = if idx < func_sig.param_origins.len() {
            let expected_type = &func_sig.param_origins[idx];
            TypePromoter::promote_func_params(arg_val, expected_type, builder)?
        } else {
            arg_val
        };

        arg_vals.push(final_arg_val);

        // 检查是否是数组参数（根据param_origins）
        if idx < func_sig.param_origins.len() {
            if let Type::ArrayPtr { .. } = func_sig.param_origins[idx] {
                // 数组参数后需要紧跟内存令牌
                if let Some((token_val, token_ref)) = get_array_token(ast_ctx, arg, builder) {
                    arg_vals.push(token_val);
                    // 记录这个token在返回值中的位置（跳过计算返回值）
                    let token_return_pos = if func_sig.return_origins.is_some() {
                        1 + token_positions.len()
                    } else {
                        token_positions.len()
                    };
                    token_positions.push((token_ref, token_return_pos));
                }
            }
        }
    }

    // 调用函数
    let inst = builder.ins().call(func_ref, &arg_vals);

    // 处理返回值
    let current_block = builder.current_block().unwrap();

    if builder.func.dfg.has_results(inst) {
        // 先收集结果值
        let results: Vec<Value> = builder.func.dfg.inst_results(inst).to_vec();

        // return[0] 主返回值
        let return_val = if func_sig.return_origins.is_some() && !results.is_empty() {
            results[0]
        } else {
            create_unit_value(&mut builder.func.dfg)
        };

        // return[i] 内存令牌返回
        for (token_ref, token_pos) in token_positions {
            if token_pos < results.len() {
                let new_token = results[token_pos];
                builder.def_var(token_ref, new_token, current_block);
            }
        }

        Ok(return_val)
    } else {
        // 无返回值，返回 unit
        Ok(create_unit_value(&mut builder.func.dfg))
    }
}

/// 计算表达式深度 - 使用迭代避免栈溢出
fn calculate_expr_depth(ast_ctx: &AstContext, expr_ref: ast::ExprRef) -> usize {
    let mut max_depth = 0;
    let mut stack = vec![(expr_ref, 1)]; // (expr_ref, current_depth)

    while let Some((current_expr, depth)) = stack.pop() {
        max_depth = max_depth.max(depth);

        if let Some(expr) = ast_ctx.exprs.get(current_expr) {
            match &expr.kind {
                ast::ExprKind::Binary { lhs, rhs, .. } => {
                    stack.push((*lhs, depth + 1));
                    stack.push((*rhs, depth + 1));
                }
                ast::ExprKind::Unary { expr, .. } => {
                    stack.push((*expr, depth + 1));
                }
                ast::ExprKind::Call { args, .. } => {
                    for arg in args {
                        stack.push((*arg, depth + 1));
                    }
                }
                ast::ExprKind::Index { array, index } => {
                    stack.push((*array, depth + 1));
                    stack.push((*index, depth + 1));
                }
                ast::ExprKind::Paren(inner) => {
                    stack.push((*inner, depth));
                }
                _ => {}
            }
        }
    }

    max_depth
}

//==============================================================================
// 数组处理 & 内存令牌
//==============================================================================

/// 获取数组表达式对应的内存令牌
/// Index{Index{...}} / Ident{ar} -> (token, token_ref)
fn get_array_token(
    ast_ctx: &AstContext,
    expr_ref: ast::ExprRef,
    builder: &mut FunctionBuilder,
) -> Option<(Value, NameRef)> {
    // 对于数组表达式，找到基础数组并获取其内存令牌
    match ast_ctx.exprs.get(expr_ref)?.kind {
        ast::ExprKind::Ident(ref name) => {
            let ptr_ref = builder.name_ref(name);
            let token_ref = builder.ctx.name_ref_token_ref(ptr_ref);

            if builder.name_type(token_ref).is_some() {
                let token_val = builder.use_var(token_ref, Type::MemToken);
                Some((token_val, token_ref))
            } else {
                None
            }
        }
        ast::ExprKind::Index { .. } => {
            // 对于数组索引表达式，获取基础数组
            let (base_expr, _) = parse_array_indices(ast_ctx, expr_ref).ok()?;
            get_array_token(ast_ctx, base_expr, builder)
        }
        _ => None,
    }
}

/// 收集需要返回的内存令牌
/// 用于return找到内存令牌使用
/// returns(token1', token2', ...)
fn collect_token_returns(
    builder: &mut FunctionBuilder,
    skip_count: usize,
) -> Result<Vec<Value>, String> {
    // 先收集需要的token名称，避免借用冲突
    let token_names: Vec<NameRef> = {
        let sig = &builder.func.signature;
        let mut names = Vec::new();

        // 从 skip_count 开始处理后续的内存令牌
        for i in skip_count..sig.returns.len() {
            if sig.returns[i] == Type::MemToken {
                // 根据return_names找到对应的参数内存令牌
                if let Some(Some(token_name_ref)) = sig.return_names.get(i) {
                    names.push(*token_name_ref);
                }
            }
        }
        names
    };

    // 现在使用收集到的名称来获取值
    let mut token_vals = Vec::new();
    for token_name_ref in token_names {
        let token_val = builder.use_var(token_name_ref, Type::MemToken);
        token_vals.push(token_val);
    }

    Ok(token_vals)
}

/// 解析数组索引
/// Index{Index{Index{ar, d1}, d2}, d3} -> (ar, [d1, d2, d3])
fn parse_array_indices(
    ast_ctx: &AstContext,
    expr_ref: ast::ExprRef,
) -> Result<(ast::ExprRef, Vec<ast::ExprRef>), String> {
    let mut indices = Vec::new();
    let mut current_expr = expr_ref;

    // 从外向内收集索引
    loop {
        if let Some(expr) = ast_ctx.exprs.get(current_expr) {
            if let ast::ExprKind::Index { array, index } = &expr.kind {
                indices.push(*index);
                current_expr = *array;
            } else {
                // 找到基础数组
                break;
            }
        } else {
            return Err("Invalid expression reference".to_string());
        }
    }

    indices.reverse(); // 反转得到正确的索引顺序
    Ok((current_expr, indices))
}

/// 解析数组初始化，分离常量和非常量值
/// 返回 (ArrayInit, Vec<(indices, value)>) - 常量初始化和非常量初始化列表
fn parse_array_init_v2(
    ast_ctx: &AstContext,
    init_expr: ast::ExprRef,
    array_ty: &ast::TypeNode,
    builder: &mut FunctionBuilder,
) -> Result<(ArrayInit, Vec<(Vec<Value>, Value)>), String> {
    // 计算数组维度和总大小
    let dims: Vec<usize> = array_ty
        .dimensions
        .iter()
        .filter_map(|d| d.map(|v| v as usize))
        .collect();
    let total_size: usize = dims.iter().product();

    if total_size == 0 || dims.is_empty() {
        return Err(format!("Invalid array dimensions: {:?}", dims));
    }

    if let Some(expr) = ast_ctx.exprs.get(init_expr) {
        if let ast::ExprKind::InitList(items) = &expr.kind {
            // 检查是否为空初始化列表 {} - 应该使用零初始化
            if items.is_empty() {
                return Ok((ArrayInit::Zero, Vec::new()));
            }

            // 递归解析初始化列表
            let mut values = Vec::new();
            let mut non_const_inits = Vec::new();
            parse_init_list_v2(
                ast_ctx,
                array_ty,
                builder,
                items,
                &dims,
                0,
                &mut values,
                &mut non_const_inits,
            )?;

            // 检查是否有非常量值
            if !non_const_inits.is_empty() {
                // 有非常量值，需要重新构建只包含常量的 values
                let mut const_values = Vec::new();
                let elem_ty = convert_base_type(array_ty.base);
                let zero_val = create_default_value(elem_ty, &mut builder.func.dfg);

                // 创建一个所有值都是零的数组
                const_values.resize(total_size, zero_val);

                // 填充常量值
                let mut idx = 0;
                for &val in &values {
                    if idx < total_size && builder.func.dfg.valuedata(val).is_const() {
                        const_values[idx] = val;
                    }
                    idx += 1;
                }

                let values_ref = builder.func.dfg.make_valuevec(&const_values);
                return Ok((ArrayInit::List { vals: values_ref }, non_const_inits));
            }

            // 否则按照原来的逻辑处理（全部是常量）
            let elem_ty = convert_base_type(array_ty.base);

            // 检查零初始化的几种情况：
            // 1. 单个零值 {0} - 无论数组大小都使用零初始化
            if values.len() == 1 && is_zero_value(values[0], &builder.func.dfg) {
                return Ok((ArrayInit::Zero, Vec::new()));
            }

            // 2. 所有提供的值都是零 - 使用零初始化
            let mut all_zeros = true;
            for &val in &values {
                if !is_zero_value(val, &builder.func.dfg) {
                    all_zeros = false;
                    break;
                }
            }

            if all_zeros {
                return Ok((ArrayInit::Zero, Vec::new()));
            }

            // 填充剩余的元素为零
            let zero_val = create_default_value(elem_ty, &mut builder.func.dfg);
            while values.len() < total_size {
                values.push(zero_val);
            }
            values.truncate(total_size); // 截断多余的元素
            let values_ref = builder.func.dfg.make_valuevec(&values);
            Ok((ArrayInit::List { vals: values_ref }, Vec::new()))
        } else {
            // 单个表达式初始化（应该不会出现）
            Err("Array must be initialized with init list".to_string())
        }
    } else {
        Ok((ArrayInit::Zero, Vec::new()))
    }
}

/// 递归解析多维数组初始化列表
fn parse_init_list_v2(
    ast_ctx: &AstContext,
    array_ty: &ast::TypeNode,
    builder: &mut FunctionBuilder,
    items: &[ast::ExprRef],
    dims: &[usize],
    dim_idx: usize,
    values: &mut Vec<Value>,
    non_const_inits: &mut Vec<(Vec<Value>, Value)>,
) -> Result<(), String> {
    if dim_idx >= dims.len() {
        return Ok(());
    }

    // 计算当前维度每个元素占用的空间
    let elem_size = if dim_idx + 1 < dims.len() {
        dims[dim_idx + 1..].iter().product()
    } else {
        1 // 最后一维的元素
    };

    for &item in items {
        if let Some(expr) = ast_ctx.exprs.get(item) {
            match &expr.kind {
                ast::ExprKind::InitList(sub_items) => {
                    if dim_idx + 1 < dims.len() {
                        // 嵌套初始化列表，递归处理下一维度
                        let old_len = values.len();
                        parse_init_list_v2(
                            ast_ctx,
                            array_ty,
                            builder,
                            sub_items,
                            dims,
                            dim_idx + 1,
                            values,
                            non_const_inits,
                        )?;
                        // 填充当前子数组剩余元素为零
                        let expected_len = old_len + elem_size;
                        let elem_ty = convert_base_type(array_ty.base);
                        let zero_val = create_default_value(elem_ty, &mut builder.func.dfg);
                        while values.len() < expected_len {
                            values.push(zero_val);
                        }
                    } else {
                        // 最内层仍然是初始化列表，展开处理
                        for &sub_item in sub_items {
                            let val = convert_expr(ast_ctx, sub_item, builder)?;
                            let linear_idx = values.len();

                            // 检查是否是非常量值
                            if !builder.func.dfg.valuedata(val).is_const() {
                                // 计算多维索引
                                let indices = compute_multidim_indices(linear_idx, dims);
                                let idx_values: Vec<Value> = indices
                                    .iter()
                                    .map(|&i| builder.ins().iconst32(i as i32))
                                    .collect();
                                non_const_inits.push((idx_values, val));
                            }

                            values.push(val);
                        }
                    }
                }
                _ => {
                    // 标量值
                    let val = convert_expr(ast_ctx, item, builder)?;
                    let linear_idx = values.len();

                    // 检查是否是非常量值
                    if !builder.func.dfg.valuedata(val).is_const() {
                        // 计算多维索引
                        let indices = compute_multidim_indices(linear_idx, dims);
                        let idx_values: Vec<Value> = indices
                            .iter()
                            .map(|&i| builder.ins().iconst32(i as i32))
                            .collect();
                        non_const_inits.push((idx_values, val));
                    }

                    values.push(val);
                }
            }
        } else {
            return Err(format!(
                "Invalid expression reference in init list: {:?}",
                item
            ));
        }
    }
    Ok(())
}

/// 根据线性索引计算多维索引
fn compute_multidim_indices(linear_idx: usize, dims: &[usize]) -> Vec<usize> {
    let mut indices = vec![0; dims.len()];
    let mut idx = linear_idx;

    // 从最后一维开始计算
    for i in (0..dims.len()).rev() {
        indices[i] = idx % dims[i];
        idx /= dims[i];
    }

    indices
}

//==============================================================================
// 值处理
//==============================================================================

/// 创建默认值
pub fn create_default_value(ty: Type, dfg: &mut DataFlowGraph) -> Value {
    let value_data = match ty {
        Type::Int32 => ValueData::Const {
            ty,
            c: ConstValue::Int32 { val: 0 },
        },
        Type::Float32 => ValueData::Const {
            ty,
            c: ConstValue::Float32 { val: 0.0 },
        },
        Type::Bool => ValueData::Const {
            ty,
            c: ConstValue::Bool { val: false },
        },
        Type::Unit => ValueData::Const {
            ty,
            c: ConstValue::Unit,
        },
        _ => panic!("Cannot create default value for type: {:?}", ty),
    };
    // TODO: make_value 内部使用的是 insert! 意味着多个相同的值无法重用! 可能需要考虑内存问题
    // dfg.make_value(value_data)
    dfg.make_value(value_data) // 使用 intern_value 进行去重
}

/// 创建 unit 值
pub fn create_unit_value(dfg: &mut DataFlowGraph) -> Value {
    let value_data = ValueData::Const {
        ty: Type::Unit,
        c: ConstValue::Unit,
    };
    dfg.make_value(value_data) // 使用 intern_value 进行去重
}

/// 检查值是否为零
pub fn is_zero_value(val: Value, dfg: &DataFlowGraph) -> bool {
    match dfg.valuedata(val) {
        ValueData::Const { c, .. } => c.is_zero(),
        _ => false,
    }
}

//==============================================================================
// 类型处理
//==============================================================================

/// 转换基本类型
pub fn convert_base_type(base_ty: ast::BaseType) -> Type {
    match base_ty {
        ast::BaseType::Int => Type::Int32,
        ast::BaseType::Float => Type::Float32,
        ast::BaseType::Void => Type::Unit,
    }
}

/// 转换类型
pub fn convert_type(ty: &ast::TypeNode, ctx: &mut CoreContext) -> Result<Type, String> {
    if ty.dimensions.is_empty() {
        Ok(convert_base_type(ty.base))
    } else {
        // 数组类型
        let elem_ty = match ty.base {
            ast::BaseType::Int => ArrayElemType::Int32,
            ast::BaseType::Float => ArrayElemType::Float32,
            _ => return Err("Invalid array element type".to_string()),
        };

        let dims_vec: Vec<Option<u32>> =
            ty.dimensions.iter().map(|&d| d.map(|v| v as u32)).collect();
        let dims = ctx.make_type_dims(dims_vec);

        Ok(Type::ArrayPtr {
            elem: elem_ty,
            dims,
        })
    }
}

/// 处理二元运算的类型提升和运算
fn perform_binary(
    op: ast::BinOp,
    lhs_val: Value,
    rhs_val: Value,
    builder: &mut FunctionBuilder,
) -> Result<Value, String> {
    // 对比较运算使用专门的类型提升，其他运算使用通用提升
    let promotion = match op {
        ast::BinOp::Lt
        | ast::BinOp::Le
        | ast::BinOp::Gt
        | ast::BinOp::Ge
        | ast::BinOp::Eq
        | ast::BinOp::Ne => TypePromoter::promote_compare(lhs_val, rhs_val, builder)?,
        _ => TypePromoter::promote_binary(lhs_val, rhs_val, builder)?,
    };

    // 根据操作符和提升后的类型选择相应的指令
    let result = if promotion.use_float_ops {
        match op {
            ast::BinOp::Add => builder.ins().fadd(promotion.left_val, promotion.right_val),
            ast::BinOp::Sub => builder.ins().fsub(promotion.left_val, promotion.right_val),
            ast::BinOp::Mul => builder.ins().fmul(promotion.left_val, promotion.right_val),
            ast::BinOp::Div => builder.ins().fdiv(promotion.left_val, promotion.right_val),
            ast::BinOp::Mod => {
                return Err("Modulo operation is not supported for float types".to_string());
            }
            ast::BinOp::Lt => builder.ins().lt(promotion.left_val, promotion.right_val),
            ast::BinOp::Le => builder.ins().le(promotion.left_val, promotion.right_val),
            ast::BinOp::Gt => builder.ins().gt(promotion.left_val, promotion.right_val),
            ast::BinOp::Ge => builder.ins().ge(promotion.left_val, promotion.right_val),
            ast::BinOp::Eq => builder.ins().eq(promotion.left_val, promotion.right_val),
            ast::BinOp::Ne => builder.ins().ne(promotion.left_val, promotion.right_val),
            ast::BinOp::And | ast::BinOp::Or => {
                return Err("Short-circuit operators should be handled separately".to_string());
            }
        }
    } else {
        match op {
            ast::BinOp::Add => builder.ins().iadd(promotion.left_val, promotion.right_val),
            ast::BinOp::Sub => builder.ins().isub(promotion.left_val, promotion.right_val),
            ast::BinOp::Mul => builder.ins().imul(promotion.left_val, promotion.right_val),
            ast::BinOp::Div => builder.ins().idiv(promotion.left_val, promotion.right_val),
            ast::BinOp::Mod => builder.ins().imod(promotion.left_val, promotion.right_val),
            ast::BinOp::Lt => builder.ins().lt(promotion.left_val, promotion.right_val),
            ast::BinOp::Le => builder.ins().le(promotion.left_val, promotion.right_val),
            ast::BinOp::Gt => builder.ins().gt(promotion.left_val, promotion.right_val),
            ast::BinOp::Ge => builder.ins().ge(promotion.left_val, promotion.right_val),
            ast::BinOp::Eq => builder.ins().eq(promotion.left_val, promotion.right_val),
            ast::BinOp::Ne => builder.ins().ne(promotion.left_val, promotion.right_val),
            ast::BinOp::And | ast::BinOp::Or => {
                return Err("Short-circuit operators should be handled separately".to_string());
            }
        }
    };

    Ok(result)
}

//==============================================================================
// 核心转换
//==============================================================================

/// 执行 AST 到 SSA 的转换
pub fn convert_ast_to_ssa(
    ctx: &AstContext,
    ast_root: ast::CompUnitRef,
    _module_name: String,
) -> Result<(CoreContext, Vec<Function>), String> {
    let unit = ctx
        .comp_units
        .get(ast_root)
        .ok_or("Invalid compilation unit reference")?;

    let mut core_ctx = CoreContext::new();
    let mut functions = Vec::new();

    // 处理外部函数
    let ext_funcs = super::runtime::make_runtime_functions();
    for ext in ext_funcs {
        ExtFuncData::declare_extfunc(&ext, &mut core_ctx);
    }

    // 处理全局变量 & 函数声明 & 外部函数
    super::globals::declare_globals(ast_root, ctx, &mut core_ctx);

    // 转换所有函数
    for &item_ref in &unit.items {
        if let Some(decl) = ctx.decls.get(item_ref) {
            if let ast::DeclKind::Function(func_decl) = &decl.kind {
                if func_decl.body.is_some() {
                    let func = convert_function(ctx, func_decl, &mut core_ctx)?;
                    functions.push(func);
                }
            }
        }
    }

    Ok((core_ctx, functions))
}
