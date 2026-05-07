//! 解释器执行环境

use super::memory::MemoryId;
use crate::prelude::*;
use crate::ssa::{
    ctx::CoreContext, dfg::DataFlowGraph, function::Function, globals::GlobalInit, ir::*,
};
use std::collections::HashMap;

/// 解释器执行环境
pub struct Environment {
    /// 全局变量存储
    pub globals: HashMap<Global, RuntimeValue>,
    /// 函数映射表
    pub functions: HashMap<FuncRef, usize>, // 函数名到函数索引的映射
    /// 调用栈
    pub call_stack: Vec<CallFrame>,
    /// 内存分配器
    pub memory: super::memory::MemoryManager,
    /// 捕获的输出（包括putint、putch、putfloat等）
    pub captured_output: String,
    /// 捕获的输入请求（用于从pipeline提供的输入读取）
    pub input_buffer: Vec<String>,
    pub input_index: usize,
    /// 字符缓冲区（用于getch）
    pub char_buffer: Vec<char>,
    pub char_index: usize,
    /// 统计信息
    pub stats: ExecutionStats,
}

/// 调用栈帧
pub struct CallFrame {
    /// 当前函数
    pub function: Function,
    /// 当前块
    pub current_block: Block,
    /// 下一条要执行的指令索引
    pub next_inst: usize,
    /// 局部值存储
    pub locals: HashMap<Value, RuntimeValue>,
    /// 块参数映射
    pub block_params: HashMap<(Block, u32), RuntimeValue>,
    /// 返回地址（调用者的块和指令索引）
    pub return_address: Option<(Block, usize)>,
}

/// 运行时值
#[derive(Clone, Debug)]
pub enum RuntimeValue {
    /// 单元值
    Unit,
    /// 布尔值
    Bool(bool),
    /// 32位整数
    Int32(i32),
    /// 64位整数
    Int64(i64),
    /// 32位浮点数
    Float32(f32),
    /// 64位浮点数
    Float64(f64),
    /// 内存地址
    Address(u64),
    /// 数组指针
    ArrayPtr {
        memory_id: super::memory::MemoryId,
        elem_type: ArrayElemType,
        dims: Vec<u32>,
        /// 切片的基础索引（用于切片访问时的偏移计算）
        base_indices: Vec<u32>,
    },
    /// 内存令牌
    MemToken { object: MemoryObject, version: u32 },
}

/// 执行统计
#[derive(Default, Debug)]
pub struct ExecutionStats {
    /// 执行的指令数
    pub instruction_count: u64,
    /// 函数调用次数
    pub call_count: HashMap<FuncRef, u64>,
    /// 内存分配次数
    pub alloc_count: u64,
    /// 内存读取次数
    pub load_count: u64,
    /// 内存写入次数  
    pub store_count: u64,
}

impl Environment {
    /// 创建新的执行环境
    pub fn new(core_context: &CoreContext) -> CEResult<Self> {
        let mut env = Self {
            globals: HashMap::new(),
            functions: HashMap::new(),
            call_stack: Vec::new(),
            memory: super::memory::MemoryManager::new(),
            captured_output: String::new(),
            input_buffer: Vec::new(),
            input_index: 0,
            char_buffer: Vec::new(),
            char_index: 0,
            stats: ExecutionStats::default(),
        };

        // 初始化全局变量
        env.init_globals(core_context)?;

        Ok(env)
    }

    /// 设置输入缓冲区（从文件或其他源提供）
    pub fn set_input_buffer(&mut self, input: Vec<String>) {
        // 将所有行的内容按C语言标准的空白字符分割成单独的标记
        // C语言空白字符包括：空格、制表符、换行符、回车符、垂直制表符、换页符
        let mut tokens = Vec::new();
        let mut chars = Vec::new();

        // 同时保存所有字符用于getch，并处理token
        for line in input.iter() {
            // 添加行内容的所有字符
            chars.extend(line.chars());
            // 每行都添加换行符，确保 getstr 等函数能正确检测行结束
            chars.push('\n');

            // split_whitespace() 已经按照Unicode标准处理所有空白字符，符合C语言标准
            for token in line.split_whitespace() {
                if !token.is_empty() {
                    tokens.push(token.to_string());
                }
            }
        }

        self.input_buffer = tokens;
        self.input_index = 0;
        self.char_buffer = chars;
        self.char_index = 0;

        // 调试输出
        // eprintln!("字符缓冲区长度: {}", self.char_buffer.len());
        // eprintln!("字符缓冲区内容: {:?}", self.char_buffer);
    }

