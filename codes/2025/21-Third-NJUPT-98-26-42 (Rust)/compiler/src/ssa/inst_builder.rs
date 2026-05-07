//! 指令构建器
use super::ctx::*;
use super::dfg::*;
use super::ir::*;
use crate::prelude::*;
use core::marker::PhantomData;

//==============================================================================
// 抽象构建器trait
//==============================================================================

/// 最基础构建器
/// TODO: 至今未解决类型推断的问题
/// 由于类型推导需要获取上下文，会有复杂的借用权问题，因此需要采用如下依赖注入:
///
/// ```rust,no-run
/// // compiler/src/ssa/builder.rs:496
/// pub struct FuncInstBuilder<'short, 'long: 'short> {
///     builder: &'short mut FunctionBuilder<'long>,
///     block: Block,
/// }
///
/// // compiler/src/ssa/builder.rs:544
/// fn build(self, data: InstructionData) -> (Inst, &'short mut DataFlowGraph) {
///     // 推导类型并创建结果 -- 不会出现借用权问题
///     let types = self
///         .builder
///         .func
///         .infer_inst_result_types(inst, self.builder.ctx);
///     self.builder.func.dfg.make_inst_results(inst, types);
/// }
/// ```
///
/// 在 FunctionBuilder 和 FuncInstBuilder 推导类型的方式使用:
/// - 上下文注入：FuncInstBuilder 持有 &mut FunctionBuilder，间接访问所有需要的上下文
/// - 职责分离：类型推导逻辑集中在 Function::infer_inst_result_types() 中
/// - 设计缺陷：先创建指令，再根据指令内容推导类型，要反复获取两次
pub trait InstBuilderBase<'f>: Sized {
    fn dfg(&self) -> &DataFlowGraph;

    fn dfg_mut(&mut self) -> &mut DataFlowGraph;

    fn core_ctx(&self) -> &CoreContext;

    fn core_ctx_mut(&mut self) -> &mut CoreContext;

    fn dfg_ctx_mut(&mut self) -> (&mut DataFlowGraph, &mut CoreContext);

    /// 插入一个新的指令，返回指令引用Inst
    /// 它会消耗掉调用它的 builder 对象，这通常意味着这是一个构建过程的终结步骤
    fn build(self, data: InstructionData) -> (Inst, &'f mut DataFlowGraph);
}

