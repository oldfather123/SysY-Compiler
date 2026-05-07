pub mod cfg;
pub mod to_string;

use crate::ast::table::AnySymbolRef;
use crate::hir::hir::{IRBasicType, IRNumber};
use std::collections::{HashMap, VecDeque};
use std::hash::{Hash, Hasher};
use std::mem;
use crate::util::HashInsertVec;

pub struct MIRRefPool {
    /// don't ask me why i design this struct in this way.
    names: HashMap<AnySymbolRef, u32>,
    names_reverse: HashMap<u32, AnySymbolRef>,
    vars: HashMap<AnySymbolRef, MIRVarRef>,
    arrays: HashMap<AnySymbolRef, MIRArrayRef>,
    index: u32,
    var_global_id: u64,
    ele_ref_global_id: u64,
}

impl MIRRefPool {
    pub fn new() -> Self {
        Self {
            names: Default::default(),
            names_reverse: Default::default(),
            vars: Default::default(),
            arrays: Default::default(),
            index: 1, // if this is an mir_name = 0 , we view this mir_name as nothing
            var_global_id: 0,
            ele_ref_global_id: u64::MAX,
        }
    }
    pub fn add_fn(&mut self, fn_name: AnySymbolRef) -> MIRName {
        self.names.insert(fn_name, self.index);
        self.names_reverse.insert(self.index, fn_name);
        let mir_name = MIRName::new(self.index);
        self.index += 1;
        mir_name
    }

    pub fn add_label(&mut self, label_name: AnySymbolRef) -> MIRName {
        self.names.insert(label_name, self.index);
        self.names_reverse.insert(self.index, label_name);
        let mir_name = MIRName::new(self.index);
        self.index += 1;
        mir_name
    }

    pub fn add_var_by_mir_type(&mut self, symbol_ref: AnySymbolRef, var_type: MIRType, active_type: MIRVarActiveType) -> MIRVarRef {
        self.names.insert(symbol_ref, self.index);
        self.names_reverse.insert(self.index, symbol_ref);
        let mir_name = MIRName::new(self.index);
        self.index += 1;
        self.var_global_id += 1;
        let mut mir_ref = MIRVarRef::new(mir_name, var_type, active_type);
        mir_ref.global_id = self.var_global_id;
        self.vars.insert(symbol_ref, mir_ref.clone());
        mir_ref
    }

    pub fn add_var_by_hir_type(&mut self, symbol_ref: AnySymbolRef, var_type: IRBasicType, active_type: MIRVarActiveType) -> MIRVarRef {
        self.names.insert(symbol_ref, self.index);
        self.names_reverse.insert(self.index, symbol_ref);
        let mir_name = MIRName::new(self.index);
        self.index += 1;
        self.var_global_id += 1;
        match var_type {
            IRBasicType::VOID => {
                panic!("NOT SUPPORT")
            }
            _ => {
                let mut mir_ref = MIRVarRef::new(mir_name, var_type.get_correspond_mir_type(), active_type);
                mir_ref.global_id = self.var_global_id;
                self.vars.insert(symbol_ref, mir_ref.clone());
                mir_ref
            }
        }
    }

    pub fn add_array(&mut self, symbol_ref: AnySymbolRef, dims: Vec<usize>, ele_num: usize, var_type: IRBasicType) -> MIRArrayRef {
        self.names.insert(symbol_ref, self.index);
        self.names_reverse.insert(self.index, symbol_ref);
        let mir_name = MIRName::new(self.index);
        self.index += 1;
        match var_type {
            IRBasicType::VOID => {
                panic!("ERROR HIR")
            }
            _ => {
                let mir_array_ref = MIRArrayRef {
                    name: mir_name,
                    dims,
                    ele_num,
                    array_type: var_type.get_correspond_mir_type(),
                };
                self.arrays.insert(symbol_ref, mir_array_ref.clone());
                mir_array_ref
            }
        }
    }
    pub fn get_lvar_ref(&self, symbol_ref: &AnySymbolRef) -> MIRVarRef {
        self.vars.get(symbol_ref).unwrap().clone()
    }
    pub fn get_larray_ref(&self, symbol_ref: &AnySymbolRef) -> MIRArrayRef {
        self.arrays.get(symbol_ref).unwrap().clone()
    }
    pub fn get_fn_name(&self, symbol_ref: &AnySymbolRef) -> MIRName {
        let index = *self.names.get(symbol_ref).unwrap();
        MIRName::new(index)
    }
    pub fn get_sym_ref(&self, mir_name: &MIRName) -> &AnySymbolRef {
        &self.names_reverse[&mir_name.id]
    }
    // 该函数可以具备对同一数组的同一引用的 设置相同的全局ID , 目前不具备
    pub fn create_array_ele_ref(&mut self, p0: MIRName, p1: MIRType, p2: MIRVarRef) -> MIRArrayEleRef {
        let mut ele_ref = MIRArrayEleRef::new(p0, p1, p2);
        self.ele_ref_global_id -= 1;
        ele_ref.global_id = self.ele_ref_global_id;
        ele_ref
    }
}

