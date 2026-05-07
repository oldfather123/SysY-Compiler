use crate::allow_nothing;
use crate::ast::table::AnySymbolRef;
use std::cell::RefCell;
use std::collections::VecDeque;
use std::hash::{Hash, Hasher};
use std::mem;
use std::ops::{Add, Div, Mul, Rem, Sub};
use std::rc::Rc;

// HIR 不允许有语法错误
// HIR 不允许出现重名变量

/// 变量类型 HIR阶段反映为变量
#[derive(Copy, Clone, Eq, PartialEq, Hash, Debug)]
pub enum IRBasicType {
    INT,
    FLOAT,
    VOID,
}

/// fixme 注意这里的Hash实现, 是对变量的名称进行哈希,而不包含下标
/// 但是Eq,PartialEq 却实现了包含下标,引用次数等等
#[derive(Clone, PartialEq, Eq, Debug)]
pub struct IRName {
    sym_ref: AnySymbolRef,
    ///SSA形式下该变量的使用次数，方便于活跃检测，只统计右值
    this_ref_num: Rc<RefCell<usize>>,
    ///该变量的总使用次数，死代码删除可能有用，只统计右值
    all_ref_num: Rc<RefCell<usize>>,
    ssa_sub: usize,
}

impl Hash for IRName {
    fn hash<H: Hasher>(&self, hasher: &mut H) {
        self.sym_ref.hash(hasher);
        hasher.finish();
    }
}

impl IRName {
    pub fn get_sym_ref(&self) -> AnySymbolRef {
        self.sym_ref
    }

    pub fn from_ref_zero_sub(symbol_ref: AnySymbolRef) -> Self {
        IRName {
            sym_ref: symbol_ref,
            this_ref_num: Rc::new(RefCell::new(0)),
            all_ref_num: Rc::new(RefCell::new(0)),
            ssa_sub: 0,
        }
    }

    pub fn ref_plus_1(&mut self) {
        *self.this_ref_num.borrow_mut() += 1;
        *self.all_ref_num.borrow_mut() += 1;
    }

    #[allow(unused)]
    pub fn ref_minus_1(&mut self) {
        *self.this_ref_num.borrow_mut() -= 1;
        *self.all_ref_num.borrow_mut() -= 1;
    }

    pub fn get_all_ref_num(&self) -> usize {
        *self.all_ref_num.borrow()
    }

    pub fn get_sub(&self) -> usize {
        self.ssa_sub
    }
}

#[derive(Clone, Eq, PartialEq, Debug)]
pub struct IRTypedVar {
    name: IRName,
    var_type: IRBasicType,
    is_temp:bool
}

pub type IRNamedVar = IRTypedVar;

impl IRTypedVar {
    pub fn new(name: IRName, var_type: IRBasicType) -> Self {
        Self { name, var_type, is_temp: false }
    }

    pub fn get_name(&self) -> &IRName {
        &self.name
    }

    pub fn copy_name(&self) -> IRName {
        self.name.clone()
    }

    pub fn get_name_sym_ref(&self) -> &AnySymbolRef {
        &self.name.sym_ref
    }

    pub fn add_sub(&mut self) {
        self.name.ssa_sub += 1;
    }

    pub fn get_sub(&self) -> usize {
        self.name.ssa_sub
    }

    pub fn new_this_ref(&mut self) {
        self.name.this_ref_num = Rc::new(RefCell::new(0));
    }

    pub fn ref_plus_1(&mut self) {
        *self.name.this_ref_num.borrow_mut() += 1;
        *self.name.all_ref_num.borrow_mut() += 1;
    }

    #[allow(unused)]
    pub fn ref_minus_1(&mut self) {
        *self.name.this_ref_num.borrow_mut() -= 1;
        *self.name.all_ref_num.borrow_mut() -= 1;
    }

    pub fn get_type(&self) -> IRBasicType {
        self.var_type
    }

    pub fn set_temp(&mut self) {
        self.is_temp = true
    }

}

/// 只有当不同类型需要包裹不同的成员的时候，rust enum 会较为方便
#[derive(Clone, Debug)]
pub enum IRTypedParam {
    Var(IRTypedVar),
    /// # Sysy在这里的设计本身就很头大
    Array(IRTypedArray),
}

