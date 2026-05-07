//! 内存管理器

use super::environment::RuntimeValue;
use crate::prelude::*;
use crate::ssa::dfg::DataFlowGraph;
use crate::ssa::ir::*;
use std::collections::HashMap;

/// 内存ID
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct MemoryId(pub u64);

/// 内存块
pub struct MemoryBlock {
    /// 内存对象标识
    pub object: MemoryObject,
    /// 元素类型
    pub elem_type: ArrayElemType,
    /// 维度信息
    pub dims: Vec<u32>,
    /// 数据存储（展平为一维）
    pub data: Vec<RuntimeElement>,
    /// 当前版本号
    pub version: u32,
}

/// 运行时元素值
#[derive(Clone, Debug)]
pub enum RuntimeElement {
    Bool(bool),
    Int32(i32),
    Float32(f32),
}

impl RuntimeElement {
    /// 转换为i32（用于输出）
    pub fn to_i32(&self) -> i32 {
        match self {
            RuntimeElement::Bool(b) => {
                if *b {
                    1
                } else {
                    0
                }
            }
            RuntimeElement::Int32(i) => *i,
            RuntimeElement::Float32(f) => *f as i32,
        }
    }

    /// 转换为f32
    pub fn to_f32(&self) -> f32 {
        match self {
            RuntimeElement::Bool(b) => {
                if *b {
                    1.0
                } else {
                    0.0
                }
            }
            RuntimeElement::Int32(i) => *i as f32,
            RuntimeElement::Float32(f) => *f,
        }
    }

    /// 获取零值
    pub fn zero(elem_type: ArrayElemType) -> Self {
        match elem_type {
            ArrayElemType::Bool => RuntimeElement::Bool(false),
            ArrayElemType::Int32 => RuntimeElement::Int32(0),
            ArrayElemType::Float32 => RuntimeElement::Float32(0.0),
        }
    }
}

/// 内存管理器
pub struct MemoryManager {
    /// 内存块映射
    blocks: HashMap<MemoryId, MemoryBlock>,
    /// 内存对象到ID的映射
    object_to_id: HashMap<MemoryObject, MemoryId>,
    /// 下一个可用ID
    next_id: u64,
    /// 待初始化的StackSlot映射
    pending_stack_inits: HashMap<StackSlot, (MemoryId, ArrayInit)>,
}

impl Default for MemoryManager {
    fn default() -> Self {
        Self::new()
    }
}

impl MemoryManager {
    /// 创建新的内存管理器
    pub fn new() -> Self {
        Self {
            blocks: HashMap::new(),
            object_to_id: HashMap::new(),
            next_id: 0,
            pending_stack_inits: HashMap::new(),
        }
    }

    /// 分配内存（栈上）
    pub fn allocate(
        &mut self,
        object: MemoryObject,
        elem_type: ArrayElemType,
        dims: &[u32],
        init: Option<&[RuntimeElement]>,
    ) -> CEResult<MemoryId> {
        // 根据内存对象类型决定分配策略
        let memory_id = MemoryId(self.next_id);

        // 计算总元素数和字节大小
        let total_size = dims.iter().product::<u32>() as usize;
        let elem_size = self.get_elem_type_size(elem_type) as usize;
        let total_bytes = total_size * elem_size;

        match object {
            MemoryObject::Global { .. } => {
                // 全局变量使用连续内存布局，与编译器假设一致
                // 按实际大小分配，保证地址连续
                self.next_id += total_bytes as u64;
                // 对齐到8字节边界以提高访问效率
                self.next_id = (self.next_id + 7) & !7;
            }
            _ => {
                // 栈变量等仍然使用64KB间隔，方便调试和检测越界访问
                self.next_id += 0x10000;
            }
        }

        // 初始化数据
        let data = if let Some(init_data) = init {
            if init_data.len() != total_size {
                return Err(CErr::runtime_err(format!(
                    "Array init size mismatch: expected {}, got {}",
                    total_size,
                    init_data.len()
                )));
            }
            init_data.to_vec()
        } else {
            let zero_elem = RuntimeElement::zero(elem_type);
            vec![zero_elem; total_size]
        };

        let block = MemoryBlock {
            object,
            elem_type,
            dims: dims.to_vec(),
            data,
            version: 0,
        };

        self.blocks.insert(memory_id, block);
        self.object_to_id.insert(object, memory_id);

        Ok(memory_id)
    }