pub type MIRLit = IRNumber;

pub struct MIRArrayInit {
    pub inits: Vec<(usize, MIRLit)>,
}

pub struct MIRModule {
    units: Vec<MIRUnit>,
}

impl MIRModule {
    pub fn new() -> MIRModule {
        MIRModule { units: vec![] }
    }
    pub fn add_unit(&mut self, mir_unit: MIRUnit) {
        self.units.push(mir_unit);
    }
    pub fn take_units(&mut self) -> Vec<MIRUnit> {
        mem::replace(&mut self.units, vec![])
    }
}

/// 全局常量可以折叠啊！
pub enum MIRGlDef {
    /// bool 指示这是不是一个 const
    Var(bool, MIRVarRef, MIRLit),
    /// bool 指示这是不是一个 const
    Array(bool, MIRArrayRef, MIRArrayInit),
}

pub enum MIRUnit {
    Fun(MIRFun),
    Def(MIRGlDef),
}

/// MIRName 由RefPool 构造，可以保证 id 是绝对唯一
#[derive(Clone, Copy, Hash, Eq, PartialEq, Debug)]
pub struct MIRName {
    id: u32,
}

impl MIRName {
    /// # 仅仅允许由RefPool调用
    pub fn new(index: u32) -> MIRName {
        Self { id: index }
    }

    /// if this is a label or name equal zero , we view this block as special name
    pub fn special() -> MIRName {
        Self { id: 0 }
    }
}

pub type MIRLabel = MIRName;

#[derive(Clone, Copy, Eq, PartialEq, Debug, Hash)]
pub enum MIRType {
    Int,
    Float,
    Bool,
    Void,
}

/// the definition of temp active
/// 1. definition once
/// 2. use once
/// 3. the use and definition can't cross basic blocks
#[derive(Clone, Copy, Debug, Hash, Eq, PartialEq)]
pub enum MIRVarActiveType {
    /// unspecified variable active type
    #[allow(unused)]
    Unknown,
    /// a simple normal variable
    Normal,
    /// a global variable
    Global,
    /// a temp variable
    Temp,
}

#[derive(Clone, Copy, Debug, Hash, Eq, PartialEq)]
pub struct MIRVarRef {
    name: MIRName,
    var_type: MIRType,
    pub active_type: MIRVarActiveType,
    ssa_sub: u32,
    global_id: u64,
}

impl MIRVarRef {
    pub fn new(mir_name: MIRName, var_type: MIRType, active_type: MIRVarActiveType) -> MIRVarRef {
        MIRVarRef {
            name: mir_name,
            var_type,
            active_type,
            ssa_sub: 0,
            global_id: 0,
        }
    }
    pub fn get_name(&self) -> &MIRName {
        &self.name
    }
    pub fn copy_name(&self) -> MIRName {
        self.name
    }
    pub fn get_type(&self) -> &MIRType {
        &self.var_type
    }

    pub fn global_id(&self) -> u64 {
        self.global_id
    }
}

#[derive(Clone, Hash, Debug)]
pub struct MIRArrayRef {
    name: MIRName,
    /// 纯纯大怨种 MIR ，HIR 不想处理的事情全丢给 MIR
    dims: Vec<usize>,
    pub ele_num: usize,
    array_type: MIRType,
}