#[derive(Clone, Debug)]
pub struct IRTypedArray {
    name: IRName,
    dims: Vec<usize>,
    array_type: IRBasicType,
}

// 不要问我为什么这么设计，我也没办法，Sysy的设计本身很头疼
#[derive(Clone, Debug)]
pub struct IRNamedArray {
    name: IRName,
    basic_type: IRBasicType,
    dims: Vec<IRRValue>,
}

impl IRNamedArray {
    pub fn new(name: IRName, basic_type: IRBasicType, dims: Vec<IRRValue>) -> Self {
        Self { name, basic_type, dims }
    }
    pub fn get_type(&self) -> IRBasicType {
        self.basic_type
    }
    pub fn get_name(&self) -> IRName {
        self.name.clone()
    }
    pub fn take_dims(&mut self) -> Vec<IRRValue> {
        mem::replace(&mut self.dims, vec![])
    }
}

impl IRTypedArray {
    pub fn new(name: IRName, dims: Vec<usize>, array_type: IRBasicType) -> Self {
        IRTypedArray { name, dims, array_type }
    }
    pub fn get_type(&self) -> IRBasicType {
        self.array_type
    }
    pub fn get_irname(&self) -> IRName {
        self.name.clone()
    }
    pub fn get_dims_len(&self) -> usize {
        self.dims.len()
    }
    pub fn take_dims(&mut self) -> Vec<usize> {
        mem::replace(&mut self.dims, vec![])
    }
    pub fn get_array_size(&self) -> usize {
        let mut ret = 1;
        for dim in &self.dims {
            ret *= dim;
        }
        ret
    }
}

#[derive(Clone, Debug)]
pub struct IRNamedArrayEle {
    name: IRName,
    dims: Vec<IRRValue>,
    array_type: IRBasicType,
}

impl IRNamedArrayEle {
    pub fn new(name: IRName, array_type: IRBasicType, dims: Vec<IRRValue>) -> IRNamedArrayEle {
        IRNamedArrayEle { name, dims, array_type }
    }
    /// # 危险 这个函数会清空dimension
    pub fn take_dims(&mut self) -> Vec<IRRValue> {
        let ir_rvalue = mem::replace(&mut self.dims, vec![]);
        return ir_rvalue;
    }
    pub fn get_type(&self) -> IRBasicType {
        self.array_type
    }
    pub fn get_dims(&self) -> &Vec<IRRValue> {
        &self.dims
    }
    pub fn get_name(&self) -> IRName {
        self.name.clone()
    }
    pub fn get_name_sym_ref(&self) -> &AnySymbolRef {
        &self.name.sym_ref
    }
}

/// HIR数学符号
#[derive(Clone, Debug)]
pub enum IRMathOpe {
    Add,
    Sub,
    Mul,
    Div,
    /// Remainder
    Rem,
}

/// HIR 的关系符号
#[derive(Clone, Debug)]
pub enum IRBoolOpe {
    Less,
    Greater,
    GreaterOrEqual,
    LessOrEqual,
    Equal,
    NotEqual,

    #[allow(unused)]
    And,
    #[allow(unused)]
    Or,
}

/// 这里是Number 而不是立即数
#[derive(Clone, Debug)]
pub enum IRNumber {
    I32(i32),
    F32(f32),
}

impl Hash for IRNumber {
    fn hash<H: Hasher>(&self, state: &mut H) {
        match self {
            IRNumber::I32(num) => num.hash(state),
            IRNumber::F32(num) => (*num).to_bits().hash(state),
        }
    }
}

impl IRNumber {
    pub fn get_type(&self) -> IRBasicType {
        match self {
            IRNumber::I32(_) => IRBasicType::INT,
            IRNumber::F32(_) => IRBasicType::FLOAT,
        }
    }
    pub fn check_change_type(self,target_type:IRBasicType) -> IRNumber{
        match &self {
            IRNumber::I32(i) => {
                if target_type == IRBasicType::INT {
                    self
                }else {
                    IRNumber::F32(*i as f32)
                }
            }
            IRNumber::F32(f) => {
                if target_type == IRBasicType::FLOAT {
                    self
                }else {
                    IRNumber::I32(*f as i32)
                }
            }
        }
    }
}