    /// 分配全局内存
    pub fn allocate_global(
        &mut self,
        global: Global,
        elem_type: ArrayElemType,
        dims: &[u32],
        init: &[ConstValue],
    ) -> CEResult<MemoryId> {
        let object = MemoryObject::Global { symbol: global };

        // 转换常量值为运行时元素
        let runtime_init: Vec<RuntimeElement> = init
            .iter()
            .map(|c| match c {
                ConstValue::Bool { val } => RuntimeElement::Bool(*val),
                ConstValue::Int32 { val } => RuntimeElement::Int32(*val),
                ConstValue::Float32 { val } => RuntimeElement::Float32(*val),
                _ => panic!("Invalid const value for array element"),
            })
            .collect();

        self.allocate(object, elem_type, dims, Some(&runtime_init))
    }

    /// 分配零初始化的全局内存
    pub fn allocate_global_zero(
        &mut self,
        global: Global,
        elem_type: ArrayElemType,
        dims: &[u32],
    ) -> CEResult<MemoryId> {
        let object = MemoryObject::Global { symbol: global };
        self.allocate(object, elem_type, dims, None)
    }

    /// 读取内存
    pub fn load(
        &self,
        memory_id: MemoryId,
        indices: &[u32],
        expected_version: u32,
    ) -> CEResult<RuntimeElement> {
        let block = self
            .blocks
            .get(&memory_id)
            .ok_or_else(|| CErr::runtime_err("Invalid memory ID"))?;

        // 对于读操作，使用宽松的版本检查
        // 只要内存对象正确，就允许读取，因为在复杂控制流中不同路径可能有不同版本的令牌

        // 计算线性索引
        let linear_index = self.compute_linear_index(&block.dims, indices)?;

        if linear_index >= block.data.len() {
            // 最终内存边界检查 - 这是真正的越界，必须报错
            return Err(CErr::runtime_err(format!(
                "Memory access out of bounds: linear index {} >= memory size {}",
                linear_index,
                block.data.len()
            )));
        }
        let element = &block.data[linear_index];
        Ok(element.clone())
    }

    /// 写入内存
    pub fn store(
        &mut self,
        memory_id: MemoryId,
        indices: &[u32],
        value: RuntimeElement,
        expected_version: u32,
    ) -> CEResult<u32> {
        // 先获取维度信息和验证版本号
        let dims = {
            let block = self
                .blocks
                .get(&memory_id)
                .ok_or_else(|| CErr::runtime_err("Invalid memory ID"))?;

            // 对于写操作，使用宽松的版本检查
            // 允许使用任何版本的令牌进行写操作，但会更新到新版本

            block.dims.clone()
        };

        // 计算线性索引
        let linear_index = self.compute_linear_index(&dims, indices)?;

        // 现在可以安全地获取可变借用
        let block = self
            .blocks
            .get_mut(&memory_id)
            .ok_or_else(|| CErr::runtime_err("Invalid memory ID"))?;

        // 写入数据 - 最终内存边界检查
        if linear_index >= block.data.len() {
            return Err(CErr::runtime_err(format!(
                "Memory access out of bounds: linear index {} >= memory size {}",
                linear_index,
                block.data.len()
            )));
        }

        block.data[linear_index] = value;

        // 增加版本号
        block.version += 1;

        Ok(block.version)
    }

    /// 计算线性索引
    fn compute_linear_index(&self, dims: &[u32], indices: &[u32]) -> CEResult<usize> {
        if indices.len() != dims.len() {
            return Err(CErr::runtime_err(format!(
                "Index dimension mismatch: expected {}, got {}",
                dims.len(),
                indices.len()
            )));
        }

        let mut linear_index = 0;
        let mut stride = 1;
        let mut has_bounds_warning = false;

        // 从最后一维开始计算
        for i in (0..dims.len()).rev() {
            if indices[i] >= dims[i] {
                // 数组越界访问 - 输出警告但继续执行（C语言未定义行为）
                eprintln!(
                    "WARNING: Array index out of bounds: index {} >= dim {} (undefined behavior)",
                    indices[i], dims[i]
                );
                has_bounds_warning = true;
            }
            linear_index += indices[i] as usize * stride;
            stride *= dims[i] as usize;
        }

        Ok(linear_index)
    }

