//! SSA IR
//!
//! 具体设计如下:
//! - 无显式Phi指令: 使用参数化的 Terminator(args) 和 BlockParam 机制, 无显式Phi指令
//!     —— 因为块参数本质上就是一个共同约定的寄存器/栈位置，完全不需要知道前驱信息; 块参数不会像Phi那样显示记录自己的前驱信息
//! - 无显式 VarName: 使用 Value::Inst(InstructionRef) 来封装指令生成的值, 纯粹的SSA形式
//! - 显式的内存状态追踪: 类似于 MemorySSA 的内存令牌，但不同之处在于:
//!     - 指针与内存状态分离: ArrayPtr 只负责描述指针本身(元素类型，维度), 不包含任何内存状态信息
//!     - 内存对象 MemoryObject: 内存对象代表一个绝对唯一的内存分配单元，只有三种情况--来源于Alloc，来源于Global，以及未知(作为函数参数传入的)
//!     - 内存令牌 MemoryToken: 内存令牌现在是一个 First-class Value (一等公民值), 具有TypeData::MemToken类型，能够在IR中传递，并与一个MemoryObject关联, 并拥有版本号 —— 代表该内存在某个程序点上的状态
//!         注意: 内存令牌存在哪?: 虽然内存令牌是一个一等公民值, 但是并没有 Value::MemToken {...} 这样的类型，原因是内存令牌只可能由 函数参数, 全局变量, 内存分配, 内存赋值产生
//!               这四个对应于: Value::FnParam, Value::GlobalSymbol, Value::Inst(InstructionRef, idx); 因此实际上内存令牌是通过 Analyzer 根据 get_token(Value) 推导/查询缓存产生的
//!         对于全局变量/全局数组，其初始内存令牌都是 Value::GlobalSymbol { ty: Type::MemToken, sym: Global::new(symbol) }，而当在进行赋值后虽然地址不变，但却产生了新的token
//!     - 显示依赖: 所有内存读写指令Get/Put 都必须显式地消费/产生内存令牌, 从而在SSA图中构建出清晰、无歧义的内存依赖链
//! - 精细化的内存操作语义:
//!     - ArrayAlloc: 创建一个新的 MemoryObject, 并返回两个值 —— Value::Inst(ty, InstRef, idx) (类型为ArrayPtr) 和 Value::Inst(ty, InstRef, idx) (类型为MemToken, 版本为0)
//!     - ArrayPut: 状态修改操作，消费一个 MemToken 并只返回一个值(因为没有修改指针) —— Value::Inst(ty: MemToken, InstRef, idx: 0) 代表内存新状态, 版本号增加
//!     - ArrayGet: 是只读操作, 只能返回基本类型的值, 消费一个 MemToken 来证明其读取的内存状态是合法的, 但不产生新的令牌; 返回的值是 Value::Inst(InstRef) (类型为Int32/Float32/Bool)
//!     - ArraySlice: 是一个纯粹的指针计算操作，完全不涉及内存令牌, 接收一个 ArrayPtr类型的值，产生一个新的 Value::Inst(InstRef) (类型为ArrayPtr)
//!     - Call: 是潜在的状态修改操作, 消费其ptr参数对应的所有 MemToken, 并(在保守情况下)为它们各自产生一个新的 MemToken 作为返回值
//!         函数调用带有约定: 所有传入的指针必须紧随其后传入内存令牌，形成: ret_val, token1', token2', ..., tokenX' = Call @func(..., ptr1, token1, ptr2, token2, ...., ptrX, tokenX)
//!         为了简单起见，Call会返回每一个传入ptr的token，而且不会传入/传出全局变量的token
//! - 低级内存操作语义 (Phase 2引入):
//!     - StackAddr/GlobalAddr: 获取栈槽/全局变量的地址，返回I64类型地址值
//!     - Load: 从地址加载值，消费内存令牌确保正确的内存状态
//!     - Store: 向地址存储值，消费并产生新的内存令牌，更新内存状态版本
//! - 数组显式展平为一维内存条带: 所有数组仍然被看作一维内存条带模型，多维索引的地址计算在后端处理, 不会出现嵌套指针, ArrayPtr的类型dims携带维度信息
//! - 两阶段内存操作设计:
//!     - Phase 1 (opts1): 使用高级数组操作 ArrayAlloc/ArrayGet/ArrayPut/ArraySlice，保持内存令牌语义
//!     - Phase 2 (opts2): 地址展开将高级操作转换为低级操作 StackAddr/GlobalAddr/Load/Store
//!         - ArrayAlloc 被转换为 StackSlot + StackAddr，本质上是换汤不换药，StackSlot 仍保留了完整的类型和初始化信息
//!         - ArrayInit (尽管是 ValueVec 但保证全部元素为常量) 会在后端替换为高效的 memset/memcpy 指令
//!     - 重要: ArrayXXX 操作和 Load/Store 操作在 IR 层面被视为两套完全独立的平行语义
//!         - 即使底层实现相同，它们在 IR 中互不相交，绝对禁止在非转换期间混用
//!         - 只有 addr_expand pass 负责从高级到低级的单向转换
//!         - 保留 ArrayAlloc 可为未来 malloc 产生的堆数组做准备
//!     ArrayGet 被设计为加载索引值，只返回基本元素类型; 而数组切片则必须要使用 ArraySlice
//! - 全局变量对应唯一的内存对象: 每个全局变量都定义一个唯一的 MemoryObject, 对全局变量的访问也必须通过传递其对应的MemToken来进行
//!     - 将全局标量视作 int a[1] 的特殊数组，意味着所有 Global 都使用 ArrayInit 来进行初始化, 但只能通过 GlobalScalarGet/Set 来操作
//!     - 全局数组的访问仍然使用 ArrayGet/ArrayPut 操作, 但需要传入 GlobalSymbol 来获取内存地址
//! - 严格SSA语义的多返回值: 该特性现在至关重要, 用于 ArrayAlloc (返回ptr和token) 和 Call 指令 (返回计算结果和多个新token)
//!     - 具体原理: 使用 Value::Inst(InstructionRef, idx) 来表示多返回值的情况, 尤其是处理带有副作用的函数调用时, 需要消耗和返回token
//!     - %inst1.0, %inst1.1, ... , %inst1.X = Call ... 这并不违反SSA语义, 而且使用该办法解决了多个返回值的指令
//!
//! 其他备注:
//! - 无显式 Phi + BlockParam 来源于 Cranelift编译器的设计, 数据流会更直观, 避免PhiWall
//! - "函数式数组" 汇编上永远都是同一块内存基址。新模型通过分离指针和令牌，更精确地体现了这一点
//! - 竞赛编译器无需考虑编译器性能开销问题
//! - ILSE 天然处理多个函数返回值问题
//! - 需要增强的IR验证器: 该模型依赖于严格的约定
//!
//! 值得注意:
//!
//! A. 内存依赖的显式化
//!     内存令牌在SSA图中创建了不可否认的依赖边，这从根本上保证了内存操作的顺序正确性，杜绝了优化器进行不安全的重排
//!     ```
//!     %ptrA:ptr, $memA_v0:token = array.alloc i32[10]
//!     // alloc创建了对象A，并返回指针ptrA和对象A的初始状态memA_v0
//!     %ptrA:ptr, %memA_v0:token = array.alloc i32[10]
//!     // put消费状态v0，产生新状态v1。这条指令显式依赖 %memA_v0
//!     %memA_v1:token = array.put %ptrA, [0], 42, (%memA_v0)
//!     // get消费状态v1。这条指令显式依赖 %memA_v1
//!     %val:i32 = array.get %ptrA, [1], (%memA_v1)
//!     // 依赖链: %memA_v0 -> %memA_v1 -> %val。优化器绝不会将get移动到put之前。
//!     ```
//!
//! B. 独立内存对象的并行优化
//!     这是Per-Object模型的核心优势。不同内存对象的操作位于独立的依赖链上，互不干扰，为优化器提供了巨大的并行和重排空间
//!     ```
//!     %ptrA:ptr, %memA0:token = array.alloc i32[10]
//!     %ptrB:ptr, %memB0:token = array.alloc i32[10]
//!
//!     // 写A：消费memA0，产生memA1
//!     %memA1:token = array.put %ptrA, [0], 1, (%memA0)
//!
//!     // 写B：消费memB0，产生memB1
//!     %memB1:token = array.put %ptrB, [0], 2, (%memB0)
//!
//!     // 优化器看到，put B操作依赖memB0，与memA0/memA1无关。
//!     // 因此，两个put操作的顺序可以任意调换，甚至可以并行执行。
//!   ```
//!
//! C. 函数调用的保守建模与过程间分析
//!     模型通过"抽象令牌"为函数内部的未知别名情况提供了保底的正确性，并通过IPA来消除这种保守性
//!     这个抽象别名作为 MemoryObject::FnParamUnknown { func: FunctionRef },
//!     例如对于如下C语言代码:
//!     ```c
//!     int array_arg(int a0[], int a1[][2], int a2[][2][2]) {
//!       return a0[0] +
//!              a1[1][0] +
//!              a2[0][0][1];
//!     }
//!     
//!     int main() {
//!         int array[2][2][2] = {0};
//!         int result = array_arg(array[0][0],array[0], array);
//!         array[1][1][1] = 3;
//!         return array[0][1][1];
//!     }   
//!     ```
//!   1. 函数内部-保守: array_arg 内部对所有参数指针的操作，都依赖于一个统一的、抽象的入口令牌 (使用FnParamUnknown构造的内存对象)
//!      ```
//!      // fn array_arg(a0:ptr, a0_tok:token, a1:ptr, a1_tok:token, ...)
//!      // 在没有IPA信息时，编译器保守地将a0_tok, a1_tok等都视为同一个抽象令牌。
//!      // 如果函数是只读的，它最后会原样返回收到的入口令牌。
//!      ```
//!   2. 调用点-具体: main函数在调用时，提供了具体的令牌
//!      ```
//!      // %memA0是具体的、已知的令牌
//!      // call保守地认为内存状态会被改变，故返回一个新的令牌%memA1
//!      %result:i32, %memA1:token = call @array_arg(%slice1, %memA0, %slice2, %memA0, ...)
//!      // 后续操作必须依赖 %memA1
//!      %memA2:token = array.put %ptrA, [1], 99, (%memA1)
//!      ```
//!   3. IPA优化 IPA分析array_arg后发现是readonly的，于是知道memA1和memA0实际上是等价的:
//!     优化器可以将所有对memA1的使用替换为memA0，从而消除memA0 -> %memA1这条伪依赖，使得array.put不再依赖于call
//!
//! D. 通过指针与令牌分析实现"去别名分析"
//!   "去别名分析"的目标，是证明一个get操作可以安全地使用一个更早版本的内存令牌，从而打破依赖
//!   ```
//!   %ptrA:ptr, %memA0:token = array.alloc i32[10]
//!   // 写 a[0]
//!   %memA1:token = array.put %ptrA, [0], 42, (%memA0)
//!   // 读 a[1]
//!   %val:i32 = array.get %ptrA, [1], (%memA1)
//!   ```
//!   分析与优化过程:
//!   1. 优化器看到 %val 依赖于 %memA1
//!   2. 它追溯 %memA1 的来源，找到了 array.put 指令
//!   3. 关键分析: 它比较put的索引[0]和get的索引[1]。通过简单的常量分析，它证明了两者-不重叠
//!   4. 变换: 既然put操作没有影响到get要读取的位置，那么get操作就不需要等待put完成, 它可以安全地使用put操作 - 之前 - 的内存状态
//!   5. IR重写: 优化器将get指令重写为：
//!      ```ssa
//!      // 重写后的get现在依赖v0状态，不再依赖put指令
//!      %val:i32 = array.get %ptrA, [1], (%memA0)
//!      ```
//!   这个变换打破了put和get之间的依赖，为指令调度和其它优化创造了机会，这才是内存令牌模型下，"去别名分析"的真正作用

