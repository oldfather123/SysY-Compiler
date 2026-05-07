//! 极其暴力的全局变量常量求值
use super::ctx::*;
use super::function::*;
use super::ir::*;
use crate::parser::ast::{self, AstContext};
use crate::prelude::*;

/// 全局初始化值
#[derive(Debug, Clone, PartialEq)]
pub enum GlobalInit {
    /// 零初始化
    Zero,
    /// 标量常量值
    Scalar(ConstValue),
    /// 数组初始化列表（平铺）
    Array(Vec<ConstValue>),
}

/// 全局符号
#[derive(Debug, Clone, PartialEq)]
pub struct GlobalData {
    pub name: String,     // 符号名
    pub ty: Type,         // 符号类型
    pub dims: U32OptVec,  // 维度信息 -- 全局标量视作 int a[1] 的特殊数组
    pub init: GlobalInit, // 初始值
    pub is_const: bool,   // 是否为常量
}

/// 常量求值上下文 - 用于查找已定义的全局常量
struct ConstEvalContext<'a> {
    ast_ctx: &'a AstContext,
    globals: &'a RefSparseMap<Global, GlobalData>,
    names: &'a Names,
    type_dims: &'a U32OptVecPool,
    // 临时存储已计算的常量值
    computed_constants: &'a std::collections::HashMap<NameRef, ConstValue>,
}

/// 根据目标类型转换常量值
fn convert_const_value_to_type(value: ConstValue, target_type: Type) -> ConstValue {
    match (value, target_type) {
        // 同类型直接返回
        (ConstValue::Int32 { .. }, Type::Int32) => value,
        (ConstValue::Float32 { .. }, Type::Float32) => value,
        (ConstValue::Bool { .. }, Type::Bool) => value,

        // 浮点数到整数的转换（截断）
        (ConstValue::Float32 { val }, Type::Int32) => ConstValue::Int32 { val: val as i32 },

        // 整数到浮点数的转换
        (ConstValue::Int32 { val }, Type::Float32) => ConstValue::Float32 { val: val as f32 },

        // 整数/浮点数到布尔值的转换
        (ConstValue::Int32 { val }, Type::Bool) => ConstValue::Bool { val: val != 0 },
        (ConstValue::Float32 { val }, Type::Bool) => ConstValue::Bool { val: val != 0.0 },

        // 布尔值到整数/浮点数的转换
        (ConstValue::Bool { val }, Type::Int32) => ConstValue::Int32 {
            val: if val { 1 } else { 0 },
        },
        (ConstValue::Bool { val }, Type::Float32) => ConstValue::Float32 {
            val: if val { 1.0 } else { 0.0 },
        },

        // 其他情况保持原值
        _ => value,
    }
}

