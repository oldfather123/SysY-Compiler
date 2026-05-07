use crate::allow_nothing;
use crate::ast::BasicType;
use crate::hir::hir::{IRBasicType, IRBoolVar, IRName, IRNamedVar, IRNumber, IRTypedArray, IRTypedParam, IRTypedVar};
use crate::lexer::symbol::Symbol;
use crate::util::generate_random_str;
use std::cell::{RefCell, UnsafeCell};
use std::collections::hash_map::DefaultHasher;
use std::collections::HashMap;
use std::fmt::{Display, Formatter};
use std::hash::{Hash, Hasher};
use std::mem::transmute;
use std::rc::Rc;
use WarpedType::{Array, Var};

/// todo 这个结构体会和常量传播相关联，后期适配

pub enum AvailableQueryResult {
    NotConst,
    NotFound,
    /// 表明该量不可用，而且一定是非常量
    NotConstUnAvailable,
    Array,
    Available(IRNumber),
}

#[derive(Eq, PartialEq)]
pub enum VarQueryResult {
    Const,
    Array,
    NotFound,
    Ok(IRNamedVar),
}

#[derive(Eq, PartialEq)]
pub enum AnyQueryResult {
    Array,
    ConstArray,
    Var,
    ConstVar,
    NotFound,
}

/// 注意NameRef 的构造必须只能从NameTable中
#[derive(Hash, Eq, PartialEq, Clone, Debug, Copy)]
pub struct AnySymbolRef {
    id: u64,
}

#[derive(Clone, Hash, Eq, PartialEq)]
pub struct AnySymbol {
    name: String,
}

impl Display for AnySymbol {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.name)
    }
}

impl AnySymbol {
    pub fn new_rand(suffix: &str) -> Self {
        let str = generate_random_str(10);
        Self {
            name: format!("{}{}", suffix, str),
        }
    }

    pub fn new_rand_underscore(suffix: &str) -> Self {
        let str = generate_random_str(10);
        Self {
            name: format!("{}_{}", suffix, str),
        }
    }
    #[allow(unused)]
    pub fn new_rand_underscore_with_colon(suffix: &str) -> Self {
        let str = generate_random_str(10);
        Self {
            name: format!("{}_{}:", suffix, str),
        }
    }

    pub fn from_symbol(symbol: Symbol) -> AnySymbol {
        Self { name: symbol.string }
    }

    pub fn new(str: &str) -> AnySymbol {
        Self { name: str.to_string() }
    }

    pub fn get_string(&self) -> String {
        self.name.clone()
    }
}

pub struct SymbolTable {
    names: HashMap<AnySymbol, AnySymbolRef>,
    names_reverse: HashMap<AnySymbolRef, AnySymbol>,
}

impl SymbolTable {
    pub fn new() -> Self {
        Self {
            names: HashMap::new(),
            names_reverse: HashMap::new(),
        }
    }

    pub fn add_symbol(&mut self, symbol: AnySymbol) -> AnySymbolRef {
        let mut s = DefaultHasher::new();
        symbol.hash(&mut s);
        let hashcode = s.finish();
        let name_ref = AnySymbolRef { id: hashcode };
        let ref_of_symbol = name_ref.clone();
        self.names.insert(symbol.clone(), name_ref.clone());
        self.names_reverse.insert(name_ref, symbol);
        ref_of_symbol
    }

    pub fn add_symbol_with_mangle(&mut self, symbol: AnySymbol) -> AnySymbolRef {
        let sym_ref = self.get_symbol_ref(&symbol);
        let need_mangle = match sym_ref {
            None => false,
            Some(_) => true,
        };
        let symbol = if !need_mangle {
            symbol
        } else {
            // 这里默认采用了带下划线
            AnySymbol::new_rand_underscore(&symbol.name)
        };
        let mut s = DefaultHasher::new();
        symbol.hash(&mut s);
        let hashcode = s.finish();
        let name_ref = AnySymbolRef { id: hashcode };
        let ref_of_symbol = name_ref.clone();
        self.names.insert(symbol.clone(), name_ref.clone());
        self.names_reverse.insert(name_ref, symbol);
        ref_of_symbol
    }