impl MIRArrayRef {
    pub fn get_dims(&self) -> &Vec<usize> {
        &self.dims
    }
    pub fn copy_name(&self) -> MIRName {
        self.name
    }
    pub fn get_name(&self) -> &MIRName {
        &self.name
    }
    pub fn get_type(&self) -> MIRType {
        self.array_type.clone()
    }
}

#[derive(Hash, Debug)]
pub struct MIRNamedArrayParam {
    name: MIRName,
    pub offset: MIRVarRef,
    array_type: MIRType,
}

impl MIRNamedArrayParam {
    pub fn new(name: MIRName, offset: MIRVarRef, array_type: MIRType) -> Self {
        MIRNamedArrayParam { name, offset, array_type }
    }
    pub fn get_name(&self) -> &MIRName {
        &self.name
    }
    #[allow(unused)]
    pub fn get_type(&self) -> MIRType {
        self.array_type
    }
    pub fn get_offset(&self) -> &MIRVarRef {
        &self.offset
    }
}

#[derive(Hash, Eq, PartialEq, Clone, Debug)]
pub struct MIRArrayEleRef {
    name: MIRName,
    array_type: MIRType,
    pub offset: MIRVarRef,
    global_id: u64,
}

impl MIRArrayEleRef {
    pub fn new(name: MIRName, array_type: MIRType, offset: MIRVarRef) -> MIRArrayEleRef {
        MIRArrayEleRef {
            name,
            offset,
            array_type,
            global_id: 0,
        }
    }
    pub fn get_name(&self) -> &MIRName {
        &self.name
    }
    pub fn get_offset(&self) -> &MIRVarRef {
        &self.offset
    }
    pub fn get_type(&self) -> MIRType {
        self.array_type
    }
    #[allow(unused)]
    pub fn global_id(&self) -> u64 {
        self.global_id
    }
}

#[derive(Hash, Eq, PartialEq, Clone, Debug)]
pub enum MIRMathLRef {
    Var(MIRVarRef),
    // 如果把数组移出去会影响性能
    ArrayEle(MIRArrayEleRef),
}

impl MIRMathLRef {
    pub fn unwrap_get_var_lref_uncheck(&self) -> &MIRVarRef {
        match self {
            MIRMathLRef::Var(var) => &var,
            MIRMathLRef::ArrayEle(_) => {
                panic!("NEVER HERE")
            }
        }
    }

    pub fn global_id(&self) -> u64 {
        match self {
            MIRMathLRef::Var(var) => var.global_id,
            MIRMathLRef::ArrayEle(ele) => ele.global_id,
        }
    }
}

#[derive(Hash, Debug)]
pub enum MIRNamedParam {
    Right(MIRRight),
    Array(MIRNamedArrayParam),
}

/// MIR的参数不允许出现函数调用，必须转为左值表达式
#[derive(Hash, Debug)]
pub struct MIRInvoke {
    name: MIRName,
    params: Vec<MIRNamedParam>,
    ret_type: MIRType,
}

impl MIRInvoke {
    pub fn new(name: MIRName, params: Vec<MIRNamedParam>, ret_type: MIRType) -> MIRInvoke {
        MIRInvoke { name, params, ret_type }
    }
    pub fn get_ret_type(&self) -> MIRType {
        self.ret_type
    }
    pub fn get_name(&self) -> MIRName {
        self.name
    }
    pub fn params(self) -> Vec<MIRNamedParam> {
        self.params
    }
}

/// MIR 右值
#[derive(Hash, Debug)]
pub enum MIRRight {
    Lit(MIRLit),
    VarRef(MIRVarRef),
    ArrEleRef(MIRArrayEleRef),
    /// bool -> int
    #[allow(unused)]
    CmpRef(MIRBoolLRef),
    Invoke(MIRInvoke),
}

impl MIRRight {
    pub fn get_type(&self) -> MIRType {
        match self {
            MIRRight::Lit(lit) => {
                return match lit {
                    MIRLit::I32(_) => MIRType::Int,
                    MIRLit::F32(_) => MIRType::Float,
                };
            }
            MIRRight::VarRef(l_ref) => l_ref.var_type,
            MIRRight::CmpRef(_) => MIRType::Bool,
            MIRRight::Invoke(invoke) => invoke.ret_type,
            MIRRight::ArrEleRef(arr) => arr.array_type,
        }
    }
}

