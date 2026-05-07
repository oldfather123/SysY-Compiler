//! 类型提升
//!
//! 根据 C 类型提升规则处理类型
use crate::prelude::*;
use crate::ssa::builder::FunctionBuilder;
use crate::ssa::inst_builder::InstBuilder;

use crate::ssa::ir::Type;

//==============================================================================
// 赋值处理
//==============================================================================

/// 统一赋值处理器 - 封装所有类型转换逻辑
pub struct AssignmentHandler;

impl AssignmentHandler {
    /// 处理数组元素赋值的类型转换
    /// 根据数组元素类型自动转换右值
    pub fn convert_for_array_assignment(
        rval: Value,
        array_type: &Type,
        builder: &mut FunctionBuilder,
    ) -> Result<Value, String> {
        if let Type::ArrayPtr { elem, .. } = array_type {
            // 使用ArrayElemType的to_type方法获取目标类型
            let target_type = elem.to_type();
            TypePromoter::promote_func_params(rval, &target_type, builder)
        } else {
            // 不是数组类型，直接返回原值
            Ok(rval)
        }
    }

    /// 处理变量赋值的类型转换
    /// 根据变量声明类型自动转换右值
    pub fn convert_for_variable_assignment(
        rval: Value,
        declared_type: &Type,
        builder: &mut FunctionBuilder,
    ) -> Result<Value, String> {
        TypePromoter::promote_func_params(rval, declared_type, builder)
    }

    /// 处理全局变量赋值的类型转换
    /// 根据全局变量类型自动转换右值
    pub fn convert_for_global_assignment(
        rval: Value,
        global_type: &Type,
        builder: &mut FunctionBuilder,
    ) -> Result<Value, String> {
        TypePromoter::promote_func_params(rval, global_type, builder)
    }
}

//==============================================================================
// 类型提升
//==============================================================================

/// 统一类型提升处理器
pub struct TypePromoter;

/// 二元运算提升结果
#[derive(Debug, Clone)]
pub struct BinaryTypePromotion {
    pub left_val: Value,
    pub right_val: Value,
    pub result_type: Type,
    pub use_float_ops: bool,
}

impl TypePromoter {
    /// 二元运算的类型提升
    /// - 如果任一操作数是 Float32，提升为 Float32 运算
    /// - 否则使用 Int32 运算
    pub fn promote_binary(
        lhs_val: Value,
        rhs_val: Value,
        builder: &mut FunctionBuilder,
    ) -> Result<BinaryTypePromotion, String> {
        let lhs_type = builder.func.dfg.value_type(lhs_val);
        let rhs_type = builder.func.dfg.value_type(rhs_val);

        let promotion = match (lhs_type, rhs_type) {
            // 两个都是整数
            (Type::Int32, Type::Int32) => BinaryTypePromotion {
                left_val: lhs_val,
                right_val: rhs_val,
                result_type: Type::Int32,
                use_float_ops: false,
            },

            // 两个都是浮点数
            (Type::Float32, Type::Float32) => BinaryTypePromotion {
                left_val: lhs_val,
                right_val: rhs_val,
                result_type: Type::Float32,
                use_float_ops: true,
            },

            // 混合类型：提升为浮点数
            (Type::Int32, Type::Float32) => BinaryTypePromotion {
                left_val: builder.ins().cast(lhs_val, Type::Float32),
                right_val: rhs_val,
                result_type: Type::Float32,
                use_float_ops: true,
            },

            (Type::Float32, Type::Int32) => BinaryTypePromotion {
                left_val: lhs_val,
                right_val: builder.ins().cast(rhs_val, Type::Float32),
                result_type: Type::Float32,
                use_float_ops: true,
            },

            // 布尔值参与运算时转换为整数
            (Type::Bool, other) | (other, Type::Bool) => {
                let (converted_lhs, converted_rhs, target_type, use_float) = match other {
                    Type::Float32 => {
                        let bool_val = if lhs_type == Type::Bool {
                            lhs_val
                        } else {
                            rhs_val
                        };
                        let other_val = if lhs_type == Type::Bool {
                            rhs_val
                        } else {
                            lhs_val
                        };
                        let int_val = builder.ins().cast(bool_val, Type::Int32);
                        let converted_bool = builder.ins().cast(int_val, Type::Float32);
                        if lhs_type == Type::Bool {
                            (converted_bool, other_val, Type::Float32, true)
                        } else {
                            (other_val, converted_bool, Type::Float32, true)
                        }
                    }
                    _ => {
                        let bool_val = if lhs_type == Type::Bool {
                            lhs_val
                        } else {
                            rhs_val
                        };
                        let other_val = if lhs_type == Type::Bool {
                            rhs_val
                        } else {
                            lhs_val
                        };
                        let converted_bool = builder.ins().cast(bool_val, Type::Int32);
                        if lhs_type == Type::Bool {
                            (converted_bool, other_val, Type::Int32, false)
                        } else {
                            (other_val, converted_bool, Type::Int32, false)
                        }
                    }
                };

                BinaryTypePromotion {
                    left_val: converted_lhs,
                    right_val: converted_rhs,
                    result_type: target_type,
                    use_float_ops: use_float,
                }
            }

            _ => {
                return Err(format!(
                    "Unsupported binary operation types: {:?} and {:?}",
                    lhs_type, rhs_type
                ));
            }
        };

        Ok(promotion)
    }