/// 直接使用的指令构建器
/// 应该包含所有具体的指令构建方法
pub trait InstBuilder<'f>: InstBuilderBase<'f> {
    // 控制流指令
    fn jump<'a>(
        mut self,
        block_call_label: Block,
        block_call_args: impl IntoIterator<Item = &'a Value>,
    ) -> Inst {
        let block_call = self.dfg_mut().block_call(block_call_label, block_call_args);
        let (inst, _dfg) = self.Jump(block_call);
        inst
    }

    fn return_<'a>(mut self, returns: impl IntoIterator<Item = &'a Value>) -> Inst {
        let returns = self.dfg_mut().make_valuevec(returns);
        let (inst, _dfg) = self.Return_(returns);
        inst
    }

    fn return_call<'a>(mut self, func: FuncRef, args: impl IntoIterator<Item = &'a Value>) -> Inst {
        let args = self.dfg_mut().make_valuevec(args);
        let (inst, _dfg) = self.ReturnCall(func, args);
        inst
    }

    fn brif<'a>(
        mut self,
        cond: Value,
        then_block: Block,
        then_args: impl IntoIterator<Item = &'a Value>,
        else_block: Block,
        else_args: impl IntoIterator<Item = &'a Value>,
    ) -> Inst {
        let then_call = self.dfg_mut().block_call(then_block, then_args);
        let else_call = self.dfg_mut().block_call(else_block, else_args);
        let (inst, _dfg) = self.Brif(cond, [then_call, else_call]);
        inst
    }

    // 整数算术指令
    fn iadd(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IAdd, x, y);
        dfg.first_result(inst)
    }

    fn isub(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::ISub, x, y);
        dfg.first_result(inst)
    }

    fn imul(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IMul, x, y);
        dfg.first_result(inst)
    }

    fn idiv(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IDiv, x, y);
        dfg.first_result(inst)
    }

    fn imod(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IMod, x, y);
        dfg.first_result(inst)
    }

    // 位运算
    fn ishl(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IShl, x, y);
        dfg.first_result(inst)
    }

    fn ishr(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IShr, x, y);
        dfg.first_result(inst)
    }

    fn isar(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::ISar, x, y);
        dfg.first_result(inst)
    }

    fn iand(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IAnd, x, y);
        dfg.first_result(inst)
    }

    fn ior(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IOr, x, y);
        dfg.first_result(inst)
    }

    fn ixor(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::IXor, x, y);
        dfg.first_result(inst)
    }

    // 浮点算术
    fn fadd(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::FAdd, x, y);
        dfg.first_result(inst)
    }

    fn fsub(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::FSub, x, y);
        dfg.first_result(inst)
    }

    fn fmul(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::FMul, x, y);
        dfg.first_result(inst)
    }

    fn fdiv(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::FDiv, x, y);
        dfg.first_result(inst)
    }

    // 比较指令
    fn eq(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::Eq, x, y);
        dfg.first_result(inst)
    }

    fn ne(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::Ne, x, y);
        dfg.first_result(inst)
    }

    fn lt(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::Lt, x, y);
        dfg.first_result(inst)
    }

    fn le(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::Le, x, y);
        dfg.first_result(inst)
    }

    fn gt(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::Gt, x, y);
        dfg.first_result(inst)
    }

    fn ge(self, x: Value, y: Value) -> Value {
        let (inst, dfg) = self.Binary(BinOp::Ge, x, y);
        dfg.first_result(inst)
    }

    // 一元运算
    fn ineg(self, val: Value) -> Value {
        let (inst, dfg) = self.Unary(UnaryOp::INeg, val);
        dfg.first_result(inst)
    }

    fn bnot(self, val: Value) -> Value {
        let (inst, dfg) = self.Unary(UnaryOp::BNot, val);
        dfg.first_result(inst)
    }

    fn fneg(self, val: Value) -> Value {
        let (inst, dfg) = self.Unary(UnaryOp::FNeg, val);
        dfg.first_result(inst)
    }

    fn not(self, val: Value) -> Value {
        let (inst, dfg) = self.Unary(UnaryOp::Not, val);
        dfg.first_result(inst)
    }

    // 常量指令 - 直接创建常量值，不通过指令
    fn iconst32(mut self, val: i32) -> Value {
        let const_val = ConstValue::Int32 { val };
        let value_data = ValueData::Const {
            ty: Type::Int32,
            c: const_val,
        };
        self.dfg_mut().make_value(value_data)
    }

    fn iconst64(mut self, val: i64) -> Value {
        let const_val = ConstValue::Int64 { val };
        let value_data = ValueData::Const {
            ty: Type::Int64,
            c: const_val,
        };
        self.dfg_mut().make_value(value_data)
    }

    fn fconst32(mut self, val: f32) -> Value {
        let const_val = ConstValue::Float32 { val };
        let value_data = ValueData::Const {
            ty: Type::Float32,
            c: const_val,
        };
        self.dfg_mut().make_value(value_data)
    }

    fn bconst(mut self, val: bool) -> Value {
        let const_val = ConstValue::Bool { val };
        let value_data = ValueData::Const {
            ty: Type::Bool,
            c: const_val,
        };
        self.dfg_mut().make_value(value_data)
    }

    // 函数调用
    fn call<'a>(mut self, func: FuncRef, args: impl IntoIterator<Item = &'a Value>) -> Inst {
        let args = self.dfg_mut().make_valuevec(args);
        let (inst, _dfg) = self.Call(func, args);
        inst
    }

    // 类型转换
    fn cast(self, val: Value, to: Type) -> Value {
        let (inst, dfg) = self.Cast(val, to);
        dfg.first_result(inst)
    }

    // 数组操作
    fn array_alloc(
        self,
        elem_ty: Type,
        dims: U32OptVec,
        is_const: bool,
        mem_loc: MemoryLocation,
        init: ArrayInit,
    ) -> Inst {
        let (inst, _dfg) = self.ArrayAlloc(elem_ty, dims, is_const, mem_loc, init);
        inst
    }

    fn array_get<'a>(
        mut self,
        ptr: Value,
        indices: impl IntoIterator<Item = &'a Value>,
        token: Value,
    ) -> Value {
        let indices = self.dfg_mut().make_valuevec(indices);
        let (inst, dfg) = self.ArrayGet(ptr, indices, token);
        dfg.first_result(inst)
    }

    fn array_put<'a>(
        mut self,
        ptr: Value,
        indices: impl IntoIterator<Item = &'a Value>,
        value: Value,
        token: Value,
    ) -> Value {
        let indices = self.dfg_mut().make_valuevec(indices);
        let (inst, dfg) = self.ArrayPut(ptr, indices, value, token);
        dfg.first_result(inst)
    }

    fn array_slice<'a>(mut self, ptr: Value, sdims: impl IntoIterator<Item = &'a Value>) -> Value {
        let sdims = self.dfg_mut().make_valuevec(sdims);
        let (inst, dfg) = self.ArraySlice(ptr, sdims);
        dfg.first_result(inst)
    }

    // 全局标量操作
    fn global_scalar_get(self, global: Value, token: Value) -> Value {
        let (inst, dfg) = self.GlobalScalarGet(global, token);
        dfg.first_result(inst)
    }

    fn global_scalar_set(self, global: Value, value: Value, token: Value) -> Value {
        let (inst, dfg) = self.GlobalScalarSet(global, value, token);
        dfg.first_result(inst)
    }

    // 低级内存操作 - 用于指令展开后的低级IR
    fn stack_addr(self, slot: StackSlot) -> Inst {
        let (inst, _dfg) = self.StackAddr(slot);
        inst
    }

    fn global_addr(self, global: Global) -> Inst {
        let (inst, _dfg) = self.GlobalAddr(global);
        inst
    }

    fn load(self, addr: Value, offset: i32, ty: Type, token: Value) -> Value {
        let (inst, dfg) = self.Load(addr, offset, ty, token);
        dfg.first_result(inst)
    }

    fn store(self, addr: Value, offset: i32, value: Value, ty: Type, token: Value) -> Value {
        let (inst, dfg) = self.Store(addr, offset, value, ty, token);
        dfg.first_result(inst)
    }

    // 内部构建方法
    #[allow(non_snake_case)]
    fn Jump(self, block_call: BlockCall) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Jump { target: block_call };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn Return_(self, returns: ValueVec) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Return { returns };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn ReturnCall(self, func: FuncRef, args: ValueVec) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::ReturnCall { func, args };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn Brif(self, cond: Value, targets: [BlockCall; 2]) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Brif { cond, targets };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn Binary(self, op: BinOp, lhs: Value, rhs: Value) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Binary { op, lhs, rhs };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn Unary(self, op: UnaryOp, val: Value) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Unary { op, val };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn Call(self, func: FuncRef, args: ValueVec) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Call { func, args };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn Cast(self, val: Value, to: Type) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Cast { val, to };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn ArrayAlloc(
        self,
        elem: Type,
        dims: U32OptVec,
        is_const: bool,
        mem_loc: MemoryLocation,
        init: ArrayInit,
    ) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::ArrayAlloc {
            elem,
            dims,
            is_const,
            mem_loc,
            init,
        };
        // ArrayAlloc 返回两个值：ptr 和 token
        // TODO: 检查是否能够确定正确的类型
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn ArrayGet(
        self,
        ptr: Value,
        indices: ValueVec,
        token: Value,
    ) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::ArrayGet {
            ptr,
            indices,
            token,
        };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn ArrayPut(
        self,
        ptr: Value,
        indices: ValueVec,
        value: Value,
        token: Value,
    ) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::ArrayPut {
            ptr,
            indices,
            value,
            token,
        };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn ArraySlice(self, ptr: Value, sdims: ValueVec) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::ArraySlice { ptr, sdims };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn GlobalScalarGet(self, global: Value, token: Value) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::GlobalScalarGet { global, token };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn GlobalScalarSet(
        self,
        global: Value,
        value: Value,
        token: Value,
    ) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::GlobalScalarSet {
            global,
            value,
            token,
        };
        self.build(data)
    }

    // 低级内存操作指令的内部构建方法
    #[allow(non_snake_case)]
    fn StackAddr(self, slot: StackSlot) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::StackAddr { slot };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn GlobalAddr(self, global: Global) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::GlobalAddr { global };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn Load(
        self,
        addr: Value,
        offset: i32,
        ty: Type,
        token: Value,
    ) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Load {
            addr,
            offset,
            ty,
            token,
        };
        self.build(data)
    }

    #[allow(non_snake_case)]
    fn Store(
        self,
        addr: Value,
        offset: i32,
        value: Value,
        ty: Type,
        token: Value,
    ) -> (Inst, &'f mut DataFlowGraph) {
        let data = InstructionData::Store {
            addr,
            offset,
            value,
            ty,
            token,
        };
        self.build(data)
    }
}