    /// 获取内存块信息（用于调试）
    pub fn get_block_info(&self, memory_id: MemoryId) -> Option<(MemoryObject, u32)> {
        self.blocks
            .get(&memory_id)
            .map(|block| (block.object, block.version))
    }

    /// 通过内存对象获取内存ID
    pub fn get_memory_id(&self, object: &MemoryObject) -> Option<MemoryId> {
        self.object_to_id.get(object).copied()
    }

    /// 获取数组的维度信息
    pub fn get_dims(&self, memory_id: MemoryId) -> Option<Vec<u32>> {
        self.blocks.get(&memory_id).map(|block| block.dims.clone())
    }

    /// 计算数组切片后的内存ID和新维度
    pub fn compute_slice(
        &self,
        memory_id: MemoryId,
        slice_indices: &[u32],
    ) -> CEResult<(MemoryId, Vec<u32>)> {
        let block = self
            .blocks
            .get(&memory_id)
            .ok_or_else(|| CErr::runtime_err("Invalid memory ID"))?;

        if slice_indices.len() > block.dims.len() {
            return Err(CErr::runtime_err("Too many slice indices"));
        }

        // 新的维度是去掉前N个维度
        let new_dims = block.dims[slice_indices.len()..].to_vec();

        // 对于切片，仍然返回相同的memory_id，
        // 因为切片只是视图，实际的内存偏移在访问时计算
        Ok((memory_id, new_dims))
    }

    /// 获取内存块的当前版本
    pub fn get_version(&self, memory_id: MemoryId) -> CEResult<u32> {
        let block = self
            .blocks
            .get(&memory_id)
            .ok_or_else(|| CErr::runtime_err("Invalid memory ID"))?;
        Ok(block.version)
    }

    /// 根据内存对象获取内存ID和当前版本
    pub fn get_memory_version_by_object(&self, object: &MemoryObject) -> Option<(MemoryId, u32)> {
        if let Some(memory_id) = self.object_to_id.get(object) {
            if let Some(block) = self.blocks.get(memory_id) {
                return Some((*memory_id, block.version));
            }
        }
        None
    }

    // === 低级内存操作 ===

    pub fn allocate_stack_slot(
        &mut self,
        slot: StackSlot,
        stack_slots: &StackSlots,
    ) -> CEResult<(u64, RuntimeValue)> {
        let object = MemoryObject::StackSlot { slot };
        if let Some(mem_id) = self.object_to_id.get(&object) {
            let block = &self.blocks[mem_id];
            let token = RuntimeValue::MemToken {
                object,
                version: block.version,
            };
            // Return the actual memory ID, not an allocated address
            return Ok((mem_id.0, token));
        }

        let slot_data = &stack_slots[slot];
        let elem_type = ArrayElemType::from_type(slot_data.elem).unwrap();
        let dims = vec![slot_data.size as u32 / self.get_elem_type_size(elem_type)];
        let mem_id = self.allocate(object, elem_type, &dims, None)?;
        let token = RuntimeValue::MemToken { object, version: 0 };

        // Store slot info for potential delayed initialization
        match &slot_data.init {
            ArrayInit::List { .. } => {
                self.pending_stack_inits
                    .insert(slot, (mem_id, slot_data.init));
            }
            _ => {
                // ArrayInit::Zero and ArrayInit::Undef don't need runtime values
            }
        }

        // Return the actual memory ID, not an allocated address
        Ok((mem_id.0, token))
    }

    /// 获取所有待初始化的StackSlot
    pub fn pending_stack_slots(&self) -> Vec<StackSlot> {
        self.pending_stack_inits.keys().cloned().collect()
    }

    /// 获取所有内存块的ID（用于地址查找）
    pub fn get_memory_ids(&self) -> Vec<MemoryId> {
        self.blocks.keys().cloned().collect()
    }