use crate::prelude::*;
use core::fmt;

/// 别名
pub type ValueVecData = Vec<ValueData>;
pub type U32OptVecData = Vec<Option<u32>>;

// 两个向量池 -- 因为RefMap是带有反向引用去重的,因此U32OptVecPool仍然需要使用RefMap
pub type ValueVecPool = VecPool<ValueVec, Value>;
pub type U32OptVecPool = RefMap<U32OptVec, Vec<Option<u32>>>;

// 全局符号引用 & 函数引用 - 统一使用NameRef作为主键
pub type Global = NameRef;
pub type FuncRef = NameRef;

/// 指令集
#[derive(Clone, Debug)]
pub struct Insts(pub RefMap<Inst, InstructionData>);
impl_refmap_wrapper!(Insts, Inst, InstructionData);

/// 块集
#[derive(Clone, Debug)]
pub struct Blocks(pub RefMap<Block, BlockData>);
impl_refmap_wrapper!(Blocks, Block, BlockData);

impl Blocks {
    pub fn add(&mut self, pool: &mut ValueVecPool) -> Block {
        self.0.insert(BlockData::new(pool))
    }
}

/// 名称集
#[derive(Clone, Debug)]
pub struct Names(pub RefMap<NameRef, String>);
impl_refmap_wrapper!(Names, NameRef, String);

