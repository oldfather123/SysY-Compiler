//! SSA指令执行器

use super::{
    Environment,
    debugger::Debugger,
    environment::RuntimeValue,
    memory::{MemoryId, RuntimeElement},
};
use crate::prelude::*;
use crate::ssa::{ctx::CoreContext, function::Function, ir::*};
use crate::utils::prelude::*;
use std::io::{BufRead, Write};

/// SSA指令执行器
pub struct Executor<'a> {
    core_context: &'a CoreContext,
    env: &'a mut Environment,
    functions: &'a [Function],
}

impl<'a> Executor<'a> {
    pub fn new(
        core_context: &'a CoreContext,
        env: &'a mut Environment,
        functions: &'a [Function],
    ) -> Self {
        Self {
            core_context,
            env,
            functions,
        }
    }

    /// 执行函数（不带调试器）
    pub fn execute(&mut self, main_func: &Function) -> CEResult<i32> {
        let results = self.execute_function(main_func)?;
        // 取第一个返回值作为程序返回值
        let main_result = results.into_iter().next().unwrap_or(RuntimeValue::Unit);
        Ok(self.runtime_value_to_i32(main_result))
    }

    /// 执行函数（带调试器）
    pub fn execute_with_debugger(
        &mut self,
        main_func: &Function,
        debugger: &mut Debugger,
    ) -> CEResult<i32> {
        debugger.on_start();
        let results = self.execute_function(main_func)?;
        debugger.on_finish();
        // 取第一个返回值作为程序返回值
        let main_result = results.into_iter().next().unwrap_or(RuntimeValue::Unit);
        Ok(self.runtime_value_to_i32(main_result))
    }

    /// 将运行时值转换为i32（用于程序退出码）
    fn runtime_value_to_i32(&self, value: RuntimeValue) -> i32 {
        match value {
            RuntimeValue::Unit => 0,
            RuntimeValue::Bool(b) => {
                if b {
                    1
                } else {
                    0
                }
            }
            RuntimeValue::Int32(n) => n,
            RuntimeValue::Int64(n) => n as i32,
            RuntimeValue::Float32(f) => f as i32,
            RuntimeValue::Float64(f) => f as i32,
            RuntimeValue::Address(a) => a as i32,
            RuntimeValue::ArrayPtr { .. } => 0, // 指针返回0
            RuntimeValue::MemToken { .. } => 0, // 令牌返回0
        }
    }

    /// 执行函数
    fn execute_function(&mut self, function: &Function) -> CEResult<Vec<RuntimeValue>> {
        // 压入调用栈帧
        self.env.push_frame(function.clone());

        // 初始化入口块参数（函数参数）
        self.init_entry_params(function)?;

        // 执行指令
        loop {
            let frame = self
                .env
                .current_frame()
                .ok_or_else(|| CErr::runtime_err("No active call frame"))?;

            let current_block = frame.current_block;
            let next_inst_idx = frame.next_inst;

            // 获取当前块的指令列表
            let insts: Vec<_> = function.layout.block_insts(current_block).collect();

            if next_inst_idx >= insts.len() {
                return Err(CErr::runtime_err("Instruction index out of bounds"));
            }

            let inst = insts[next_inst_idx];
            let inst_data = &function.dfg.insts[inst];

            // 更新统计
            self.env.update_stats(inst_data);

            // 执行指令
            match self.execute_instruction(function, inst, inst_data)? {
                ExecutionResult::Continue => {
                    self.env.current_frame_mut().unwrap().next_inst += 1;
                }
                ExecutionResult::Jump(target_block) => {
                    self.env.current_frame_mut().unwrap().current_block = target_block;
                    self.env.current_frame_mut().unwrap().next_inst = 0;
                }
                ExecutionResult::Return(values) => {
                    self.env.pop_frame();
                    return Ok(values);
                }
            }
        }
    }

    /// 初始化入口块参数
    fn init_entry_params(&mut self, _function: &Function) -> CEResult<()> {
        // TODO: 如果main函数有参数，需要处理
        Ok(())
    }

    /// 执行单条指令
    fn execute_instruction(
        &mut self,
        function: &Function,
        inst: Inst,
        inst_data: &InstructionData,
    ) -> CEResult<ExecutionResult> {
        match inst_data {
            // 算术运算
            InstructionData::Binary { op, lhs, rhs } => {
                self.execute_binary(function, inst, *op, *lhs, *rhs)?;
                Ok(ExecutionResult::Continue)
            }

            InstructionData::Unary { op, val } => {
                self.execute_unary(function, inst, *op, *val)?;
                Ok(ExecutionResult::Continue)
            }

            // 控制流
            InstructionData::Jump { target } => {
                self.execute_jump(function, target)?;
                Ok(ExecutionResult::Jump(target.block()))
            }

            InstructionData::Brif { cond, targets } => {
                let target = self.execute_brif(function, *cond, targets)?;
                Ok(ExecutionResult::Jump(target))
            }

            InstructionData::BranchTable { idx: cond, table } => {
                let target = self.execute_branch_table(function, *cond, *table)?;
                Ok(ExecutionResult::Jump(target))
            }

            InstructionData::Return { returns } => {
                let values = self.execute_return(function, returns)?;
                Ok(ExecutionResult::Return(values))
            }

            InstructionData::ReturnCall { func, args } => {
                // 尾调用：执行函数调用并立即返回其结果
                // 注意：这是一个简化的实现，真正的尾调用优化需要在汇编层面处理
                let values = self.execute_return_call(function, *func, args)?;
                Ok(ExecutionResult::Return(values))
            }

            InstructionData::Unreachable => {
                Err(CErr::runtime_err("Unreachable instruction executed"))
            }

            // 函数调用
            InstructionData::Call { func, args } => {
                self.execute_call(function, inst, *func, args)?;
                Ok(ExecutionResult::Continue)
            }

            // 类型转换
            InstructionData::Cast { val, to } => {
                self.execute_cast(function, inst, *val, *to)?;
                Ok(ExecutionResult::Continue)
            }

            // 内存操作
            InstructionData::ArrayAlloc {
                elem,
                dims,
                is_const,
                mem_loc,
                init,
            } => {
                self.execute_array_alloc(function, inst, *elem, dims, *is_const, mem_loc, init)?;
                Ok(ExecutionResult::Continue)
            }

            InstructionData::ArrayGet {
                ptr,
                indices,
                token,
            } => {
                self.execute_array_get(function, inst, *ptr, indices, *token)?;
                Ok(ExecutionResult::Continue)
            }

            InstructionData::ArrayPut {
                ptr,
                indices,
                value,
                token,
            } => {
                self.execute_array_put(function, inst, *ptr, indices, *value, *token)?;
                Ok(ExecutionResult::Continue)
            }

            InstructionData::ArraySlice { ptr, sdims } => {
                self.execute_array_slice(function, inst, *ptr, sdims)?;
                Ok(ExecutionResult::Continue)
            }

            // 全局变量操作
            InstructionData::GlobalScalarGet { global, token } => {
                self.execute_global_scalar_get(function, inst, *global, *token)?;
                Ok(ExecutionResult::Continue)
            }

            InstructionData::GlobalScalarSet {
                global,
                value,
                token,
            } => {
                self.execute_global_scalar_set(function, inst, *global, *value, *token)?;
                Ok(ExecutionResult::Continue)
            }

            // === 低级内存操作 ===
            InstructionData::StackAddr { slot } => {
                self.execute_stack_addr(function, inst, *slot)?;
                Ok(ExecutionResult::Continue)
            }
            InstructionData::GlobalAddr { global } => {
                self.execute_global_addr(function, inst, *global)?;
                Ok(ExecutionResult::Continue)
            }
            InstructionData::Load {
                addr,
                offset,
                ty,
                token,
            } => {
                self.execute_load(function, inst, *addr, *offset, *ty, *token)?;
                Ok(ExecutionResult::Continue)
            }
            InstructionData::Store {
                addr,
                offset,
                value,
                ty,
                token,
            } => {
                self.execute_store(function, inst, *addr, *offset, *value, *token)?;
                Ok(ExecutionResult::Continue)
            }
        }
    }