    /// 尝试初始化待初始化的StackSlot（如果所有依赖值都可用）
    pub fn try_initialize_stack_slot(
        &mut self,
        slot: StackSlot,
        dfg: &DataFlowGraph,
        get_value: impl Fn(Value) -> Option<RuntimeValue>,
    ) -> CEResult<bool> {
        if let Some((mem_id, init)) = self.pending_stack_inits.remove(&slot) {
            match init {
                ArrayInit::Zero | ArrayInit::Undef => {
                    // 这些不需要运行时值，直接完成
                    return Ok(true);
                }
                ArrayInit::List { vals } => {
                    let values = &dfg.value_vecs[vals];
                    // 检查是否所有值都可用
                    let mut all_available = true;
                    for &val in values {
                        if get_value(val).is_none() {
                            all_available = false;
                            break;
                        }
                    }

                    if all_available {
                        // 所有值都可用，执行初始化
                        let block = self
                            .blocks
                            .get_mut(&mem_id)
                            .ok_or_else(|| CErr::runtime_err("Memory block not found"))?;
                        for (i, &val) in values.iter().enumerate() {
                            if let Some(runtime_val) = get_value(val) {
                                let elem = match runtime_val {
                                    RuntimeValue::Int32(v) => RuntimeElement::Int32(v),
                                    RuntimeValue::Float32(v) => RuntimeElement::Float32(v),
                                    RuntimeValue::Bool(v) => RuntimeElement::Bool(v),
                                    _ => {
                                        return Err(CErr::runtime_err(format!(
                                            "Invalid value type for array initialization: {:?}",
                                            runtime_val
                                        )));
                                    }
                                };
                                if i < block.data.len() {
                                    block.data[i] = elem;
                                }
                            }
                        }
                        return Ok(true);
                    } else {
                        // 还有值不可用，重新加入待初始化队列
                        self.pending_stack_inits.insert(slot, (mem_id, init));
                        return Ok(false);
                    }
                }
            }
        }
        Ok(false)
    }

    pub fn load_with_addr(
        &self,
        addr: RuntimeValue,
        offset: Offset,
        ty: Type,
        token: RuntimeValue,
    ) -> CEResult<RuntimeValue> {
        let (mem_id, base_offset) = match addr {
            RuntimeValue::Address(addr) => (MemoryId(addr), 0),
            RuntimeValue::Int64(addr) => {
                // For low-level address operations from stack_addr/global_addr
                let addr = addr as u64;

                // For address arithmetic results, find the largest memory ID that is <= addr
                let mut found_mem_id: Option<MemoryId> = None;

                // Check memory IDs in descending order to find the correct base
                for existing_mem_id in self.get_memory_ids() {
                    if existing_mem_id.0 <= addr
                        && (found_mem_id.is_none() || existing_mem_id.0 > found_mem_id.unwrap().0)
                    {
                        found_mem_id = Some(existing_mem_id);
                    }
                }

                if let Some(mem_id) = found_mem_id {
                    let base_offset = (addr - mem_id.0) as usize;

                    (mem_id, base_offset)
                } else {
                    return Err(CErr::runtime_err(format!(
                        "Cannot find valid memory ID for address {}",
                        addr
                    )));
                }
            }
            _ => {
                return Err(CErr::runtime_err(
                    "Load address must be an address or Int64 type",
                ));
            }
        };

        let block = self
            .blocks
            .get(&mem_id)
            .ok_or_else(|| CErr::runtime_err(format!("Invalid memory ID: {:?}", mem_id)))?;
        let elem_size = self.get_elem_type_size(block.elem_type);
        // Fix: Address lowering already calculated byte addresses with element size,
        // so we should divide by element size to get the linear index
        let linear_index = (base_offset + offset as usize) / elem_size as usize;

        // Direct linear memory access, bypassing dimension checks
        if linear_index >= block.data.len() {
            return Err(CErr::runtime_err(format!(
                "Memory access out of bounds: linear index {} >= memory size {}, base_offset={}, offset={}, elem_size={}",
                linear_index,
                block.data.len(),
                base_offset,
                offset,
                elem_size
            )));
        }

        let element = &block.data[linear_index];
        Ok(self.element_to_runtime(element.clone()))
    }