impl Names {
    pub fn name_ref_owned(&mut self, s: String) -> NameRef {
        self.0.intern_owned(s)
    }

    pub fn name_ref<S: AsRef<str>>(&mut self, s: S) -> NameRef {
        self.0.intern(&s.as_ref().to_string())
    }

    pub fn ref_name(&self, name_ref: NameRef) -> &str {
        &self.0[name_ref]
    }
}

//==============================================================================
// 类型系统
//==============================================================================

/// 数组元素类型 - 防止ArrayPtr递归
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ArrayElemType {
    Bool,
    Int32,
    Float32,
}

impl ArrayElemType {
    /// 转换为对应的Type
    pub fn to_type(self) -> Type {
        match self {
            ArrayElemType::Bool => Type::Bool,
            ArrayElemType::Int32 => Type::Int32,
            ArrayElemType::Float32 => Type::Float32,
        }
    }

    /// 从Type转换，如果不是基本类型则返回None
    pub fn from_type(ty: Type) -> Option<Self> {
        match ty {
            Type::Bool => Some(ArrayElemType::Bool),
            Type::Int32 => Some(ArrayElemType::Int32),
            Type::Float32 => Some(ArrayElemType::Float32),
            _ => None,
        }
    }
}

/// 类型数据
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Type {
    Unit,
    Bool,
    Int32,
    Int64,
    Float32,
    Float64,
    // 多维数组平铺为单层 ArrayPtr
    // 例如 int[2][3]   => ArrayPtr { elem: Int32, dims: [2, 3] }
    //      int[][3]    => ArrayPtr { elem: Int32, dims: [None, 3] }
    //      int[]       => ArrayPtr { elem: Int32, dims: [None] }
    ArrayPtr {
        elem: ArrayElemType, // 基本元素类型
        dims: U32OptVec,     // 切片维度 -- None表示未知维度 -- 注意这不是原始维度
    },
    /// 内存令牌类型
    MemToken,
}

