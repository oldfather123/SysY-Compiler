//! SSA IR 格式化打印
//!
//! 带上下文的格式化打印，包括：
//! - FunctionFormatter: 单个函数的格式化打印
//! - BuilderContextFormatter: 全局变量和函数集合的打印

use super::ctx::*;
use super::function::Function;
use super::globals::{GlobalData, GlobalInit};
use super::ir::*;
use crate::prelude::*;
use std::fmt::{self, Write};

/// 从函数入口开始按逆后序收集所有块
fn collect_blocks_in_reverse_postorder(entry: Block, func: &Function) -> Vec<Block> {
    let mut blocks = Vec::new();
    let mut current = Some(entry);

    // 从入口块开始，使用Layout API收集所有块
    while let Some(block) = current {
        blocks.push(block);
        current = func.layout.next_block(block);
    }

    blocks
}

/// 函数格式化器
pub struct FunctionFormatter<'a> {
    pub func: &'a Function,
    pub ctx: &'a CoreContext,
}

impl<'a> FunctionFormatter<'a> {
    pub fn new(func: &'a Function, ctx: &'a CoreContext) -> Self {
        Self { func, ctx }
    }

    /// 格式化函数参数
    fn format_params(&self) -> String {
        let sig = &self.func.signature;
        let mut params = Vec::new();

        for (i, (ty, name)) in sig.params.iter().zip(&sig.param_names).enumerate() {
            if let Some(name_ref) = name {
                let name = self.ctx.ref_name(*name_ref);
                if *ty == Type::MemToken {
                    params.push(format!("${}:token", name));
                } else {
                    params.push(format!("%{}:{}", name, self.format_type(ty)));
                }
            } else {
                params.push(format!("%arg{}:{}", i, self.format_type(ty)));
            }
        }

        params.join(", ")
    }

    /// 格式化返回类型
    fn format_returns(&self) -> String {
        let sig = &self.func.signature;
        if sig.returns.is_empty() {
            "()".to_string()
        } else {
            let mut returns = Vec::new();
            for (ty, name) in sig.returns.iter().zip(&sig.return_names) {
                if let Some(name_ref) = name {
                    let name = self.ctx.ref_name(*name_ref);
                    if *ty == Type::MemToken {
                        returns.push(format!("${}:token", name));
                    } else {
                        returns.push(format!("%{}:{}", name, self.format_type(ty)));
                    }
                } else {
                    returns.push(self.format_type(ty).to_string());
                }
            }
            format!("({})", returns.join(", "))
        }
    }

    /// 格式化类型
    pub fn format_type(&self, ty: &Type) -> String {
        match ty {
            Type::Unit => "unit".to_string(),
            Type::Bool => "bool".to_string(),
            Type::Int32 => "i32".to_string(),
            Type::Float32 => "f32".to_string(),
            Type::ArrayPtr { elem, dims } => {
                let dims_data = &self.ctx.type_dims[*dims];
                let dims_str: Vec<String> = dims_data
                    .iter()
                    .map(|d| match d {
                        Some(size) => size.to_string(),
                        None => "?".to_string(),
                    })
                    .collect();
                format!(
                    "{}[{}]*",
                    format!("{:?}", elem.to_type()),
                    dims_str.join("][")
                )
            }
            Type::Int64 => "i64".to_string(), // 新增的 64 位整数支持
            Type::Float64 => "f64".to_string(), // 新增的 64 位浮点支持
            Type::MemToken => "token".to_string(),
        }
    }

    /// 格式化值
    pub fn format_value(&self, value: Value) -> String {
        let data = self.func.dfg.valuedata(value);
        match data {
            ValueData::Const { c, .. } => format!("{}", c),
            ValueData::Inst { inst, idx, ty } => {
                if *idx == 0 {
                    format!("%inst{}({})", inst.index(), self.format_type(ty))
                } else {
                    format!("%inst{}.{}({})", inst.index(), idx, self.format_type(ty))
                }
            }
            ValueData::BlockParam { block, idx, .. } => {
                format!("%bb{}.{}", block.index(), idx)
            }
            ValueData::GlobalSymbol { sym, ty } => {
                let name = self.ctx.ref_name(*sym);
                if *ty == Type::MemToken {
                    format!("@{}#mem", name)
                } else {
                    format!("@{}", name)
                }
            }
            ValueData::Alias { original, .. } => {
                format!("{} (alias)", self.format_value(*original))
            }
            ValueData::Union { x, y, .. } => {
                format!(
                    "union({}, {})",
                    self.format_value(*x),
                    self.format_value(*y)
                )
            }
        }
    }