//==============================================================================
// 插入构建
//==============================================================================

pub trait InstInserterBase<'f>: Sized {
    fn dfg(&self) -> &DataFlowGraph;

    fn dfg_mut(&mut self) -> &mut DataFlowGraph;

    fn core_ctx(&self) -> &CoreContext;

    fn core_ctx_mut(&mut self) -> &mut CoreContext;

    fn dfg_ctx_mut(&mut self) -> (&mut DataFlowGraph, &mut CoreContext);

    /// Insert a new instruction which belongs to the DFG.
    fn insert_built_inst(self, inst: Inst) -> &'f mut DataFlowGraph;
}

/// 插入构建器 - 用于在函数中插入指令
pub struct InsertBuilder<'f, IIB: InstInserterBase<'f>> {
    inserter: IIB,
    unused: PhantomData<&'f u32>,
}

impl<'f, IIB: InstInserterBase<'f>> InsertBuilder<'f, IIB> {
    pub fn new(inserter: IIB) -> Self {
        Self {
            inserter,
            unused: PhantomData,
        }
    }

    pub fn with_results<Array>(self, reuse: Array) -> InsertReuseBuilder<'f, IIB, Array>
    where
        Array: AsRef<[Option<Value>]>,
    {
        InsertReuseBuilder {
            inserter: self.inserter,
            reuse,
            unused: PhantomData,
        }
    }

    pub fn with_result(self, v: Value) -> InsertReuseBuilder<'f, IIB, [Option<Value>; 1]> {
        self.with_results([Some(v)])
    }
}