    /// 执行函数参数的类型转换
    ///
    /// 将实际参数类型转换为期望的参数类型
    pub fn promote_func_params(
        arg_val: Value,
        expected_type: &Type,
        builder: &mut FunctionBuilder,
    ) -> Result<Value, String> {
        let actual_type = builder.func.dfg.value_type(arg_val);

        if actual_type == *expected_type {
            return Ok(arg_val);
        }

        let converted = match (&actual_type, expected_type) {
            // 基本类型转换
            (Type::Float32, Type::Int32) => builder.ins().cast(arg_val, Type::Int32),
            (Type::Int32, Type::Float32) => builder.ins().cast(arg_val, Type::Float32),
            (Type::Bool, Type::Int32) => builder.ins().cast(arg_val, Type::Int32),
            (Type::Bool, Type::Float32) => builder.ins().cast(arg_val, Type::Float32),
            (Type::Int32, Type::Bool) => builder.ins().cast(arg_val, Type::Bool),
            (Type::Float32, Type::Bool) => builder.ins().cast(arg_val, Type::Bool),

            // 数组指针转换（数组衰减）
            // 在 C 语言中，数组参数会衰减为指针，维度信息在函数内部不重要
            (
                Type::ArrayPtr {
                    elem: actual_elem, ..
                },
                Type::ArrayPtr {
                    elem: expected_elem,
                    ..
                },
            ) => {
                // 检查元素类型是否兼容
                if actual_elem == expected_elem {
                    // 元素类型相同，可以直接传递（数组衰减）
                    arg_val
                } else {
                    return Err(format!(
                        "Array element type mismatch: {:?} vs {:?}",
                        actual_elem, expected_elem
                    ));
                }
            }

            _ => {
                return Err(format!(
                    "Cannot convert argument from {:?} to {:?}",
                    actual_type, expected_type
                ));
            }
        };

        Ok(converted)
    }

    /// 执行条件表达式的布尔转换
    ///
    /// 将任意类型转换为布尔值用于条件判断
    pub fn promote_to_bool(val: Value, builder: &mut FunctionBuilder) -> Result<Value, String> {
        let val_type = builder.func.dfg.value_type(val);

        match val_type {
            Type::Bool => Ok(val),
            Type::Int32 | Type::Float32 => Ok(builder.ins().cast(val, Type::Bool)),
            _ => Err(format!("Cannot convert {:?} to boolean", val_type)),
        }
    }

    /// 处理逻辑运算的类型转换
    ///
    /// C语言语义：逻辑运算（&&、||）总是返回int类型（0或1）
    /// 操作数会被转换为bool进行运算，然后结果转换为int
    pub fn promote_logical(
        lhs_val: Value,
        rhs_val: Value,
        builder: &mut FunctionBuilder,
    ) -> Result<(Value, Value), String> {
        let bool_lhs = Self::promote_to_bool(lhs_val, builder)?;
        let bool_rhs = Self::promote_to_bool(rhs_val, builder)?;
        Ok((bool_lhs, bool_rhs))
    }

    /// 处理比较运算的类型转换
    ///
    /// 确保比较运算的两个操作数类型一致
    /// Bool类型会被转换为Int32进行比较
    pub fn promote_compare(
        lhs_val: Value,
        rhs_val: Value,
        builder: &mut FunctionBuilder,
    ) -> Result<BinaryTypePromotion, String> {
        let lhs_type = builder.func.dfg.value_type(lhs_val);
        let rhs_type = builder.func.dfg.value_type(rhs_val);

        // 对于比较运算，Bool类型需要转换为Int32
        let (converted_lhs, converted_rhs) = match (lhs_type, rhs_type) {
            (Type::Bool, Type::Bool) => {
                // 两个bool都转换为int进行比较
                (
                    builder.ins().cast(lhs_val, Type::Int32),
                    builder.ins().cast(rhs_val, Type::Int32),
                )
            }
            (Type::Bool, Type::Int32) => (builder.ins().cast(lhs_val, Type::Int32), rhs_val),
            (Type::Int32, Type::Bool) => (lhs_val, builder.ins().cast(rhs_val, Type::Int32)),
            (Type::Bool, Type::Float32) => {
                let int_lhs = builder.ins().cast(lhs_val, Type::Int32);
                (builder.ins().cast(int_lhs, Type::Float32), rhs_val)
            }
            (Type::Float32, Type::Bool) => {
                let int_rhs = builder.ins().cast(rhs_val, Type::Int32);
                (lhs_val, builder.ins().cast(int_rhs, Type::Float32))
            }
            _ => (lhs_val, rhs_val), // 其他情况使用通用提升
        };

        // 对转换后的操作数应用通用提升规则
        Self::promote_binary(converted_lhs, converted_rhs, builder)
    }

    /// 处理一元Not运算的类型转换
    ///
    /// C语言语义：!操作符将任何标量类型转换为int类型（0或1）
    pub fn promote_unary_not(val: Value, builder: &mut FunctionBuilder) -> Result<Value, String> {
        let val_type = builder.func.dfg.value_type(val);

        match val_type {
            Type::Bool => {
                // Bool -> Bool 的not运算，然后转换为int
                let bool_result = builder.ins().not(val);
                Ok(builder.ins().cast(bool_result, Type::Int32))
            }
            Type::Int32 | Type::Float32 => {
                // 先转换为bool，然后not，最后转换为int
                let bool_val = builder.ins().cast(val, Type::Bool);
                let bool_result = builder.ins().not(bool_val);
                Ok(builder.ins().cast(bool_result, Type::Int32))
            }
            _ => Err(format!("Cannot apply ! operator to {:?}", val_type)),
        }
    }
}