// 设计问题，封装不合理，对下一层的传递信息过多，全局变量情况下这里只能是IRNumber但是传递是IRRValue,所以传递信息过多。
// 后期优化的时候，可以采用新建IRGlArrayDefine
// 有时候后端需要知道这个调用需不需要遵循ABI规范，所以需要添加额外的字段来说明是不是外部函数或者是不是一个接口函数
#[derive(Clone, Debug)]
pub struct IRVarDefine {
    name: IRName,
    is_const: bool,
    basic_type: IRBasicType,
    // 如果在全局变量的情况下，init一定是IRNumber
    init: Option<IRRValue>,
    is_temp:bool
}

impl IRVarDefine {
    pub fn new(name: IRName, basic_type: IRBasicType, init: Option<IRRValue>, is_const: bool,is_temp:bool) -> Self {
        Self {
            name,
            is_const,
            basic_type,
            init,
            is_temp,
        }
    }
    pub fn get_type(&self) -> IRBasicType {
        self.basic_type
    }
    pub fn take_init(&mut self) -> Option<IRRValue> {
        mem::replace(&mut self.init, None)
    }
    pub fn get_init(&self) -> &Option<IRRValue> {
        &self.init
    }
    pub fn get_name(&self) -> &IRName {
        &self.name
    }
    pub fn is_const(&self) -> bool {
        self.is_const
    }
    pub fn is_temp(&self) -> bool {
        self.is_temp
    }
}

#[derive(Clone, Debug)]
pub struct IRArrayDefine {
    name: IRName,
    dims: Vec<usize>,
    array_type: IRBasicType,
    is_const: bool,
    init: IRArrayInit,
}

impl IRArrayDefine {
    pub fn new(name: IRName, dims: Vec<usize>, array_type: IRBasicType, is_const: bool, init: IRArrayInit) -> Self {
        Self {
            name,
            dims,
            array_type,
            is_const,
            init,
        }
    }
    pub fn get_type(&self) -> IRBasicType {
        self.array_type
    }
    pub fn take_init(&mut self) -> IRArrayInit {
        let saved = mem::replace(&mut self.init, IRArrayInit::new());
        return saved;
    }
    pub fn get_init(&self) -> &IRArrayInit {
        &self.init
    }
    pub fn is_const(&self) -> bool {
        self.is_const
    }
    pub fn get_name(&self) -> IRName {
        self.name.clone()
    }
    pub fn copy_dims(&self) -> Vec<usize> {
        self.dims.clone()
    }
    pub fn take_dims(&mut self) -> Vec<usize> {
        mem::replace(&mut self.dims, vec![])
    }
    pub fn compute_ele_num(&self) -> usize {
        let mut ret = 1;
        for dim in &self.dims {
            ret *= dim;
        }
        ret
    }
}

#[derive(Clone, Debug)]
pub enum IRVarOrArrayDefine {
    Var(IRVarDefine),
    Array(IRArrayDefine),
}

#[derive(Clone, Debug)]
pub enum IRBoolOrVarDefine {
    VarDef(IRVarDefine),
    BoolDef(IRBoolVarDefine),
}

pub struct HirModule {
    pub units: Vec<HirUnit>,
}

impl HirModule {
    pub fn new() -> Self {
        Self { units: vec![] }
    }
    pub fn add_unit(&mut self, unit: HirUnit) {
        self.units.push(unit)
    }
}

#[derive(Clone, Debug)]
pub struct IRFunDefine {
    name: IRName,
    params: Vec<IRTypedParam>,
    statements: Vec<IRStatement>,
    return_type: IRBasicType,
}

