//! SSA IR 方法实现
use super::ir::*;
use crate::prelude::*;

//==============================================================================
// 值系统
//==============================================================================

impl Blocks {
    /// 提供 iter 方法，返回所有块的迭代器
    pub fn iter(&self) -> impl Iterator<Item = (Block, &BlockData)> {
        self.iter_all()
    }
}

impl Insts {
    /// 提供 iter 方法，返回所有指令的迭代器
    pub fn iter(&self) -> impl Iterator<Item = (Inst, &InstructionData)> {
        self.iter_all()
    }
}

impl StackSlots {
    /// 提供 iter 方法，返回所有栈槽的迭代器
    pub fn iter(&self) -> impl Iterator<Item = (StackSlot, &StackSlotData)> {
        self.iter_all()
    }
}

impl ValueData {
    /// 判断是否为常量
    pub fn is_const(&self) -> bool {
        matches!(self, ValueData::Const { .. })
    }

    /// 判断是否为指令结果
    pub fn is_inst(&self) -> bool {
        matches!(self, ValueData::Inst { .. })
    }

    /// 获取指令引用 -- 如果是指令结果
    pub fn as_inst(&self) -> Option<(Inst, u32)> {
        match self {
            ValueData::Inst { inst, idx, .. } => Some((*inst, *idx)),
            _ => None,
        }
    }

    /// 获取值的类型
    #[inline(always)]
    pub fn ty(&self) -> Type {
        match self {
            ValueData::Const { ty, .. } => *ty,
            ValueData::Inst { ty, .. } => *ty,
            ValueData::BlockParam { ty, .. } => *ty,
            ValueData::GlobalSymbol { ty, .. } => *ty,
            ValueData::Alias { ty, .. } => *ty,
            ValueData::Union { ty, .. } => *ty,
        }
    }

    /// 创建常量值的便捷方法
    pub fn unit_val(ty: Type) -> ValueData {
        ValueData::Const {
            ty,
            c: ConstValue::Unit,
        }
    }

    pub fn bool_val(ty: Type, val: bool) -> ValueData {
        ValueData::Const {
            ty,
            c: ConstValue::Bool { val },
        }
    }

    pub fn int32_val(ty: Type, val: i32) -> ValueData {
        ValueData::Const {
            ty,
            c: ConstValue::Int32 { val },
        }
    }

    pub fn float32_val(ty: Type, val: f32) -> ValueData {
        ValueData::Const {
            ty,
            c: ConstValue::Float32 { val },
        }
    }

    pub fn bbp_val(ty: Type, block: Block, idx: u32) -> ValueData {
        ValueData::BlockParam { ty, block, idx }
    }
}

impl ConstValue {
    const EPSILON: f32 = 1e-6;

    fn float_eq(a: f32, b: f32) -> bool {
        (a - b).abs() < Self::EPSILON
    }

    pub fn is_zero(&self) -> bool {
        match self {
            ConstValue::Unit => true,
            ConstValue::Bool { val } => !val,
            ConstValue::Int32 { val } => *val == 0,
            ConstValue::Int64 { val } => *val == 0,
            ConstValue::Float32 { val } => val.abs() < Self::EPSILON,
            ConstValue::Float64 { val } => (*val as f32).abs() < Self::EPSILON, // 临时使用f32精度
        }
    }

    // 获取字节
    pub fn to_abi_bytes(&self) -> Vec<u8> {
        match self {
            ConstValue::Unit => vec![],
            ConstValue::Bool { val } => vec![if *val { 1 } else { 0 }, 0, 0, 0],
            ConstValue::Int32 { val } => val.to_le_bytes().to_vec(),
            ConstValue::Int64 { val } => val.to_le_bytes().to_vec(),
            ConstValue::Float32 { val } => val.to_le_bytes().to_vec(),
            ConstValue::Float64 { val } => val.to_le_bytes().to_vec(),
        }
    }
}