    /// 初始化全局变量
    fn init_globals(&mut self, core_context: &CoreContext) -> CEResult<()> {
        for (global_ref, global_data) in core_context.globals.iter() {
            match &global_data.init {
                GlobalInit::Array(values) => {
                    // 获取元素类型
                    let elem_type = match global_data.ty {
                        Type::Bool => ArrayElemType::Bool,
                        Type::Int32 => ArrayElemType::Int32,
                        Type::Float32 => ArrayElemType::Float32,
                        Type::ArrayPtr { elem, .. } => elem,
                        _ => return Err(CErr::runtime_err("Invalid global array type")),
                    };

                    // 获取维度信息
                    let dims: Vec<u32> = core_context.type_dims[global_data.dims]
                        .iter()
                        .map(|d| {
                            d.ok_or_else(|| {
                                CErr::runtime_err("Global array dimension must be constant")
                            })
                        })
                        .collect::<Result<Vec<_>, _>>()?;

                    // 分配内存并初始化
                    let memory_id = self
                        .memory
                        .allocate_global(global_ref, elem_type, &dims, values)?;

                    // 为了与address lowering兼容，存储Int64地址而不是ArrayPtr
                    let addr_value = RuntimeValue::Int64(memory_id.0 as i64);

                    // 创建初始内存令牌
                    let token_value = RuntimeValue::MemToken {
                        object: MemoryObject::Global { symbol: global_ref },
                        version: 0,
                    };

                    // 存储全局变量的地址（兼容address lowering）
                    self.globals.insert(global_ref, addr_value);
                    // 令牌通过特殊的全局符号值处理，在这里不需要额外存储
                }
                GlobalInit::Zero => {
                    // 获取元素类型和维度信息
                    let (elem_type, dims) = match global_data.ty {
                        Type::Bool => (ArrayElemType::Bool, vec![1u32]), // 标量作为单元素数组
                        Type::Int32 => (ArrayElemType::Int32, vec![1u32]), // 标量作为单元素数组
                        Type::Float32 => (ArrayElemType::Float32, vec![1u32]), // 标量作为单元素数组
                        Type::ArrayPtr { elem, .. } => {
                            // 真正的数组，获取维度信息
                            let dims: Vec<u32> = core_context.type_dims[global_data.dims]
                                .iter()
                                .map(|d| {
                                    d.ok_or_else(|| {
                                        CErr::runtime_err("Global array dimension must be constant")
                                    })
                                })
                                .collect::<Result<Vec<_>, _>>()?;
                            (elem, dims)
                        }
                        _ => return Err(CErr::runtime_err("Invalid global type")),
                    };

                    // 分配零初始化的内存
                    let memory_id = self
                        .memory
                        .allocate_global_zero(global_ref, elem_type, &dims)?;

                    // 为了与address lowering兼容，存储Int64地址而不是ArrayPtr
                    let addr_value = RuntimeValue::Int64(memory_id.0 as i64);

                    let token_value = RuntimeValue::MemToken {
                        object: MemoryObject::Global { symbol: global_ref },
                        version: 0,
                    };

                    self.globals.insert(global_ref, addr_value);
                    // 令牌通过特殊的全局符号值处理，在这里不需要额外存储
                }
                GlobalInit::Scalar(val) => {
                    // 标量全局变量作为单元素数组处理
                    let elem_type = match global_data.ty {
                        Type::Bool => ArrayElemType::Bool,
                        Type::Int32 => ArrayElemType::Int32,
                        Type::Float32 => ArrayElemType::Float32,
                        _ => {
                            return Err(CErr::runtime_err("Invalid global scalar type"));
                        }
                    };

                    let memory_id = self.memory.allocate_global(
                        global_ref,
                        elem_type,
                        &[1], // 标量作为单元素数组
                        &[*val],
                    )?;

                    // 为了与address lowering兼容，存储Int64地址而不是ArrayPtr
                    let addr_value = RuntimeValue::Int64(memory_id.0 as i64);

                    let token_value = RuntimeValue::MemToken {
                        object: MemoryObject::Global { symbol: global_ref },
                        version: 0,
                    };

                    self.globals.insert(global_ref, addr_value);
                    // 令牌通过特殊的全局符号值处理，在这里不需要额外存储
                }
            }
        }
        Ok(())
    }