    pub fn store_with_addr(
        &mut self,
        addr: RuntimeValue,
        offset: Offset,
        value: RuntimeValue,
        token: RuntimeValue,
    ) -> CEResult<RuntimeValue> {
        let (mem_id, base_offset) = match addr {
            RuntimeValue::Address(addr) => (MemoryId(addr), 0),
            RuntimeValue::Int64(addr) => {
                // For low-level address operations from stack_addr/global_addr
                let addr = addr as u64;

                // For address arithmetic results, find the largest memory ID that is <= addr
                let mut found_mem_id: Option<MemoryId> = None;

                // Check memory IDs in descending order to find the correct base
                for existing_mem_id in self.get_memory_ids() {
                    if existing_mem_id.0 <= addr
                        && (found_mem_id.is_none() || existing_mem_id.0 > found_mem_id.unwrap().0)
                    {
                        found_mem_id = Some(existing_mem_id);
                    }
                }

                if let Some(mem_id) = found_mem_id {
                    let base_offset = (addr - mem_id.0) as usize;

                    (mem_id, base_offset)
                } else {
                    return Err(CErr::runtime_err(format!(
                        "Cannot find valid memory ID for address {}",
                        addr
                    )));
                }
            }
            _ => {
                return Err(CErr::runtime_err(
                    "Store address must be an address or Int64 type",
                ));
            }
        };

        let (elem_size, object) = {
            let block = self
                .blocks
                .get(&mem_id)
                .ok_or_else(|| CErr::runtime_err(format!("Invalid memory ID: {:?}", mem_id)))?;
            (self.get_elem_type_size(block.elem_type), block.object)
        };
        let linear_index = (base_offset + offset as usize) / elem_size as usize;
        let elem_to_store = self.runtime_to_element(value)?;

        // Direct linear memory access, bypassing dimension checks
        let block = self
            .blocks
            .get_mut(&mem_id)
            .ok_or_else(|| CErr::runtime_err("Invalid memory ID"))?;

        if linear_index >= block.data.len() {
            eprintln!(
                "BOUNDS ERROR: Trying to store to linear_index {} in memory block with {} elements",
                linear_index,
                block.data.len()
            );
            eprintln!("  mem_id={:?}, object={:?}", mem_id, block.object);
            eprintln!(
                "  base_offset={}, offset={}, elem_size={}",
                base_offset, offset, elem_size
            );
            eprintln!(
                "  Calculation: linear_index = ({} + {}) / {} = {}",
                base_offset, offset, elem_size, linear_index
            );
            return Err(CErr::runtime_err(format!(
                "Memory access out of bounds in store: linear index {} >= memory size {}, base_offset={}, offset={}, elem_size={}",
                linear_index,
                block.data.len(),
                base_offset,
                offset,
                elem_size
            )));
        }

        block.data[linear_index] = elem_to_store;
        block.version += 1;

        Ok(RuntimeValue::MemToken {
            object,
            version: block.version,
        })
    }

    fn get_elem_type_size(&self, ty: ArrayElemType) -> u32 {
        match ty {
            ArrayElemType::Bool => 1,
            ArrayElemType::Int32 => 4,
            ArrayElemType::Float32 => 4,
        }
    }

    fn element_to_runtime(&self, elem: RuntimeElement) -> RuntimeValue {
        match elem {
            RuntimeElement::Bool(b) => RuntimeValue::Bool(b),
            RuntimeElement::Int32(i) => RuntimeValue::Int32(i),
            RuntimeElement::Float32(f) => RuntimeValue::Float32(f),
        }
    }

    fn runtime_to_element(&self, val: RuntimeValue) -> CEResult<RuntimeElement> {
        match val {
            RuntimeValue::Bool(b) => Ok(RuntimeElement::Bool(b)),
            RuntimeValue::Int32(i) => Ok(RuntimeElement::Int32(i)),
            RuntimeValue::Float32(f) => Ok(RuntimeElement::Float32(f)),
            _ => Err(CErr::runtime_err(
                "Invalid runtime value for memory element",
            )),
        }
    }

    /// 调试方法：获取所有内存ID
    pub fn debug_memory_ids(&self) -> Vec<MemoryId> {
        self.blocks.keys().copied().collect()
    }

    /// 调试方法：获取所有object_to_id映射
    pub fn debug_object_mappings(&self) -> Vec<(MemoryObject, MemoryId)> {
        self.object_to_id
            .iter()
            .map(|(&obj, &id)| (obj, id))
            .collect()
    }
}