impl PartialEq for ConstValue {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (ConstValue::Unit, ConstValue::Unit) => true,
            (ConstValue::Bool { val: a }, ConstValue::Bool { val: b }) => a == b,
            (ConstValue::Int32 { val: a }, ConstValue::Int32 { val: b }) => a == b,
            (ConstValue::Int64 { val: a }, ConstValue::Int64 { val: b }) => a == b,
            (ConstValue::Float32 { val: a }, ConstValue::Float32 { val: b }) => {
                Self::float_eq(*a, *b)
            }
            (ConstValue::Float64 { val: a }, ConstValue::Float64 { val: b }) => {
                // 临时使用f32精度进行比较
                Self::float_eq(*a as f32, *b as f32)
            }
            _ => false,
        }
    }
}

impl Eq for ConstValue {}

impl std::hash::Hash for ConstValue {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        match self {
            ConstValue::Unit => {
                0u8.hash(state);
            }
            ConstValue::Bool { val: b } => {
                1u8.hash(state);
                b.hash(state);
            }
            ConstValue::Int32 { val: i } => {
                2u8.hash(state);
                i.hash(state);
            }
            ConstValue::Int64 { val: i } => {
                4u8.hash(state);
                i.hash(state);
            }
            ConstValue::Float32 { val: f } => {
                3u8.hash(state);
                // 浮点数的hash需要特殊处理，因为NaN != NaN
                if f.is_nan() {
                    // 所有NaN都映射到同一个hash值
                    0u32.hash(state);
                } else {
                    // 使用位表示进行hash，避免-0.0和+0.0的问题
                    f.to_bits().hash(state);
                }
            }
            ConstValue::Float64 { val: f } => {
                5u8.hash(state);
                // 浮点数的hash需要特殊处理，因为NaN != NaN
                if f.is_nan() {
                    // 所有NaN都映射到同一个hash值
                    0u64.hash(state);
                } else {
                    // 使用位表示进行hash，避免-0.0和+0.0的问题
                    f.to_bits().hash(state);
                }
            }
        }
    }
}

//==============================================================================
// 指令系统
//==============================================================================

/// 用于遍历指令中所有值的trait
///
/// 这个trait统一了指令参数的遍历逻辑，避免在多个地方重复相同的模式匹配
/// 通过实现 walk_value 方法，可以定义对每个值的处理逻辑。
///
/// 注意：这里只遍历指令的直接参数，不包括块参数
pub trait InstructionValueWalker {
    /// 对每个值执行操作
    fn walk_value(&mut self, value: Value);

    /// 遍历指令中的所有值（不包括控制流参数）
    fn walk_instruction(&mut self, inst: &InstructionData, pool: &ValueVecPool) {
        use InstructionData::*;
        match inst {
            Binary { lhs, rhs, .. } => {
                self.walk_value(*lhs);
                self.walk_value(*rhs);
            }
            Unary { val, .. } | Cast { val, .. } => self.walk_value(*val),
            Call { args, .. } | ReturnCall { args, .. } | Return { returns: args } => {
                if let Some(values) = pool.get(*args) {
                    for v in values {
                        self.walk_value(*v);
                    }
                }
            }
            ArrayAlloc {
                init: ArrayInit::List { vals },
                ..
            } => {
                if let Some(values) = pool.get(*vals) {
                    for v in values {
                        self.walk_value(*v);
                    }
                }
            }
            ArrayGet {
                ptr,
                indices,
                token,
            } => {
                self.walk_value(*ptr);
                if let Some(values) = pool.get(*indices) {
                    for v in values {
                        self.walk_value(*v);
                    }
                }
                self.walk_value(*token);
            }
            ArrayPut {
                ptr,
                indices,
                value,
                token,
            } => {
                self.walk_value(*ptr);
                if let Some(values) = pool.get(*indices) {
                    for v in values {
                        self.walk_value(*v);
                    }
                }
                self.walk_value(*value);
                self.walk_value(*token);
            }
            ArraySlice { ptr, sdims } => {
                self.walk_value(*ptr);
                if let Some(values) = pool.get(*sdims) {
                    for v in values {
                        self.walk_value(*v);
                    }
                }
            }
            Load { addr, token, .. } => {
                self.walk_value(*addr);
                self.walk_value(*token);
            }
            Store {
                addr, value, token, ..
            } => {
                self.walk_value(*addr);
                self.walk_value(*value);
                self.walk_value(*token);
            }
            GlobalScalarGet { global, token } => {
                self.walk_value(*global);
                self.walk_value(*token);
            }
            GlobalScalarSet {
                global,
                value,
                token,
            } => {
                self.walk_value(*global);
                self.walk_value(*value);
                self.walk_value(*token);
            }
            Brif { cond, .. } | BranchTable { idx: cond, .. } => self.walk_value(*cond),
            // Jump: target.args 是控制流参数（传递给目标块），不是指令的直接操作数
            // 根据设计哲学，控制流参数应通过 branch_destination() 访问
            Jump { .. } => {}
            // StackAddr: slot 是 StackSlot 类型，不是 Value 类型
            StackAddr { .. } => {}
            // GlobalAddr: global 是 Global 类型，不是 Value 类型
            GlobalAddr { .. } => {}
            // Unreachable: 没有任何参数
            Unreachable => {}
            // ArrayAlloc: init 字段的 ArrayInit::List 情况已在上面处理（217-226行）
            // 这里覆盖 ArrayInit::Zero 和 ArrayInit::Undef 的情况，不包含值
            ArrayAlloc { .. } => {}
        }
    }
}