impl IRFunDefine {
    pub fn new(name: IRName, params: Vec<IRTypedParam>, statements: Vec<IRStatement>, return_type: IRBasicType) -> Self {
        Self {
            name,
            params,
            statements,
            return_type,
        }
    }
    pub fn get_name(&self) -> IRName {
        self.name.clone()
    }
    pub fn get_ret_type(&self) -> IRBasicType {
        self.return_type
    }
    pub fn copy_params(&self) -> Vec<IRTypedParam> {
        self.params.clone()
    }
    pub fn take_params(&mut self) -> Vec<IRTypedParam> {
        mem::replace(&mut self.params, vec![])
    }
    pub fn take_stmts(&mut self) -> Vec<IRStatement> {
        let stmt = mem::replace(&mut self.statements, vec![]);
        return stmt;
    }
    pub fn get_stmts(&self) -> &Vec<IRStatement> {
        &self.statements
    }

}

// because the positive signal can be directly ignored ,
// so we do not need to add positive member in here.
#[derive(Clone, Debug, Copy)]
pub enum IRFirstSuffix {
    /// None
    None,
    /// "-"
    Negative,
}

#[derive(Clone, Debug, Copy)]
pub enum IRSecondSuffix {
    /// None
    None,
    /// 偶数次
    EvenBang,
    /// 奇数次
    OddBang,
}

// 关于AST::UnaryOpe 的探讨
// 1. 正号无效 + + + + + - ! + + + + + 1000 = - ! 1000  , 对于基础表达式中出现的正号可以全部去除
// 2. 在！之后的偶数个 - 符无效，但数据会变成 1
// 3. ! 可将后面的表达式压缩成两种值 0 1 , 偶数个!将表达式压缩成0，1(0压缩成0，非0压缩成1),奇数!将表达式压缩后会逆转0 1
// 4. 符号可归整，例如 - + - + - + - + - + - + ! ! ! ! + - + - 1000;
// 首先利用正号无效 简化得到 - - - - - - ! ! ! ! - - 1000;
// 其次 偶数个 - 号 简化得到  ! ! ! ! 1000;
// - ! - ! <number> = -!!<number> =!<number>
// 所以得出结论,!后面的除了!之外的符号可以省略，而!符号可以归约则!最多可以出现两个
// 所以我们一定可以得到如下表达式
// [-] [!{0,2}]<number or var>
#[derive(Clone, Debug, Copy)]
pub struct IRExprPrefix {
    first: IRFirstSuffix,
    second: IRSecondSuffix,
}

impl IRExprPrefix {
    pub fn process(&self, ir_num: IRNumber) -> IRNumber {
        let ir_num = match self.second {
            IRSecondSuffix::None => ir_num,
            IRSecondSuffix::EvenBang => match ir_num {
                IRNumber::I32(i32) => IRNumber::I32(!!i32),
                IRNumber::F32(f32) => IRNumber::I32(!!(f32 as i32)),
            },
            IRSecondSuffix::OddBang => match ir_num {
                IRNumber::I32(i32) => IRNumber::I32(!i32),
                IRNumber::F32(f32) => IRNumber::I32(!(f32 as i32)),
            },
        };
        let ir_num_end = match self.first {
            IRFirstSuffix::None => ir_num,
            IRFirstSuffix::Negative => IRNumber::F32(-1.0) * ir_num,
        };
        return ir_num_end;
    }

    pub fn none() -> Self {
        Self {
            first: IRFirstSuffix::None,
            second: IRSecondSuffix::None,
        }
    }

    pub fn new(first: IRFirstSuffix, second: IRSecondSuffix) -> Self {
        Self { first, second }
    }

    pub fn get_first(&self) -> IRFirstSuffix {
        self.first.clone()
    }

    pub fn get_second(&self) -> IRSecondSuffix {
        self.second.clone()
    }