    /// 压入新的调用栈帧
    pub fn push_frame(&mut self, function: Function) {
        let frame = CallFrame {
            current_block: function.entry,
            function,
            next_inst: 0,
            locals: HashMap::new(),
            block_params: HashMap::new(),
            return_address: None,
        };
        self.call_stack.push(frame);
    }

    /// 弹出调用栈帧
    pub fn pop_frame(&mut self) -> Option<CallFrame> {
        self.call_stack.pop()
    }

    /// 获取当前栈帧
    pub fn current_frame(&self) -> Option<&CallFrame> {
        self.call_stack.last()
    }

    /// 获取当前栈帧（可变）
    pub fn current_frame_mut(&mut self) -> Option<&mut CallFrame> {
        self.call_stack.last_mut()
    }

    /// 获取值
    pub fn get_value(&self, value: Value, dfg: &DataFlowGraph) -> CEResult<RuntimeValue> {
        // 首先解析所有别名，确保使用的是最终的非别名值
        let resolved_value = dfg.resolve_aliases(value);
        let value_data = dfg.valuedata(resolved_value);

        match value_data {
            ValueData::Const { ty, c } => Ok(self.const_to_runtime(c)),

            ValueData::Inst { .. } => {
                let frame = self
                    .current_frame()
                    .ok_or_else(|| CErr::runtime_err("No active call frame"))?;

                // 尝试查找解析后的值
                if let Some(runtime_value) = frame.locals.get(&resolved_value) {
                    return Ok(runtime_value.clone());
                }

                // 如果解析后的值没找到，尝试查找原始值（向后兼容）
                if resolved_value != value {
                    if let Some(runtime_value) = frame.locals.get(&value) {
                        return Ok(runtime_value.clone());
                    }
                }

                // Check if this value represents an instruction that was removed during lowering
                // In that case, we should look for its alias mapping
                if let Some(ValueData::Inst { inst, idx, .. }) = dfg.all_values().get(value) {
                    // This value was supposed to be an instruction result
                    // but the instruction might have been removed during lowering
                    eprintln!(
                        "DEBUG: Value {:?} is instruction result {}[{}] but not found in locals",
                        value, inst, idx
                    );
                    eprintln!("DEBUG: Current function: {:?}", frame.function.name);
                    eprintln!("DEBUG: Current block: {:?}", frame.current_block);
                    eprintln!("DEBUG: Next instruction index: {}", frame.next_inst);
                    eprintln!("DEBUG: Checking if instruction {} exists in layout", inst);

                    // Check if instruction exists in the layout of current function
                    let mut inst_exists_in_current_func = false;
                    for block in frame.function.layout.blocks() {
                        for layout_inst in frame.function.layout.block_insts(block) {
                            if layout_inst == *inst {
                                inst_exists_in_current_func = true;
                                break;
                            }
                        }
                        if inst_exists_in_current_func {
                            break;
                        }
                    }

                    if !inst_exists_in_current_func {
                        eprintln!(
                            "DEBUG: Instruction {} does not exist in current function '{}' - this is a cross-function reference!",
                            inst, frame.function.name
                        );
                    }

                    if inst_exists_in_current_func {
                        eprintln!(
                            "DEBUG: Instruction {} exists in current function but value not in locals",
                            inst
                        );

                        // Check what the current instruction (that's trying to use this value) is
                        let current_block_insts: Vec<_> = frame
                            .function
                            .layout
                            .block_insts(frame.current_block)
                            .collect();
                        if frame.next_inst < current_block_insts.len() {
                            let current_inst = current_block_insts[frame.next_inst];
                            let current_inst_data = &frame.function.dfg.insts[current_inst];
                            eprintln!(
                                "DEBUG: Current instruction {} is: {:?}",
                                current_inst, current_inst_data
                            );
                        }

                        // Check what instruction 2 actually is
                        let inst_data = &frame.function.dfg.insts[*inst];
                        eprintln!(
                            "DEBUG: Instruction {} (that should have produced Value(5v1)) is: {:?}",
                            inst, inst_data
                        );
                    }
                }

                Err(CErr::runtime_err(format!(
                    "Value {:?} (resolved from {:?}) not found in locals",
                    resolved_value, value
                )))
            }

            ValueData::BlockParam { block, idx, .. } => {
                let frame = self
                    .current_frame()
                    .ok_or_else(|| CErr::runtime_err("No active call frame"))?;

                frame
                    .block_params
                    .get(&(*block, *idx))
                    .cloned()
                    .ok_or_else(|| {
                        CErr::runtime_err(format!("Block param {:?}:{} not found", block, idx))
                    })
            }

            ValueData::GlobalSymbol { sym, ty } => {
                match ty {
                    Type::MemToken => {
                        // 返回全局变量的内存令牌，使用实际的当前版本
                        let mem_obj = MemoryObject::Global { symbol: *sym };
                        if let Some((_, version)) =
                            self.memory.get_memory_version_by_object(&mem_obj)
                        {
                            Ok(RuntimeValue::MemToken {
                                object: mem_obj,
                                version,
                            })
                        } else {
                            Err(CErr::runtime_err(format!(
                                "Global memory object {:?} not found",
                                sym
                            )))
                        }
                    }
                    _ => {
                        // 返回全局变量的数组指针
                        self.globals
                            .get(sym)
                            .cloned()
                            .ok_or_else(|| CErr::runtime_err(format!("Global {:?} not found", sym)))
                    }
                }
            }

            ValueData::Alias { original, .. } => {
                // 这种情况不应该发生，因为已经解析了别名
                // 但为了安全起见，递归查找
                self.get_value(*original, dfg)
            }

            ValueData::Union { x, .. } => {
                // Union 节点表示两个等价的值
                // 在解释器中，我们选择第一个值（通常是原始值）
                self.get_value(*x, dfg)
            }
        }
    }