/// 收集参数的实现
struct ArgumentCollector(Vec<Value>);

impl InstructionValueWalker for ArgumentCollector {
    fn walk_value(&mut self, value: Value) {
        self.0.push(value);
    }
}

/// 指令映射器trait - 简化版本，基于访问器模式
pub trait InstructionMapper {
    fn map_value(&mut self, value: Value) -> Value;
    fn map_func_ref(&mut self, func_ref: FuncRef) -> FuncRef;
    fn map_global(&mut self, global: Global) -> Global {
        global
    }
    fn map_jump_table(&mut self, table: JumpTable, pool: &mut ValueVecPool) -> JumpTable;

    fn map_value_vec(&mut self, value_vec: ValueVec, pool: &mut ValueVecPool) -> ValueVec {
        if let Some(values) = pool.get(value_vec) {
            let mapped: Vec<Value> = values.iter().map(|v| self.map_value(*v)).collect();
            pool.insert(mapped)
        } else {
            value_vec
        }
    }

    fn map_block_call(&mut self, call: BlockCall, pool: &mut ValueVecPool) -> BlockCall {
        BlockCall {
            block: call.block,
            args: self.map_value_vec(call.args, pool),
        }
    }

    fn map_array_init(&mut self, init: ArrayInit, pool: &mut ValueVecPool) -> ArrayInit {
        match init {
            ArrayInit::List { vals } => ArrayInit::List {
                vals: self.map_value_vec(vals, pool),
            },
            other => other,
        }
    }
}

/// 为可变引用实现InstructionMapper
impl<T: InstructionMapper> InstructionMapper for &mut T {
    fn map_value(&mut self, value: Value) -> Value {
        (**self).map_value(value)
    }

    fn map_value_vec(&mut self, value_vec: ValueVec, pool: &mut ValueVecPool) -> ValueVec {
        (**self).map_value_vec(value_vec, pool)
    }

    fn map_block_call(&mut self, block_call: BlockCall, pool: &mut ValueVecPool) -> BlockCall {
        (**self).map_block_call(block_call, pool)
    }

    fn map_func_ref(&mut self, func_ref: FuncRef) -> FuncRef {
        (**self).map_func_ref(func_ref)
    }

    fn map_jump_table(&mut self, table: JumpTable, pool: &mut ValueVecPool) -> JumpTable {
        (**self).map_jump_table(table, pool)
    }

    fn map_global(&mut self, global: Global) -> Global {
        (**self).map_global(global)
    }

    fn map_array_init(&mut self, init: ArrayInit, pool: &mut ValueVecPool) -> ArrayInit {
        (**self).map_array_init(init, pool)
    }
}

impl InstructionData {
    pub fn branch_destination<'a>(&'a self, jump_tables: &'a JumpTables) -> &'a [BlockCall] {
        match self {
            Self::Jump { target } => std::slice::from_ref(target),
            Self::Brif { targets, .. } => targets.as_slice(),
            Self::BranchTable { table, .. } => jump_tables.get(*table).unwrap().all_branches(),
            _ => &[],
        }
    }