    pub fn get_symbol_ref(&self, name: &AnySymbol) -> Option<AnySymbolRef> {
        return match self.names.get(name) {
            None => None,
            Some(name) => Some(name.clone()),
        };
    }
    pub fn get_symbol(&self, sym_ref: &AnySymbolRef) -> AnySymbol {
        // NameRef 没有公有构造，所以这里肯定是安全的
        self.names_reverse.get(sym_ref).unwrap().clone()
    }
}

#[derive(Clone)]
enum WarpedType {
    Var(bool, IRTypedVar),
    Array(bool, IRTypedArray),
}

#[derive(Clone)]
pub struct FnDefQuery {
    pub ret_type: BasicType,
    pub fn_name: IRName,
    pub params: Vec<IRTypedParam>,
}

/// VariableLayer 与 IR无关，必须让IR在结构上彻底与该结构体脱离
pub(crate) struct VariableLayer<'a> {
    var_pool: HashMap<Symbol, WarpedType>,
    bool_pool: HashMap<Symbol, IRBoolVar>,
    fn_pool: HashMap<Symbol, FnDefQuery>,
    available_pool: HashMap<Symbol, IRNumber>,
    last_layer: Option<Rc<RefCell<&'a mut VariableLayer<'a>>>>, // fixme 我只是比较懒
}

impl<'a> VariableLayer<'a> {
    pub(crate) fn new() -> Self {
        VariableLayer {
            var_pool: HashMap::new(),
            bool_pool: Default::default(),
            fn_pool: HashMap::new(),
            available_pool: HashMap::new(),
            last_layer: None,
        }
    }