    /// 格式化指令
    fn format_instruction(&self, inst: Inst) -> String {
        let inst_data = &self.func.dfg.insts[inst];
        let results = self.func.dfg.inst_results(inst);

        let mut s = String::new();

        // 格式化结果
        if !results.is_empty() {
            let result_strs: Vec<String> = results.iter().map(|v| self.format_value(*v)).collect();
            write!(s, "{} = ", result_strs.join(", ")).unwrap();
        }

        // 格式化指令本身
        match inst_data {
            InstructionData::Binary { op, lhs, rhs } => {
                write!(
                    s,
                    "{} {}, {}",
                    op,
                    self.format_value(*lhs),
                    self.format_value(*rhs)
                )
                .unwrap();
            }
            InstructionData::Unary { op, val } => {
                write!(s, "{} {}", op, self.format_value(*val)).unwrap();
            }
            InstructionData::Call { func, args } => {
                let func_name = self.ctx.ref_name(*func);
                let args_vec = self.func.dfg.valuevec(*args);
                let args_str: Vec<String> =
                    args_vec.iter().map(|v| self.format_value(*v)).collect();
                write!(s, "call @{}({})", func_name, args_str.join(", ")).unwrap();
            }
            InstructionData::Cast { val, to } => {
                write!(
                    s,
                    "cast {} to {}",
                    self.format_value(*val),
                    self.format_type(to)
                )
                .unwrap();
            }
            InstructionData::ArrayAlloc {
                elem,
                dims,
                is_const,
                mem_loc,
                init,
            } => {
                let elem_str = self.format_type(elem);
                let dims_data = &self.ctx.type_dims[*dims];
                let dims_str: Vec<String> = dims_data
                    .iter()
                    .map(|d| match d {
                        Some(size) => size.to_string(),
                        None => "?".to_string(),
                    })
                    .collect();
                let const_str = if *is_const { "const " } else { "" };
                let loc_str = format!("{:?}", mem_loc).to_lowercase();
                write!(
                    s,
                    "array.alloc {}{}[{}] @ {}",
                    const_str,
                    elem_str,
                    dims_str.join("]["),
                    loc_str
                )
                .unwrap();

                match init {
                    ArrayInit::Zero => write!(s, " zero").unwrap(),
                    ArrayInit::List { vals } => {
                        let vals_vec = self.func.dfg.valuevec(*vals);
                        if vals_vec.len() <= 8 {
                            let vals_str: Vec<String> =
                                vals_vec.iter().map(|v| self.format_value(*v)).collect();
                            write!(s, " = [{}]", vals_str.join(", ")).unwrap();
                        } else {
                            write!(s, " = [{} values...]", vals_vec.len()).unwrap();
                        }
                    }
                    ArrayInit::Undef => write!(s, " undef").unwrap(),
                }
            }
            InstructionData::ArrayGet {
                ptr,
                indices,
                token,
            } => {
                let indices_vec = self.func.dfg.valuevec(*indices);
                let indices_str: Vec<String> =
                    indices_vec.iter().map(|v| self.format_value(*v)).collect();
                write!(
                    s,
                    "array.get {}[{}], ({})",
                    self.format_value(*ptr),
                    indices_str.join("]["),
                    self.format_value(*token)
                )
                .unwrap();
            }
            InstructionData::ArrayPut {
                ptr,
                indices,
                value,
                token,
            } => {
                let indices_vec = self.func.dfg.valuevec(*indices);
                let indices_str: Vec<String> =
                    indices_vec.iter().map(|v| self.format_value(*v)).collect();
                write!(
                    s,
                    "array.put {}[{}], {}, ({})",
                    self.format_value(*ptr),
                    indices_str.join("]["),
                    self.format_value(*value),
                    self.format_value(*token)
                )
                .unwrap();
            }
            InstructionData::ArraySlice { ptr, sdims } => {
                let sdims_vec = self.func.dfg.valuevec(*sdims);
                let sdims_str: Vec<String> =
                    sdims_vec.iter().map(|v| self.format_value(*v)).collect();
                write!(
                    s,
                    "array.slice {}[{}]",
                    self.format_value(*ptr),
                    sdims_str.join("][")
                )
                .unwrap();
            }
            InstructionData::GlobalScalarGet { global, token } => {
                write!(
                    s,
                    "global.get {}, ({})",
                    self.format_value(*global),
                    self.format_value(*token)
                )
                .unwrap();
            }
            InstructionData::GlobalScalarSet {
                global,
                value,
                token,
            } => {
                write!(
                    s,
                    "global.set {}, {}, ({})",
                    self.format_value(*global),
                    self.format_value(*value),
                    self.format_value(*token)
                )
                .unwrap();
            }

            // === 低级内存操作 ===
            InstructionData::StackAddr { slot } => {
                write!(s, "stack.addr %slot{}", slot.index()).unwrap();
            }
            InstructionData::GlobalAddr { global } => {
                let name = self.ctx.ref_name(*global);
                write!(s, "global.addr @{}", name).unwrap();
            }
            InstructionData::Load {
                addr,
                offset,
                ty,
                token,
            } => {
                write!(
                    s,
                    "load.{}({}, {}), ({})",
                    self.format_type(ty),
                    self.format_value(*addr),
                    offset,
                    self.format_value(*token)
                )
                .unwrap();
            }
            InstructionData::Store {
                addr,
                offset,
                value,
                ty,
                token,
            } => {
                write!(
                    s,
                    "store.{}({}, {}), {}, ({})",
                    self.format_type(ty),
                    self.format_value(*addr),
                    offset,
                    self.format_value(*value),
                    self.format_value(*token)
                )
                .unwrap();
            }

            InstructionData::Jump { target } => {
                let args = target.args(&self.func.dfg.value_vecs);
                if args.is_empty() {
                    write!(s, "jump bb{}", target.block().index()).unwrap();
                } else {
                    let args_str: Vec<String> =
                        args.iter().map(|v| self.format_value(*v)).collect();
                    write!(
                        s,
                        "jump bb{}({})",
                        target.block().index(),
                        args_str.join(", ")
                    )
                    .unwrap();
                }
            }
            InstructionData::Brif { cond, targets } => {
                let [then_target, else_target] = targets;
                write!(s, "brif {}, ", self.format_value(*cond)).unwrap();

                // Then branch
                let then_args = then_target.args(&self.func.dfg.value_vecs);
                if then_args.is_empty() {
                    write!(s, "bb{}", then_target.block().index()).unwrap();
                } else {
                    let args_str: Vec<String> =
                        then_args.iter().map(|v| self.format_value(*v)).collect();
                    write!(
                        s,
                        "bb{}({})",
                        then_target.block().index(),
                        args_str.join(", ")
                    )
                    .unwrap();
                }

                write!(s, ", ").unwrap();

                // Else branch
                let else_args = else_target.args(&self.func.dfg.value_vecs);
                if else_args.is_empty() {
                    write!(s, "bb{}", else_target.block().index()).unwrap();
                } else {
                    let args_str: Vec<String> =
                        else_args.iter().map(|v| self.format_value(*v)).collect();
                    write!(
                        s,
                        "bb{}({})",
                        else_target.block().index(),
                        args_str.join(", ")
                    )
                    .unwrap();
                }
            }
            InstructionData::Return { returns } => {
                let returns_vec = self.func.dfg.valuevec(*returns);
                if returns_vec.is_empty() {
                    write!(s, "return").unwrap();
                } else {
                    let returns_str: Vec<String> =
                        returns_vec.iter().map(|v| self.format_value(*v)).collect();
                    write!(s, "return {}", returns_str.join(", ")).unwrap();
                }
            }
            InstructionData::ReturnCall { func, args } => {
                let func_name = self.ctx.ref_name(*func);
                let args_vec = self.func.dfg.valuevec(*args);
                let args_str: Vec<String> =
                    args_vec.iter().map(|v| self.format_value(*v)).collect();
                write!(s, "return_call @{}({})", func_name, args_str.join(", ")).unwrap();
            }
            InstructionData::Unreachable => {
                write!(s, "unreachable").unwrap();
            }
            InstructionData::BranchTable { .. } => {
                write!(s, "br_table ...").unwrap(); // TODO: 实现跳转表格式化
            }
        }

        s
    }