    pub fn branch_destination_mut<'a>(
        &'a mut self,
        jump_tables: &'a mut JumpTables,
    ) -> &'a mut [BlockCall] {
        match self {
            Self::Jump { target } => std::slice::from_mut(target),
            Self::Brif { targets, .. } => targets.as_mut_slice(),
            Self::BranchTable { table, .. } => {
                jump_tables.get_mut(*table).unwrap().all_branches_mut()
            }
            _ => &mut [],
        }
    }

    /// 获取指令的直接操作参数 (Instruction Arguments)
    ///
    /// 设计哲学: 采用分离设计，严格区分指令参数和控制流参数
    /// - 指令参数: 指令本身直接操作/消费的值 (由此方法返回)
    /// - 控制流参数: 传递给目标块的值 (应通过 branch_destination() 等方法访问)
    ///
    /// 这种设计遵循 Cranelift 的设计哲学，将数据流和控制流清晰分离:
    /// - Jump/Brif 等控制流指令只返回条件值，不包含块参数
    /// - 算术/内存指令返回其直接操作的所有值
    /// - 有利于独立的数据流分析和控制流分析
    pub fn arguments(&self, pool: &ValueVecPool) -> Vec<Value> {
        let mut collector = ArgumentCollector(Vec::new());
        collector.walk_instruction(self, pool);
        collector.0
    }

    /// 判断指令是否有副作用
    /// 特别注意: 内存读操作也被视作有副作用 (在内存令牌语义下)
    pub fn has_side_effect(&self) -> bool {
        use InstructionData::*;
        matches!(
            self,
            // 内存写操作
            ArrayPut { .. } | Store { .. } | GlobalScalarSet { .. }
            // 内存读操作 -- 需要内存令牌，表示有内存依赖
            | ArrayGet { .. } | Load { .. } | GlobalScalarGet { .. }
            // 函数调用
            | Call { .. } | ReturnCall { .. }
            // 控制流
            | Jump { .. } | Brif { .. } | BranchTable { .. }
            | Return { .. } | Unreachable
            // 内存分配（会影响栈布局）
            | ArrayAlloc { .. }
        )
    }

    /// 判断指令是否为终结指令
    pub fn is_terminator(&self) -> bool {
        matches!(
            self,
            Self::Jump { .. }
                | Self::Return { .. }
                | Self::ReturnCall { .. }
                | Self::Brif { .. }
                | Self::BranchTable { .. }
                | Self::Unreachable
        )
    }

    /// 判断指令是否有高昂的计算成本（对于复制指令成本过高）
    pub fn has_expensive_cost(&self) -> bool {
        use InstructionData::*;
        match self {
            // 除法和取模运算成本高
            Binary {
                op: BinOp::IDiv | BinOp::IMod | BinOp::FDiv | BinOp::FMod,
                ..
            } => true,

            // 函数调用成本高
            Call { .. } | ReturnCall { .. } => true,

            // 内存操作成本高
            Load { .. } | Store { .. } => true,
            ArrayAlloc { .. } | ArrayGet { .. } | ArrayPut { .. } => true,
            GlobalScalarGet { .. } | GlobalScalarSet { .. } => true,

            // 涉及浮点数的类型转换成本较高
            Cast { to, .. } => {
                // 需要检查源类型是否为浮点（通过val的类型）
                // 但这里只能访问到指令数据，无法获取val的类型
                // 保守起见，只要目标是浮点就认为成本高
                matches!(to, Type::Float32 | Type::Float64)
            }

            // 其他指令成本较低，可以复制
            // Binary的其他运算、Unary、Jump、Brif、Return、Unreachable
            // 以及地址计算: GlobalAddr、StackAddr、ArraySlice
            _ => false,
        }
    }

    /// 判断是否是分支指令
    ///
    pub fn is_branch(&self) -> bool {
        matches!(
            self,
            Self::Jump { .. } | Self::Brif { .. } | Self::BranchTable { .. }
        )
    }

    /// 如果是调用指令，返回被调用函数的引用
    pub fn callee(&self) -> Option<FuncRef> {
        match self {
            InstructionData::Call { func, .. } => Some(*func),
            InstructionData::ReturnCall { func, .. } => Some(*func),
            _ => None,
        }
    }

    /// 判断指令是否为返回指令
    pub fn is_return(&self) -> bool {
        matches!(self, Self::Return { .. })
    }

    /// 深度克隆
    pub fn deep_clone(&self, pool: &mut ValueVecPool) -> Self {
        struct CloneMapper;
        impl InstructionMapper for CloneMapper {
            fn map_value(&mut self, value: Value) -> Value {
                value
            }
            fn map_func_ref(&mut self, func: FuncRef) -> FuncRef {
                func
            }
            fn map_jump_table(&mut self, table: JumpTable, _pool: &mut ValueVecPool) -> JumpTable {
                table
            }
        }
        self.map(CloneMapper, pool)
    }

    /// 使用宏简化的map实现
    ///
    /// 通过宏 map_v! 和 map_vec! 减少重复代码，提高可读性。
    /// 所有值映射操作都通过统一的接口进行。
    pub fn map(&self, mut mapper: impl InstructionMapper, pool: &mut ValueVecPool) -> Self {
        use InstructionData::*;

        // 辅助宏：映射单个值
        macro_rules! map_v {
            ($v:expr) => {
                mapper.map_value($v)
            };
        }

        // 辅助宏：映射值向量
        macro_rules! map_vec {
            ($vec:expr) => {
                mapper.map_value_vec($vec, pool)
            };
        }

        match self {
            Binary { op, lhs, rhs } => Binary {
                op: *op,
                lhs: map_v!(*lhs),
                rhs: map_v!(*rhs),
            },
            Unary { op, val } => Unary {
                op: *op,
                val: map_v!(*val),
            },
            Call { func, args } => Call {
                func: mapper.map_func_ref(*func),
                args: map_vec!(*args),
            },
            Cast { val, to } => Cast {
                val: map_v!(*val),
                to: *to,
            },
            ArrayAlloc {
                elem,
                dims,
                is_const,
                mem_loc,
                init,
            } => ArrayAlloc {
                elem: *elem,
                dims: *dims,
                is_const: *is_const,
                mem_loc: *mem_loc,
                init: mapper.map_array_init(*init, pool),
            },
            ArrayGet {
                ptr,
                indices,
                token,
            } => ArrayGet {
                ptr: map_v!(*ptr),
                indices: map_vec!(*indices),
                token: map_v!(*token),
            },
            ArrayPut {
                ptr,
                indices,
                value,
                token,
            } => ArrayPut {
                ptr: map_v!(*ptr),
                indices: map_vec!(*indices),
                value: map_v!(*value),
                token: map_v!(*token),
            },
            ArraySlice { ptr, sdims } => ArraySlice {
                ptr: map_v!(*ptr),
                sdims: map_vec!(*sdims),
            },
            StackAddr { slot } => StackAddr { slot: *slot },
            GlobalAddr { global } => GlobalAddr { global: *global },
            Load {
                addr,
                offset,
                ty,
                token,
            } => Load {
                addr: map_v!(*addr),
                offset: *offset,
                ty: *ty,
                token: map_v!(*token),
            },
            Store {
                addr,
                offset,
                value,
                ty,
                token,
            } => Store {
                addr: map_v!(*addr),
                offset: *offset,
                value: map_v!(*value),
                ty: *ty,
                token: map_v!(*token),
            },
            GlobalScalarGet { global, token } => GlobalScalarGet {
                global: map_v!(*global),
                token: map_v!(*token),
            },
            GlobalScalarSet {
                global,
                value,
                token,
            } => GlobalScalarSet {
                global: map_v!(*global),
                value: map_v!(*value),
                token: map_v!(*token),
            },
            Jump { target } => Jump {
                target: mapper.map_block_call(*target, pool),
            },
            BranchTable { idx: cond, table } => BranchTable {
                idx: map_v!(*cond),
                table: mapper.map_jump_table(*table, pool),
            },
            Brif { cond, targets } => Brif {
                cond: map_v!(*cond),
                targets: [
                    mapper.map_block_call(targets[0], pool),
                    mapper.map_block_call(targets[1], pool),
                ],
            },
            ReturnCall { func, args } => ReturnCall {
                func: mapper.map_func_ref(*func),
                args: map_vec!(*args),
            },
            Return { returns } => Return {
                returns: map_vec!(*returns),
            },
            Unreachable => Unreachable,
        }
    }
}