impl Type {
    /// 获取ArrayPtr基本元素类型
    pub fn elem_type(&self) -> Option<Type> {
        if let Type::ArrayPtr { elem, .. } = self {
            Some(elem.to_type())
        } else {
            None
        }
    }

    /// 获取ArrayPtr维度信息
    pub fn dims(&self) -> Option<&U32OptVec> {
        if let Type::ArrayPtr { dims, .. } = self {
            Some(dims)
        } else {
            None
        }
    }

    /// 判断是否为指针类型
    pub fn is_ptr(&self) -> bool {
        matches!(self, Type::ArrayPtr { .. })
    }

    /// 二进制大小
    pub fn abi_size(&self) -> usize {
        match self {
            Type::Bool => 1,
            Type::Int32 | Type::Float32 => 4,
            Type::Int64 | Type::Float64 | Type::ArrayPtr { .. } => 8,
            Type::Unit | Type::MemToken => 0,
        }
    }

    /// 二进制对齐 2^align 字节
    pub fn abi_align(&self) -> u8 {
        match self {
            Type::Bool => 0,
            Type::Int32 | Type::Float32 => 2,
            Type::Int64 | Type::Float64 => 3,
            Type::ArrayPtr { .. } => 3,
            _ => panic!("unsupported type abi size: {:?}", self),
        }
    }
}