    /// 格式化块参数
    fn format_block_params(&self, block: Block) -> String {
        let params = self.func.dfg.block_params(block);
        if params.is_empty() {
            String::new()
        } else {
            let param_strs: Vec<String> = params
                .iter()
                .map(|v| {
                    let ty = self.func.dfg.value_type(*v);
                    format!("{}:{}", self.format_value(*v), self.format_type(&ty))
                })
                .collect();
            format!("({})", param_strs.join(", "))
        }
    }

    fn format_array_init(&self, init: &ArrayInit) -> String {
        match init {
            ArrayInit::Zero => "= zeroinitializer".to_string(),
            ArrayInit::Undef => "= undef".to_string(),
            ArrayInit::List { vals } => {
                let values = &self.func.dfg.value_vecs[*vals];
                if values.is_empty() {
                    "= []".to_string()
                } else {
                    let val_strs: Vec<String> =
                        values.iter().map(|v| self.format_value(*v)).collect();
                    format!("= [{}]", val_strs.join(", "))
                }
            }
        }
    }
}

impl fmt::Display for FunctionFormatter<'_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        // 打印函数签名
        writeln!(
            f,
            "function @{}({}) -> {} {{",
            self.func.name,
            self.format_params(),
            self.format_returns()
        )?;

        // 打印 StackSlot 信息（如果有的话）
        if !self.func.dfg.stack_slots.0.is_empty() {
            writeln!(f)?;
            for (slot, slot_data) in self.func.dfg.stack_slots.0.iter() {
                writeln!(
                    f,
                    "  %slot{}: {} size={} align={} {}{}",
                    slot.index(),
                    self.format_type(&slot_data.elem),
                    slot_data.size,
                    slot_data.align,
                    if slot_data.is_const { "const " } else { "" },
                    self.format_array_init(&slot_data.init)
                )?;
            }
            writeln!(f)?;
        }

        // 收集逆后序块列表
        let blocks_in_order = collect_blocks_in_reverse_postorder(self.func.entry, self.func);

        // 打印每个块
        for &block in &blocks_in_order {
            // 打印块标签和参数
            let block_params = self.format_block_params(block);
            writeln!(f, "bb{}{}:", block.index(), block_params)?;

            // 打印块中的指令（使用Layout API）
            let insts: Vec<_> = self.func.layout.block_insts(block).collect();
            // eprintln!(
            //     "DEBUG: ir_fmt - block {:?} 有 {} 个指令",
            //     block,
            //     insts.len()
            // );
            for inst in insts {
                // eprintln!("DEBUG: ir_fmt - 格式化指令 {:?}", inst);
                writeln!(f, "    {}", self.format_instruction(inst))?;
            }
        }

        writeln!(f, "}}")
    }
}