    /// 创建一个继承上一个VariableLayer的VariableLayer
    pub(crate) fn new_nested_layer(&self) -> VariableLayer<'a> {
        let mut layer = VariableLayer::new();
        let ret = UnsafeCell::new(self);
        // fixme 这实在是没有办法的办法，除非找到更好的办法，否则这里不要动
        // 另外，证明为什么这里是安全的 ？
        let need = unsafe { transmute(ret) };
        layer.last_layer = Some(Rc::new(RefCell::new(need)));
        layer
    }

    /// 释放该层变量
    pub(crate) fn drop(self) {
        allow_nothing!();
    }

    /// 添加任意量 , 因为 symbol_ref 已经存，所以字符池是就绪的，所以这里不检查上下级冲突，但是任然检查同级冲突
    pub(crate) fn add_any_by_sym_with_ref(
        &mut self,
        symbol: Symbol,
        name_ref: AnySymbolRef,
        declare_type: IRBasicType,
        available: Option<IRNumber>,
        is_const: bool,
    ) -> Result<IRTypedVar, ()> {
        if self.var_pool.contains_key(&symbol) {
            return Err(());
        }
        match declare_type {
            IRBasicType::INT => {
                let var = IRTypedVar::new(IRName::from_ref_zero_sub(name_ref), IRBasicType::INT);
                self.var_pool.insert(symbol.clone(), Var(is_const, var.clone()));
                if available.is_some() {
                    let ir = available.unwrap();
                    self.available_pool.insert(symbol, ir);
                }
                return Ok(var);
            }
            IRBasicType::FLOAT => {
                let var = IRTypedVar::new(IRName::from_ref_zero_sub(name_ref), IRBasicType::FLOAT);
                self.var_pool.insert(symbol.clone(), Var(is_const, var.clone()));
                if available.is_some() {
                    let ir = available.unwrap();
                    self.available_pool.insert(symbol, ir);
                }
                return Ok(var);
            }
            IRBasicType::VOID => {
                panic!("VOID NOT SUPPORT")
            }
        }
    }

    /// 添加任意量，支持变量和常量 , 如果遇到上级命名冲突，会mangle ， 本级命名冲突会报错
    pub(crate) fn add_any(
        &mut self,
        symbol: Symbol,
        declare_type: IRBasicType,
        name_table: &mut SymbolTable,
        available: Option<IRNumber>,
        is_const: bool,
    ) -> Result<IRTypedVar, ()> {
        if self.var_pool.contains_key(&symbol) {
            return Err(());
        }
        let need_mangle_ref = if self.last_layer.is_none() {
            false
        } else {
            let ret = self.last_layer.as_ref().unwrap().borrow().query_any_var(&symbol);
            match ret {
                Ok(_) => true,
                Err(_) => false,
            }
        };

        let name = if !need_mangle_ref {
            name_table.add_symbol(AnySymbol::from_symbol(symbol.clone()))
        } else {
            let new_any_symbol = AnySymbol::new_rand_underscore(&symbol.string);
            let new_symbol = Symbol::new(&new_any_symbol.name);
            name_table.add_symbol(AnySymbol::from_symbol(new_symbol))
        };

        match declare_type {
            IRBasicType::INT => {
                let var = IRTypedVar::new(IRName::from_ref_zero_sub(name), IRBasicType::INT);
                self.var_pool.insert(symbol.clone(), Var(is_const, var.clone()));
                if available.is_some() {
                    let ir = available.unwrap();
                    self.available_pool.insert(symbol, ir);
                }
                return Ok(var);
            }
            IRBasicType::FLOAT => {
                let var = IRTypedVar::new(IRName::from_ref_zero_sub(name), IRBasicType::FLOAT);
                self.var_pool.insert(symbol.clone(), Var(is_const, var.clone()));
                if available.is_some() {
                    let ir = available.unwrap();
                    self.available_pool.insert(symbol, ir);
                }
                return Ok(var);
            }
            IRBasicType::VOID => {
                panic!("VOID NOT SUPPORT")
            }
        }
    }

    /// 返回的结果指示这个变量是否存在 , 注意没支持Bool变量
    pub(crate) fn update_available_var(&mut self, var_sym: &Symbol, available: Option<IRNumber>) -> Result<(),()> {
        if self.var_pool.contains_key(var_sym) {
            if available.is_some() {
                self.available_pool.insert(var_sym.clone(), available.unwrap());
            } else {
                self.available_pool.remove(var_sym);
            };
            return Ok(());
        } else if self.last_layer.is_some() {
            let last_layer = self.last_layer.as_mut().unwrap();
            let mut layer = RefCell::borrow_mut(last_layer);
            return (*layer).update_available_var(var_sym, available)
        } else {
            return Err(());
        }
    }

    pub(crate) fn add_temp_bool_var(&mut self, symbol_table: &mut SymbolTable) -> IRBoolVar {
        let symbol = AnySymbol::new_rand_underscore("tb_");
        let sym = Symbol::new(&symbol.name);
        let name = symbol_table.add_symbol(symbol);
        let ir_bool = IRBoolVar::new(IRName::from_ref_zero_sub(name));
        self.bool_pool.insert(sym, ir_bool.clone());
        ir_bool
    }

    /// 添加数组 ， 支持不同级冲突mangle处理
    pub(crate) fn add_array(
        &mut self,
        symbol: Symbol,
        declare_type: IRBasicType,
        dims: Vec<usize>,
        is_const: bool,
        name_table: &mut SymbolTable,
    ) -> Result<IRTypedArray, ()> {
        if self.var_pool.contains_key(&symbol) {
            return Err(());
        }
        let need_mangle_ref = if self.last_layer.is_none() {
            false
        } else {
            let ret = self.last_layer.as_ref().unwrap().borrow().query_array(&symbol);
            match ret {
                Ok(_) => true,
                Err(_) => false,
            }
        };

        let name = if !need_mangle_ref {
            name_table.add_symbol(AnySymbol::from_symbol(symbol.clone()))
        } else {
            let new_any_symbol = AnySymbol::new_rand_underscore(&symbol.string);
            let new_symbol = Symbol::new(&new_any_symbol.name);
            name_table.add_symbol(AnySymbol::from_symbol(new_symbol))
        };

        let typed_array = IRTypedArray::new(IRName::from_ref_zero_sub(name), dims, declare_type);
        match declare_type {
            IRBasicType::VOID => {
                panic!("VOID NOT SUPPORT")
            }
            _ => {
                self.var_pool.insert(symbol, Array(is_const, typed_array.clone()));
                return Ok(typed_array);
            }
        }
    }

    /// 添加数组 , 因为 symbol_ref 已经存在，所以字符是就绪的，所以这里不检查上下级冲突，但是任然检查同级冲突
    pub(crate) fn add_array_by_sym_with_ref(
        &mut self,
        symbol: Symbol,
        symbol_ref: AnySymbolRef,
        declare_type: IRBasicType,
        dims: Vec<usize>,
        is_const: bool,
    ) -> Result<IRTypedArray, ()> {
        if self.var_pool.contains_key(&symbol) {
            return Err(());
        }
        let typed_array = IRTypedArray::new(IRName::from_ref_zero_sub(symbol_ref), dims, declare_type);
        match declare_type {
            IRBasicType::VOID => {
                panic!("VOID NOT SUPPORT")
            }
            _ => {
                self.var_pool.insert(symbol, Array(is_const, typed_array.clone()));
                return Ok(typed_array);
            }
        }
    }

    /// 查询具有Init变量和常量，常量的值在这一阶段一定经过了传播
    /// 查询结果可优化，未查到，非const
    pub(crate) fn query_available(&self, symbol: &Symbol, need_const: bool) -> AvailableQueryResult {
        if self.var_pool.contains_key(symbol) {
            let var = &self.var_pool[symbol];
            match var {
                Var(is_const, _) => {
                    let is_available = self.available_pool.contains_key(symbol);
                    if is_available {
                        let ir = self.available_pool[symbol].clone();
                        if *is_const {
                            return AvailableQueryResult::Available(ir);
                        }
                        if need_const {
                            return AvailableQueryResult::NotConst;
                        }
                        return AvailableQueryResult::Available(ir);
                    } else {
                        // 常量一定是可用的，所以这只能是非常量
                        return AvailableQueryResult::NotConstUnAvailable;
                    }
                }
                Array(_, _) => {
                    return AvailableQueryResult::Array;
                }
            }
        } else {
            if self.last_layer.is_none() {
                return AvailableQueryResult::NotFound;
            }
            return self.last_layer.as_ref().unwrap().borrow().query_available(symbol, need_const);
        }
    }
    /// 查询变量和常量，但是这里不查询Available
    pub(crate) fn query_any_var(&self, symbol: &Symbol) -> Result<IRNamedVar, ()> {
        if self.var_pool.contains_key(symbol) {
            let var = self.var_pool[symbol].clone();
            return match var {
                Var(_, var) => Ok(var),
                Array(_, _) => Err(()),
            };
        } else {
            if self.last_layer.is_none() {
                return Err(());
            }
            return self.last_layer.as_ref().unwrap().borrow().query_any_var(symbol);
        }
    }

    pub(crate) fn query_type_any(&self, symbol: &Symbol) -> AnyQueryResult {
        if self.var_pool.contains_key(symbol) {
            let var = &self.var_pool[symbol];
            match var {
                Var(is_const, _) => {
                    if *is_const {
                        return AnyQueryResult::ConstVar;
                    } else {
                        return AnyQueryResult::Var;
                    }
                }
                Array(is_const, _) => {
                    if *is_const {
                        return AnyQueryResult::ConstArray;
                    } else {
                        return AnyQueryResult::Array;
                    }
                }
            }
        } else {
            if self.last_layer.is_none() {
                return AnyQueryResult::NotFound;
            }
            return self.last_layer.as_ref().unwrap().borrow().query_type_any(symbol);
        }
    }

    /// # 查询且提升SSA变量的下标
    /// 查询变量和常量，用在左值查询中
    pub(crate) fn query_var_incremented(&mut self, symbol: &Symbol) -> VarQueryResult {
        if self.var_pool.contains_key(symbol) {
            let var = self.var_pool[symbol].clone();
            return match var {
                Var(is_const, mut var) => {
                    if is_const {
                        return VarQueryResult::Const;
                    }
                    var.add_sub();
                    var.new_this_ref();
                    self.var_pool.insert(symbol.clone(), WarpedType::Var(false, var.clone()));
                    VarQueryResult::Ok(var)
                }
                Array(_, _) => VarQueryResult::Array,
            };
        } else {
            if self.last_layer.is_none() {
                return VarQueryResult::NotFound;
            }
            let last_layer = self.last_layer.as_mut().unwrap();
            let mut layer = RefCell::borrow_mut(last_layer);

            return (*layer).query_var_incremented(&symbol);
        }
    }

    pub(crate) fn query_var(&self, symbol: &Symbol) -> VarQueryResult {
        if self.var_pool.contains_key(symbol) {
            let var = self.var_pool[symbol].clone();
            return match var {
                Var(is_const, var) => {
                    if is_const {
                        VarQueryResult::Const
                    } else {
                        VarQueryResult::Ok(var)
                    }
                }
                Array(_, _) => VarQueryResult::Array,
            };
        } else {
            if self.last_layer.is_none() {
                return VarQueryResult::NotFound;
            }
            return self.last_layer.as_ref().unwrap().borrow().query_var(symbol);
        }
    }

    /// 查询数组
    pub(crate) fn query_array(&self, symbol: &Symbol) -> Result<IRTypedArray, ()> {
        let var = self.var_pool.get(symbol);
        if var.is_none() {
            if self.last_layer.is_none() {
                return Err(());
            }
            return self.last_layer.as_ref().unwrap().borrow().query_array(symbol);
        }
        let var = var.clone().unwrap().clone();
        return match var {
            Var(_, _) => Err(()),
            Array(_, typed_array) => Ok(typed_array),
        };
    }
    // 这个函数的设计简直狗屎，
    pub(crate) fn add_fn(&mut self, symbol: Symbol, params: Vec<IRTypedParam>, ret_type: BasicType, name_table: &mut SymbolTable) -> Result<(), ()> {
        if self.fn_pool.contains_key(&symbol) {
            return Err(());
        }
        let name_ref = name_table.add_symbol(AnySymbol::from_symbol(symbol.clone()));
        let ir_name = IRName::from_ref_zero_sub(name_ref);
        self.fn_pool.insert(
            symbol,
            FnDefQuery {
                ret_type,
                fn_name: ir_name,
                params,
            },
        );
        return Ok(());
    }

    pub(crate) fn query_fn(&self, symbol: &Symbol) -> Result<FnDefQuery, ()> {
        let ret = self.fn_pool.get(symbol);
        if ret.is_none() {
            if self.last_layer.is_none() {
                return Err(());
            }
            return self.last_layer.as_ref().unwrap().borrow().query_fn(symbol);
        }
        return Ok(ret.unwrap().clone());
    }
}