    /// 执行二元运算
    fn execute_binary(
        &mut self,
        function: &Function,
        inst: Inst,
        op: BinOp,
        lhs: Value,
        rhs: Value,
    ) -> CEResult<()> {
        let lhs_val = self.env.get_value(lhs, &function.dfg)?;
        let rhs_val = self.env.get_value(rhs, &function.dfg)?;

        let result = match (op, &lhs_val, &rhs_val) {
            // 整数运算
            (BinOp::IAdd, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Int32(a.wrapping_add(*b))
            }
            (BinOp::ISub, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Int32(a.wrapping_sub(*b))
            }
            (BinOp::IMul, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Int32(a.wrapping_mul(*b))
            }
            (BinOp::IDiv, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                if *b == 0 {
                    return Err(CErr::runtime_err("Division by zero"));
                }
                RuntimeValue::Int32(a / b)
            }
            (BinOp::IMod, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                if *b == 0 {
                    return Err(CErr::runtime_err("Division by zero"));
                }
                RuntimeValue::Int32(a % b)
            }

            // 64位整数运算
            (BinOp::IAdd, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Int64(a.wrapping_add(*b))
            }
            (BinOp::ISub, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Int64(a.wrapping_sub(*b))
            }
            (BinOp::IMul, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Int64(a.wrapping_mul(*b))
            }
            (BinOp::IDiv, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                if *b == 0 {
                    return Err(CErr::runtime_err("Division by zero"));
                }
                RuntimeValue::Int64(a / b)
            }
            (BinOp::IMod, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                if *b == 0 {
                    return Err(CErr::runtime_err("Division by zero"));
                }
                RuntimeValue::Int64(a % b)
            }

            // 浮点运算
            (BinOp::FAdd, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Float32(a + b)
            }
            (BinOp::FSub, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Float32(a - b)
            }
            (BinOp::FMul, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Float32(a * b)
            }
            (BinOp::FDiv, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Float32(a / b)
            }

            // 比较运算（整数）
            (BinOp::Eq, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(a == b)
            }
            (BinOp::Ne, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(a != b)
            }
            (BinOp::Lt, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(a < b)
            }
            (BinOp::Le, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(a <= b)
            }
            (BinOp::Gt, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(a > b)
            }
            (BinOp::Ge, RuntimeValue::Int32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(a >= b)
            }

            // 比较运算（64位整数）
            (BinOp::Eq, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Bool(a == b)
            }
            (BinOp::Ne, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Bool(a != b)
            }
            (BinOp::Lt, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Bool(a < b)
            }
            (BinOp::Le, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Bool(a <= b)
            }
            (BinOp::Gt, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Bool(a > b)
            }
            (BinOp::Ge, RuntimeValue::Int64(a), RuntimeValue::Int64(b)) => {
                RuntimeValue::Bool(a >= b)
            }

            // 比较运算（浮点）
            (BinOp::Eq, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool((a - b).abs() < f32::EPSILON)
            }
            (BinOp::Ne, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool((a - b).abs() >= f32::EPSILON)
            }
            (BinOp::Lt, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool(a < b)
            }
            (BinOp::Le, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool(a <= b)
            }
            (BinOp::Gt, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool(a > b)
            }
            (BinOp::Ge, RuntimeValue::Float32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool(a >= b)
            }

            // 混合类型比较（Float32 vs Int32）- 临时修复SSA层未插入类型转换的问题
            (BinOp::Lt, RuntimeValue::Float32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(*a < *b as f32)
            }
            (BinOp::Le, RuntimeValue::Float32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(*a <= *b as f32)
            }
            (BinOp::Gt, RuntimeValue::Float32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(*a > *b as f32)
            }
            (BinOp::Ge, RuntimeValue::Float32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool(*a >= *b as f32)
            }
            (BinOp::Eq, RuntimeValue::Float32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool((*a - *b as f32).abs() < f32::EPSILON)
            }
            (BinOp::Ne, RuntimeValue::Float32(a), RuntimeValue::Int32(b)) => {
                RuntimeValue::Bool((*a - *b as f32).abs() >= f32::EPSILON)
            }
            // 反向混合类型比较（Int32 vs Float32）
            (BinOp::Lt, RuntimeValue::Int32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool((*a as f32) < *b)
            }
            (BinOp::Le, RuntimeValue::Int32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool((*a as f32) <= *b)
            }
            (BinOp::Gt, RuntimeValue::Int32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool((*a as f32) > *b)
            }
            (BinOp::Ge, RuntimeValue::Int32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool((*a as f32) >= *b)
            }
            (BinOp::Eq, RuntimeValue::Int32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool(((*a as f32) - *b).abs() < f32::EPSILON)
            }
            (BinOp::Ne, RuntimeValue::Int32(a), RuntimeValue::Float32(b)) => {
                RuntimeValue::Bool(((*a as f32) - *b).abs() >= f32::EPSILON)
            }
            // 删除混合类型逻辑运算 - 现在由SSA层的显式cast处理
            _ => {
                return Err(CErr::runtime_err(format!(
                    "Type mismatch in binary operation {:?}: {:?} vs {:?}",
                    op, lhs_val, rhs_val
                )));
            }
        };

        let result_value = function.dfg.inst_result(inst, 0);
        self.env.set_value(result_value, result)?;
        Ok(())
    }

    /// 执行一元运算
    fn execute_unary(
        &mut self,
        function: &Function,
        inst: Inst,
        op: UnaryOp,
        val: Value,
    ) -> CEResult<()> {
        let operand = self.env.get_value(val, &function.dfg)?;

        let result = match (op, &operand) {
            (UnaryOp::INeg, RuntimeValue::Int32(n)) => RuntimeValue::Int32(-n),
            (UnaryOp::BNot, RuntimeValue::Int32(n)) => RuntimeValue::Int32(!n), // 按位取反
            (UnaryOp::FNeg, RuntimeValue::Float32(f)) => RuntimeValue::Float32(-f),
            (UnaryOp::Not, RuntimeValue::Bool(b)) => RuntimeValue::Bool(!b),
            // 删除混合类型Not运算纵容 - 现在由SSA层的显式cast处理
            _ => {
                return Err(CErr::runtime_err(format!(
                    "Type mismatch in unary operation {:?}",
                    op
                )));
            }
        };

        let result_value = function.dfg.inst_result(inst, 0);
        self.env.set_value(result_value, result)?;
        Ok(())
    }

    /// 执行跳转
    fn execute_jump(&mut self, function: &Function, target: &BlockCall) -> CEResult<()> {
        // 设置块参数
        let args = target.args(&function.dfg.value_vecs);
        for (idx, arg) in args.iter().enumerate() {
            let value = self.env.get_value(*arg, &function.dfg)?;
            self.env
                .current_frame_mut()
                .unwrap()
                .block_params
                .insert((target.block(), idx as u32), value);
        }
        Ok(())
    }

    /// 执行条件分支
    fn execute_brif(
        &mut self,
        function: &Function,
        cond: Value,
        targets: &[BlockCall; 2],
    ) -> CEResult<Block> {
        let cond_val = self.env.get_value(cond, &function.dfg)?;

        let (target_idx, target) = match cond_val {
            RuntimeValue::Bool(true) => (0, &targets[0]),
            RuntimeValue::Bool(false) => (1, &targets[1]),
            // 删除混合类型条件纵容 - 现在由SSA层的显式convert_to_bool处理
            _ => return Err(CErr::runtime_err("Condition must be bool type")),
        };

        // 设置块参数
        let args = target.args(&function.dfg.value_vecs);
        for (idx, arg) in args.iter().enumerate() {
            let value = self.env.get_value(*arg, &function.dfg)?;
            self.env
                .current_frame_mut()
                .unwrap()
                .block_params
                .insert((target.block(), idx as u32), value);
        }

        Ok(target.block())
    }

    /// 执行跳转表
    fn execute_branch_table(
        &mut self,
        function: &Function,
        cond: Value,
        table: JumpTable,
    ) -> CEResult<Block> {
        let cond_val = self.env.get_value(cond, &function.dfg)?;

        let index = match cond_val {
            RuntimeValue::Int32(i) => i as usize,
            _ => {
                return Err(CErr::runtime_err("Branch table condition must be integer"));
            }
        };

        let branches = function.dfg.jump_tables[table].all_branches();
        let target = if index < branches.len() {
            &branches[index]
        } else {
            &branches[branches.len() - 1] // 默认分支
        };

        // 设置块参数
        let args = target.args(&function.dfg.value_vecs);
        for (idx, arg) in args.iter().enumerate() {
            let value = self.env.get_value(*arg, &function.dfg)?;
            self.env
                .current_frame_mut()
                .unwrap()
                .block_params
                .insert((target.block(), idx as u32), value);
        }

        Ok(target.block())
    }

    /// 执行返回
    fn execute_return(
        &mut self,
        function: &Function,
        returns: &ValueVec,
    ) -> CEResult<Vec<RuntimeValue>> {
        let values = function.dfg.value_vecs[*returns].as_slice();

        if values.is_empty() {
            Ok(vec![RuntimeValue::Unit])
        } else {
            // 返回所有返回值
            let mut results = Vec::new();
            for value in values {
                results.push(self.env.get_value(*value, &function.dfg)?);
            }
            Ok(results)
        }
    }