//==============================================================================
// 数组/内存信息
//==============================================================================

/// 内存位置
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum MemoryLocation {
    Unknown,
    Stack,
    Global,
    Heap,
}

/// 数组初始化模式
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum ArrayInit {
    Zero,                    // 零初始化 -- 全为0/全局数组
    List { vals: ValueVec }, // 使用初始化列表
    Undef,                   // 未定义 -- 局部数组
}

/// 内存对象 -- 唯一且不重叠的内存分配单元
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, strum::Display)]
pub enum MemoryObject {
    /// 必须由 ArrayAlloc 在栈/堆上分配 -- 指向ArrayAlloc指令
    #[strum(serialize = "alloc({inst})")]
    ArrayAlloc { inst: Inst },
    /// 栈槽
    #[strum(serialize = "stack({slot})")]
    StackSlot { slot: StackSlot },
    /// 全局标量内存 + 全局数组内存
    #[strum(serialize = "global({symbol})")]
    Global { symbol: Global },
    /// 代表函数参数传入 -- 来源不明的参数所指向的内存对象
    #[strum(serialize = "fn-p({func})")]
    FnParamUnknown { func: FuncRef },
    // 注意: 没有 BlockParam 类型的内存对象!
    // 因为控制流不会改变内存对象的性质，控制流不会重分配内存
    // 因此控制流上的内存对象一定是清晰的
}

/// 内存令牌
/// 内存令牌存在哪?: 虽然内存令牌是一个一等公民值, 但是并没有 Value::MemToken {...} 这样的类型
/// 原因是内存令牌只可能由 函数参数, 全局变量, 内存分配, 内存赋值产生
/// 这四个对应于: Value::FnParam, Value::GlobalSymbol, Value::Inst(InstructionRef, idx)
/// 因此实际上内存令牌是通过 Analyzer 根据 get_token(Value) 推导/查询缓存产生的
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct MemoryToken {
    /// 内存对象引用
    pub object: MemoryObject,
    /// 内存版本号 -- 类似于 MemorySSA
    pub version: u32,
}

impl fmt::Display for MemoryToken {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}:{}", self.object, self.version)
    }
}

//==============================================================================
// 低级内存操作
//==============================================================================

/// 栈槽数据
#[derive(Clone, Debug, Copy, PartialEq, Eq, Hash)]
pub struct StackSlotData {
    pub elem: Type,     // 元素类型
    pub is_const: bool, // 是否为常量栈槽
    pub size: usize,    // 栈槽字节
    pub align: u8,      // 2^align
    pub init: ArrayInit,
}

/// 栈槽集
#[derive(Clone, Debug)]
pub struct StackSlots(pub RefMap<StackSlot, StackSlotData>);
impl_refmap_wrapper!(StackSlots, StackSlot, StackSlotData);

impl StackSlots {
    pub fn add(
        &mut self,
        elem: Type,
        is_const: bool,
        size: usize,
        align: u8,
        init: ArrayInit,
    ) -> StackSlot {
        self.0.insert(StackSlotData {
            elem,
            is_const,
            size,
            align,
            init,
        })
    }
}

//==============================================================================
// 值系统
//==============================================================================