impl<'f, IIB: InstInserterBase<'f>> InstBuilderBase<'f> for InsertBuilder<'f, IIB> {
    fn dfg(&self) -> &DataFlowGraph {
        self.inserter.dfg()
    }

    fn dfg_mut(&mut self) -> &mut DataFlowGraph {
        self.inserter.dfg_mut()
    }

    fn core_ctx(&self) -> &CoreContext {
        self.inserter.core_ctx()
    }

    fn core_ctx_mut(&mut self) -> &mut CoreContext {
        self.inserter.core_ctx_mut()
    }

    fn dfg_ctx_mut(&mut self) -> (&mut DataFlowGraph, &mut CoreContext) {
        self.inserter.dfg_ctx_mut()
    }

    fn build(mut self, data: InstructionData) -> (Inst, &'f mut DataFlowGraph) {
        let inst;
        {
            let (dfg, ctx) = self.inserter.dfg_ctx_mut();
            inst = dfg.make_inst(data);
            let types = infer_inst_result_types(dfg, ctx, inst);
            dfg.make_inst_results(inst, types);
        }
        (inst, self.inserter.insert_built_inst(inst))
    }
}

//==============================================================================
// 重用插入构建
//==============================================================================

/// 重用插入构建
pub struct InsertReuseBuilder<'f, IIB, Array>
where
    IIB: InstInserterBase<'f>,
    Array: AsRef<[Option<Value>]>,
{
    inserter: IIB,
    reuse: Array,
    unused: PhantomData<&'f u32>,
}