/// MIR右值前缀
#[derive(Eq, PartialEq, Clone, Copy, Hash, Debug)]
pub enum MIRMathPrefix {
    None,
    /// -
    Neg,
    /// !
    Bang,
    /// !!
    BangBang,
    /// -!
    NegBang,
    /// -!!
    NegBangBang,
}

/// MIR 带数学处理的右值
#[derive(Hash, Debug)]
pub struct MIRMathRight {
    pub prefix: MIRMathPrefix,
    pub math: MIRRight,
}

impl MIRMathRight {
    pub fn new(prefix: MIRMathPrefix, right: MIRRight) -> MIRMathRight {
        MIRMathRight { prefix, math: right }
    }
    /// 查看是否支持解开包装
    /// 数学左值引用，立即数，布尔左值引用,数组元素 支持解开包装
    pub fn is_support_unwrap(&self) -> bool {
        return if self.prefix == MIRMathPrefix::None {
            match &self.math {
                MIRRight::Lit(_) => true,
                MIRRight::VarRef(_) => true,
                MIRRight::ArrEleRef(_) => true,
                MIRRight::CmpRef(_) => true,
                MIRRight::Invoke(_) => false,
            }
        } else {
            false
        };
    }
    pub fn try_unwraped_to_var(&self) -> Option<MIRVarRef> {
        return if self.prefix == MIRMathPrefix::None {
            if let MIRRight::VarRef(var) = self.math {
                Some(var)
            } else {
                None
            }
        } else {
            None
        }
    }
    pub fn into_right_uncheck(self) -> MIRRight {
        self.math
    }
    pub fn get_right(&self) -> &MIRRight {
        &self.math
    }

    #[allow(unused)]
    pub fn right(self) -> MIRRight {
        self.math
    }

    #[allow(unused)]
    pub fn prefix(self) -> MIRMathPrefix {
        self.prefix
    }
    pub fn get_prefix(&self) -> &MIRMathPrefix {
        &self.prefix
    }
}

#[derive(Clone, Copy, Hash, Debug, Eq, PartialEq)]
pub enum MIRMathOper {
    Add,
    Sub,
    Mul,
    Div,
    Rem,
}

/// 只支持数学类型转换,如果变量名相同，不对同一个变量转换
/// 例如从 bool 转换到 math 是不支持的
#[derive(Hash, Debug)]
pub struct MIRMathCvt {
    pub is_def: bool,
    pub left: MIRMathLRef,
    pub right: MIRMathLRef,
}

impl MIRMathCvt {
    pub fn new(is_def: bool, left: MIRMathLRef, right: MIRMathLRef) -> MIRMathCvt {
        MIRMathCvt { is_def, left, right }
    }
}

#[derive(Hash, Debug)]
pub enum MIRBoolOper {
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,

    And,
    Or,
}

/// 与调用的参数类似，在初始化的时候,不允许出现调用
#[derive(Hash, Debug)]
pub struct MIRArrayDef {
    is_const: bool,
    pub array_ref: MIRArrayRef,
    pub init: Vec<(usize, MIRRight)>,
}

impl MIRArrayDef {
    pub fn new(is_const: bool, array_ref: MIRArrayRef, init: Vec<(usize, MIRRight)>) -> MIRArrayDef {
        Self { is_const, array_ref, init }
    }
}

#[derive(Hash, Debug)]
pub struct MIRVarCopyAssign {
    pub is_def: bool,
    pub left: MIRVarRef,
    pub right: Option<MIRMathRight>,
}

impl MIRVarCopyAssign {
    pub fn new(is_def: bool, left: MIRVarRef, right: MIRMathRight) -> MIRVarCopyAssign {
        Self { is_def, left, right:Some(right) }
    }
    pub fn just_def(var:MIRVarRef) -> MIRVarCopyAssign{
        Self{
            is_def: true,
            left: var,
            right: None,
        }
    }
    #[allow(unused)]
    pub fn get_left(&self) -> &MIRVarRef {
        &self.left
    }