/// 值定义
/// 在无显式VarName的系统中，Value与其说"等于什么值"，不如说是"要去哪里找这个值"
/// impl ValueData 位于 ir_impl.rs 中
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, strum::Display)]
pub enum ValueData {
    /// 常量
    #[strum(serialize = "{c}")]
    Const { ty: Type, c: ConstValue },
    /// 指令结果
    #[strum(serialize = "%inst.{inst}:{idx}")]
    Inst {
        ty: Type,
        inst: Inst,
        idx: u32, // 只有Call指令可能有多个返回值，因此其他指令都是idx: 0
    },
    /// 块参数
    #[strum(serialize = "%bb{block}.{idx}")]
    BlockParam { ty: Type, block: Block, idx: u32 },
    /// 全局变量的符号 -- 标量/数组的符号均使用 GlobalSymbol
    #[strum(serialize = "@{sym}")]
    GlobalSymbol { ty: Type, sym: Global },
    /// 别名值
    Alias { ty: Type, original: Value },
    /// Union -- 表示两个等价的值用于e-graph优化
    #[strum(serialize = "union({x}, {y})")]
    Union { ty: Type, x: Value, y: Value },
    // 不再有函数参数 -- 函数参数通过入口块块参数来处理
    // #[strum(serialize = "%arg{func}.{idx}")]
    // FnParam { ty: Type, func: FuncRef, idx: u32 },
}

/// 常量值
#[derive(Debug, Copy, Clone, strum::Display)] // PartialEq, Eq 在 ir_impl.rs 中 手动实现了
pub enum ConstValue {
    /// 单元类型
    #[strum(serialize = "unit")]
    Unit,
    /// 布尔值
    #[strum(serialize = "{val}(bool)")]
    Bool { val: bool },
    /// 32位整数
    #[strum(serialize = "{val}(i32)")]
    Int32 { val: i32 },
    /// 64位整数
    #[strum(serialize = "{val}(i64)")]
    Int64 { val: i64 },
    /// 32位浮点数
    #[strum(serialize = "{val}(f32)")]
    Float32 { val: f32 },
    /// 64位浮点数
    #[strum(serialize = "{val}(f64)")]
    Float64 { val: f64 },
}

//==============================================================================
// 操作集
//==============================================================================

/// 二元运算符
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, strum::Display)]
pub enum BinOp {
    // 整数运算
    // INeg → 0 - val, FNeg → 0.0 - val, Not → val == false
    #[strum(to_string = "iadd")]
    IAdd,
    #[strum(to_string = "isub")]
    ISub,
    #[strum(to_string = "imul")]
    IMul,
    #[strum(to_string = "idiv")]
    IDiv,
    #[strum(to_string = "imod")]
    IMod,
    // 位运算
    #[strum(to_string = "ishl")]
    IShl, // 左移 <<
    #[strum(to_string = "ishr")]
    IShr, // 逻辑右移 >>>
    #[strum(to_string = "isar")]
    ISar, // 算术右移 >>
    #[strum(to_string = "iand")]
    IAnd, // 位与 &
    #[strum(to_string = "ior")]
    IOr, // 位或 |
    #[strum(to_string = "ixor")]
    IXor, // 位异或 ^
    // 浮点运算
    #[strum(to_string = "fadd")]
    FAdd,
    #[strum(to_string = "fsub")]
    FSub,
    #[strum(to_string = "fmul")]
    FMul,
    #[strum(to_string = "fdiv")]
    FDiv,
    #[strum(to_string = "fmod")]
    FMod,
    // 比较运算
    #[strum(to_string = "eq")]
    Eq, // ==
    #[strum(to_string = "ne")]
    Ne, // !=
    #[strum(to_string = "lt")]
    Lt, // <
    #[strum(to_string = "le")]
    Le, // <=
    #[strum(to_string = "gt")]
    Gt, // >
    #[strum(to_string = "ge")]
    Ge, // >=
        // ⚠️警告: 绝对不能在二元运算中加入逻辑运算
        // 因为是短路 (short-circuiting) 运算符，它们隐含了控制流
        // 逻辑运算 And-&& / Or-||
}

/// 一元运算符
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, strum::Display)]
pub enum UnaryOp {
    // 整数运算
    #[strum(to_string = "ineg")]
    INeg,
    // 位运算
    #[strum(to_string = "bnot")]
    BNot, // ~ 按位取反
    // 浮点运算
    #[strum(to_string = "fneg")]
    FNeg,
    // 逻辑运算
    #[strum(to_string = "not")]
    Not, // !
}