    /// 执行函数调用
    fn execute_call(
        &mut self,
        function: &Function,
        inst: Inst,
        func: FuncRef,
        args: &ValueVec,
    ) -> CEResult<()> {
        let func_name = &self.core_context.names[func];

        // 获取参数值
        let arg_values: Vec<RuntimeValue> = function.dfg.value_vecs[*args]
            .iter()
            .map(|arg| self.env.get_value(*arg, &function.dfg))
            .collect::<Result<Vec<_>, _>>()?;

        // 处理运行时函数
        match func_name.as_str() {
            "getint" => {
                let value = self.runtime_getint()?;
                let result_value = function.dfg.inst_result(inst, 0);
                self.env
                    .set_value(result_value, RuntimeValue::Int32(value))?;
            }
            "putint" => {
                if let RuntimeValue::Int32(n) = &arg_values[0] {
                    self.runtime_putint(*n)?;
                } else {
                    eprintln!("putint error: expected Int32, got {:?}", arg_values[0]);
                    return Err(CErr::runtime_err(format!(
                        "putint: expected Int32, got {:?}",
                        arg_values[0]
                    )));
                }
            }
            "putch" => {
                if let RuntimeValue::Int32(n) = &arg_values[0] {
                    self.runtime_putch(*n)?;
                }
            }
            "getfloat" => {
                let value = self.runtime_getfloat()?;
                let result_value = function.dfg.inst_result(inst, 0);
                self.env
                    .set_value(result_value, RuntimeValue::Float32(value))?;
            }
            "putfloat" => {
                if let RuntimeValue::Float32(f) = &arg_values[0] {
                    self.runtime_putfloat(*f)?;
                } else {
                    // 自动类型转换：将Int32转换为Float32
                    match &arg_values[0] {
                        RuntimeValue::Int32(i) => {
                            self.runtime_putfloat(*i as f32)?;
                        }
                        _ => {
                            eprintln!("putfloat error: expected Float32, got {:?}", arg_values[0]);
                            return Err(CErr::runtime_err(format!(
                                "putfloat: expected Float32, got {:?}",
                                arg_values[0]
                            )));
                        }
                    }
                }
            }
            "getch" => {
                let value = self.runtime_getch()?;
                let result_value = function.dfg.inst_result(inst, 0);
                self.env
                    .set_value(result_value, RuntimeValue::Int32(value))?;
            }
            "getarray" => {
                // getarray(int a[]) -> int: 读取数组元素，返回数组大小

                if arg_values.len() >= 2 {
                    if let RuntimeValue::MemToken { object, version } = &arg_values[1] {
                        // Handle both ArrayPtr (original) and Int64 (after address lowering)
                        let n = match &arg_values[0] {
                            ptr_data @ RuntimeValue::ArrayPtr { .. } => {
                                self.runtime_getarray(ptr_data)?
                            }
                            RuntimeValue::Int64(addr) => {
                                // Handle raw address from address lowering pass
                                self.runtime_getarray_from_addr(*addr, object)?
                            }
                            _ => {
                                eprintln!(
                                    "DEBUG: getarray unexpected first argument type: {:?}",
                                    arg_values[0]
                                );
                                return Err(CErr::runtime_err(
                                    "getarray: first argument must be ArrayPtr or Int64",
                                ));
                            }
                        };

                        let result1 = function.dfg.inst_result(inst, 0); // 返回数组大小
                        let result2 = function.dfg.inst_result(inst, 1); // 返回新的内存令牌
                        self.env.set_value(result1, RuntimeValue::Int32(n))?;
                        // 返回更新版本的内存令牌
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: version + 1,
                        };
                        self.env.set_value(result2, new_token)?;
                    } else {
                        return Err(CErr::runtime_err(
                            "getarray: second argument must be MemToken",
                        ));
                    }
                } else {
                    return Err(CErr::runtime_err("getarray: insufficient arguments"));
                }
            }
            "getfarray" => {
                // getfarray(float a[]) -> int: 读取浮点数组元素，返回数组大小
                if arg_values.len() >= 2 {
                    if let RuntimeValue::MemToken { object, version } = &arg_values[1] {
                        // Handle both ArrayPtr (original) and Int64 (after address lowering)
                        let (n, final_version) = match &arg_values[0] {
                            ptr_data @ RuntimeValue::ArrayPtr { .. } => {
                                let n = self.runtime_getfarray(ptr_data)?;
                                // Get the final version from the memory manager
                                let final_version = self
                                    .env
                                    .memory
                                    .get_memory_version_by_object(object)
                                    .map(|(_, v)| v)
                                    .unwrap_or(*version);
                                (n, final_version)
                            }
                            RuntimeValue::Int64(addr) => {
                                // Handle raw address from address lowering pass
                                let n = self.runtime_getfarray_from_addr(*addr, object)?;
                                // Get the final version from the memory manager
                                let final_version = self
                                    .env
                                    .memory
                                    .get_memory_version_by_object(object)
                                    .map(|(_, v)| v)
                                    .unwrap_or(*version);
                                (n, final_version)
                            }
                            _ => {
                                return Err(CErr::runtime_err(
                                    "getfarray: first argument must be ArrayPtr or Int64",
                                ));
                            }
                        };

                        let result1 = function.dfg.inst_result(inst, 0); // 返回数组大小
                        let result2 = function.dfg.inst_result(inst, 1); // 返回新的内存令牌
                        self.env.set_value(result1, RuntimeValue::Int32(n))?;
                        // 返回更新版本的内存令牌
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: final_version,
                        };
                        self.env.set_value(result2, new_token)?;
                    } else {
                        return Err(CErr::runtime_err(
                            "getfarray: second argument must be MemToken",
                        ));
                    }
                } else {
                    return Err(CErr::runtime_err("getfarray: insufficient arguments"));
                }
            }
            "putarray" => {
                // putarray(int n, int a[]): 输出数组
                if arg_values.len() >= 3 {
                    if let (RuntimeValue::Int32(n), RuntimeValue::MemToken { object, version }) =
                        (&arg_values[0], &arg_values[2])
                    {
                        // Handle both ArrayPtr (original) and Int64 (after address lowering)
                        match &arg_values[1] {
                            ptr_data @ RuntimeValue::ArrayPtr { .. } => {
                                self.runtime_putarray(*n, ptr_data)?;
                            }
                            RuntimeValue::Int64(addr) => {
                                // Handle raw address from address lowering pass
                                self.runtime_putarray_from_addr(*n, *addr, object)?;
                            }
                            _ => {
                                return Err(CErr::runtime_err(
                                    "putarray: second argument must be ArrayPtr or Int64",
                                ));
                            }
                        }

                        let result_value = function.dfg.inst_result(inst, 0); // 返回新的内存令牌
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: *version, // putarray是只读操作，不增加版本
                        };
                        self.env.set_value(result_value, new_token)?;
                    } else {
                        return Err(CErr::runtime_err("putarray: invalid argument types"));
                    }
                } else {
                    return Err(CErr::runtime_err("putarray: insufficient arguments"));
                }
            }
            "putfarray" => {
                // putfarray(int n, float a[]): 输出浮点数组
                if arg_values.len() >= 3 {
                    if let (RuntimeValue::Int32(n), RuntimeValue::MemToken { object, version }) =
                        (&arg_values[0], &arg_values[2])
                    {
                        // Handle both ArrayPtr (original) and Int64 (after address lowering)
                        match &arg_values[1] {
                            ptr_data @ RuntimeValue::ArrayPtr { .. } => {
                                self.runtime_putfarray(*n, ptr_data)?;
                            }
                            RuntimeValue::Int64(addr) => {
                                // Handle raw address from address lowering pass
                                self.runtime_putfarray_from_addr(*n, *addr, object)?;
                            }
                            _ => {
                                return Err(CErr::runtime_err(
                                    "putfarray: second argument must be ArrayPtr or Int64",
                                ));
                            }
                        }

                        let result_value = function.dfg.inst_result(inst, 0); // 返回新的内存令牌
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: *version, // putfarray是只读操作，不增加版本
                        };
                        self.env.set_value(result_value, new_token)?;
                    } else {
                        return Err(CErr::runtime_err("putfarray: invalid argument types"));
                    }
                } else {
                    return Err(CErr::runtime_err("putfarray: insufficient arguments"));
                }
            }
            "putf" => {
                // putf(char a[], ...): 格式化输出
                if arg_values.len() >= 2 {
                    if let (
                        ptr_data @ RuntimeValue::ArrayPtr { .. },
                        RuntimeValue::MemToken { object, version },
                    ) = (&arg_values[0], &arg_values[1])
                    {
                        self.runtime_putf(ptr_data)?;
                        let result_value = function.dfg.inst_result(inst, 0); // 返回新的内存令牌
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: *version, // putf是只读操作，不增加版本
                        };
                        self.env.set_value(result_value, new_token)?;
                    }
                }
            }
            "starttime" => {
                self.runtime_starttime()?;
            }
            "stoptime" => {
                self.runtime_stoptime()?;
            }
            "_sysy_starttime" => {
                if let RuntimeValue::Int32(lineno) = &arg_values[0] {
                    self.runtime_sysy_starttime(*lineno)?;
                }
            }
            "_sysy_stoptime" => {
                if let RuntimeValue::Int32(lineno) = &arg_values[0] {
                    self.runtime_sysy_stoptime(*lineno)?;
                }
            }
            _ => {
                // 查找用户定义函数
                if let Some(target_func) =
                    self.functions.iter().find(|f| f.name == func_name.as_str())
                {
                    // 调用用户定义函数
                    let results =
                        self.call_user_function(function, inst, target_func, &arg_values)?;

                    // 设置所有返回值（仅当函数有返回值时）
                    // 检查是否是Unit返回类型的函数
                    if results.len() == 1 && matches!(results[0], RuntimeValue::Unit) {
                        // Unit返回类型的函数不需要设置返回值
                    } else {
                        // 获取指令实际声明的结果数量
                        let declared_results = function.dfg.inst_results(inst).len();
                        let actual_results = results.len();

                        // 只设置指令声明的结果数量，避免类型不匹配
                        for (idx, result_value) in
                            results.into_iter().take(declared_results).enumerate()
                        {
                            let value_key = function.dfg.inst_result(inst, idx as u32);
                            self.env.set_value(value_key, result_value)?;
                        }

                        // 如果实际返回值多于声明的结果，发出警告但不crash
                        if actual_results > declared_results {
                            eprintln!(
                                "Warning: Function {} returned {} values but only {} were expected",
                                func_name, actual_results, declared_results
                            );
                        }
                    }
                } else {
                    return Err(CErr::runtime_err(format!(
                        "Function not found: {}",
                        func_name
                    )));
                }
            }
        }

        Ok(())
    }

    /// 调用用户定义函数
    fn call_user_function(
        &mut self,
        _caller_function: &Function,
        _inst: Inst,
        target_func: &Function,
        args: &[RuntimeValue],
    ) -> CEResult<Vec<RuntimeValue>> {
        // 压入新的调用栈帧
        self.env.push_frame(target_func.clone());

        // 设置函数参数（作为入口块的块参数）
        let entry_block = target_func.entry;
        let entry_params = target_func.dfg.block_params(entry_block);

        // 验证参数数量匹配
        if args.len() != entry_params.len() {
            return Err(CErr::runtime_err(format!(
                "Function {} expects {} arguments, got {}",
                target_func.name,
                entry_params.len(),
                args.len()
            )));
        }

        // 设置入口块参数值
        for (param_value, arg_value) in entry_params.iter().zip(args.iter()) {
            // 从param_value中提取实际的块参数索引
            if let ValueData::BlockParam { idx: param_idx, .. } =
                &target_func.dfg.valuedata(*param_value)
            {
                self.env
                    .current_frame_mut()
                    .unwrap()
                    .block_params
                    .insert((entry_block, *param_idx), arg_value.clone());
            }
        }

        // 执行函数
        let results = self.execute_function_recursive(target_func)?;

        // 弹出调用栈帧
        self.env.pop_frame();

        Ok(results)
    }

    /// 递归执行函数（避免与主execute方法混淆）
    fn execute_function_recursive(&mut self, function: &Function) -> CEResult<Vec<RuntimeValue>> {
        // 执行指令循环（与execute_function类似，但用于递归调用）
        loop {
            let frame = self
                .env
                .current_frame()
                .ok_or_else(|| CErr::runtime_err("No active call frame"))?;

            let current_block = frame.current_block;
            let next_inst_idx = frame.next_inst;

            // 获取当前块的指令列表
            let insts: Vec<_> = function.layout.block_insts(current_block).collect();

            if next_inst_idx >= insts.len() {
                return Err(CErr::runtime_err("Instruction index out of bounds"));
            }

            let inst = insts[next_inst_idx];
            let inst_data = &function.dfg.insts[inst];

            // 更新统计
            self.env.update_stats(inst_data);

            // 执行指令
            match self.execute_instruction(function, inst, inst_data)? {
                ExecutionResult::Continue => {
                    self.env.current_frame_mut().unwrap().next_inst += 1;
                }
                ExecutionResult::Jump(target_block) => {
                    self.env.current_frame_mut().unwrap().current_block = target_block;
                    self.env.current_frame_mut().unwrap().next_inst = 0;
                }
                ExecutionResult::Return(values) => {
                    return Ok(values);
                }
            }
        }
    }

    /// 执行类型转换
    fn execute_cast(
        &mut self,
        function: &Function,
        inst: Inst,
        val: Value,
        to: Type,
    ) -> CEResult<()> {
        let from_val = self.env.get_value(val, &function.dfg)?;

        let result = match (&from_val, to) {
            // 相同类型转换（无操作）
            (RuntimeValue::Int32(_), Type::Int32) => from_val.clone(),
            (RuntimeValue::Int64(_), Type::Int64) => from_val.clone(),
            (RuntimeValue::Float32(_), Type::Float32) => from_val.clone(),
            (RuntimeValue::Float64(_), Type::Float64) => from_val.clone(),
            (RuntimeValue::Bool(_), Type::Bool) => from_val.clone(),
            // 整数类型转换
            (RuntimeValue::Int32(i), Type::Int64) => RuntimeValue::Int64(*i as i64),
            (RuntimeValue::Int64(i), Type::Int32) => RuntimeValue::Int32(*i as i32),
            // 浮点类型转换
            (RuntimeValue::Float32(f), Type::Float64) => RuntimeValue::Float64(*f as f64),
            (RuntimeValue::Float64(f), Type::Float32) => RuntimeValue::Float32(*f as f32),
            // 整数到浮点转换
            (RuntimeValue::Int32(i), Type::Float32) => RuntimeValue::Float32(*i as f32),
            (RuntimeValue::Int32(i), Type::Float64) => RuntimeValue::Float64(*i as f64),
            (RuntimeValue::Int64(i), Type::Float32) => RuntimeValue::Float32(*i as f32),
            (RuntimeValue::Int64(i), Type::Float64) => RuntimeValue::Float64(*i as f64),
            // 浮点到整数转换
            (RuntimeValue::Float32(f), Type::Int32) => RuntimeValue::Int32(*f as i32),
            (RuntimeValue::Float32(f), Type::Int64) => RuntimeValue::Int64(*f as i64),
            (RuntimeValue::Float64(f), Type::Int32) => RuntimeValue::Int32(*f as i32),
            (RuntimeValue::Float64(f), Type::Int64) => RuntimeValue::Int64(*f as i64),
            // 布尔转换
            (RuntimeValue::Bool(b), Type::Int32) => RuntimeValue::Int32(if *b { 1 } else { 0 }),
            (RuntimeValue::Bool(b), Type::Int64) => RuntimeValue::Int64(if *b { 1 } else { 0 }),
            (RuntimeValue::Bool(b), Type::Float32) => {
                RuntimeValue::Float32(if *b { 1.0 } else { 0.0 })
            }
            (RuntimeValue::Bool(b), Type::Float64) => {
                RuntimeValue::Float64(if *b { 1.0 } else { 0.0 })
            }
            // 到布尔转换
            (RuntimeValue::Int32(i), Type::Bool) => RuntimeValue::Bool(*i != 0),
            (RuntimeValue::Int64(i), Type::Bool) => RuntimeValue::Bool(*i != 0),
            (RuntimeValue::Float32(f), Type::Bool) => RuntimeValue::Bool(*f != 0.0),
            (RuntimeValue::Float64(f), Type::Bool) => RuntimeValue::Bool(*f != 0.0),
            _ => {
                return Err(CErr::runtime_err(format!(
                    "Invalid cast from {:?} to {:?}",
                    from_val, to
                )));
            }
        };

        let result_value = function.dfg.inst_result(inst, 0);
        self.env.set_value(result_value, result)?;
        Ok(())
    }

    /// 执行数组分配
    fn execute_array_alloc(
        &mut self,
        function: &Function,
        inst: Inst,
        elem: Type,
        dims: &U32OptVec,
        is_const: bool,
        mem_loc: &MemoryLocation,
        init: &ArrayInit,
    ) -> CEResult<()> {
        // 计算维度
        let concrete_dims: Vec<u32> = self.core_context.type_dims[*dims]
            .iter()
            .map(|d| d.ok_or_else(|| CErr::runtime_err("Array dimension must be constant")))
            .collect::<Result<Vec<_>, _>>()?;

        // 获取元素类型
        let elem_type = match elem {
            Type::Bool => ArrayElemType::Bool,
            Type::Int32 => ArrayElemType::Int32,
            Type::Float32 => ArrayElemType::Float32,
            _ => return Err(CErr::runtime_err("Invalid array element type")),
        };

        // 创建内存对象
        let mem_obj = MemoryObject::ArrayAlloc { inst };

        // 分配内存
        let memory_id = match init {
            ArrayInit::Zero => {
                self.env
                    .memory
                    .allocate(mem_obj, elem_type, &concrete_dims, None)?
            }
            ArrayInit::List { vals } => {
                // 获取初始化值
                let init_values = function.dfg.value_vecs[*vals]
                    .iter()
                    .map(|v| {
                        let val = self.env.get_value(*v, &function.dfg)?;
                        match val {
                            RuntimeValue::Bool(b) => Ok(RuntimeElement::Bool(b)),
                            RuntimeValue::Int32(i) => Ok(RuntimeElement::Int32(i)),
                            RuntimeValue::Float32(f) => Ok(RuntimeElement::Float32(f)),
                            _ => Err(CErr::runtime_err("Invalid array init value")),
                        }
                    })
                    .collect::<Result<Vec<_>, _>>()?;

                self.env
                    .memory
                    .allocate(mem_obj, elem_type, &concrete_dims, Some(&init_values))?
            }
            ArrayInit::Undef => {
                self.env
                    .memory
                    .allocate(mem_obj, elem_type, &concrete_dims, None)?
            }
        };

        // 创建指针值
        let ptr_value = RuntimeValue::ArrayPtr {
            memory_id,
            elem_type,
            dims: concrete_dims,
            base_indices: Vec::new(), // 新分配的数组没有基础索引
        };

        // 创建内存令牌
        let token_value = RuntimeValue::MemToken {
            object: mem_obj,
            version: 0,
        };

        // 存储结果（多返回值）
        // 使用正确的方式创建多返回值
        // ArrayAlloc返回两个值：指针和令牌
        // 这需要特殊处理，因为一条指令有多个结果
        let ptr_result = function.dfg.inst_result(inst, 0);
        let token_result = function.dfg.inst_result(inst, 1);
        self.env.set_value(ptr_result, ptr_value)?;
        self.env.set_value(token_result, token_value)?;
        // 对于第二个返回值，需要创建一个特殊的Value
        // 这里简化处理，可能需要更复杂的方案

        Ok(())
    }

    /// 执行数组读取
    fn execute_array_get(
        &mut self,
        function: &Function,
        inst: Inst,
        ptr: Value,
        indices: &ValueVec,
        token: Value,
    ) -> CEResult<()> {
        let ptr_val = self.env.get_value(ptr, &function.dfg)?;
        let token_val = self.env.get_value(token, &function.dfg)?;

        // 提取内存ID、基础索引和令牌版本
        let (memory_id, base_indices, expected_version) = match (&ptr_val, &token_val) {
            (
                RuntimeValue::ArrayPtr {
                    memory_id,
                    base_indices,
                    ..
                },
                RuntimeValue::MemToken { version, .. },
            ) => (*memory_id, base_indices.clone(), *version),
            (RuntimeValue::Int64(addr), RuntimeValue::MemToken { object, version }) => {
                // Handle address lowering case - find memory_id from the token's object
                let mem_id = self
                    .env
                    .memory
                    .get_memory_version_by_object(object)
                    .map(|(id, _)| id)
                    .ok_or_else(|| CErr::runtime_err("Array memory object not found"))?;
                (mem_id, vec![], *version)
            }
            _ => {
                return Err(CErr::runtime_err("Invalid array get operands"));
            }
        };

        // 获取访问索引值
        let access_indices: Vec<u32> = function.dfg.value_vecs[*indices]
            .iter()
            .map(|idx| match self.env.get_value(*idx, &function.dfg)? {
                RuntimeValue::Int32(i) => {
                    if i < 0 {
                        return Err(CErr::runtime_err(format!(
                            "Array index cannot be negative: {}",
                            i
                        )));
                    }
                    Ok(i as u32)
                }
                _ => Err(CErr::runtime_err("Array index must be integer")),
            })
            .collect::<Result<Vec<_>, _>>()?;

        // 结合基础索引和访问索引
        let mut full_indices = base_indices;
        full_indices.extend_from_slice(&access_indices);

        // 读取内存
        let elem = self
            .env
            .memory
            .load(memory_id, &full_indices, expected_version)?;

        // 转换为运行时值
        let result = match elem {
            RuntimeElement::Bool(b) => RuntimeValue::Bool(b),
            RuntimeElement::Int32(i) => RuntimeValue::Int32(i),
            RuntimeElement::Float32(f) => RuntimeValue::Float32(f),
        };

        let result_value = function.dfg.inst_result(inst, 0);
        self.env.set_value(result_value, result)?;
        Ok(())
    }

    /// 执行数组写入
    fn execute_array_put(
        &mut self,
        function: &Function,
        inst: Inst,
        ptr: Value,
        indices: &ValueVec,
        value: Value,
        token: Value,
    ) -> CEResult<()> {
        let ptr_val = self.env.get_value(ptr, &function.dfg)?;
        let token_val = self.env.get_value(token, &function.dfg)?;
        let value_to_store = self.env.get_value(value, &function.dfg)?;

        // 提取内存ID、基础索引、对象和令牌版本
        let (memory_id, base_indices, mem_obj, expected_version) = match (&ptr_val, &token_val) {
            (
                RuntimeValue::ArrayPtr {
                    memory_id,
                    base_indices,
                    ..
                },
                RuntimeValue::MemToken { object, version },
            ) => (*memory_id, base_indices.clone(), *object, *version),
            (RuntimeValue::Int64(addr), RuntimeValue::MemToken { object, version }) => {
                // Handle address lowering case - find memory_id from the token's object
                let mem_id = self
                    .env
                    .memory
                    .get_memory_version_by_object(object)
                    .map(|(id, _)| id)
                    .ok_or_else(|| CErr::runtime_err("Array memory object not found"))?;
                (mem_id, vec![], *object, *version)
            }
            _ => return Err(CErr::runtime_err("Invalid array put operands")),
        };

        // 获取访问索引值
        let access_indices: Vec<u32> = function.dfg.value_vecs[*indices]
            .iter()
            .map(|idx| match self.env.get_value(*idx, &function.dfg)? {
                RuntimeValue::Int32(i) => {
                    if i < 0 {
                        return Err(CErr::runtime_err(format!(
                            "Array index cannot be negative: {}",
                            i
                        )));
                    }
                    Ok(i as u32)
                }
                _ => Err(CErr::runtime_err("Array index must be integer")),
            })
            .collect::<Result<Vec<_>, _>>()?;

        // 结合基础索引和访问索引
        let mut full_indices = base_indices;
        full_indices.extend_from_slice(&access_indices);

        // 转换要存储的值
        let elem_to_store = match value_to_store {
            RuntimeValue::Bool(b) => RuntimeElement::Bool(b),
            RuntimeValue::Int32(i) => RuntimeElement::Int32(i),
            RuntimeValue::Float32(f) => RuntimeElement::Float32(f),
            _ => {
                return Err(CErr::runtime_err("Invalid value type for array put"));
            }
        };

        // 写入内存
        let new_version =
            self.env
                .memory
                .store(memory_id, &full_indices, elem_to_store, expected_version)?;

        // 创建新的内存令牌
        let new_token = RuntimeValue::MemToken {
            object: mem_obj,
            version: new_version,
        };

        let result_value = function.dfg.inst_result(inst, 0);
        self.env.set_value(result_value, new_token)?;
        Ok(())
    }

    /// 执行数组切片
    fn execute_array_slice(
        &mut self,
        function: &Function,
        inst: Inst,
        ptr: Value,
        sdims: &ValueVec,
    ) -> CEResult<()> {
        let ptr_val = self.env.get_value(ptr, &function.dfg)?;

        let (memory_id, elem_type, old_dims, old_base_indices) = match &ptr_val {
            RuntimeValue::ArrayPtr {
                memory_id,
                elem_type,
                dims,
                base_indices,
            } => (*memory_id, *elem_type, dims.clone(), base_indices.clone()),
            RuntimeValue::Int64(addr) => {
                // 处理全局地址（当地址展开被禁用时）
                let memory_id = MemoryId(*addr as u64);

                // 从内存管理器获取数组信息
                let (memory_object, _version) =
                    self.env.memory.get_block_info(memory_id).ok_or_else(|| {
                        CErr::runtime_err(format!("Memory block not found for address {}", addr))
                    })?;

                let dims = self.env.memory.get_dims(memory_id).ok_or_else(|| {
                    CErr::runtime_err(format!("Dimensions not found for memory {}", addr))
                })?;

                // 推导元素类型（从内存对象获取）
                let elem_type = match memory_object {
                    MemoryObject::Global { symbol } => {
                        // 从全局符号的类型信息推导元素类型
                        // 这需要查找核心上下文中的全局变量定义
                        // 为了简化，先使用默认的Int32类型
                        // 更完整的实现需要传递核心上下文或缓存类型信息
                        ArrayElemType::Int32 // 临时简化实现
                    }
                    _ => ArrayElemType::Int32, // 默认类型
                };

                (memory_id, elem_type, dims, Vec::new()) // 全局数组没有基础索引
            }
            _ => {
                return Err(CErr::runtime_err(
                    "Array slice requires array pointer or global address",
                ));
            }
        };

        // 获取切片索引
        let slice_indices: Vec<u32> = function.dfg.value_vecs[*sdims]
            .iter()
            .map(|idx| match self.env.get_value(*idx, &function.dfg)? {
                RuntimeValue::Int32(i) => Ok(i as u32),
                _ => Err(CErr::runtime_err("Slice index must be integer")),
            })
            .collect::<Result<Vec<_>, _>>()?;

        // 计算新的维度
        let (_, new_dims) = self.env.memory.compute_slice(memory_id, &slice_indices)?;

        // 计算新的基础索引：旧的基础索引 + 当前切片索引
        let mut new_base_indices = old_base_indices;
        new_base_indices.extend_from_slice(&slice_indices);

        // 创建新的数组指针（指向相同的内存，但维度不同）
        let new_ptr = RuntimeValue::ArrayPtr {
            memory_id,
            elem_type,
            dims: new_dims,
            base_indices: new_base_indices,
        };

        let result_value = function.dfg.inst_result(inst, 0);
        self.env.set_value(result_value, new_ptr)?;
        Ok(())
    }

    // === 低级内存操作实现 ===

    fn execute_stack_addr(
        &mut self,
        function: &Function,
        inst: Inst,
        slot: StackSlot,
    ) -> CEResult<()> {
        let (ptr_val, token_val) = self.env.get_stack_addr(slot, &function.dfg.stack_slots)?;
        self.env
            .set_value(function.dfg.inst_result(inst, 0), ptr_val)?;
        self.env
            .set_value(function.dfg.inst_result(inst, 1), token_val)?;
        Ok(())
    }

    fn execute_global_addr(
        &mut self,
        function: &Function,
        inst: Inst,
        global: Global,
    ) -> CEResult<()> {
        let (ptr_val, token_val) = self.env.get_global_addr(global)?;
        self.env
            .set_value(function.dfg.inst_result(inst, 0), ptr_val)?;
        self.env
            .set_value(function.dfg.inst_result(inst, 1), token_val)?;
        Ok(())
    }

    fn execute_load(
        &mut self,
        function: &Function,
        inst: Inst,
        addr: Value,
        offset: Offset,
        ty: Type,
        token: Value,
    ) -> CEResult<()> {
        let addr_val = self.env.get_value(addr, &function.dfg)?;
        let token_val = self.env.get_value(token, &function.dfg)?;
        let loaded_val = self
            .env
            .memory
            .load_with_addr(addr_val, offset, ty, token_val)?;
        self.env
            .set_value(function.dfg.inst_result(inst, 0), loaded_val)?;
        Ok(())
    }

    fn execute_store(
        &mut self,
        function: &Function,
        inst: Inst,
        addr: Value,
        offset: Offset,
        value: Value,
        token: Value,
    ) -> CEResult<()> {
        let addr_val = self.env.get_value(addr, &function.dfg)?;
        let token_val = self.env.get_value(token, &function.dfg)?;
        let value_to_store = self.env.get_value(value, &function.dfg)?;
        let new_token =
            self.env
                .memory
                .store_with_addr(addr_val, offset, value_to_store, token_val)?;
        self.env
            .set_value(function.dfg.inst_result(inst, 0), new_token)?;
        Ok(())
    }

    /// 执行全局标量读取
    fn execute_global_scalar_get(
        &mut self,
        function: &Function,
        inst: Inst,
        global: Value,
        token: Value,
    ) -> CEResult<()> {
        // 获取全局变量引用
        let global_ref = match function.dfg.valuedata(global) {
            ValueData::GlobalSymbol { sym, .. } => *sym,
            _ => {
                return Err(CErr::runtime_err(
                    "Global scalar get requires global symbol",
                ));
            }
        };

        // 从环境中获取全局变量的 ArrayPtr
        let ptr_val = self
            .env
            .globals
            .get(&global_ref)
            .ok_or_else(|| CErr::runtime_err("Global variable not found"))?
            .clone();

        let token_val = self.env.get_value(token, &function.dfg)?;

        // 提取内存ID和令牌版本
        let (memory_id, expected_version) = match (&ptr_val, &token_val) {
            (RuntimeValue::ArrayPtr { memory_id, .. }, RuntimeValue::MemToken { version, .. }) => {
                (*memory_id, *version)
            }
            (
                RuntimeValue::Int64(addr),
                RuntimeValue::MemToken {
                    object, version, ..
                },
            ) => {
                // Handle address lowering case - get memory_id from the token's object
                let mem_id = self
                    .env
                    .memory
                    .get_memory_version_by_object(object)
                    .map(|(id, _)| id)
                    .ok_or_else(|| CErr::runtime_err("Global memory object not found"))?;
                (mem_id, *version)
            }
            _ => {
                return Err(CErr::runtime_err("Invalid global scalar get operands"));
            }
        };

        // 标量作为单元素数组存储，索引为 [0]
        let indices = vec![0u32];

        // 读取内存
        let elem = self
            .env
            .memory
            .load(memory_id, &indices, expected_version)?;

        // 转换为运行时值
        let result = match elem {
            RuntimeElement::Bool(b) => RuntimeValue::Bool(b),
            RuntimeElement::Int32(i) => RuntimeValue::Int32(i),
            RuntimeElement::Float32(f) => RuntimeValue::Float32(f),
        };

        let result_value = function.dfg.inst_result(inst, 0);
        self.env.set_value(result_value, result)?;
        Ok(())
    }

    /// 执行全局标量写入
    fn execute_global_scalar_set(
        &mut self,
        function: &Function,
        inst: Inst,
        global: Value,
        value: Value,
        token: Value,
    ) -> CEResult<()> {
        // 获取全局变量引用
        let global_ref = match function.dfg.valuedata(global) {
            ValueData::GlobalSymbol { sym, .. } => *sym,
            _ => {
                return Err(CErr::runtime_err(
                    "Global scalar set requires global symbol",
                ));
            }
        };

        // 从环境中获取全局变量的 ArrayPtr
        let ptr_val = self
            .env
            .globals
            .get(&global_ref)
            .ok_or_else(|| CErr::runtime_err("Global variable not found"))?
            .clone();

        let token_val = self.env.get_value(token, &function.dfg)?;
        let value_to_store = self.env.get_value(value, &function.dfg)?;

        // 提取内存ID、对象和令牌版本
        let (memory_id, mem_obj, expected_version) = match (&ptr_val, &token_val) {
            (
                RuntimeValue::ArrayPtr { memory_id, .. },
                RuntimeValue::MemToken { object, version },
            ) => (*memory_id, *object, *version),
            (
                RuntimeValue::Int64(addr),
                RuntimeValue::MemToken {
                    object, version, ..
                },
            ) => {
                // Handle address lowering case - get memory_id from the token's object
                let mem_id = self
                    .env
                    .memory
                    .get_memory_version_by_object(object)
                    .map(|(id, _)| id)
                    .ok_or_else(|| CErr::runtime_err("Global memory object not found"))?;
                (mem_id, *object, *version)
            }
            _ => {
                return Err(CErr::runtime_err("Invalid global scalar set operands"));
            }
        };

        // 标量作为单元素数组存储，索引为 [0]
        let indices = vec![0u32];

        // 转换要存储的值
        let elem_to_store = match value_to_store {
            RuntimeValue::Bool(b) => RuntimeElement::Bool(b),
            RuntimeValue::Int32(i) => RuntimeElement::Int32(i),
            RuntimeValue::Float32(f) => RuntimeElement::Float32(f),
            _ => {
                return Err(CErr::runtime_err(
                    "Invalid value type for global scalar set",
                ));
            }
        };

        // 写入内存
        let new_version =
            self.env
                .memory
                .store(memory_id, &indices, elem_to_store, expected_version)?;

        // 创建新的内存令牌
        let new_token = RuntimeValue::MemToken {
            object: mem_obj,
            version: new_version,
        };

        let result_value = function.dfg.inst_result(inst, 0);
        self.env.set_value(result_value, new_token)?;
        Ok(())
    }

    /// 运行时函数实现
    fn runtime_getint(&mut self) -> CEResult<i32> {
        // 从输入缓冲区读取标记，如果缓冲区为空则从标准输入读取
        if self.env.input_index < self.env.input_buffer.len() {
            let token = &self.env.input_buffer[self.env.input_index];

            // 尝试解析为整数
            match token.parse::<i32>() {
                Ok(value) => {
                    let token_str = token.clone(); // 避免借用冲突
                    self.env.input_index += 1;

                    // 同步char_index：跳过token对应的字符和后续空白符
                    self.sync_char_index_with_token(&token_str);

                    Ok(value)
                }
                Err(_) => {
                    // 特判：如果是非数字token（如"P2"），跳过它并读取下一个token
                    if token.len() <= 4 && token.chars().all(|c| c.is_ascii_alphanumeric()) {
                        // 这看起来像是格式标识符（如"P2", "P5"等），跳过它
                        self.env.input_index += 1;

                        // 递归调用读取下一个token
                        self.runtime_getint()
                    } else {
                        Err(CErr::runtime_err(
                            "Failed to parse integer: invalid digit found in string",
                        ))
                    }
                }
            }
        } else {
            // 从标准输入读取一个标记
            use std::io::{self, Write};
            let mut input = String::new();
            print!("Enter integer: ");
            io::stdout().flush().unwrap();
            io::stdin()
                .read_line(&mut input)
                .map_err(|e| CErr::runtime_err(format!("Failed to read input: {}", e)))?;

            // 取第一个标记
            let token = input
                .split_whitespace()
                .next()
                .ok_or_else(|| CErr::runtime_err("No input provided"))?;
            token
                .parse::<i32>()
                .map_err(|e| CErr::runtime_err(format!("Failed to parse integer: {}", e)))
        }
    }

    fn runtime_getfloat(&mut self) -> CEResult<f32> {
        // 从输入缓冲区读取标记，如果缓冲区为空则从标准输入读取
        if self.env.input_index < self.env.input_buffer.len() {
            let token = self.env.input_buffer[self.env.input_index].clone(); // 克隆避免借用冲突
            self.env.input_index += 1;

            // 同步char_index：跳过token对应的字符和后续空白符
            self.sync_char_index_with_token(&token);

            parse_float_literal(&token).map_err(|e| {
                CErr::runtime_err("Failed to parse float: invalid float literal".to_string())
            })
        } else {
            // 从标准输入读取一个标记
            use std::io::{self, Write};
            let mut input = String::new();
            print!("Enter float: ");
            io::stdout().flush().unwrap();
            io::stdin()
                .read_line(&mut input)
                .map_err(|e| CErr::runtime_err(format!("Failed to read input: {}", e)))?;

            // 取第一个标记
            let token = input
                .split_whitespace()
                .next()
                .ok_or_else(|| CErr::runtime_err("No input provided"))?;
            parse_float_literal(token).map_err(|e| {
                CErr::runtime_err("Failed to parse float: invalid float literal".to_string())
            })
        }
    }

    fn runtime_putint(&mut self, n: i32) -> CEResult<()> {
        // 只捕获输出，实际输出由pipeline处理
        self.env.capture_output(&n.to_string());
        Ok(())
    }

    fn runtime_putch(&mut self, ch: i32) -> CEResult<()> {
        let ch = ch as u8 as char;
        // 只捕获输出，实际输出由pipeline处理
        self.env.capture_output(&ch.to_string());
        Ok(())
    }

    fn runtime_putfloat(&mut self, f: f32) -> CEResult<()> {
        // 按照C标准库printf("%a", f)的格式输出十六进制浮点数
        let hex_output = Self::format_hex_float(f);
        self.env.capture_output(&hex_output);
        Ok(())
    }

    /// 格式化浮点数为C风格的十六进制格式 (如 0x1.921fb6p+1)
    /// 使用系统的sprintf函数来确保与C标准完全一致
    fn format_hex_float(f: f32) -> String {
        use std::ffi::CString;
        use std::os::raw::{c_char, c_double};

        // 声明C函数
        unsafe extern "C" {
            fn sprintf(s: *mut c_char, format: *const c_char, ...) -> i32;
        }

        // 创建足够大的缓冲区
        let mut buffer = vec![0u8; 64];
        let format_str = CString::new("%a").unwrap();

        unsafe {
            sprintf(
                buffer.as_mut_ptr() as *mut c_char,
                format_str.as_ptr(),
                f as c_double, // 转换为c_double
            );
        }

        // 找到字符串的结尾（null terminator）
        let end = buffer.iter().position(|&x| x == 0).unwrap_or(buffer.len());

        // 转换为Rust字符串
        String::from_utf8_lossy(&buffer[..end]).to_string()
    }

    /// 同步char_index以跳过已消费的token
    fn sync_char_index_with_token(&mut self, token: &str) {
        // 从当前char_index开始，找到并跳过这个token

        // 跳过前面的空白符
        while self.env.char_index < self.env.char_buffer.len()
            && self.env.char_buffer[self.env.char_index].is_whitespace()
        {
            self.env.char_index += 1;
        }

        // 跳过token的字符
        for ch in token.chars() {
            if self.env.char_index < self.env.char_buffer.len()
                && self.env.char_buffer[self.env.char_index] == ch
            {
                self.env.char_index += 1;
            }
        }

        // 跳过token后的空白符，直到下一个非空白字符或行结束
        while self.env.char_index < self.env.char_buffer.len()
            && self.env.char_buffer[self.env.char_index].is_whitespace()
            && self.env.char_buffer[self.env.char_index] != '\n'
        {
            self.env.char_index += 1;
        }
    }

    fn runtime_getch(&mut self) -> CEResult<i32> {
        // 从字符缓冲区读取下一个字符
        if self.env.char_index < self.env.char_buffer.len() {
            let ch = self.env.char_buffer[self.env.char_index];
            self.env.char_index += 1;
            Ok(ch as i32)
        } else {
            Err(CErr::runtime_err("No input available for getch"))
        }
    }

    fn runtime_getarray(&mut self, ptr_data: &RuntimeValue) -> CEResult<i32> {
        let (memory_id, base_indices) = match ptr_data {
            RuntimeValue::ArrayPtr {
                memory_id,
                base_indices,
                ..
            } => (*memory_id, base_indices.clone()),
            _ => {
                return Err(CErr::runtime_err("Expected ArrayPtr for getarray"));
            }
        };
        // 读取数组大小
        if self.env.input_index >= self.env.input_buffer.len() {
            return Err(CErr::runtime_err("No input available for getarray size"));
        }
        let size_token = &self.env.input_buffer[self.env.input_index];
        self.env.input_index += 1;
        let size = size_token
            .parse::<i32>()
            .map_err(|e| CErr::runtime_err(format!("Failed to parse array size: {}", e)))?;

        // 读取数组元素并存储
        for i in 0..size {
            if self.env.input_index >= self.env.input_buffer.len() {
                return Err(CErr::runtime_err(format!(
                    "No input available for array element {}",
                    i
                )));
            }
            let token = &self.env.input_buffer[self.env.input_index];
            self.env.input_index += 1;
            let value = token.parse::<i32>().map_err(|e| {
                CErr::runtime_err(format!("Failed to parse array element {}: {}", i, e))
            })?;

            // 使用内存管理器的store方法，合并基础索引和访问索引
            let element = RuntimeElement::Int32(value);
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            self.env
                .memory
                .store(memory_id, &full_indices, element, 0)?;
        }

        // 返回数组大小
        Ok(size)
    }

    fn runtime_getarray_from_addr(
        &mut self,
        addr: i64,
        object: &crate::ssa::ir::MemoryObject,
    ) -> CEResult<i32> {
        // Handle getarray with raw address (from address lowering pass)
        // We need to map the address back to a memory_id

        let memory_id = match object {
            crate::ssa::ir::MemoryObject::Global { symbol } => {
                // Find the memory_id for this global symbol
                if let Some(mem_id) = self.env.memory.get_memory_id(object) {
                    mem_id
                } else {
                    return Err(CErr::runtime_err(
                        "getarray_from_addr: Global array memory not found",
                    ));
                }
            }
            crate::ssa::ir::MemoryObject::StackSlot { slot } => {
                // Handle stack-allocated arrays (local arrays in functions)
                if let Some(mem_id) = self.env.memory.get_memory_id(object) {
                    mem_id
                } else {
                    return Err(CErr::runtime_err(
                        "getarray_from_addr: Stack array memory not found",
                    ));
                }
            }
            _ => {
                return Err(CErr::runtime_err(format!(
                    "getarray_from_addr: Unsupported memory object type: {:?}",
                    object
                )));
            }
        };

        // Base indices depend on whether it's global or local (same logic as putarray functions)
        let base_indices = match object {
            crate::ssa::ir::MemoryObject::Global { .. } => {
                // For global arrays, if addr is 0, we're accessing the first row
                if addr == 0 {
                    vec![0u32] // Accessing first row
                } else {
                    vec![addr as u32]
                }
            }
            crate::ssa::ir::MemoryObject::StackSlot { .. } => {
                // For local arrays, use empty base indices (direct access)
                Vec::new()
            }
            _ => Vec::new(),
        };

        // Read array size from input (same logic as runtime_getarray)
        if self.env.input_index >= self.env.input_buffer.len() {
            return Err(CErr::runtime_err("No input available for getarray size"));
        }
        let size_token = &self.env.input_buffer[self.env.input_index];
        self.env.input_index += 1;
        let size = size_token
            .parse::<i32>()
            .map_err(|e| CErr::runtime_err(format!("Failed to parse array size: {}", e)))?;

        // Read array elements and store them
        for i in 0..size {
            if self.env.input_index >= self.env.input_buffer.len() {
                return Err(CErr::runtime_err(format!(
                    "No input available for array element {}",
                    i
                )));
            }
            let token = &self.env.input_buffer[self.env.input_index];
            self.env.input_index += 1;
            let value = token.parse::<i32>().map_err(|e| {
                CErr::runtime_err(format!("Failed to parse array element {}: {}", i, e))
            })?;

            // Store the element in memory
            let element = RuntimeElement::Int32(value);
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            self.env
                .memory
                .store(memory_id, &full_indices, element, 0)?;
        }

        Ok(size)
    }

    fn runtime_getfarray(&mut self, ptr_data: &RuntimeValue) -> CEResult<i32> {
        let (memory_id, base_indices) = match ptr_data {
            RuntimeValue::ArrayPtr {
                memory_id,
                base_indices,
                ..
            } => (*memory_id, base_indices.clone()),
            _ => {
                return Err(CErr::runtime_err("Expected ArrayPtr for getfarray"));
            }
        };
        // 读取数组大小
        if self.env.input_index >= self.env.input_buffer.len() {
            return Err(CErr::runtime_err("No input available for getfarray size"));
        }
        let size_token = &self.env.input_buffer[self.env.input_index];
        self.env.input_index += 1;
        let size = size_token
            .parse::<i32>()
            .map_err(|e| CErr::runtime_err(format!("Failed to parse array size: {}", e)))?;

        // 读取数组元素并存储
        for i in 0..size {
            if self.env.input_index >= self.env.input_buffer.len() {
                return Err(CErr::runtime_err(format!(
                    "No input available for array element {}",
                    i
                )));
            }
            let token = &self.env.input_buffer[self.env.input_index];
            self.env.input_index += 1;
            let value = parse_float_literal(token).map_err(|e| {
                CErr::runtime_err(format!(
                    "Failed to parse array element {}: invalid float literal",
                    i
                ))
            })?;

            // 使用内存管理器的store方法，合并基础索引和访问索引
            let element = RuntimeElement::Float32(value);
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            self.env
                .memory
                .store(memory_id, &full_indices, element, 0)?;
        }

        // 返回数组大小
        Ok(size)
    }

    fn runtime_getfarray_from_addr(
        &mut self,
        addr: i64,
        object: &crate::ssa::ir::MemoryObject,
    ) -> CEResult<i32> {
        // Handle getfarray with raw address (from address lowering pass)
        let memory_id = match object {
            crate::ssa::ir::MemoryObject::Global { symbol } => {
                // For global arrays, look up the actual memory ID
                let result = self.env.memory.get_memory_version_by_object(object);
                result
                    .map(|(id, _)| id)
                    .ok_or_else(|| CErr::runtime_err("Global memory object not found"))?
            }
            crate::ssa::ir::MemoryObject::ArrayAlloc { inst } => {
                // For local arrays allocated with ArrayAlloc, look up the actual memory ID
                let result = self.env.memory.get_memory_version_by_object(object);
                result
                    .map(|(id, _)| id)
                    .ok_or_else(|| CErr::runtime_err("ArrayAlloc memory object not found"))?
            }
            crate::ssa::ir::MemoryObject::StackSlot { slot } => {
                // For stack slot arrays, look up the actual memory ID
                let result = self.env.memory.get_memory_version_by_object(object);
                result
                    .map(|(id, _)| id)
                    .ok_or_else(|| CErr::runtime_err("StackSlot memory object not found"))?
            }
            _ => {
                return Err(CErr::runtime_err(
                    "getfarray_from_addr: Unsupported memory object type",
                ));
            }
        };

        // For address lowering, we use empty base indices since the address already includes any offset
        let base_indices = Vec::new();

        // Read array size from input (same logic as runtime_getfarray)
        if self.env.input_index >= self.env.input_buffer.len() {
            return Err(CErr::runtime_err("No input available for getfarray size"));
        }
        let size_token = &self.env.input_buffer[self.env.input_index];
        self.env.input_index += 1;
        let size = size_token
            .parse::<i32>()
            .map_err(|e| CErr::runtime_err(format!("Failed to parse array size: {}", e)))?;

        // Read array elements and store them
        for i in 0..size {
            if self.env.input_index >= self.env.input_buffer.len() {
                return Err(CErr::runtime_err(format!(
                    "No input available for array element {}",
                    i
                )));
            }
            let token = &self.env.input_buffer[self.env.input_index];
            self.env.input_index += 1;
            let value = parse_float_literal(token).map_err(|e| {
                CErr::runtime_err(format!(
                    "Failed to parse array element {}: invalid float literal",
                    i
                ))
            })?;

            // Store the element in memory
            let element = RuntimeElement::Float32(value);
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            self.env
                .memory
                .store(memory_id, &full_indices, element, 0)?;
        }

        Ok(size)
    }

    fn runtime_putarray(&mut self, n: i32, ptr_data: &RuntimeValue) -> CEResult<()> {
        let (memory_id, base_indices) = match ptr_data {
            RuntimeValue::ArrayPtr {
                memory_id,
                base_indices,
                ..
            } => (*memory_id, base_indices.clone()),
            _ => {
                return Err(CErr::runtime_err("Expected ArrayPtr for putarray"));
            }
        };
        // 输出数组大小和冒号
        self.env.capture_output(&format!("{}:", n));

        // 输出数组元素
        for i in 0..n {
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            let element = self.env.memory.load(memory_id, &full_indices, 0)?;
            if let RuntimeElement::Int32(value) = element {
                self.env.capture_output(&format!(" {}", value));
            }
        }
        self.env.capture_output("\n");

        Ok(())
    }

    fn runtime_putarray_from_addr(
        &mut self,
        n: i32,
        addr: i64,
        object: &crate::ssa::ir::MemoryObject,
    ) -> CEResult<()> {
        // Handle putarray with raw address (from address lowering pass)
        // Use address arithmetic to find the proper memory ID instead of synthetic construction
        let addr_val = RuntimeValue::Int64(addr);
        let dummy_token = RuntimeValue::MemToken {
            object: *object,
            version: 0,
        };

        // Find the correct memory ID using address arithmetic (same logic as load_with_addr/store_with_addr)
        let memory_id = match addr_val {
            RuntimeValue::Int64(addr) => {
                let addr = addr as u64;
                let mut found_mem_id: Option<super::memory::MemoryId> = None;

                for existing_mem_id in self.env.memory.get_memory_ids() {
                    if existing_mem_id.0 <= addr
                        && (found_mem_id.is_none() || existing_mem_id.0 > found_mem_id.unwrap().0)
                    {
                        found_mem_id = Some(existing_mem_id);
                    }
                }

                found_mem_id.ok_or_else(|| {
                    CErr::runtime_err(format!(
                        "Cannot find valid memory ID for putarray address {}",
                        addr
                    ))
                })?
            }
            _ => return Err(CErr::runtime_err("Invalid address type for putarray")),
        };

        // Base indices depend on whether it's global or local
        let base_indices = match object {
            crate::ssa::ir::MemoryObject::Global { .. } => {
                // For global arrays, if addr is 0, we're accessing the first row
                if addr == 0 {
                    vec![0u32] // Accessing first row
                } else {
                    vec![addr as u32]
                }
            }
            crate::ssa::ir::MemoryObject::ArrayAlloc { .. }
            | crate::ssa::ir::MemoryObject::StackSlot { .. } => {
                // For local arrays, use empty base indices (direct access)
                Vec::new()
            }
            _ => Vec::new(),
        };

        // 输出数组大小和冒号
        self.env.capture_output(&format!("{}:", n));

        // 输出数组元素
        for i in 0..n {
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            let element = self.env.memory.load(memory_id, &full_indices, 0)?;
            if let RuntimeElement::Int32(value) = element {
                self.env.capture_output(&format!(" {}", value));
            }
        }
        self.env.capture_output("\n");

        Ok(())
    }

    fn runtime_putfarray(&mut self, n: i32, ptr_data: &RuntimeValue) -> CEResult<()> {
        let (memory_id, base_indices) = match ptr_data {
            RuntimeValue::ArrayPtr {
                memory_id,
                base_indices,
                ..
            } => (*memory_id, base_indices.clone()),
            _ => {
                return Err(CErr::runtime_err("Expected ArrayPtr for putfarray"));
            }
        };
        // 输出数组大小和冒号
        self.env.capture_output(&format!("{}:", n));

        // 输出数组元素
        for i in 0..n {
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            let element = self.env.memory.load(memory_id, &full_indices, 0)?;
            if let RuntimeElement::Float32(value) = element {
                // 按照C标准库printf(" %a", value)的格式输出十六进制浮点数
                let hex_value = Self::format_hex_float(value);
                self.env.capture_output(&format!(" {}", hex_value));
            }
        }
        self.env.capture_output("\n");

        Ok(())
    }

    fn runtime_putfarray_from_addr(
        &mut self,
        n: i32,
        addr: i64,
        object: &crate::ssa::ir::MemoryObject,
    ) -> CEResult<()> {
        // Handle putfarray with raw address (from address lowering pass)
        // Use address arithmetic to find the proper memory ID instead of synthetic construction
        let addr_val = RuntimeValue::Int64(addr);

        // Find the correct memory ID using address arithmetic (same logic as load_with_addr/store_with_addr)
        let memory_id = match addr_val {
            RuntimeValue::Int64(addr) => {
                let addr = addr as u64;
                let mut found_mem_id: Option<super::memory::MemoryId> = None;

                for existing_mem_id in self.env.memory.get_memory_ids() {
                    if existing_mem_id.0 <= addr
                        && (found_mem_id.is_none() || existing_mem_id.0 > found_mem_id.unwrap().0)
                    {
                        found_mem_id = Some(existing_mem_id);
                    }
                }

                found_mem_id.ok_or_else(|| {
                    CErr::runtime_err(format!(
                        "Cannot find valid memory ID for putfarray address {}",
                        addr
                    ))
                })?
            }
            _ => return Err(CErr::runtime_err("Invalid address type for putfarray")),
        };

        // Base indices depend on whether it's global or local
        let base_indices = match object {
            crate::ssa::ir::MemoryObject::Global { .. } => {
                if addr == 0 {
                    vec![0u32]
                } else {
                    vec![addr as u32]
                }
            }
            crate::ssa::ir::MemoryObject::ArrayAlloc { .. }
            | crate::ssa::ir::MemoryObject::StackSlot { .. } => Vec::new(),
            _ => Vec::new(),
        };

        // 输出数组大小和冒号
        self.env.capture_output(&format!("{}:", n));

        // 输出数组元素
        for i in 0..n {
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            let element = self.env.memory.load(memory_id, &full_indices, 0)?;
            if let RuntimeElement::Float32(value) = element {
                // 按照C标准库printf(" %a", value)的格式输出十六进制浮点数
                let hex_value = Self::format_hex_float(value);
                self.env.capture_output(&format!(" {}", hex_value));
            }
        }
        self.env.capture_output("\n");

        Ok(())
    }

    fn runtime_putf(&mut self, ptr_data: &RuntimeValue) -> CEResult<()> {
        let (memory_id, base_indices) = match ptr_data {
            RuntimeValue::ArrayPtr {
                memory_id,
                base_indices,
                ..
            } => (*memory_id, base_indices.clone()),
            _ => return Err(CErr::runtime_err("Expected ArrayPtr for putf")),
        };
        // 简化版本的putf，只是输出字符串
        let mut output = String::new();
        let mut i = 0;
        loop {
            let mut full_indices = base_indices.clone();
            full_indices.push(i as u32);
            let element = self.env.memory.load(memory_id, &full_indices, 0)?;
            if let RuntimeElement::Int32(ch) = element {
                if ch == 0 {
                    break;
                } // 遇到null terminator
                output.push(ch as u8 as char);
                i += 1;
            } else {
                break;
            }
        }
        self.env.capture_output(&output);

        Ok(())
    }

    fn runtime_starttime(&mut self) -> CEResult<()> {
        // 暂时空实现，实际需要记录时间
        Ok(())
    }

    fn runtime_stoptime(&mut self) -> CEResult<()> {
        // 暂时空实现，实际需要计算时间差
        Ok(())
    }

    fn runtime_sysy_starttime(&mut self, _lineno: i32) -> CEResult<()> {
        // 暂时空实现，实际需要记录时间和行号
        Ok(())
    }

    fn runtime_sysy_stoptime(&mut self, _lineno: i32) -> CEResult<()> {
        // 暂时空实现，实际需要计算时间差并输出
        Ok(())
    }

    /// 执行尾调用
    fn execute_return_call(
        &mut self,
        function: &Function,
        func: FuncRef,
        args: &ValueVec,
    ) -> CEResult<Vec<RuntimeValue>> {
        let func_name = &self.core_context.names[func];

        // 获取参数值
        let arg_values: Vec<RuntimeValue> = function.dfg.value_vecs[*args]
            .iter()
            .map(|arg| self.env.get_value(*arg, &function.dfg))
            .collect::<Result<Vec<_>, _>>()?;

        // 处理运行时函数
        match func_name.as_str() {
            "getint" => {
                let value = self.runtime_getint()?;
                Ok(vec![RuntimeValue::Int32(value)])
            }
            "putint" => {
                if let RuntimeValue::Int32(n) = &arg_values[0] {
                    self.runtime_putint(*n)?;
                }
                Ok(vec![RuntimeValue::Unit])
            }
            "putch" => {
                if let RuntimeValue::Int32(n) = &arg_values[0] {
                    self.runtime_putch(*n)?;
                }
                Ok(vec![RuntimeValue::Unit])
            }
            "getfloat" => {
                let value = self.runtime_getfloat()?;
                Ok(vec![RuntimeValue::Float32(value)])
            }
            "putfloat" => {
                if let RuntimeValue::Float32(f) = &arg_values[0] {
                    self.runtime_putfloat(*f)?;
                }
                Ok(vec![RuntimeValue::Unit])
            }
            "getch" => {
                let value = self.runtime_getch()?;
                Ok(vec![RuntimeValue::Int32(value)])
            }
            "getarray" => {
                // getarray(int a[]) -> int: 读取数组元素，返回数组大小和新的内存令牌
                if arg_values.len() >= 2 {
                    if let (
                        ptr_data @ RuntimeValue::ArrayPtr { .. },
                        RuntimeValue::MemToken { object, version },
                    ) = (&arg_values[0], &arg_values[1])
                    {
                        let n = self.runtime_getarray(ptr_data)?;
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: version + 1,
                        };
                        Ok(vec![RuntimeValue::Int32(n), new_token])
                    } else {
                        Err(CErr::runtime_err("Invalid arguments for getarray"))
                    }
                } else {
                    Err(CErr::runtime_err("Not enough arguments for getarray"))
                }
            }
            "getfarray" => {
                // getfarray(float a[]) -> int: 读取浮点数组元素，返回数组大小和新的内存令牌
                if arg_values.len() >= 2 {
                    if let (
                        ptr_data @ RuntimeValue::ArrayPtr { .. },
                        RuntimeValue::MemToken { object, version },
                    ) = (&arg_values[0], &arg_values[1])
                    {
                        let n = self.runtime_getfarray(ptr_data)?;
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: version + 1,
                        };
                        Ok(vec![RuntimeValue::Int32(n), new_token])
                    } else {
                        Err(CErr::runtime_err("Invalid arguments for getfarray"))
                    }
                } else {
                    Err(CErr::runtime_err("Not enough arguments for getfarray"))
                }
            }
            "putarray" => {
                // putarray(int n, int a[]): 输出数组，返回内存令牌
                if arg_values.len() >= 3 {
                    if let (
                        RuntimeValue::Int32(n),
                        ptr_data @ RuntimeValue::ArrayPtr { .. },
                        RuntimeValue::MemToken { object, version },
                    ) = (&arg_values[0], &arg_values[1], &arg_values[2])
                    {
                        self.runtime_putarray(*n, ptr_data)?;
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: *version, // putarray是只读操作，不增加版本
                        };
                        Ok(vec![new_token])
                    } else {
                        Err(CErr::runtime_err("Invalid arguments for putarray"))
                    }
                } else {
                    Err(CErr::runtime_err("Not enough arguments for putarray"))
                }
            }
            "putfarray" => {
                // putfarray(int n, float a[]): 输出浮点数组，返回内存令牌
                if arg_values.len() >= 3 {
                    if let (
                        RuntimeValue::Int32(n),
                        ptr_data @ RuntimeValue::ArrayPtr { .. },
                        RuntimeValue::MemToken { object, version },
                    ) = (&arg_values[0], &arg_values[1], &arg_values[2])
                    {
                        self.runtime_putfarray(*n, ptr_data)?;
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: *version, // putfarray是只读操作，不增加版本
                        };
                        Ok(vec![new_token])
                    } else {
                        Err(CErr::runtime_err("Invalid arguments for putfarray"))
                    }
                } else {
                    Err(CErr::runtime_err("Not enough arguments for putfarray"))
                }
            }
            "putf" => {
                // putf(char a[], ...): 格式化输出，返回内存令牌
                if arg_values.len() >= 2 {
                    if let (
                        ptr_data @ RuntimeValue::ArrayPtr { .. },
                        RuntimeValue::MemToken { object, version },
                    ) = (&arg_values[0], &arg_values[1])
                    {
                        self.runtime_putf(ptr_data)?;
                        let new_token = RuntimeValue::MemToken {
                            object: *object,
                            version: *version, // putf是只读操作，不增加版本
                        };
                        Ok(vec![new_token])
                    } else {
                        Err(CErr::runtime_err("Invalid arguments for putf"))
                    }
                } else {
                    Err(CErr::runtime_err("Not enough arguments for putf"))
                }
            }
            "starttime" => {
                self.runtime_starttime()?;
                Ok(vec![RuntimeValue::Unit])
            }
            "stoptime" => {
                self.runtime_stoptime()?;
                Ok(vec![RuntimeValue::Unit])
            }
            "_sysy_starttime" => {
                if let RuntimeValue::Int32(lineno) = &arg_values[0] {
                    self.runtime_sysy_starttime(*lineno)?;
                }
                Ok(vec![RuntimeValue::Unit])
            }
            "_sysy_stoptime" => {
                if let RuntimeValue::Int32(lineno) = &arg_values[0] {
                    self.runtime_sysy_stoptime(*lineno)?;
                }
                Ok(vec![RuntimeValue::Unit])
            }
            _ => {
                // 查找用户定义函数
                if let Some(target_func) =
                    self.functions.iter().find(|f| f.name == func_name.as_str())
                {
                    // 尾调用：直接执行函数并返回其结果
                    // 压入新的调用栈帧
                    self.env.push_frame(target_func.clone());

                    // 设置函数参数（作为入口块的块参数）
                    let entry_block = target_func.entry;
                    let entry_params = target_func.dfg.block_params(entry_block);

                    // 验证参数数量匹配
                    if arg_values.len() != entry_params.len() {
                        return Err(CErr::runtime_err(format!(
                            "Function {} expects {} arguments, got {}",
                            target_func.name,
                            entry_params.len(),
                            arg_values.len()
                        )));
                    }

                    // 设置入口块参数值
                    for (idx, (param_value, arg_value)) in
                        entry_params.iter().zip(arg_values.iter()).enumerate()
                    {
                        self.env
                            .current_frame_mut()
                            .unwrap()
                            .block_params
                            .insert((entry_block, idx as u32), arg_value.clone());
                    }

                    // Debug: trace tree function calls
                    if target_func.name == "tree" {
                        let arg_values: Vec<RuntimeValue> = function.dfg.value_vecs[*args]
                            .iter()
                            .map(|arg| self.env.get_value(*arg, &function.dfg))
                            .collect::<Result<Vec<_>, _>>()?;
                        eprintln!(
                            "DEBUG: Calling tree({:?}, {:?})",
                            arg_values[0], arg_values[1]
                        );
                    }

                    // 执行函数
                    let results = self.execute_function_recursive(target_func)?;

                    // 弹出调用栈帧
                    self.env.pop_frame();

                    Ok(results)
                } else {
                    Err(CErr::runtime_err(format!(
                        "Function not found: {}",
                        func_name
                    )))
                }
            }
        }
    }

    /// 打印执行统计信息
    pub fn print_statistics(&self) {
        println!("\n=== Execution Statistics ===");
        println!(
            "Total instructions executed: {}",
            self.env.stats.instruction_count
        );
        println!("Memory allocations: {}", self.env.stats.alloc_count);
        println!("Memory loads: {}", self.env.stats.load_count);
        println!("Memory stores: {}", self.env.stats.store_count);

        if !self.env.stats.call_count.is_empty() {
            println!("\nFunction call counts:");
            for (func, count) in &self.env.stats.call_count {
                let name = &self.core_context.names[*func];
                println!("  {}: {}", name, count);
            }
        }
    }
}

/// 执行结果
enum ExecutionResult {
    /// 继续执行下一条指令
    Continue,
    /// 跳转到指定块
    Jump(Block),
    /// 函数返回
    Return(Vec<RuntimeValue>),
}