    pub fn is_just_define(&self) -> bool{
        match self.right {
            None => {
                if self.is_def{
                    true
                }else {
                    false
                }
            }
            Some(_) => {
                false
            }
        }
    }

    pub fn unwrap_math_right(&self) -> &MIRMathRight {
        self.right.as_ref().unwrap()
    }
    pub fn is_def(&self) -> bool {
        self.is_def
    }
    pub fn is_temp(&self) -> bool {
        self.left.active_type == MIRVarActiveType::Temp
    }
}

#[derive(Hash, Debug)]
pub struct MIRArrayEleCopyAssign {
    pub left: MIRArrayEleRef,
    pub right: MIRMathRight,
}

impl MIRArrayEleCopyAssign {
    pub fn new(left: MIRArrayEleRef, right: MIRMathRight) -> MIRArrayEleCopyAssign {
        Self { left, right }
    }
    #[allow(unused)]
    pub fn get_left(&self) -> &MIRArrayEleRef {
        &self.left
    }
    #[allow(unused)]
    pub fn get_math_right(&self) -> &MIRMathRight {
        &self.right
    }
}

// 语句
#[derive(Hash, Debug)]
#[allow(unused)]
pub struct MIRCopyAssign {
    pub is_def: bool,
    pub is_temp: bool,
    pub left: MIRMathLRef,
    pub right: MIRMathRight,
}

impl MIRCopyAssign {
    #[allow(unused)]
    pub fn new(is_def: bool, is_temp: bool, left: MIRMathLRef, right: MIRMathRight) -> MIRCopyAssign {
        Self {
            is_def,
            is_temp,
            left,
            right,
        }
    }
    #[allow(unused)]
    pub fn get_left(&self) -> &MIRMathLRef {
        &self.left
    }
    #[allow(unused)]
    pub fn get_math_right(&self) -> &MIRMathRight {
        &self.right
    }
    #[allow(unused)]
    pub fn is_def(&self) -> bool {
        self.is_def
    }
}

#[derive(Hash, Debug)]
pub struct MIRVarComputeAssign {
    pub is_def: bool,
    pub left: MIRVarRef,
    pub first: MIRMathLRef,
    pub oper: MIRMathOper,
    pub second: MIRMathLRef,
}

impl MIRVarComputeAssign {
    pub fn new(is_def: bool, left: MIRVarRef, first: MIRMathLRef, oper: MIRMathOper, second: MIRMathLRef) -> MIRVarComputeAssign {
        MIRVarComputeAssign {
            is_def,
            left,
            first,
            oper,
            second,
        }
    }
    pub fn is_def(&self) -> bool {
        self.is_def
    }
    pub fn get_left(&self) -> &MIRVarRef {
        &self.left
    }
    pub fn get_first(&self) -> &MIRMathLRef {
        &self.first
    }
    pub fn get_second(&self) -> &MIRMathLRef {
        &self.second
    }
    pub fn get_oper(&self) -> MIRMathOper {
        self.oper
    }
    pub fn is_temp(&self) -> bool {
        self.left.active_type == MIRVarActiveType::Temp
    }
}


// 语句
#[derive(Hash, Debug)]
pub struct MIRComputeAssign {
    pub is_def: bool,
    /// 标志define是不是对临时变量的define,临时变量只能被使用一次,仅当左引用是变量的时候有效
    pub is_temp: bool,
    pub left: MIRMathLRef,
    pub first: MIRMathLRef,
    pub oper: MIRMathOper,
    pub second: MIRMathLRef,
}

impl MIRComputeAssign {
    pub fn new(is_def: bool, is_temp: bool, left: MIRMathLRef, first: MIRMathLRef, oper: MIRMathOper, second: MIRMathLRef) -> MIRComputeAssign {
        MIRComputeAssign {
            is_def,
            is_temp,
            left,
            first,
            oper,
            second,
        }
    }
    pub fn is_def(&self) -> bool {
        self.is_def
    }
    pub fn is_temp(&self) -> bool {
        self.is_temp
    }
    pub fn get_left(&self) -> &MIRMathLRef {
        &self.left
    }
    pub fn get_first(&self) -> &MIRMathLRef {
        &self.first
    }
    pub fn get_second(&self) -> &MIRMathLRef {
        &self.second
    }
    pub fn get_oper(&self) -> MIRMathOper {
        self.oper
    }
}