impl UnaryOp {
    /// 返回指令的结果类型，基于控制类型变量
    pub fn result_types(self, ctrl_typevar: Type) -> Vec<Type> {
        match self {
            // 数值运算：返回与输入相同的类型
            UnaryOp::INeg => vec![ctrl_typevar],
            UnaryOp::BNot => vec![ctrl_typevar], // 位运算：返回与输入相同的整数类型
            UnaryOp::FNeg => vec![ctrl_typevar],

            // 逻辑运算：总是返回Bool
            UnaryOp::Not => vec![Type::Bool],
        }
    }

    /// 判断是否可以从操作数推导控制类型
    pub fn infer_from_operand(self) -> bool {
        match self {
            UnaryOp::INeg | UnaryOp::BNot | UnaryOp::FNeg | UnaryOp::Not => true,
        }
    }
}

//==============================================================================
// 指令集
//==============================================================================

/// 立即数偏移量
/// RISCV的立即数偏移量非常小，这个虽然是u32，但通常情况下为0就行了
pub type Offset = i32;

/// 指令定义
/// impl InstructionData 位于 ir_impl.rs 中
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum InstructionData {
    // === 算术运算 ===
    /// 二元运算
    Binary { op: BinOp, lhs: Value, rhs: Value },
    /// 一元运算
    Unary { op: UnaryOp, val: Value },

    // === 控制流 ===
    /// 函数调用
    Call { func: FuncRef, args: ValueVec },

    // === 类型转换 ===
    /// 类型转换
    Cast { val: Value, to: Type },

    // === 局部数组操作 ===
    /// 分配数组 -- 返回两个值  %ptrA:ptr, %memA0:token = array.alloc i32[10]
    ArrayAlloc {
        elem: Type,              // 元素类型
        dims: U32OptVec,         // 维度信息 -- 理论上直接构造时不允许有None
        is_const: bool,          // 是否为常量数组
        mem_loc: MemoryLocation, // 内存位置枚举
        init: ArrayInit,         // 初始化模式
    },
    /// 读取数组元素 -- 只允许基本类型 Int32/Float32/Bool, 切片请使用 ArraySlice
    ///
    ArrayGet {
        ptr: Value,        // 数组指针 允许是Value::GlobalSymbol(GlobalRef)
        indices: ValueVec, // 多维索引
        token: Value,      // 内存令牌 -- 必须是Value::MemToken(MemoryToken)
    },
    /// 更新数组元素 -- 产生新版本
    ///
    ArrayPut {
        ptr: Value,        // 数组指针
        indices: ValueVec, // 多维索引
        value: Value,      // 要存储的值
        token: Value,      // 内存令牌
    },
    /// 数组切片 - 获取子数组视图 -- 不涉及任何内存令牌, 只涉及指针运算
    /// int a[2][3][4];     => %inst1.0, %inst1.1: ArrayPtr{elem: Int32, dims: [2, 3, 4]}, MemToken (类型)
    ///                                           = ArrayAlloc { elem: Int32, dims: [2, 3, 4], ... }
    /// int b1[] = a[1][2]; => %inst2.0: ArrayPtr{elem: Int32, dims: [4]}
    ///                                 = ArraySlice { ptr: %inst1.0, sdims: [%c1, %c2] }  // %c1=1, %c2=2
    /// int b2[][4] = a[i]; => %inst3.0: ArrayPtr{elem: Int32, dims: [3, 4]}
    ///                                 = ArraySlice { ptr: %inst1.0, sdims: [%i] }  // %i 是变量
    /// int b3[][3][4] = a; => 这种情况不使用 ArraySlice，直接使用 %inst1.0
    ///
    /// ArraySlice.sdims 语义:
    /// sdims 是一个 Value 列表，表示要固定的前几个维度的索引
    /// - sdims 的长度 M 决定了要"消耗"多少个维度
    /// - 如果原始数组是 N 维 [D_1, D_2, ..., D_N]，且 sdims 长度为 M (M < N)
    /// - 则结果是 (N-M) 维数组 [D_{M+1}, ..., D_N]
    /// - sdims 中的每个 Value 可以是常量（立即数）或变量（运行时值）
    ///
    /// 地址计算示例:
    /// 对于 a[2][3][4]，如果 b = a[i]，那么：
    /// - b 的类型是 ArrayPtr{dims: [3, 4]}
    /// - b[j][k] 的地址 = a 的基地址 + (i * 3 * 4 + j * 4 + k) * sizeof(elem)
    /// 其中 i 是运行时确定的值，而 3 和 4 是编译时已知的维度
    ArraySlice {
        ptr: Value, // 原数组指针 -- 要么是Value::GlobalSymbol(GlobalRef), 要么是 Value::Inst(InstructionRef)
        sdims: ValueVec, // 切片索引 -- 每个元素是一个Value，可以是常量或变量
    },

    // === 低级内存操作 ===
    // 注意: 第一阶段数组操作仍然通过高级指令，在后续pass才会展开为地址计算
    /// 获取栈槽地址 => (I64, token)
    /// 在第二阶段，所有ArrayAlloc指令都会被消除，转为 DataFlowGraph 中的 StackSlots
    StackAddr {
        slot: StackSlot, // 栈槽引用
    },
    /// 获取全局变量地址 => (I64, token)
    GlobalAddr {
        global: Global, // 全局变量符号
    },
    /// 内存加载
    Load {
        addr: Value,    // 内存地址 -- 必须是I64
        offset: Offset, // 偏移量 -- 为了适应RISCV架构，尽量是小常量 TODO: 目前强制为0
        ty: Type,       // 加载的类型
        token: Value,   // 内存令牌
    },
    /// 内存存储 -- 产生一个内存令牌
    Store {
        addr: Value,
        offset: Offset,
        value: Value,
        ty: Type, // 存储的类型
        token: Value,
    },

    // === 全局标量操作 ===
    /// 读取全局标量
    GlobalScalarGet {
        global: Value, // Value::GlobalSymbol(GlobalRef) 的包装 -- 但是只允许是标量
        token: Value,  // 内存令牌
    },
    /// 写入全局标量
    GlobalScalarSet {
        global: Value,
        value: Value,
        token: Value,
    },

    // === 终结指令 -- 合并进入Instruction ===
    /// 无条件跳转
    Jump { target: BlockCall },
    /// 跳转表
    BranchTable { idx: Value, table: JumpTable },
    /// 条件分支
    Brif {
        cond: Value,
        targets: [BlockCall; 2], // [true_block, false_block]
    },
    /// 尾调用 - 调用函数并立即返回其结果
    ReturnCall { func: FuncRef, args: ValueVec },
    /// 函数返回
    Return { returns: ValueVec },
    /// 不可达
    Unreachable,
}