/// 常量求值 - 用于全局变量初始化
fn eval_const_expr(ctx: &ConstEvalContext, expr_ref: ast::ExprRef) -> Result<ConstValue, String> {
    let expr = ctx
        .ast_ctx
        .exprs
        .get(expr_ref)
        .ok_or("Invalid expression reference")?;

    match &expr.kind {
        ast::ExprKind::Int(val) => Ok(ConstValue::Int32 { val: *val }),
        ast::ExprKind::Float(val) => Ok(ConstValue::Float32 { val: *val }),
        ast::ExprKind::Ident(name) => {
            // 查找已定义的全局常量
            // 查找现有的name_ref
            let name_ref = ctx.names.keys().find(|&k| ctx.names.ref_name(k) == name);
            if let Some(name_ref) = name_ref {
                // 首先检查临时计算的常量
                if let Some(val) = ctx.computed_constants.get(&name_ref) {
                    return Ok(*val);
                }

                if let Some(global) = ctx.globals.get(name_ref) {
                    if !global.is_const {
                        return Err(format!("'{}' is not a constant", name));
                    }
                    match &global.init {
                        GlobalInit::Scalar(val) => Ok(*val),
                        _ => Err(format!("'{}' is not a scalar constant", name)),
                    }
                } else {
                    Err(format!("Undefined global constant: {}", name))
                }
            } else {
                Err(format!("Undefined global constant: {}", name))
            }
        }
        ast::ExprKind::Index { .. } => {
            // 解析数组索引链
            let (array_name, indices) = parse_const_array_indices(ctx.ast_ctx, expr_ref)?;

            // 获取数组名
            if let Some(base_expr) = ctx.ast_ctx.exprs.get(array_name) {
                if let ast::ExprKind::Ident(name) = &base_expr.kind {
                    let name_ref = ctx.names.keys().find(|&k| ctx.names.ref_name(k) == name);
                    if let Some(name_ref) = name_ref {
                        if let Some(global) = ctx.globals.get(name_ref) {
                            if !global.is_const {
                                return Err(format!("'{}' is not a constant array", name));
                            }
                            // 计算索引值
                            let mut index_vals = Vec::new();
                            for idx_expr in indices {
                                match eval_const_expr(ctx, idx_expr)? {
                                    ConstValue::Int32 { val } => {
                                        if val < 0 {
                                            return Err(
                                                "Negative array index in constant expression"
                                                    .to_string(),
                                            );
                                        }
                                        index_vals.push(val as usize);
                                    }
                                    _ => {
                                        return Err(
                                            "Array index must be integer in constant expression"
                                                .to_string(),
                                        );
                                    }
                                }
                            }

                            // 获取数组元素
                            match &global.init {
                                GlobalInit::Array(values) => {
                                    let flat_index = compute_flat_index(
                                        &global.dims,
                                        &index_vals,
                                        ctx.type_dims,
                                    )?;
                                    values
                                        .get(flat_index)
                                        .copied()
                                        .ok_or_else(|| "Array index out of bounds".to_string())
                                }
                                _ => Err(format!("'{}' is not an array", name)),
                            }
                        } else {
                            Err(format!("Undefined global array: {}", name))
                        }
                    } else {
                        Err(format!("Undefined global array: {}", name))
                    }
                } else {
                    Err("Complex array expressions not supported in constant context".to_string())
                }
            } else {
                Err("Invalid array base expression".to_string())
            }
        }
        ast::ExprKind::Unary { op, expr } => {
            let val = eval_const_expr(ctx, *expr)?;
            match (op, val) {
                (ast::UnaryOp::Plus, v) => Ok(v),
                (ast::UnaryOp::Minus, ConstValue::Int32 { val }) => {
                    Ok(ConstValue::Int32 { val: -val })
                }
                (ast::UnaryOp::Minus, ConstValue::Float32 { val }) => {
                    Ok(ConstValue::Float32 { val: -val })
                }
                _ => Err("Invalid unary operation in constant expression".to_string()),
            }
        }
        ast::ExprKind::Binary { lhs, op, rhs } => {
            let lhs_val = eval_const_expr(ctx, *lhs)?;
            let rhs_val = eval_const_expr(ctx, *rhs)?;

            use ast::BinOp;
            match (lhs_val, op, rhs_val) {
                (ConstValue::Int32 { val: l }, BinOp::Add, ConstValue::Int32 { val: r }) => {
                    Ok(ConstValue::Int32 { val: l + r })
                }
                (ConstValue::Int32 { val: l }, BinOp::Sub, ConstValue::Int32 { val: r }) => {
                    Ok(ConstValue::Int32 { val: l - r })
                }
                (ConstValue::Int32 { val: l }, BinOp::Mul, ConstValue::Int32 { val: r }) => {
                    Ok(ConstValue::Int32 { val: l * r })
                }
                (ConstValue::Int32 { val: l }, BinOp::Div, ConstValue::Int32 { val: r }) => {
                    if r == 0 {
                        Err("Division by zero in constant expression".to_string())
                    } else {
                        Ok(ConstValue::Int32 { val: l / r })
                    }
                }
                (ConstValue::Float32 { val: l }, BinOp::Add, ConstValue::Float32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l + r })
                }
                (ConstValue::Float32 { val: l }, BinOp::Sub, ConstValue::Float32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l - r })
                }
                (ConstValue::Float32 { val: l }, BinOp::Mul, ConstValue::Float32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l * r })
                }
                (ConstValue::Float32 { val: l }, BinOp::Div, ConstValue::Float32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l / r })
                }
                // 支持混合类型运算 (int op float)
                (ConstValue::Int32 { val: l }, BinOp::Add, ConstValue::Float32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l as f32 + r })
                }
                (ConstValue::Int32 { val: l }, BinOp::Sub, ConstValue::Float32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l as f32 - r })
                }
                (ConstValue::Int32 { val: l }, BinOp::Mul, ConstValue::Float32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l as f32 * r })
                }
                (ConstValue::Int32 { val: l }, BinOp::Div, ConstValue::Float32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l as f32 / r })
                }
                // 支持混合类型运算 (float op int)
                (ConstValue::Float32 { val: l }, BinOp::Add, ConstValue::Int32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l + r as f32 })
                }
                (ConstValue::Float32 { val: l }, BinOp::Sub, ConstValue::Int32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l - r as f32 })
                }
                (ConstValue::Float32 { val: l }, BinOp::Mul, ConstValue::Int32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l * r as f32 })
                }
                (ConstValue::Float32 { val: l }, BinOp::Div, ConstValue::Int32 { val: r }) => {
                    Ok(ConstValue::Float32 { val: l / r as f32 })
                }
                _ => Err("Invalid binary operation in constant expression".to_string()),
            }
        }
        _ => Err("Non-constant expression in global initializer".to_string()),
    }
}