#[derive(Hash, Debug)]
pub enum MIRCmpRight {
    // 调用，数组元素引用，以及变量
    MathL(MIRMathLRef),
    // 立即数
    Lit(MIRLit),
    // 比较结果,必须是Bool类型变量
    CmpL(MIRBoolLRef),
}

impl MIRCmpRight {
    pub fn get_source_type(&self) -> MIRType {
        match self {
            MIRCmpRight::MathL(mathl) => match mathl {
                MIRMathLRef::Var(var) => var.get_type().clone(),
                MIRMathLRef::ArrayEle(ele) => ele.get_type(),
            },
            MIRCmpRight::Lit(lit) => lit.get_type().get_correspond_mir_type(),
            MIRCmpRight::CmpL(cmp) => cmp.get_var().var_type,
        }
    }
}

/// 当然，完全有可能出现bool数组
#[derive(Hash, Debug)]
pub struct MIRBoolLRef {
    var: MIRVarRef,
}

impl MIRBoolLRef {
    pub fn new(name: MIRVarRef) -> MIRBoolLRef {
        MIRBoolLRef { var: name }
    }
    pub fn get_var(&self) -> MIRVarRef {
        self.var
    }
}

// 语句
#[derive(Hash, Debug)]
#[allow(unused)]
pub struct MIRBoolCopyAssign {
    is_def: bool,
    is_temp: bool,
    pub left: MIRBoolLRef,
    pub right: MIRCmpRight,
}

impl MIRBoolCopyAssign {
    #[allow(unused)]
    pub fn new(is_def: bool, is_temp: bool, left: MIRBoolLRef, right: MIRCmpRight) -> MIRBoolCopyAssign {
        MIRBoolCopyAssign {
            is_def,
            is_temp,
            left,
            right,
        }
    }
    #[allow(unused)]
    pub fn is_def(&self) -> bool {
        self.is_def
    }
}

// 语句
#[derive(Hash, Debug)]
pub struct MIRBoolCmpAssign {
    pub is_def: bool,
    pub is_temp: bool,
    pub left: MIRBoolLRef,
    pub right: MIRCmpExpr,
}

impl MIRBoolCmpAssign {
    pub fn new(is_def: bool, is_temp: bool, left: MIRBoolLRef, right: MIRCmpExpr) -> MIRBoolCmpAssign {
        MIRBoolCmpAssign {
            is_def,
            is_temp,
            left,
            right,
        }
    }
}

/// # 禁止直接构造，必须使用new
#[derive(Hash, Debug)]
pub struct MIRCmpExpr {
    pub first: MIRCmpRight,
    pub oper: MIRBoolOper,
    pub second: MIRCmpRight,
}

impl MIRCmpExpr {
    pub fn new(first: MIRCmpRight, oper: MIRBoolOper, second: MIRCmpRight) -> Self {
        Self { first, oper, second }
    }
}

#[derive(Hash, Debug)]
pub enum MIRBool {
    Cmp(MIRCmpExpr),
    BoolRef(MIRCmpRight),
}

// 语句
#[derive(Hash, Debug)]
pub struct MIRCondJump {
    pub cond: MIRBool,
    false_exit: MIRLabel,
    true_exit: Option<MIRLabel>,
}

#[derive(Hash, Debug)]
pub struct MIRJump {
    pub label: MIRLabel,
}

impl MIRJump {
    pub fn new(label: MIRLabel) -> MIRJump {
        MIRJump { label }
    }
}

impl MIRCondJump {
    pub fn new(cond: MIRBool, false_exit: MIRLabel, true_exit: Option<MIRLabel>) -> Self {
        Self { cond, false_exit, true_exit }
    }
    pub fn get_false_exit(&self) -> MIRLabel {
        self.false_exit
    }
    pub fn get_true_exit(&self) -> Option<MIRLabel> {
        self.true_exit
    }
}