/// 跳转表
#[derive(Debug, Clone, PartialEq, Hash)]
pub struct JumpTableData {
    pub table: Vec<BlockCall>,
}

impl JumpTableData {
    pub fn all_branches(&self) -> &[BlockCall] {
        self.table.as_slice()
    }
    pub fn all_branches_mut(&mut self) -> &mut [BlockCall] {
        self.table.as_mut_slice()
    }
}

pub type JumpTables = RefMap<JumpTable, JumpTableData>;

//==============================================================================
//  块数据
//==============================================================================

pub type BlockCall2 = [BlockCall; 2];

/// 块数据
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct BlockData {
    pub params: ValueVec,
}

impl BlockData {
    fn new(pool: &mut ValueVecPool) -> Self {
        let params = pool.insert(Vec::new());
        BlockData { params }
    }
}

/// 块调用
/// 用来包装 BlockParam
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct BlockCall {
    /// 调用的块
    pub block: Block,
    /// 块参数
    pub args: ValueVec,
}

impl BlockCall {
    pub fn new(
        block: Block,
        args: impl IntoIterator<Item = Value>,
        pool: &mut ValueVecPool,
    ) -> Self {
        let args = pool.insert(args.into_iter().collect());
        Self { block, args }
    }

    pub fn block(&self) -> Block {
        self.block
    }

    pub fn set_block(&mut self, block: Block) {
        self.block = block;
    }

    pub fn append_argument(&mut self, arg: Value, pool: &mut ValueVecPool) {
        let args = pool.get_mut(self.args).unwrap();
        args.push(arg);
    }

    /// 块参数
    pub fn args<'a>(&self, pool: &'a ValueVecPool) -> &'a [Value] {
        &pool[self.args]
    }
}