    pub fn is_none(&self) -> bool {
        match self.first {
            IRFirstSuffix::None => match self.second {
                IRSecondSuffix::None => true,
                IRSecondSuffix::EvenBang => false,
                IRSecondSuffix::OddBang => false,
            },
            IRFirstSuffix::Negative => false,
        }
    }
    /// todo 未经过充分检查
    pub fn combine(&self, mut second: IRExprPrefix) -> IRExprPrefix {
        match self.first {
            IRFirstSuffix::None => {
                match self.second {
                    IRSecondSuffix::None => {
                        return second;
                    }
                    IRSecondSuffix::EvenBang => {
                        match second.get_first() {
                            IRFirstSuffix::None => {
                                match second.get_second() {
                                    IRSecondSuffix::None => {
                                        // !!(...)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::EvenBang => {
                                        // !!(!!...)
                                        return second;
                                    }
                                    IRSecondSuffix::OddBang => {
                                        // !!(!...)
                                        return second;
                                    }
                                }
                            }
                            IRFirstSuffix::Negative => {
                                match second.get_second() {
                                    IRSecondSuffix::None => {
                                        // !!(-nothing) = !!(nothing)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::EvenBang => {
                                        // !!(-!!...) = !!(!!...) = !!!! 可归约
                                        return second;
                                    }
                                    IRSecondSuffix::OddBang => {
                                        // !!(-!...)
                                        return second;
                                    }
                                }
                            }
                        }
                    }
                    IRSecondSuffix::OddBang => {
                        match second.get_first() {
                            IRFirstSuffix::None => {
                                match second.get_second() {
                                    IRSecondSuffix::None => {
                                        // !(...)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::EvenBang => {
                                        // !(!!...)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::OddBang => {
                                        // !(!...)
                                        return IRExprPrefix::new(IRFirstSuffix::None, IRSecondSuffix::EvenBang);
                                    }
                                }
                            }
                            IRFirstSuffix::Negative => {
                                match second.get_second() {
                                    IRSecondSuffix::None => {
                                        // -!(-nothing)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::EvenBang => {
                                        // -!(-!!...)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::OddBang => {
                                        // -!(-!...)
                                        return IRExprPrefix::new(IRFirstSuffix::Negative, IRSecondSuffix::EvenBang);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            IRFirstSuffix::Negative => {
                match self.second {
                    IRSecondSuffix::None => {
                        // - (second_suffix)
                        match second.first {
                            IRFirstSuffix::None => {
                                second.first = IRFirstSuffix::Negative;
                                return second.clone();
                            }
                            IRFirstSuffix::Negative => {
                                second.first = IRFirstSuffix::None;
                                return second.clone();
                            }
                        }
                    }
                    IRSecondSuffix::EvenBang => {
                        match second.get_first() {
                            IRFirstSuffix::None => {
                                match second.get_second() {
                                    IRSecondSuffix::None => {
                                        // -!!(nothing)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::EvenBang => {
                                        // -!!(!!...)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::OddBang => {
                                        // -!!(!...)
                                        return IRExprPrefix::new(IRFirstSuffix::Negative, IRSecondSuffix::OddBang);
                                    }
                                }
                            }
                            IRFirstSuffix::Negative => {
                                match second.get_second() {
                                    IRSecondSuffix::None => {
                                        // -!!(-nothing)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::EvenBang => {
                                        // -!!(-!!...)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::OddBang => {
                                        // -!!(-!...)
                                        return IRExprPrefix::new(IRFirstSuffix::Negative, IRSecondSuffix::OddBang);
                                    }
                                }
                            }
                        }
                    }
                    IRSecondSuffix::OddBang => {
                        match second.get_first() {
                            IRFirstSuffix::None => {
                                match second.get_second() {
                                    IRSecondSuffix::None => {
                                        // -!(nothing)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::EvenBang => {
                                        // -!(!!...)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::OddBang => {
                                        // -!(!...)
                                        return IRExprPrefix::new(IRFirstSuffix::Negative, IRSecondSuffix::EvenBang);
                                    }
                                }
                            }
                            IRFirstSuffix::Negative => {
                                match second.get_second() {
                                    IRSecondSuffix::None => {
                                        // -!(-nothing)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::EvenBang => {
                                        // -!(-!!...)
                                        return self.clone();
                                    }
                                    IRSecondSuffix::OddBang => {
                                        // -!(-!...)
                                        return IRExprPrefix::new(IRFirstSuffix::Negative, IRSecondSuffix::EvenBang);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#[derive(Clone, Debug)]
pub enum IRBasicExpr {
    /// 常量
    Literal(IRNumber),
    /// 变量
    Var(IRNamedVar),
    /// 数组元素引用
    ArrayEleRef(IRNamedArrayEle),
}

impl IRBasicExpr {
    pub fn get_type(&self) -> IRBasicType{
        match self {
            IRBasicExpr::Literal(lit) => {
                lit.get_type()
            }
            IRBasicExpr::Var(var) => {
                var.var_type
            }
            IRBasicExpr::ArrayEleRef(ele_ref) => {
                ele_ref.array_type
            }
        }
    }
}

/*
    sysy 不支持 (a < b) < c 这种表达式，
    在测试52——short_circuit.sy中，
*/
#[derive(Clone, Debug)]
pub enum IRExpr {
    /// 需调用
    Invoke(IRExprPrefix, IRInvoke),
    /// 基本数学表达式
    Basic(IRExprPrefix, IRBasicExpr),
    /// 需计算
    Math(IRExprPrefix, IRBasicExpr, IRMathOpe, IRBasicExpr),
    /// 布尔结果，保留作为Sysy语言扩展的接口
    #[allow(unused)]
    Bool(Box<IRBool>),
}

/// 初始化第一个值同时初始化了第三个值而不初始化第二个值的情况是存在的
#[derive(Clone, Debug)]
pub struct IRArrayInit {
    /// 如果在全局变量的情况下，init一定是IRNumber
    pub init: Vec<(usize, IRExpr)>,
}

impl IRArrayInit {
    pub fn new() -> IRArrayInit {
        IRArrayInit { init: vec![] }
    }
}

#[derive(Clone, Debug)]
pub struct IRInvoke {
    pub fun_name: IRName,
    pub return_type: IRBasicType,
    pub variables: Vec<IRNamedParam>,
    pub line_at:usize
}

pub type IRStatements = Vec<IRStatement>;

/// 右值
pub type IRRValue = IRExpr;

impl IRRValue {
    pub fn get_type_uncheck(&self) -> IRBasicType {
        match self {
            IRRValue::Invoke(_, invoke) => {
                invoke.return_type
            }
            IRRValue::Basic(_, basic) => {
                basic.get_type()
            }
            IRRValue::Math(_, _, _, _) => {
                panic!("math expr type is unknown")
            }
            IRRValue::Bool(_) => {
                panic!("NEVER HERE")
            }
        }
    }

    pub fn combine_before_suffix(self, suffix_2: IRExprPrefix) -> IRRValue {
        match self {
            IRRValue::Invoke(suffix, invoke) => IRRValue::Invoke(suffix_2.combine(suffix), invoke),
            IRRValue::Basic(suffix, basic) => IRRValue::Basic(suffix_2.combine(suffix), basic),
            IRRValue::Math(suffix, a, b, c) => IRRValue::Math(suffix_2.combine(suffix), a, b, c),
            IRRValue::Bool(_) => {
                panic!("NOT SUPPORT")
            }
        }
    }

    pub fn into_ir_number(self) -> IRNumber {
        match self {
            IRRValue::Basic(pre, var) => {
                match pre.get_first() {
                    IRFirstSuffix::None => {
                        allow_nothing!();
                    }
                    IRFirstSuffix::Negative => {
                        panic!("NEVER HERE");
                    }
                }
                match pre.get_second() {
                    IRSecondSuffix::None => {
                        allow_nothing!();
                    }
                    IRSecondSuffix::EvenBang => {
                        panic!("NEVER HERE");
                    }
                    IRSecondSuffix::OddBang => {
                        panic!("NEVER HERE");
                    }
                }
                match var {
                    IRBasicExpr::Literal(lit) => {
                        return lit;
                    }
                    _ => {
                        panic!("ERROR HIR")
                    }
                }
            }
            _ => {
                panic!("ERROR HIR")
            }
        }
    }

    pub fn try_into_ir_number(&self) -> Option<IRNumber>{
        match self {
            IRRValue::Basic(prefix, lit) => {
                if prefix.is_none() {
                    match lit {
                        IRBasicExpr::Literal(lit) => {
                            Some(lit.clone())
                        }
                        _ => {
                            None
                        }
                    }
                }else {
                    None
                }
            },
            IRRValue::Math(prefix,b,c,d) => {
                if prefix.is_none() {
                    match b {
                        IRBasicExpr::Literal(num1) => {
                            match d {
                                IRBasicExpr::Literal(num2) => {
                                    let result =  match c {
                                        IRMathOpe::Add => {
                                            num1 + num2
                                        }
                                        IRMathOpe::Sub => {
                                            num1 - num2
                                        }
                                        IRMathOpe::Mul => {
                                            num1 * num2
                                        }
                                        IRMathOpe::Div => {
                                            num1 / num2
                                        }
                                        IRMathOpe::Rem => {
                                            num1 % num2
                                        }
                                    };
                                    Some(result)
                                }
                                _ => {
                                    None
                                }
                            }
                        }
                        _ => {
                            None
                        }
                    }
                }else {
                    None
                }
            }
            _ => {
                None
            }
        }
    }
}

#[derive(Clone, Debug)]
pub enum IRNamedParam {
    /// 传递了右值，第一个值是函数的形参类型
    IRRValue(IRBasicType, IRRValue),
    /// 传递了数组
    IRNamedArray(IRNamedArray),
}

// sysy 不支持对整个数组新赋值，所以这里左值只有两种
/// 左值
#[derive(Clone, Debug)]
pub enum IRLValue {
    Var(IRNamedVar),
    ArrayEle(IRNamedArrayEle),
}

#[derive(Clone, Debug)]
pub struct IRBoolVarDefine {
    name: IRName,
    pub init: IRBool,
    is_temp:bool
}

impl IRBoolVarDefine {
    pub fn new(name: IRName, init: IRBool,is_temp:bool) -> IRBoolVarDefine {
        IRBoolVarDefine { name, init ,is_temp}
    }
    pub fn is_temp(&self) -> bool{
        self.is_temp
    }
    pub fn get_name(&self) -> IRName {
        return self.name.clone();
    }
    pub fn get_init(&self) -> &IRBool {
        &self.init
    }
}

pub type IRBoolAssign = IRBoolVarDefine;

#[derive(Clone, Debug)]
pub struct IRBoolVar {
    name: IRName,
}

impl IRBoolVar {
    pub fn get_name_sym_ref(&self) -> &AnySymbolRef {
        &self.name.sym_ref
    }
}

impl IRBoolVar {
    pub fn new(irname: IRName) -> IRBoolVar {
        IRBoolVar { name: irname }
    }
    pub fn get_name(&self) -> IRName {
        self.name.clone()
    }
}

/// # IR的布尔值来源
#[derive(Clone, Debug)]
pub enum IRRelSource {
    /// 常量
    Literal(IRNumber),
    /// 变量
    Var(IRNamedVar),
    /// 数组元素引用 ,
    ArrayEleRef(IRNamedArrayEle),
    /// 其他计算结果
    BasicBool(IRBoolVar),
}

/// # IR的Bool值
#[derive(Clone, Debug)]
pub enum IRBool {
    Basic(IRRelSource),
    Rel(IRRelSource, IRBoolOpe, IRRelSource),
}

/// # IR 的布尔值运算
#[derive(Clone, Debug)]
pub enum IRBoolExpr {
    // raw:
    // a < invoke(10);
    // simply:
    // c = invoke(10);
    // a < c;
    /// 单出口
    Bool(Vec<IRBoolOrVarDefine>, IRBool),
    /// 单出口
    And(Vec<IRBoolExpr>),
    /// 多出口
    Or(Vec<IRBoolExpr>),
}

#[derive(Clone, Debug)]
pub struct IRIf {
    pub condition: IRBoolExpr,
    pub statements: IRStatements,
}

#[derive(Clone, Debug)]
pub struct IRAssign {
    pub left: IRLValue,
    pub right: IRRValue,
}

#[derive(Clone, Debug)]
pub struct IRBranches {
    pub first_if: IRIf,
    pub elif: VecDeque<IRElseIf>,
    pub last_else: Option<IRElse>,
}

pub type IRElseIf = IRIf;

pub type IRElse = IRStatements;

pub type IRWhile = IRIf;

#[derive(Clone, Debug)]
pub enum IRStatement {
    VarDef(IRVarDefine),
    #[allow(unused)]
    BoolDef(IRBoolVarDefine),
    ArrDef(IRArrayDefine),
    Assign(IRAssign),
    #[allow(unused)]
    BoolAssign(IRBoolAssign),
    Branches(IRBranches),
    FunInvoke(IRInvoke),
    EmptyExpr(IRBoolExpr),
    Block(IRStatements),
    While(IRWhile),
    Continue,
    Break,
    Return(Option<IRExpr>),
}

/// HIR单元
/// 全局数组定义，全局变量定义，函数定义，
pub enum HirUnit {
    /// 全局变量或者常量必然是Inline的，所以这里必须是IRNumber，不能出现其他情况，
    GLVarDefine(IRVarDefine),
    GLArrayDefine(IRArrayDefine),
    GlDefines(Vec<IRVarOrArrayDefine>),
    FunDefine(IRFunDefine),
    ExternalFun(IRExternalFun),
}

pub struct IRExternalFun {
    pub name: IRName,
    pub params: Vec<IRTypedParam>,
    pub return_type: IRBasicType,
}

impl Add for &IRNumber {
    type Output = IRNumber;

    fn add(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa + numb),
                IRNumber::F32(numb) => IRNumber::I32(numa + *numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(*numa as i32 + numb),
                IRNumber::F32(numb) => IRNumber::F32(numa + numb),
            },
        }
    }
}

impl Add for IRNumber {
    type Output = IRNumber;

    fn add(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa + numb),
                IRNumber::F32(numb) => IRNumber::I32(numa + numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa as i32 + numb),
                IRNumber::F32(numb) => IRNumber::F32(numa + numb),
            },
        }
    }
}
impl Sub for &IRNumber {
    type Output = IRNumber;

    fn sub(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa - numb),
                IRNumber::F32(numb) => IRNumber::I32(numa - *numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(*numa as i32 - numb),
                IRNumber::F32(numb) => IRNumber::F32(numa - numb),
            },
        }
    }
}

impl Sub for IRNumber {
    type Output = IRNumber;

    fn sub(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa - numb),
                IRNumber::F32(numb) => IRNumber::I32(numa - numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa as i32 - numb),
                IRNumber::F32(numb) => IRNumber::F32(numa - numb),
            },
        }
    }
}
impl Mul for &IRNumber {
    type Output = IRNumber;

    fn mul(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa * numb),
                IRNumber::F32(numb) => IRNumber::I32(numa * *numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(*numa as i32 * numb),
                IRNumber::F32(numb) => IRNumber::F32(numa * numb),
            },
        }
    }
}

impl Mul for IRNumber {
    type Output = IRNumber;

    fn mul(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa * numb),
                IRNumber::F32(numb) => IRNumber::I32(numa * numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa as i32 * numb),
                IRNumber::F32(numb) => IRNumber::F32(numa * numb),
            },
        }
    }
}

impl Div for &IRNumber {
    type Output = IRNumber;

    fn div(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa / numb),
                IRNumber::F32(numb) => IRNumber::I32(numa / *numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(*numa as i32 / numb),
                IRNumber::F32(numb) => IRNumber::F32(numa / numb),
            },
        }
    }
}

impl Div for IRNumber {
    type Output = IRNumber;

    fn div(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa / numb),
                IRNumber::F32(numb) => IRNumber::I32(numa / numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa as i32 / numb),
                IRNumber::F32(numb) => IRNumber::F32(numa / numb),
            },
        }
    }
}

impl Rem for &IRNumber {
    type Output = IRNumber;

    fn rem(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa % numb),
                IRNumber::F32(numb) => IRNumber::I32(numa % *numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(*numa as i32 % numb),
                IRNumber::F32(numb) => IRNumber::F32(numa % numb),
            },
        }
    }
}

impl Rem for IRNumber {
    type Output = IRNumber;

    fn rem(self, other: Self) -> Self::Output {
        match self {
            IRNumber::I32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa % numb),
                IRNumber::F32(numb) => IRNumber::I32(numa % numb as i32),
            },
            IRNumber::F32(numa) => match other {
                IRNumber::I32(numb) => IRNumber::I32(numa as i32 % numb),
                IRNumber::F32(numb) => IRNumber::F32(numa % numb),
            },
        }
    }
}