#[test]
fn test_incremented_query() {
    let mut layer = VariableLayer::new();

    let mut table = SymbolTable::new();
    let ret = layer.add_any(Symbol::new("i"), IRBasicType::INT, &mut table, None, false);
    if ret.is_err() {
        panic!("ERROR")
    }

    let symbol = &Symbol::new("i");
    let ret = layer.query_var_incremented(&symbol);

    match ret {
        VarQueryResult::Ok(mut ret) => {
            println!("QUERY RESULT {:?}", ret);
            ret.ref_plus_1();
            ret.ref_plus_1();
            ret.ref_plus_1();
            println!("REF PLUS {:?}", ret);
        }
        _ => {}
    }

    let mut layer = layer.new_nested_layer();

    let symbol = &Symbol::new("i");
    let ret = layer.query_var_incremented(&symbol);

    match ret {
        VarQueryResult::Ok(mut ret) => {
            println!("QUERY RESULT {:?}", ret);
            ret.ref_plus_1();
            ret.ref_plus_1();
            ret.ref_plus_1();
            println!("REF PLUS {:?}", ret);
        }
        _ => {}
    }

    let mut layer = layer.new_nested_layer();

    let symbol = &Symbol::new("i");
    let ret = layer.query_var_incremented(&symbol);

    match ret {
        VarQueryResult::Ok(mut ret) => {
            println!("QUERY RESULT {:?}", ret);
            ret.ref_plus_1();
            ret.ref_plus_1();
            ret.ref_plus_1();
            println!("REF PLUS {:?}", ret);
        }
        _ => {}
    }

    let mut layer = layer.new_nested_layer();

    let symbol = &Symbol::new("i");

    let ret = layer.query_any_var(&symbol);
    if ret.is_err() {
        panic!("ERROR")
    }
    println!("END RESULT {:?}", ret);
    match ret {
        Ok(mut ret) => {
            ret.ref_plus_1();
            ret.ref_plus_1();
            println!("QUERY ADDED RESULT {:?}", ret);
        }
        Err(_) => {}
    }
}