impl<'f, IIB, Array> InstBuilderBase<'f> for InsertReuseBuilder<'f, IIB, Array>
where
    IIB: InstInserterBase<'f>,
    Array: AsRef<[Option<Value>]>,
{
    fn dfg(&self) -> &DataFlowGraph {
        self.inserter.dfg()
    }

    fn dfg_mut(&mut self) -> &mut DataFlowGraph {
        self.inserter.dfg_mut()
    }

    fn core_ctx(&self) -> &CoreContext {
        self.inserter.core_ctx()
    }

    fn core_ctx_mut(&mut self) -> &mut CoreContext {
        self.inserter.core_ctx_mut()
    }

    fn dfg_ctx_mut(&mut self) -> (&mut DataFlowGraph, &mut CoreContext) {
        self.inserter.dfg_ctx_mut()
    }

    fn build(mut self, data: InstructionData) -> (Inst, &'f mut DataFlowGraph) {
        let inst;
        {
            let (dfg, ctx) = self.inserter.dfg_ctx_mut();
            inst = dfg.make_inst(data);
            let types = infer_inst_result_types(dfg, ctx, inst);
            // Make an `Iterator<Item = Option<Value>>`.
            // let ru = self.reuse.as_ref().iter().cloned();
            // dfg.make_inst_results_reusing(inst, ctrl_typevar, ru);
        }
        (inst, self.inserter.insert_built_inst(inst))
    }
}

//==============================================================================
// 替换构建器
//==============================================================================

/// 替换构建器 - 用于替换现有指令
pub struct ReplaceBuilder<'f> {
    core_ctx: &'f mut CoreContext,
    dfg: &'f mut DataFlowGraph,
    inst: Inst,
}

impl<'f> ReplaceBuilder<'f> {
    /// Create a `ReplaceBuilder` that will overwrite `inst`.
    pub fn new(core_ctx: &'f mut CoreContext, dfg: &'f mut DataFlowGraph, inst: Inst) -> Self {
        Self {
            core_ctx,
            dfg,
            inst,
        }
    }
}

impl<'f> InstBuilderBase<'f> for ReplaceBuilder<'f> {
    fn dfg(&self) -> &DataFlowGraph {
        self.dfg
    }

    fn dfg_mut(&mut self) -> &mut DataFlowGraph {
        self.dfg
    }

    fn core_ctx(&self) -> &CoreContext {
        self.core_ctx
    }

    fn core_ctx_mut(&mut self) -> &mut CoreContext {
        self.core_ctx
    }

    fn dfg_ctx_mut(&mut self) -> (&mut DataFlowGraph, &mut CoreContext) {
        (self.dfg, self.core_ctx)
    }

    fn build(self, data: InstructionData) -> (Inst, &'f mut DataFlowGraph) {
        // Splat the new instruction on top of the old one.
        self.dfg.insts[self.inst] = data;

        if !self.dfg.has_results(self.inst) {
            // The old result values were either detached or non-existent.
            // Construct new ones.
            let types = infer_inst_result_types(self.dfg, self.core_ctx, self.inst);
            self.dfg.make_inst_results(self.inst, types);
        }

        (self.inst, self.dfg)
    }
}

// 为各种Builder实现InstBuilder trait
impl<'f> InstBuilder<'f> for ReplaceBuilder<'f> {}
impl<'f, IIB: InstInserterBase<'f>> InstBuilder<'f> for InsertBuilder<'f, IIB> {}
impl<'f, IIB: InstInserterBase<'f>, Array: AsRef<[Option<Value>]>> InstBuilder<'f>
    for InsertReuseBuilder<'f, IIB, Array>
{
}