    /// 设置值
    pub fn set_value(&mut self, value: Value, runtime_value: RuntimeValue) -> CEResult<()> {
        let frame = self
            .current_frame_mut()
            .ok_or_else(|| CErr::runtime_err("No active call frame"))?;

        // 存储原始值
        frame.locals.insert(value, runtime_value.clone());

        // 解析别名并在不同的情况下也存储
        let resolved_value = frame.function.dfg.resolve_aliases(value);
        if resolved_value != value {
            frame.locals.insert(resolved_value, runtime_value);
        }

        // 尝试初始化任何待初始化的StackSlot
        self.try_initialize_pending_stack_slots()?;

        Ok(())
    }

    /// 尝试初始化所有待初始化的StackSlot
    fn try_initialize_pending_stack_slots(&mut self) -> CEResult<()> {
        // 收集所有待初始化的StackSlot
        let pending_slots = self.memory.pending_stack_slots();

        if pending_slots.is_empty() {
            return Ok(());
        }

        // 获取当前帧信息（避免借用冲突）
        let (locals_map, dfg) = if let Some(frame) = self.current_frame() {
            (frame.locals.clone(), frame.function.dfg.clone())
        } else {
            return Ok(());
        };

        for slot in pending_slots {
            let get_value = |val: Value| {
                // 首先检查是否是常量值
                let resolved_val = dfg.resolve_aliases(val);
                let value_data = dfg.valuedata(resolved_val);
                match value_data {
                    ValueData::Const { c, .. } => {
                        let runtime_val = match c {
                            ConstValue::Unit => RuntimeValue::Unit,
                            ConstValue::Bool { val } => RuntimeValue::Bool(*val),
                            ConstValue::Int32 { val } => RuntimeValue::Int32(*val),
                            ConstValue::Int64 { val } => RuntimeValue::Int64(*val),
                            ConstValue::Float32 { val } => RuntimeValue::Float32(*val),
                            ConstValue::Float64 { val } => RuntimeValue::Float32(*val as f32), // 临时转换，可能丢失精度
                        };
                        Some(runtime_val)
                    }
                    _ => {
                        // 检查局部变量
                        if let Some(runtime_val) = locals_map.get(&val) {
                            return Some(runtime_val.clone());
                        }
                        if resolved_val != val {
                            if let Some(runtime_val) = locals_map.get(&resolved_val) {
                                return Some(runtime_val.clone());
                            }
                        }
                        None
                    }
                }
            };
            self.memory
                .try_initialize_stack_slot(slot, &dfg, get_value)?;
        }

        Ok(())
    }