#[derive(Hash, Debug)]
pub struct MIRReturn {
    // 虽然这里可以有如下优化
    // x = a + b; a0 = x; => a0 = a + b
    // 但是 ret 的使用次数毕竟是少数
    pub value: Option<MIRMathLRef>,
}

impl MIRReturn {
    pub fn just_return() -> MIRReturn {
        MIRReturn { value: None }
    }
    pub fn var_return(lref: MIRMathLRef) -> MIRReturn {
        MIRReturn { value: Some(lref) }
    }
}

// define one register and use two registers or use three registers
#[derive(Hash, Debug)]
pub enum MIRStmt {
    /// 数学拷贝赋值
    MathVarCopyAssign(MIRVarCopyAssign),
    /// 数学计算赋值
    MathVarComputeAssign(MIRVarComputeAssign),
    /// 数组赋值
    ArrayEleAssign(MIRArrayEleCopyAssign),
    /// 布尔计算赋值
    BoolCmpAssign(MIRBoolCmpAssign),
    /// 数组定义
    ArrayDef(MIRArrayDef),
    /// 数学类型转换
    MathCvt(MIRMathCvt),
    /// 条件跳转
    CondJump(MIRCondJump),
    /// 无条件跳转
    Jump(MIRJump),
    /// 返回
    Return(MIRReturn),
    /// 裸调用
    Invoke(MIRInvoke),
    /// 标签
    Label(MIRLabel),
    /// 优先寄存器
    Sync(VecDeque<MIRName>),
    /// 优先spill
    Clear(VecDeque<MIRName>),
    /// 变量生命周期结束
    Drop(MIRName),
}

/// 因为Label是唯一的，所以
/// # 仅仅是对 MIR Label的 Hash
pub struct MIRBlock {
    name_label: MIRLabel,
    pub stmts: HashInsertVec<MIRStmt>,
}

impl Hash for MIRBlock {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.name_label.hash(state)
    }
}

impl PartialEq<Self> for MIRBlock {
    fn eq(&self, other: &Self) -> bool {
        self.name_label == other.name_label
    }
}

impl MIRBlock {
    pub fn append(&mut self, stmts: Vec<MIRStmt>) {
        for stmt in stmts {
            self.stmts.push(stmt);
        }
    }
    pub fn new(name: MIRName) -> MIRBlock {
        MIRBlock {
            name_label: name,
            stmts: HashInsertVec::new(),
        }
    }
    pub fn size(&self) -> usize {
        self.stmts.len()
    }
    pub fn set_name(&mut self, name: MIRLabel) {
        self.name_label = name;
    }
    pub fn add_stmt(&mut self, stmt: MIRStmt) {
        self.stmts.push(stmt);
    }
    pub fn get_name(&self) -> &MIRName {
        &self.name_label
    }
    pub fn copy_name(&self) -> MIRName {
        self.name_label
    }
    pub fn take_stmts(&mut self) -> HashInsertVec<MIRStmt> {
        mem::replace(&mut self.stmts, HashInsertVec::new())
    }
}

pub enum MIRTypedParam {
    Var(MIRVarRef),
    Array(MIRArrayRef),
}

pub struct MIRFun {
    name: MIRName,
    pub params: Vec<MIRTypedParam>,
    blocks: Vec<MIRBlock>,
    pub ret_type: Option<MIRType>,
}

impl MIRFun {
    pub fn new(name: MIRName) -> MIRFun {
        MIRFun {
            name,
            params: vec![],
            blocks: vec![],
            ret_type: None,
        }
    }
    pub fn get_name(&self) -> &MIRName {
        &self.name
    }
    pub fn set_ret_type(&mut self, ret_type: MIRType) {
        self.ret_type = Some(ret_type)
    }
    pub fn take_blocks(&mut self) -> Vec<MIRBlock> {
        mem::replace(&mut self.blocks, vec![])
    }
    pub fn add_block(&mut self, block: MIRBlock) {
        self.blocks.push(block)
    }
    pub fn add_var_param(&mut self, var: MIRVarRef) {
        self.params.push(MIRTypedParam::Var(var))
    }
    pub fn add_array_param(&mut self, array: MIRArrayRef) {
        self.params.push(MIRTypedParam::Array(array))
    }
}