/// 解析常量数组索引 - 返回 (基础数组表达式, 索引列表)
fn parse_const_array_indices(
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

/// 计算多维数组的扁平化索引
fn compute_flat_index(
    dims: &U32OptVec,
    indices: &[usize],
    type_dims_pool: &U32OptVecPool,
) -> Result<usize, String> {
    let dims_vec = &type_dims_pool[*dims];

    if indices.len() != dims_vec.len() {
        return Err(format!(
            "Index dimension mismatch: expected {}, got {}",
            dims_vec.len(),
            indices.len()
        ));
    }

    let mut flat_index = 0;
    let mut stride = 1;

    // 从右向左计算
    for i in (0..dims_vec.len()).rev() {
        if let Some(dim) = dims_vec[i] {
            if indices[i] >= dim as usize {
                return Err(format!("Index out of bounds: {} >= {}", indices[i], dim));
            }
            flat_index += indices[i] * stride;
            stride *= dim as usize;
        } else {
            return Err("Cannot index into array with unknown dimensions".to_string());
        }
    }

    Ok(flat_index)
}

/// 常量求值初始化列表
fn eval_const_init_list(
    ctx: &ConstEvalContext,
    items: &[ast::ExprRef],
    dims: &[usize],
    elem_ty: Type,
    dim_idx: usize,
    values: &mut Vec<ConstValue>,
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
        if let Some(expr) = ctx.ast_ctx.exprs.get(item) {
            match &expr.kind {
                ast::ExprKind::InitList(sub_items) => {
                    if dim_idx + 1 < dims.len() {
                        // 嵌套初始化列表，递归处理下一维度
                        let old_len = values.len();
                        eval_const_init_list(ctx, sub_items, dims, elem_ty, dim_idx + 1, values)?;
                        // 填充当前子数组剩余元素为零
                        let expected_len = old_len + elem_size;
                        let zero_val = match elem_ty {
                            Type::Int32 => ConstValue::Int32 { val: 0 },
                            Type::Float32 => ConstValue::Float32 { val: 0.0 },
                            Type::Bool => ConstValue::Bool { val: false },
                            _ => return Err("Invalid element type for array".to_string()),
                        };
                        while values.len() < expected_len {
                            values.push(zero_val);
                        }
                    } else {
                        // 最内层仍然是初始化列表，展开处理
                        for &sub_item in sub_items {
                            let val = eval_const_expr(ctx, sub_item)?;
                            let converted_val = convert_const_value_to_type(val, elem_ty);
                            values.push(converted_val);
                        }
                    }
                }
                _ => {
                    // 标量值 - 根据数组元素类型进行类型转换
                    let val = eval_const_expr(ctx, item)?;
                    let converted_val = convert_const_value_to_type(val, elem_ty);
                    values.push(converted_val);
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

/// 处理所有全局变量&函数声明
pub fn declare_globals(ast_root: ast::CompUnitRef, ast_ctx: &AstContext, ctx: &mut CoreContext) {
    let comp_unit = &ast_ctx.comp_units[ast_root];

    // 第一遍：收集所有全局变量声明（不计算初始化值）
    for &decl_ref in &comp_unit.items {
        let decl = &ast_ctx.decls[decl_ref];
        match &decl.kind {
            ast::DeclKind::Function(func_decl) => {
                // 所有函数都要被当作 extfunc 来存放签名
                ExtFuncData::declare_extfunc(func_decl, ctx);
            }
            ast::DeclKind::Const(const_decl) => {
                for item in &const_decl.items {
                    let name_ref = ctx.name_ref(&item.name);
                    let ty = super::ast_to_ssa::convert_type(&item.ty, ctx).unwrap();

                    let dims_vec: Vec<Option<u32>> = item
                        .ty
                        .dimensions
                        .iter()
                        .map(|&d| d.map(|v| v as u32))
                        .collect();
                    let dims = ctx.make_type_dims(dims_vec);

                    // 第一遍只创建占位符
                    let global_data = GlobalData {
                        name: item.name.clone(),
                        ty,
                        dims,
                        init: GlobalInit::Zero, // 占位符
                        is_const: true,
                    };

                    ctx.globals.insert(name_ref, global_data);
                    ctx.name_types.insert(name_ref, ty);
                }
            }
            ast::DeclKind::Var(var_decl) => {
                for item in &var_decl.items {
                    let name_ref = ctx.name_ref(&item.name);
                    let ty = super::ast_to_ssa::convert_type(&item.ty, ctx).unwrap();

                    let dims_vec: Vec<Option<u32>> = item
                        .ty
                        .dimensions
                        .iter()
                        .map(|&d| d.map(|v| v as u32))
                        .collect();
                    let dims = ctx.make_type_dims(dims_vec);

                    // 第一遍只创建占位符
                    let global_data = GlobalData {
                        name: item.name.clone(),
                        ty,
                        dims,
                        init: GlobalInit::Zero, // 占位符
                        is_const: false,
                    };

                    ctx.globals.insert(name_ref, global_data);
                    ctx.name_types.insert(name_ref, ty);
                }
            }
        }
    }

    // 第二遍：计算初始化值（此时所有全局变量都已声明）
    // 首先收集所有的name_ref和相关信息
    #[derive(Clone)]
    struct InitItem {
        name_ref: NameRef,
        init_ref: Option<ast::ExprRef>,
        ty_info: ast::TypeNode,
    }

    let mut const_items: Vec<InitItem> = Vec::new();
    let mut var_items: Vec<InitItem> = Vec::new();

    for &decl_ref in &comp_unit.items {
        let decl = &ast_ctx.decls[decl_ref];
        match &decl.kind {
            ast::DeclKind::Function(_) => {}
            ast::DeclKind::Const(const_decl) => {
                for item in &const_decl.items {
                    let name_ref = ctx.name_ref(&item.name);
                    const_items.push(InitItem {
                        name_ref,
                        init_ref: Some(item.init),
                        ty_info: item.ty.clone(),
                    });
                }
            }
            ast::DeclKind::Var(var_decl) => {
                for item in &var_decl.items {
                    let name_ref = ctx.name_ref(&item.name);
                    var_items.push(InitItem {
                        name_ref,
                        init_ref: item.init,
                        ty_info: item.ty.clone(),
                    });
                }
            }
        }
    }

    // 现在进行常量求值
    let mut updates: Vec<(NameRef, GlobalInit)> = Vec::new();

    {
        // 处理常量 - 使用迭代方式处理依赖关系
        let mut computed_constants = std::collections::HashMap::new();
        let mut remaining_constants = const_items.clone();

        while !remaining_constants.is_empty() {
            let mut progress_made = false;
            let mut unprocessed = Vec::new();

            for item in remaining_constants {
                let eval_ctx = ConstEvalContext {
                    ast_ctx,
                    globals: &ctx.globals,
                    names: &ctx.names,
                    type_dims: &ctx.type_dims,
                    computed_constants: &computed_constants,
                };

                let name_ref = item.name_ref;

                // 尝试处理初始化值
                let result = if let Some(init_ref) = item.init_ref {
                    if let Some(expr) = ast_ctx.exprs.get(init_ref) {
                        if item.ty_info.dimensions.is_empty() {
                            // 标量常量 - 根据声明类型进行类型转换
                            eval_const_expr(&eval_ctx, init_ref).map(|val| {
                                let declared_type =
                                    super::ast_to_ssa::convert_base_type(item.ty_info.base);
                                let converted_val = convert_const_value_to_type(val, declared_type);
                                GlobalInit::Scalar(converted_val)
                            })
                        } else {
                            // 数组常量
                            if let ast::ExprKind::InitList(items) = &expr.kind {
                                // 检查是否为空初始化列表 {} - 应该使用零初始化
                                if items.is_empty() {
                                    Ok(GlobalInit::Zero)
                                } else {
                                    let elem_ty =
                                        super::ast_to_ssa::convert_base_type(item.ty_info.base);
                                    let dims: Vec<usize> = item
                                        .ty_info
                                        .dimensions
                                        .iter()
                                        .filter_map(|&d| d.map(|v| v as usize))
                                        .collect();
                                    let total_size: usize = dims.iter().product();
                                    let mut values = Vec::new();
                                    match eval_const_init_list(
                                        &eval_ctx,
                                        items,
                                        &dims,
                                        elem_ty,
                                        0,
                                        &mut values,
                                    ) {
                                        Ok(_) => {
                                            // 检查是否所有值都是零
                                            let all_zeros = values.iter().all(|v| match v {
                                                ConstValue::Int32 { val } => *val == 0,
                                                ConstValue::Float32 { val } => *val == 0.0,
                                                ConstValue::Bool { val } => !*val,
                                                _ => false,
                                            });

                                            // 如果所有提供的值都是零，使用零初始化
                                            if all_zeros {
                                                Ok(GlobalInit::Zero)
                                            } else {
                                                // 填充剩余的元素为零
                                                let zero_val = match elem_ty {
                                                    Type::Int32 => ConstValue::Int32 { val: 0 },
                                                    Type::Float32 => {
                                                        ConstValue::Float32 { val: 0.0 }
                                                    }
                                                    Type::Bool => ConstValue::Bool { val: false },
                                                    _ => panic!("Invalid element type"),
                                                };
                                                while values.len() < total_size {
                                                    values.push(zero_val);
                                                }
                                                values.truncate(total_size);
                                                Ok(GlobalInit::Array(values))
                                            }
                                        }
                                        Err(e) => Err(e),
                                    }
                                }
                            } else {
                                Ok(GlobalInit::Zero)
                            }
                        }
                    } else {
                        Ok(GlobalInit::Zero)
                    }
                } else {
                    Ok(GlobalInit::Zero)
                };

                match result {
                    Ok(init) => {
                        // 成功计算，记录常量值并准备更新
                        if let GlobalInit::Scalar(val) = init {
                            computed_constants.insert(name_ref, val);
                        }
                        updates.push((name_ref, init));
                        progress_made = true;
                    }
                    Err(_) => {
                        // 计算失败，可能依赖尚未计算的常量，留到下一轮
                        unprocessed.push(item);
                    }
                }
            }

            if !progress_made && !unprocessed.is_empty() {
                // 没有进展且还有未处理的常量，说明存在循环依赖或其他错误
                let first_unprocessed = &unprocessed[0];
                let name = ctx.names.ref_name(first_unprocessed.name_ref);
                panic!("Failed to resolve constant dependencies for '{}'", name);
            }

            remaining_constants = unprocessed;
        }

        // 处理变量（常量依赖已解决，可以直接处理）
        let eval_ctx = ConstEvalContext {
            ast_ctx,
            globals: &ctx.globals,
            names: &ctx.names,
            type_dims: &ctx.type_dims,
            computed_constants: &computed_constants,
        };

        for item in &var_items {
            let name_ref = item.name_ref;

            // 处理初始化值
            let init = if let Some(init_expr_ref) = item.init_ref {
                let init_expr = &ast_ctx.exprs[init_expr_ref];
                if item.ty_info.dimensions.is_empty() {
                    // 标量变量 - 根据声明类型进行类型转换
                    let val = eval_const_expr(&eval_ctx, init_expr_ref).unwrap();
                    let declared_type = super::ast_to_ssa::convert_base_type(item.ty_info.base);
                    let converted_val = convert_const_value_to_type(val, declared_type);
                    GlobalInit::Scalar(converted_val)
                } else {
                    // 数组变量
                    if let ast::ExprKind::InitList(items) = &init_expr.kind {
                        // 检查是否为空初始化列表 {} - 应该使用零初始化
                        if items.is_empty() {
                            GlobalInit::Zero
                        } else {
                            let elem_ty = super::ast_to_ssa::convert_base_type(item.ty_info.base);
                            let dims: Vec<usize> = item
                                .ty_info
                                .dimensions
                                .iter()
                                .filter_map(|&d| d.map(|v| v as usize))
                                .collect();
                            let total_size: usize = dims.iter().product();
                            let mut values = Vec::new();
                            eval_const_init_list(&eval_ctx, items, &dims, elem_ty, 0, &mut values)
                                .unwrap();

                            // 检查是否所有值都是零
                            let all_zeros = values.iter().all(|v| match v {
                                ConstValue::Int32 { val } => *val == 0,
                                ConstValue::Float32 { val } => *val == 0.0,
                                ConstValue::Bool { val } => !*val,
                                _ => false,
                            });

                            // 如果所有提供的值都是零，使用零初始化
                            if all_zeros {
                                GlobalInit::Zero
                            } else {
                                // 填充剩余的元素为零
                                let zero_val = match elem_ty {
                                    Type::Int32 => ConstValue::Int32 { val: 0 },
                                    Type::Float32 => ConstValue::Float32 { val: 0.0 },
                                    Type::Bool => ConstValue::Bool { val: false },
                                    _ => panic!("Invalid element type"),
                                };
                                while values.len() < total_size {
                                    values.push(zero_val);
                                }
                                values.truncate(total_size);
                                GlobalInit::Array(values)
                            }
                        }
                    } else {
                        GlobalInit::Zero
                    }
                }
            } else {
                GlobalInit::Zero
            };

            // 收集更新
            updates.push((name_ref, init));
        }
    }

    // 应用所有更新（常量和变量）
    for (name_ref, init) in updates {
        if let Some(global) = ctx.globals.get_mut(name_ref) {
            // 先检查是否是常量
            let is_const = const_items.iter().any(|item| item.name_ref == name_ref);

            global.init = init;

            // 对于常量，设置is_const标志
            if is_const {
                global.is_const = true;
            }
        }
    }
}