    /// 常量值转换为运行时值
    fn const_to_runtime(&self, const_val: &ConstValue) -> RuntimeValue {
        match const_val {
            ConstValue::Unit => RuntimeValue::Unit,
            ConstValue::Bool { val } => RuntimeValue::Bool(*val),
            ConstValue::Int32 { val } => RuntimeValue::Int32(*val),
            ConstValue::Int64 { val } => RuntimeValue::Int64(*val),
            ConstValue::Float32 { val } => RuntimeValue::Float32(*val),
            ConstValue::Float64 { val } => RuntimeValue::Float32(*val as f32), // 临时转换，可能丢失精度
        }
    }

    /// 更新统计信息
    pub fn update_stats(&mut self, inst: &InstructionData) {
        self.stats.instruction_count += 1;

        match inst {
            InstructionData::Call { func, .. } => {
                *self.stats.call_count.entry(*func).or_insert(0) += 1;
            }
            InstructionData::ArrayAlloc { .. } => {
                self.stats.alloc_count += 1;
            }
            InstructionData::ArrayGet { .. } | InstructionData::GlobalScalarGet { .. } => {
                self.stats.load_count += 1;
            }
            InstructionData::ArrayPut { .. } | InstructionData::GlobalScalarSet { .. } => {
                self.stats.store_count += 1;
            }
            _ => {}
        }
    }

    /// 捕获输出（用于与.out文件对比）
    pub fn capture_output(&mut self, output: &str) {
        self.captured_output.push_str(output);
    }

    /// 获取所有捕获的输出
    pub fn get_captured_output(&self) -> &str {
        &self.captured_output
    }

    // === 低级内存操作辅助函数 ===

    pub fn get_stack_addr(
        &mut self,
        slot: StackSlot,
        stack_slots: &StackSlots,
    ) -> CEResult<(RuntimeValue, RuntimeValue)> {
        let (addr, token) = self.memory.allocate_stack_slot(slot, stack_slots)?;
        Ok((RuntimeValue::Int64(addr as i64), token))
    }

    pub fn get_global_addr(&self, global: Global) -> CEResult<(RuntimeValue, RuntimeValue)> {
        let ptr_val = self
            .globals
            .get(&global)
            .ok_or_else(|| CErr::runtime_err("Global not found"))?;
        match ptr_val {
            RuntimeValue::Address(addr) => {
                // 已经是地址类型（旧的低级表示），转换为Int64
                let mem_obj = MemoryObject::Global { symbol: global };
                let version = self
                    .memory
                    .get_memory_version_by_object(&mem_obj)
                    .unwrap_or((MemoryId(0), 0))
                    .1;
                let token = RuntimeValue::MemToken {
                    object: mem_obj,
                    version,
                };
                Ok((RuntimeValue::Int64(*addr as i64), token))
            }
            RuntimeValue::Int64(addr) => {
                // 新的地址表示（与address lowering兼容）
                let mem_obj = MemoryObject::Global { symbol: global };
                let version = self
                    .memory
                    .get_memory_version_by_object(&mem_obj)
                    .map(|(_, version)| version)
                    .unwrap_or(0);
                let token = RuntimeValue::MemToken {
                    object: mem_obj,
                    version,
                };
                Ok((RuntimeValue::Int64(*addr), token))
            }
            RuntimeValue::ArrayPtr { memory_id, .. } => {
                // 向后兼容：数组指针类型（应该很少见了）
                let mem_obj = MemoryObject::Global { symbol: global };
                let version = self
                    .memory
                    .get_memory_version_by_object(&mem_obj)
                    .map(|(_, version)| version)
                    .unwrap_or(0);
                let token = RuntimeValue::MemToken {
                    object: mem_obj,
                    version,
                };
                // 将 MemoryId 转换为 Int64 地址：直接使用分配的 memory_id
                let addr = RuntimeValue::Int64(memory_id.0 as i64);
                Ok((addr, token))
            }
            _ => Err(CErr::runtime_err(format!(
                "Global {:?} is not a valid address type: {:?}",
                global, ptr_val
            ))),
        }
    }
}