/// 构建器上下文格式化器
pub struct BuilderContextFormatter<'a> {
    pub ctx: &'a CoreContext,
}

impl<'a> BuilderContextFormatter<'a> {
    pub fn new(ctx: &'a CoreContext) -> Self {
        Self { ctx }
    }

    /// 格式化全局变量
    fn format_global(&self, _name_ref: NameRef, global: &GlobalData) -> String {
        let mut s = String::new();

        // 格式化声明
        let const_str = if global.is_const { "const " } else { "" };
        write!(s, "{}global @{}: ", const_str, global.name).unwrap();

        // 格式化类型
        match global.ty {
            Type::ArrayPtr { elem, dims } => {
                let dims_data = &self.ctx.type_dims[dims];
                let dims_str: Vec<String> = dims_data
                    .iter()
                    .map(|d| match d {
                        Some(size) => size.to_string(),
                        None => "?".to_string(),
                    })
                    .collect();
                write!(
                    s,
                    "{}[{}]",
                    format!("{:?}", elem.to_type()),
                    dims_str.join("][")
                )
                .unwrap();
            }
            _ => {
                write!(s, "{:?}", global.ty).unwrap();
            }
        }

        // 格式化初始值
        match &global.init {
            GlobalInit::Zero => write!(s, " = zeroinitializer").unwrap(),
            GlobalInit::Scalar(val) => write!(s, " = {}", val).unwrap(),
            GlobalInit::Array(values) => {
                if values.len() <= 8 {
                    let vals_str: Vec<String> = values.iter().map(|v| format!("{}", v)).collect();
                    write!(s, " = [{}]", vals_str.join(", ")).unwrap();
                } else {
                    write!(s, " = [{} values...]", values.len()).unwrap();
                }
            }
        }

        s
    }
}

impl fmt::Display for BuilderContextFormatter<'_> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        // 打印全局变量
        if !self.ctx.globals.is_empty() {
            writeln!(f, ";; Global Variables")?;
            for (name_ref, global) in self.ctx.globals.iter() {
                writeln!(f, "{}", self.format_global(name_ref, global))?;
            }
            writeln!(f)?;
        }

        // 打印外部函数声明
        // if !self.ctx.ext_funcs.is_empty() {
        //     writeln!(f, ";; External Functions")?;
        //     for (func_ref, ext_func) in self.ctx.ext_funcs.iter() {
        //         write!(f, "declare @{}(", ext_func.name)?;

        //         // 格式化参数
        //         let mut params = Vec::new();
        //         for (ty, name) in ext_func
        //             .signature
        //             .params
        //             .iter()
        //             .zip(&ext_func.signature.param_names)
        //         {
        //             if let Some(name_ref) = name {
        //                 let name = self.ctx.ref_name(*name_ref);
        //                 params.push(format!("%{}:{:?}", name, ty));
        //             } else {
        //                 params.push(format!("{:?}", ty));
        //             }
        //         }
        //         write!(f, "{}", params.join(", "))?;

        //         // 格式化返回类型
        //         write!(f, ") -> ")?;
        //         if ext_func.signature.returns.is_empty() {
        //             writeln!(f, "()")?;
        //         } else {
        //             let returns: Vec<String> = ext_func
        //                 .signature
        //                 .returns
        //                 .iter()
        //                 .map(|ty| format!("{:?}", ty))
        //                 .collect();
        //             writeln!(f, "({})", returns.join(", "))?;
        //         }
        //     }
        //     writeln!(f)?;
        // }

        Ok(())
    }
}
