use crate::{
    allow_nothing,
    ast::{
        table::{AnyQueryResult, AnySymbol, AnySymbolRef, AvailableQueryResult, SymbolTable, VarQueryResult, VariableLayer},
        AddExpr, ArrayDefine, ArrayInit, ArrayRef, Assign, AstUnit, BasicType, BoolCondition, Branches, ExternalFun, FnDefine, IdentWithInit, Invoke,
        LVarRef, Literal, MathOpe, RelExpr, RelOpe, RowDefines, Stmt, Stmts, Type, UnaryExp, UnaryOp, ValueAble, VarDefine, While,
    },
    echo::{make_advise, Error, Warning},
    hir::hir::{
        HirUnit, IRArrayDefine, IRArrayInit, IRAssign, IRBasicExpr, IRBasicType, IRBool, IRBoolExpr, IRBoolOpe, IRBoolOrVarDefine, IRBoolVarDefine,
        IRBranches, IRExprPrefix, IRExternalFun, IRFirstSuffix, IRFunDefine, IRIf, IRInvoke, IRLValue, IRMathOpe, IRName, IRNamedArray,
        IRNamedArrayEle, IRNamedParam, IRNamedVar, IRNumber, IRRValue, IRRelSource, IRSecondSuffix, IRStatement, IRTypedArray, IRTypedParam,
        IRTypedVar, IRVarDefine, IRVarOrArrayDefine, IRWhile,
    },
    lexer::{symbol::Symbol, Base, LiteralKind},
    optimizer::OptLevel,
    session::CompileSession,
};
use std::{cmp::Ordering, collections::VecDeque};

// 变量传递排序规则 核心处理参数 辅助处理参数 外加特殊参数

// 注意目前有个问题，HIR想报错有时候无法定位，导致报错的时候不能出现位置信息
enum WaitAddedTypeParam {
    Array(Symbol, AnySymbolRef, IRBasicType, Vec<usize>),
    Var(Symbol, AnySymbolRef, IRBasicType),
}

impl AstUnit {
    pub(crate) fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
    ) -> Result<HirUnit, ()> {
        return match self {
            AstUnit::FnDefine(fn_def) => {
                let fun = fn_def.to_hir(variable_layer, name_table, session);
                if fun.is_err() {
                    return Err(());
                }
                Ok(HirUnit::FunDefine(fun.unwrap()))
            }
            AstUnit::VarDefine(var) => {
                let ret = var.to_gl_hir(variable_layer, name_table, session);
                if ret.is_err() {
                    return Err(());
                }
                Ok(HirUnit::GLVarDefine(ret.unwrap()))
            }
            AstUnit::ArrayDefine(array) => {
                let hir_array = array.to_hir(variable_layer, name_table, session, true);
                let ret = match hir_array {
                    Ok(array) => {
                        if !array.0.is_empty() {
                            panic!("NEVER HERE")
                        }
                        HirUnit::GLArrayDefine(array.1)
                    }
                    Err(_) => {
                        return Err(());
                    }
                };
                Ok(ret)
            }
            AstUnit::RowDefines(defs) => {
                let basic_type = defs.define_type;
                let is_const = defs.is_const;
                let mut gl_defs = vec![];
                for single in defs.defs {
                    match single {
                        IdentWithInit::Var { ident, init } => {
                            let var_def = VarDefine {
                                is_const,
                                define_type: basic_type.clone(),
                                ident,
                                with_value: init,
                            };
                            let def = var_def.to_gl_hir(variable_layer, name_table, session);
                            if def.is_err() {
                                return Err(());
                            }
                            let def = def.unwrap();
                            gl_defs.push(IRVarOrArrayDefine::Var(def));
                        }
                        IdentWithInit::Array { ident, dimensions, init } => {
                            let array_def = ArrayDefine {
                                is_const,
                                define_type: basic_type.clone(),
                                ident,
                                dimensions,
                                init,
                            };
                            let def = array_def.to_hir(variable_layer, name_table, session, true);
                            if def.is_err() {
                                return Err(());
                            }
                            let def = def.unwrap();
                            if !def.0.is_empty() {
                                panic!("NEVER HERE")
                            }
                            gl_defs.push(IRVarOrArrayDefine::Array(def.1));
                        }
                    }
                }
                Ok(HirUnit::GlDefines(gl_defs))
            }
            AstUnit::ExternalFn(fun) => {
                let fun = fun.to_hir(variable_layer, name_table, session);
                if fun.is_err() {
                    return Err(());
                }
                Ok(HirUnit::ExternalFun(fun.unwrap()))
            }
        };
    }
}

impl Branches {
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        in_while: bool,
        expected_ret_type: Option<IRBasicType>,
    ) -> Result<IRBranches, ()> {
        let enters = self.ifs;
        let mut ir_ifs = VecDeque::new();
        let mut ir_else = None;
        for single in enters {
            let cond = single.condition;
            let bool_expr_ret = match cond {
                None => {
                    // 进入了else分支
                    None
                }
                Some(cond) => {
                    let cond = cond.to_hir(variable_layer, name_table, session);
                    if cond.is_err() {
                        return Err(());
                    }
                    Some(cond.unwrap())
                }
            };
            let stats = single.stats;
            let mut new_variable_layer = variable_layer.new_nested_layer();
            let ret = stmts_to_hir(stats, &mut new_variable_layer, name_table, session, in_while, expected_ret_type);
            if ret.is_err() {
                return Err(());
            }
            let stmt_ret = ret.unwrap();
            if bool_expr_ret.is_some() {
                let ir_if = IRIf {
                    condition: bool_expr_ret.unwrap(),
                    statements: stmt_ret,
                };
                ir_ifs.push_back(ir_if);
            } else {
                // 具有else分支，且到达了else分支
                ir_else = Some(stmt_ret);
            }
            new_variable_layer.drop()
        }
        let branches = IRBranches {
            first_if: ir_ifs.pop_front().unwrap(),
            elif: ir_ifs,
            last_else: ir_else,
        };
        Ok(branches)
    }
}

impl While {
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        expected_ret_type: Option<IRBasicType>,
    ) -> Result<IRWhile, ()> {
        let bool_expr = self.condition;
        let expr = bool_expr.to_hir(variable_layer, name_table, session);
        if expr.is_err() {
            return Err(());
        }
        let stmt = self.stats;
        let mut variable_layer = variable_layer.new_nested_layer();
        let ret = stmts_to_hir(stmt, &mut variable_layer, name_table, session, true, expected_ret_type);
        if ret.is_err() {
            return Err(());
        }
        variable_layer.drop();
        let ir_while = IRWhile {
            condition: expr.unwrap(),
            statements: ret.unwrap(),
        };
        Ok(ir_while)
    }
}

impl FnDefine {
    fn to_hir(self, variable_layer: &mut VariableLayer, name_table: &mut SymbolTable, session: &mut CompileSession) -> Result<IRFunDefine, ()> {
        let fun_name_ident = self.name.clone();
        let fun_name_ref = name_table.add_symbol(AnySymbol::from_symbol(self.name.sym));

        let param = self.params;
        // 结构设计不合理导致的问题
        let mut wait_add_fun_params = vec![];
        let mut params = vec![];
        if param.is_some() {
            let fn_params = param.unwrap();
            for (param_type, param_ident) in fn_params.type_idents {
                match param_type {
                    Type::Basic(basic) => {
                        match basic {
                            BasicType::Void => {
                                session.report_error(param_ident.span.make_error("void type variable not supported", None));
                                return Err(());
                            }
                            BasicType::Float => {
                                let is_exist = variable_layer.query_type_any(&param_ident.sym);
                                let name_ref = if is_exist == AnyQueryResult::NotFound {
                                    name_table.add_symbol(AnySymbol::from_symbol(param_ident.sym.clone()))
                                } else {
                                    name_table.add_symbol_with_mangle(AnySymbol::from_symbol(param_ident.sym.clone()))
                                };
                                wait_add_fun_params.push(WaitAddedTypeParam::Var(param_ident.sym, name_ref, basic.to_hir_type()));
                                let var = IRTypedVar::new(IRName::from_ref_zero_sub(name_ref), IRBasicType::FLOAT);
                                params.push(IRTypedParam::Var(var))
                            }
                            BasicType::Int => {
                                let is_exist = variable_layer.query_type_any(&param_ident.sym);
                                let symbol_ref = if is_exist == AnyQueryResult::NotFound {
                                    name_table.add_symbol(AnySymbol::from_symbol(param_ident.sym.clone()))
                                } else {
                                    name_table.add_symbol_with_mangle(AnySymbol::from_symbol(param_ident.sym.clone()))
                                };
                                wait_add_fun_params.push(WaitAddedTypeParam::Var(param_ident.sym, symbol_ref, basic.to_hir_type()));
                                let var = IRTypedVar::new(IRName::from_ref_zero_sub(symbol_ref), IRBasicType::INT); // 暂时不加入变量池,目前的变量池还是全局池，等到函数定义添加完之后，再加入参数
                                params.push(IRTypedParam::Var(var))
                            }
                        }
                    }
                    Type::Array(array) => {
                        match array.basic_type {
                            // fixme 形式参数的数组的维度信息没有检查,但是这个参数暂时真的没用
                            BasicType::Void => {
                                session.report_error(param_ident.span.make_error("void type variable not supported", None));
                                return Err(());
                            }
                            // 大量冗余代码可消除
                            BasicType::Int => {
                                let is_exist = variable_layer.query_type_any(&param_ident.sym);
                                let array_name_ref = if is_exist == AnyQueryResult::NotFound {
                                    name_table.add_symbol(AnySymbol::from_symbol(param_ident.sym.clone()))
                                } else {
                                    name_table.add_symbol_with_mangle(AnySymbol::from_symbol(param_ident.sym.clone()))
                                };
                                let array_name = IRName::from_ref_zero_sub(array_name_ref);
                                let mut usize_dims = vec![];
                                for (index, dim) in array.dimensions.iter().enumerate() {
                                    if index == 0 {
                                        if !dim.is_unary_none() {
                                            let error = param_ident.span.make_error("array first dimensions error", None);
                                            session.report_error(error);
                                            return Err(());
                                        } else {
                                            usize_dims.push(0);
                                            continue;
                                        }
                                    }

                                    let ret_dims = dim.to_ir_number(variable_layer, session);
                                    match ret_dims {
                                        Ok(dim) => {
                                            let dim_usize = dim.to_const_usize();
                                            usize_dims.push(dim_usize);
                                        }
                                        Err(_) => {
                                            let error = param_ident.span.make_error("array dimensions can't be known at compile time error", None);
                                            session.report_error(error);
                                            return Err(());
                                        }
                                    }
                                }
                                wait_add_fun_params.push(WaitAddedTypeParam::Array(
                                    param_ident.sym.clone(),
                                    array_name_ref,
                                    array.basic_type.to_hir_type(),
                                    usize_dims.clone(),
                                ));
                                params.push(IRTypedParam::Array(IRTypedArray::new(array_name, usize_dims, IRBasicType::INT)));
                            }
                            BasicType::Float => {
                                let is_exist = variable_layer.query_type_any(&param_ident.sym);
                                let array_name_ref = if is_exist == AnyQueryResult::NotFound {
                                    name_table.add_symbol(AnySymbol::from_symbol(param_ident.sym.clone()))
                                } else {
                                    name_table.add_symbol_with_mangle(AnySymbol::from_symbol(param_ident.sym.clone()))
                                };
                                let array_name = IRName::from_ref_zero_sub(array_name_ref);

                                let mut usize_dims = vec![];
                                for (index, dim) in array.dimensions.iter().enumerate() {
                                    if index == 0 {
                                        if !dim.is_unary_none() {
                                            let error = param_ident.span.make_error("array first dimensions error", None);
                                            session.report_error(error);
                                            return Err(());
                                        } else {
                                            usize_dims.push(0);
                                            continue;
                                        }
                                    }
                                    let ret_dims = dim.to_ir_number(variable_layer, session);
                                    match ret_dims {
                                        Ok(dim) => {
                                            let dim_usize = dim.to_const_usize();
                                            usize_dims.push(dim_usize);
                                        }
                                        Err(_) => {
                                            let error = param_ident.span.make_error("array dimensions can't be known at compile time error", None);
                                            session.report_error(error);
                                            return Err(());
                                        }
                                    }
                                }
                                wait_add_fun_params.push(WaitAddedTypeParam::Array(
                                    param_ident.sym.clone(),
                                    array_name_ref,
                                    array.basic_type.to_hir_type(),
                                    usize_dims.clone(),
                                ));
                                params.push(IRTypedParam::Array(IRTypedArray::new(array_name, usize_dims, IRBasicType::FLOAT)));
                            }
                        }
                    }
                }
            }
        }

        let ret = variable_layer.add_fn(fun_name_ident.sym.clone(), params.clone(), self.ret_type.clone(), name_table);
        if ret.is_err() {
            session.report_error(fun_name_ident.span.make_error("function redefined", None));
            return Err(());
        }
        let mut variable_layer = variable_layer.new_nested_layer();
        for fun_param in wait_add_fun_params {
            match fun_param {
                WaitAddedTypeParam::Var(var, a, b) => {
                    let ret = variable_layer.add_any_by_sym_with_ref(var.clone(), a, b, None, false);
                    if ret.is_err() {
                        session.report_error(Error::new(&format!("symbol \"{}\" redefine", var.string), None));
                        return Err(());
                    }
                }
                WaitAddedTypeParam::Array(sym, b, c, dims) => {
                    let ret = variable_layer.add_array_by_sym_with_ref(sym.clone(), b, c, dims, false);
                    if ret.is_err() {
                        session.report_error(Error::new(&format!("symbol \"{}\" redefine", sym.string), None));
                        return Err(());
                    }
                }
            }
        }
        let ret = stmts_to_hir(
            self.stmts,
            &mut variable_layer,
            name_table,
            session,
            false,
            Some(self.ret_type.to_hir_type()),
        );
        if ret.is_err() {
            return Err(());
        }
        let fun_block = IRFunDefine::new(
            IRName::from_ref_zero_sub(fun_name_ref),
            params.clone(),
            ret.unwrap(),
            self.ret_type.to_hir_type(),
        );
        variable_layer.drop();
        Ok(fun_block)
    }
}

impl ExternalFun {
    pub(crate) fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
    ) -> Result<IRExternalFun, ()> {
        let fun_name = self.name;
        let name = AnySymbol::new(&fun_name.sym.string);
        let name_ref = name_table.add_symbol(name);
        let mut fun = IRExternalFun {
            name: IRName::from_ref_zero_sub(name_ref),
            params: vec![],
            return_type: self.ret.to_hir_type(),
        };

        if self.params.is_some() {
            let params = self.params.unwrap();
            for (param_type, ident) in params.type_idents {
                match param_type {
                    Type::Basic(basic) => {
                        match basic {
                            BasicType::Void => {
                                panic!("NOT SUPPORT VOID IN PARAM")
                            }
                            BasicType::Float => {
                                let name_ref = name_table.add_symbol(AnySymbol::from_symbol(ident.sym.clone()));
                                let var = IRTypedVar::new(IRName::from_ref_zero_sub(name_ref), IRBasicType::FLOAT);
                                fun.params.push(IRTypedParam::Var(var))
                            }
                            BasicType::Int => {
                                let name_ref = name_table.add_symbol(AnySymbol::from_symbol(ident.sym.clone()));
                                let var = IRTypedVar::new(IRName::from_ref_zero_sub(name_ref), IRBasicType::INT); //  手动添加符号，但是不添加到变量池里面，但是加入符号表
                                fun.params.push(IRTypedParam::Var(var))
                            }
                        }
                    }
                    Type::Array(array) => match array.basic_type {
                        BasicType::Void => {
                            panic!("NOT SUPPORT VOID IN PARAM")
                        }
                        BasicType::Int => {
                            let name_ref = name_table.add_symbol(AnySymbol::from_symbol(ident.sym.clone()));
                            let var = IRName::from_ref_zero_sub(name_ref);
                            fun.params.push(IRTypedParam::Array(IRTypedArray::new(var, vec![], IRBasicType::INT)));
                        }
                        BasicType::Float => {
                            let name_ref = name_table.add_symbol(AnySymbol::from_symbol(ident.sym.clone()));
                            let var = IRName::from_ref_zero_sub(name_ref);
                            fun.params.push(IRTypedParam::Array(IRTypedArray::new(var, vec![], IRBasicType::FLOAT)));
                        }
                    },
                }
            }
        }

        let ret = variable_layer.add_fn(fun_name.sym, fun.params.clone(), self.ret, name_table);
        if ret.is_err() {
            session.report_error(fun_name.span.make_error("function redefined", None));
            return Err(());
        }
        return Ok(fun);
    }
}

impl RelExpr {
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        symbol_table: &mut SymbolTable,
        session: &mut CompileSession,
    ) -> Result<(Vec<IRBoolOrVarDefine>, IRBool), ()> {
        return match self {
            RelExpr::Math(expr) => {
                // 关系表达式中的类型根据根据推断来计算
                let hir = expr.to_hir_guess(variable_layer, symbol_table, session, true);
                if hir.is_err() {
                    return Err(());
                }
                let (mut defs, guess_type, ir_rvalue) = hir.unwrap();
                let mut warped_def = vec![];

                if defs.len() == 0 {
                    match &ir_rvalue {
                        IRRValue::Basic(pre, basic) => {
                            if pre.is_none() {
                                return match basic {
                                    IRBasicExpr::Literal(lit) => Ok((warped_def, IRBool::Basic(IRRelSource::Literal(lit.clone())))),
                                    IRBasicExpr::Var(var) => Ok((warped_def, IRBool::Basic(IRRelSource::Var(var.clone())))),
                                    IRBasicExpr::ArrayEleRef(ele) => Ok((warped_def, IRBool::Basic(IRRelSource::ArrayEleRef(ele.clone())))),
                                };
                            }
                        }
                        _ => {}
                    }
                };
                // 这里加入类型推断
                let (temp_var_define, mut named_temp_var) = new_temp_var(symbol_table, ir_rvalue, guess_type);
                defs.push(temp_var_define);
                for single in defs {
                    warped_def.push(IRBoolOrVarDefine::VarDef(single))
                }
                named_temp_var.ref_plus_1();
                let init = IRBool::Basic(IRRelSource::Var(named_temp_var));
                Ok((warped_def, init))
            }
            RelExpr::Rel(a, b, c) => {
                let mut pre_compute = vec![];
                let a = a.to_hir(variable_layer, symbol_table, session);
                if a.is_err() {
                    return Err(());
                }
                let c = c.to_hir(variable_layer, symbol_table, session);
                if c.is_err() {
                    return Err(());
                }
                let mut a = a.unwrap();
                let mut c = c.unwrap();
                // 加入pre_compute区域
                pre_compute.append(&mut a.0);
                pre_compute.append(&mut c.0);
                // fixme 这里可以优化
                let a = a.1.to_ir_bool_source(variable_layer, symbol_table);
                let c = c.1.to_ir_bool_source(variable_layer, symbol_table);
                if a.is_err() {
                    return Err(());
                }
                let a = a.unwrap();
                if a.0.is_some() {
                    pre_compute.push(IRBoolOrVarDefine::BoolDef(a.0.unwrap()));
                }
                if c.is_err() {
                    return Err(());
                }
                let c = c.unwrap();
                if c.0.is_some() {
                    pre_compute.push(IRBoolOrVarDefine::BoolDef(c.0.unwrap()));
                }
                let ir_rel_left = a.1;
                let ir_rel_right = c.1;
                match b {
                    RelOpe::Less => {
                        let ir_bool = IRBool::Rel(ir_rel_left, IRBoolOpe::Less, ir_rel_right);
                        Ok((pre_compute, ir_bool))
                    }
                    RelOpe::LessOrEqual => {
                        let ir_bool = IRBool::Rel(ir_rel_left, IRBoolOpe::LessOrEqual, ir_rel_right);
                        Ok((pre_compute, ir_bool))
                    }
                    RelOpe::Greater => {
                        let ir_bool = IRBool::Rel(ir_rel_left, IRBoolOpe::Greater, ir_rel_right);
                        Ok((pre_compute, ir_bool))
                    }
                    RelOpe::GreaterOrEqual => {
                        let ir_bool = IRBool::Rel(ir_rel_left, IRBoolOpe::GreaterOrEqual, ir_rel_right);
                        Ok((pre_compute, ir_bool))
                    }
                    RelOpe::IsEqual => {
                        let ir_bool = IRBool::Rel(ir_rel_left, IRBoolOpe::Equal, ir_rel_right);
                        Ok((pre_compute, ir_bool))
                    }
                    RelOpe::NotEqual => {
                        let ir_bool = IRBool::Rel(ir_rel_left, IRBoolOpe::NotEqual, ir_rel_right);
                        Ok((pre_compute, ir_bool))
                    }
                }
            }
        };
    }
}

impl IRBool {
    fn to_ir_bool_source(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
    ) -> Result<(Option<IRBoolVarDefine>, IRRelSource), ()> {
        return match self {
            IRBool::Basic(expr) => Ok((None, expr)),
            IRBool::Rel(a, b, c) => {
                let rvalue = IRBool::Rel(a, b, c);
                let ir_bool_var = variable_layer.add_temp_bool_var(name_table);
                let ir_var_def = IRBoolVarDefine::new(ir_bool_var.get_name(), rvalue, true);
                let ir_rel_source = IRRelSource::BasicBool(ir_bool_var);
                Ok((Some(ir_var_def), ir_rel_source))
            }
        };
    }
}

// BoolExprs
impl BoolCondition {
    // BoolExprs 中的BoolExpr 自带预计算表达式，无需额外提供预计算表达式
    // the BoolExpr contains "pre_compute expression" , so we don't need to provide the extra .
    fn to_hir(self, variable_layer: &mut VariableLayer, name_table: &mut SymbolTable, session: &mut CompileSession) -> Result<IRBoolExpr, ()> {
        return match self {
            BoolCondition::And(a, b) => {
                let a = a.to_hir(variable_layer, name_table, session);
                let b = b.to_hir(variable_layer, name_table, session);
                if a.is_err() {
                    return Err(());
                }
                if b.is_err() {
                    return Err(());
                }
                let a = a.unwrap();
                let mut ands = vec![];
                match a {
                    IRBoolExpr::Bool(_, _) => {
                        ands.push(a);
                    }
                    IRBoolExpr::And(mut a) => ands.append(&mut a),
                    IRBoolExpr::Or(_) => ands.push(a),
                }
                let b = b.unwrap();
                match b {
                    IRBoolExpr::Bool(_, _) => {
                        ands.push(b);
                    }
                    IRBoolExpr::And(mut a) => ands.append(&mut a),
                    IRBoolExpr::Or(_) => ands.push(b),
                }
                Ok(IRBoolExpr::And(ands))
            }
            BoolCondition::Or(a, b) => {
                let a = a.to_hir(variable_layer, name_table, session);
                let b = b.to_hir(variable_layer, name_table, session);
                if a.is_err() {
                    return Err(());
                }
                if b.is_err() {
                    return Err(());
                }
                let a = a.unwrap();
                let mut ors = vec![];
                match a {
                    IRBoolExpr::Bool(_, _) => {
                        ors.push(a);
                    }
                    IRBoolExpr::And(_) => ors.push(a),
                    IRBoolExpr::Or(mut a) => ors.append(&mut a),
                }
                let b = b.unwrap();
                match b {
                    IRBoolExpr::Bool(_, _) => {
                        ors.push(b);
                    }
                    IRBoolExpr::And(_) => ors.push(b),
                    IRBoolExpr::Or(mut a) => ors.append(&mut a),
                }
                Ok(IRBoolExpr::Or(ors))
            }
            BoolCondition::RelExpr(a) => {
                let result = a.to_hir(variable_layer, name_table, session);
                if result.is_err() {
                    return Err(());
                }
                let res = result.unwrap();
                let ir_bool_expr = IRBoolExpr::Bool(res.0, res.1);
                Ok(ir_bool_expr)
            }
        };
    }
}

impl ArrayRef {
    fn is_need_pre_compute(&self) -> bool {
        for single_pos in &self.pos {
            if !single_pos.is_unary_lit_or_var_ref() {
                return false;
            }
        }
        return true;
    }
}

impl AddExpr {
    fn is_unary_none(&self) -> bool {
        return match self {
            AddExpr::Unary(unary) => match unary {
                UnaryExp::None => true,
                _ => false,
            },
            AddExpr::Expr(_, _, _) => false,
        };
    }

    /// 由于 ArrayRef是可能产生预计算表达式的，所以
    fn is_unary_lit_or_var_ref(&self) -> bool {
        match self {
            AddExpr::Unary(unary) => {
                match unary {
                    UnaryExp::Lit(_, _) => true,
                    UnaryExp::VarRef(_, _) => true,
                    UnaryExp::ArrayRef(_, array) => {
                        // 这里可以用一个递归去检查这个Array是不是需要预计算的
                        array.is_need_pre_compute()
                    }
                    UnaryExp::Invoke(_, _) => false,
                    UnaryExp::Paren(_, _) => false,
                    UnaryExp::None => false,
                }
            }
            AddExpr::Expr(_, _, _) => false,
        }
    }
    // 注意这里是 &self 但是 to_hir是 self 这里的设计是刻意的
    /// 该函数负责基本的常量传播，而且表达式中一旦出现！符号，则证明表达式有问题存在
    /// # 提供报错信息
    /// 进行常量传播，但是目前只能进行const传播
    /// 因为只有const所以一定不是产生pre_compute表达式
    fn to_ir_number(&self, variables_layer: &mut VariableLayer, session: &mut CompileSession) -> Result<IRNumber, ()> {
        // 这里到底能出现什么样的表达式 ？ ？
        /*
        const int def[10] = {10, 10, 10};
        const int x = def[0]; // 不允许
        int y = def[0];// 不允许
        const int z = 100;// 允许
        int zz = z;// 允许
        const int zzz = z; // 允许
                 */
        // 综合上面来说，这里只允许出现Literal或者Const Var
        return match self {
            ValueAble::Unary(unary) => unary.to_ir_number(variables_layer, session, true),
            ValueAble::Expr(expr1, op, expr2) => {
                let left = expr1.to_ir_number(variables_layer, session);
                let a = match left {
                    Ok(number) => number,
                    Err(_) => {
                        return Err(());
                    }
                };
                let right = expr2.to_ir_number(variables_layer, session);
                let b = match right {
                    Ok(number) => number,
                    Err(_) => {
                        return Err(());
                    }
                };
                let ret = match op {
                    MathOpe::Add => a + b,
                    MathOpe::Sub => a - b,
                    MathOpe::Mul => a * b,
                    MathOpe::Div => a / b,
                    MathOpe::Ram => a % b,
                };
                Ok(ret)
            }
        };
    }

    /// 将表达式翻译成HIR右值，
    /// 如果遇到数组维度丢失，则报错
    /// 如果遇到找不到变量则报错
    /// required type 主要作用是生成中间变量的时候需要新的类型，这个类型不能是未知的。
    fn to_hir(
        self,
        variables_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        is_in_bool: bool,
        expected_type: IRBasicType,
    ) -> Result<(Vec<IRVarDefine>, IRRValue), ()> {
        return match self {
            AddExpr::Unary(unary) => unary.to_hir(variables_layer, name_table, session, is_in_bool, expected_type),
            AddExpr::Expr(exp, ope, exp2) => {
                let mut compute_before = vec![];
                let left = if !exp.is_unary_lit_or_var_ref() {
                    let basic_left = exp.to_hir(variables_layer, name_table, session, is_in_bool, expected_type);
                    IRBasicExpr::Var(match basic_left {
                        Ok((mut defs, ir_right)) => {
                            compute_before.append(&mut defs);
                            let (def, mut var) = new_temp_var(name_table, ir_right, expected_type);
                            compute_before.push(def);
                            var.ref_plus_1();
                            var
                        }
                        Err(_) => {
                            return Err(());
                        }
                    })
                } else {
                    let basic_left = exp.to_hir(variables_layer, name_table, session, is_in_bool, expected_type);
                    match basic_left {
                        Ok((defs, irrvalue)) => {
                            if defs.len() > 0 {
                                panic!("NEVER HERE")
                            }
                            match irrvalue {
                                IRRValue::Basic(suffix, basic) => {
                                    if suffix.is_none() {
                                        basic
                                    } else {
                                        let (def, mut var) = new_temp_var(name_table, IRRValue::Basic(suffix, basic), expected_type);
                                        compute_before.push(def);
                                        var.ref_plus_1();
                                        IRBasicExpr::Var(var)
                                    }
                                }
                                _ => {
                                    panic!("NEVER HERE")
                                }
                            }
                        }
                        Err(_) => {
                            return Err(());
                        }
                    }
                };

                let right = if !exp2.is_unary_lit_or_var_ref() {
                    let basic_right = exp2.to_hir(variables_layer, name_table, session, is_in_bool, expected_type);
                    let right = match basic_right {
                        Ok(mut expr) => {
                            compute_before.append(&mut expr.0);
                            let (def, mut var) = new_temp_var(name_table, expr.1, expected_type);
                            compute_before.push(def);
                            var.ref_plus_1();
                            var
                        }
                        Err(_) => {
                            return Err(());
                        }
                    };
                    IRBasicExpr::Var(right)
                } else {
                    let basic_right = exp2.to_hir(variables_layer, name_table, session, is_in_bool, expected_type);
                    match basic_right {
                        Ok((defs, irrvalue)) => {
                            if defs.len() > 0 {
                                panic!("NEVER HERE")
                            }
                            match irrvalue {
                                IRRValue::Basic(suffix, basic) => {
                                    if suffix.is_none() {
                                        basic
                                    } else {
                                        let (def, mut var) = new_temp_var(name_table, IRRValue::Basic(suffix, basic), expected_type);
                                        compute_before.push(def);
                                        var.ref_plus_1();
                                        IRBasicExpr::Var(var)
                                    }
                                }
                                _ => {
                                    panic!("NEVER HERE")
                                }
                            }
                        }
                        Err(_) => {
                            return Err(());
                        }
                    }
                };

                let ret = (compute_before, IRRValue::Math(IRExprPrefix::none(), left, ope.to_hir_ope(), right));
                Ok(ret)
            }
        };
    }

    /// 将表达式翻译成HIR右值，
    /// 如果遇到数组维度丢失，则报错
    /// 如果遇到找不到变量则报错
    /// 与to_hir不同,to_hir_guess 会进行类型推断,一般出现float类型则推断为float类型
    fn to_hir_guess(
        self,
        variables_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        is_in_bool: bool,
    ) -> Result<(Vec<IRVarDefine>, IRBasicType, IRRValue), ()> {
        return match self {
            AddExpr::Unary(unary) => unary.to_hir_guess(variables_layer, name_table, session, is_in_bool),
            AddExpr::Expr(exp, ope, exp2) => {
                let mut compute_before = vec![];
                let (left, left_type) = if !exp.is_unary_lit_or_var_ref() {
                    let basic_left = exp.to_hir_guess(variables_layer, name_table, session, is_in_bool);
                    match basic_left {
                        Ok((mut defs, guess_type, ir_right)) => {
                            compute_before.append(&mut defs);
                            let (def, mut var) = new_temp_var(name_table, ir_right, guess_type);
                            compute_before.push(def);
                            var.ref_plus_1();
                            (IRBasicExpr::Var(var), guess_type)
                        }
                        Err(_) => {
                            return Err(());
                        }
                    }
                } else {
                    let basic_left = exp.to_hir_guess(variables_layer, name_table, session, is_in_bool);
                    match basic_left {
                        Ok((defs, guess_type, irrvalue)) => {
                            if defs.len() > 0 {
                                panic!("NEVER HERE")
                            }
                            match irrvalue {
                                IRRValue::Basic(suffix, basic) => {
                                    if suffix.is_none() {
                                        (basic, guess_type)
                                    } else {
                                        let (def, mut var) = new_temp_var(name_table, IRRValue::Basic(suffix, basic), guess_type);
                                        compute_before.push(def);
                                        var.ref_plus_1();
                                        (IRBasicExpr::Var(var), guess_type)
                                    }
                                }
                                _ => {
                                    panic!("NEVER HERE")
                                }
                            }
                        }
                        Err(_) => {
                            return Err(());
                        }
                    }
                };

                let (right, right_type) = if !exp2.is_unary_lit_or_var_ref() {
                    let basic_right = exp2.to_hir_guess(variables_layer, name_table, session, is_in_bool);
                    match basic_right {
                        Ok((mut defs, guess_type, right)) => {
                            compute_before.append(&mut defs);
                            let (def, mut var) = new_temp_var(name_table, right, guess_type);
                            compute_before.push(def);
                            var.ref_plus_1();
                            (IRBasicExpr::Var(var), guess_type)
                        }
                        Err(_) => {
                            return Err(());
                        }
                    }
                } else {
                    let basic_right = exp2.to_hir_guess(variables_layer, name_table, session, is_in_bool);
                    match basic_right {
                        Ok((defs, guess_type, irrvalue)) => {
                            if defs.len() > 0 {
                                panic!("NEVER HERE")
                            }
                            match irrvalue {
                                IRRValue::Basic(suffix, basic) => {
                                    if suffix.is_none() {
                                        (basic, guess_type)
                                    } else {
                                        let (def, mut var) = new_temp_var(name_table, IRRValue::Basic(suffix, basic), guess_type);
                                        compute_before.push(def);
                                        var.ref_plus_1();
                                        (IRBasicExpr::Var(var), guess_type)
                                    }
                                }
                                _ => {
                                    panic!("NEVER HERE");
                                }
                            }
                        }
                        Err(_) => {
                            return Err(());
                        }
                    }
                };
                let result_type = if left_type == IRBasicType::FLOAT || right_type == IRBasicType::FLOAT {
                    IRBasicType::FLOAT
                } else {
                    IRBasicType::INT
                };

                let (defs, right) = (compute_before, IRRValue::Math(IRExprPrefix::none(), left, ope.to_hir_ope(), right));
                Ok((defs, result_type, right))
            }
        };
    }

    // 注意这个函数专用于将参数转化为数组传递,数组元素的识别是单独的
    fn to_named_array(
        self,
        variables_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
    ) -> Result<(Vec<IRVarDefine>, IRNamedArray), ()> {
        return match self {
            AddExpr::Unary(unary) => {
                match unary {
                    UnaryExp::ArrayRef(_, array) => {
                        let array_query_result = variables_layer.query_array(&array.array_name.sym);
                        if array_query_result.is_err() {
                            session.report_error(Error::new("array not found", None));
                            return Err(());
                        }
                        let typed_array = array_query_result.unwrap();
                        let mut all_pos = array.pos;
                        let mut used_dim = 0;

                        let mut ir_pos = vec![];
                        let mut pre_compute = vec![];
                        loop {
                            let now_pos = all_pos.pop();
                            if now_pos.is_none() {
                                break;
                            }
                            let now_pos = now_pos.unwrap();
                            if now_pos.is_none() {
                                session.report_error(Error::new("array dimension missing", None));
                                return Err(());
                            }
                            let ret = now_pos.to_hir_guess(variables_layer, name_table, session, false);
                            if ret.is_err() {
                                return Err(());
                            }
                            let mut ir_now_pos = ret.unwrap();
                            pre_compute.append(&mut ir_now_pos.0);
                            ir_pos.push(ir_now_pos.2);
                            used_dim += 1;
                        }
                        if used_dim == typed_array.get_dims_len() {
                            session.report_error(Error::new("array element can not cast to an array", None));
                            return Err(());
                        }
                        if used_dim > typed_array.get_dims_len() {
                            session.report_error(Error::new("array reference exceed the length of array", None));
                            return Err(());
                        }
                        let name = typed_array.get_irname();
                        let array_ref = match typed_array.get_type() {
                            IRBasicType::INT => IRNamedArray::new(name, IRBasicType::INT, ir_pos),
                            IRBasicType::FLOAT => IRNamedArray::new(name, IRBasicType::FLOAT, ir_pos),
                            IRBasicType::VOID => {
                                panic!("NEVER HERE")
                            }
                        };
                        Ok((pre_compute, array_ref)) // 这里没有处理pre_compute表达式
                    }
                    UnaryExp::VarRef(prefix, var) => {
                        let array_query_result = variables_layer.query_array(&var.sym);
                        if array_query_result.is_err() {
                            session.report_error(Error::new("symbol not found", None));
                            return Err(());
                        }
                        if prefix.len() > 0 {
                            session.report_error(Error::new("error prefix before array reference", None));
                            return Err(());
                        }
                        let array = array_query_result.unwrap();
                        let array_ref = match array.get_type() {
                            IRBasicType::INT => IRNamedArray::new(array.get_irname(), IRBasicType::INT, vec![]),
                            IRBasicType::FLOAT => IRNamedArray::new(array.get_irname(), IRBasicType::FLOAT, vec![]),
                            IRBasicType::VOID => {
                                panic!("NEVER HERE")
                            }
                        };
                        Ok((vec![], array_ref))
                    }
                    _ => {
                        session.report_error(Error::new("expression can not be cast to an array", None));
                        Err(())
                    }
                }
            }
            AddExpr::Expr(_, _, _) => {
                session.report_error(Error::new("math expression can not be cast to an array", None));
                Err(())
            }
        };
    }

    fn is_none(&self) -> bool {
        return match self {
            AddExpr::Unary(expr) => match expr {
                UnaryExp::None => true,
                _ => false,
            },
            AddExpr::Expr(_, _, _) => false,
        };
    }
}

impl UnaryExp {
    /// 提供报错信息
    /// 取出立即数，如果是表达式，则计算立即数
    /// 这里的 need_const 提供给变量查询过程中，确定是否需要变量是const的
    fn to_ir_number(&self, variables_layer: &mut VariableLayer, session: &mut CompileSession, need_const: bool) -> Result<IRNumber, ()> {
        return match self {
            UnaryExp::Lit(suffix, lit) => {
                let suffix = parse_unary_suffix(&suffix, false); // fixme 注意这里的false
                if suffix.is_err() {
                    let error = lit
                        .span
                        .make_error("literal suffix error, there may be '!' used in assign expression", None);
                    session.report_error(error);
                    return Err(());
                }
                let number = check_lit(lit, session);
                Self::process_irnumber_or_err(suffix, number)
            }

            UnaryExp::VarRef(suffix, var) => {
                // 允许，但是该VarRef必须是const
                let suffix = parse_unary_suffix(&suffix, false);
                if suffix.is_err() {
                    let error = var.span.make_error("literal suffix error 1", None);
                    session.report_error(error);
                    return Err(());
                }
                let ret = variables_layer.query_available(&var.sym, need_const);
                match ret {
                    AvailableQueryResult::NotConst => {
                        session.report_error(var.span.make_error("not const symbol", Some("const variable expected")));
                        Err(())
                    }
                    AvailableQueryResult::NotFound => {
                        session.report_error(var.span.make_error("not found variable", Some("try add variable before here")));
                        Err(())
                    }
                    AvailableQueryResult::NotConstUnAvailable => {
                        session.report_error(var.span.make_error(
                            "not const variable,const variable expected",
                            Some("try to replace it with const variable or literal"),
                        ));
                        Err(())
                    }
                    AvailableQueryResult::Array => {
                        session.report_error(
                            var.span
                                .make_error("not found variable , but an array found", Some("try to replace it with variable")),
                        );
                        Err(())
                    }
                    AvailableQueryResult::Available(num) => {
                        let number = suffix.unwrap().process(num);
                        Ok(number)
                    }
                }
            }

            UnaryExp::ArrayRef(_, array) => {
                session.report_error(array.array_name.span.make_error("array reference are not allowed in here", None));
                // 不允许
                Err(())
            }

            UnaryExp::Paren(suffix, expr) => {
                let suffix = parse_unary_suffix(&suffix, false);
                if suffix.is_err() {
                    let error = Error::new("literal suffix error 2", None);
                    session.report_error(error);
                    return Err(());
                }
                let expr = expr.to_ir_number(variables_layer, session);
                // 允许，但是仅限于常量
                Self::process_irnumber_or_err(suffix, expr)
            }

            UnaryExp::Invoke(_, _invoke) => {
                session.report_error(_invoke.fun_name.span.make_error("invoke are not allowed in here.", None));
                Err(())
            }

            UnaryExp::None => {
                // 数组的第一维度没有出现，对于传递数组的情况，必须提前检测
                let error = Error::new("array dimension miss", None);
                session.report_error(error);
                Err(())
            }
        };
    }

    fn process_irnumber_or_err(suffix: Result<IRExprPrefix, ()>, number: Result<IRNumber, ()>) -> Result<IRNumber, ()> {
        match number {
            Ok(number) => {
                let number = suffix.unwrap().process(number);
                Ok(number)
            }
            Err(_) => Err(()),
        }
    }

    fn to_hir_guess(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        is_bool: bool,
    ) -> Result<(Vec<IRVarDefine>, IRBasicType, IRRValue), ()> {
        match &self {
            UnaryExp::VarRef(_, _) | UnaryExp::ArrayRef(_, _) | UnaryExp::Invoke(_, _) | UnaryExp::Lit(_, _) | UnaryExp::None => {
                let result = self.to_hir(variable_layer, name_table, session, is_bool, IRBasicType::INT); // 此处的期待类型无效
                return match result {
                    Ok((defs, ir_value)) => Ok((defs, ir_value.get_type_uncheck(), ir_value)),
                    Err(_) => Err(()),
                };
            }
            UnaryExp::Paren(_, _) => {
                allow_nothing!();
            }
        };
        if let UnaryExp::Paren(prefix, expr) = self {
            let suffix = parse_unary_suffix(&prefix, is_bool);
            if suffix.is_err() {
                let error = Error::new("expr suffix error", None);
                session.report_error(error);
                return Err(());
            }
            let suffix = suffix.unwrap();
            let expr = expr.to_hir_guess(variable_layer, name_table, session, is_bool);
            let expr = match expr {
                Ok((defs, guess_type, expr)) => (defs, guess_type, expr.combine_before_suffix(suffix)),
                Err(_) => {
                    session.report_error(Error::new("expression error", None));
                    return Err(());
                }
            };
            return Ok(expr);
        }
        panic!("NEVER HERE")
    }

    /// 转化UnaryExp为 IRValue
    /// 提供 未找到变量报错，未知前缀报错
    /// 7月31日 已经被完全替换,目前逻辑被 to_hir_guess 所沿用 ,而 to_hir_guess 中 对 UnaryExp::Paren 有自己的实现，所以这个函数后面考虑合并到 to_hir_guess 里面
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        is_bool: bool,
        expected_type: IRBasicType,
    ) -> Result<(Vec<IRVarDefine>, IRRValue), ()> {
        return match self {
            UnaryExp::Lit(suffix, literal) => {
                let suffix = parse_unary_suffix(&suffix, is_bool);
                if suffix.is_err() {
                    let error = literal.span.make_error("literal suffix error 3 ", None);
                    session.report_error(error);
                    return Err(());
                }
                let ret = check_lit(&literal, session);
                if ret.is_err() {
                    return Err(());
                }
                let rvalue = IRRValue::Basic(suffix.unwrap(), IRBasicExpr::Literal(ret.unwrap()));
                Ok((vec![], rvalue))
            }
            UnaryExp::VarRef(suffix, ident) => {
                let suffix = parse_unary_suffix(&suffix, is_bool);
                if suffix.is_err() {
                    let error = ident.span.make_error("variable suffix error", None);
                    session.report_error(error);
                    return Err(());
                }

                let var_res = variable_layer.query_any_var(&ident.sym);
                let mut var = match var_res {
                    Ok(named_var) => named_var,
                    Err(_) => {
                        session.report_error(ident.span.make_error("missing variable", None));
                        return Err(());
                    }
                };

                // opt hir available and const 基本传播
                if session.get_opt_level() >= OptLevel::O1 {
                    let var_available = variable_layer.query_available(&ident.sym, false);
                    if let AvailableQueryResult::Available(num) = var_available {
                        return Ok((vec![], IRRValue::Basic(suffix.unwrap(), IRBasicExpr::Literal(num))));
                    };
                } else {
                    let var_available = variable_layer.query_available(&ident.sym, true);
                    if let AvailableQueryResult::Available(num) = var_available {
                        return Ok((vec![], IRRValue::Basic(suffix.unwrap(), IRBasicExpr::Literal(num))));
                    };
                }

                var.ref_plus_1();
                Ok((vec![], IRRValue::Basic(suffix.unwrap(), IRBasicExpr::Var(var))))
            }
            UnaryExp::ArrayRef(suffix, array) => {
                let array_name_ident = array.array_name;
                let typed_array = variable_layer.query_array(&array_name_ident.sym);
                if typed_array.is_err() {
                    session.report_error(array_name_ident.span.make_error("missing array", None));
                    return Err(());
                }
                let pos = array.pos;
                let mut before_compute = vec![];
                let mut ir_pos = vec![];
                for single in pos {
                    let expr = single.to_hir(variable_layer, name_table, session, is_bool, IRBasicType::INT); // INT 类型寻址
                    let mut expr = match expr {
                        Ok(expr) => expr,
                        Err(_) => {
                            return Err(());
                        }
                    };
                    before_compute.append(&mut expr.0);
                    ir_pos.push(expr.1);
                }
                let declare_type = typed_array.unwrap();

                let name_array_ele = match declare_type.get_type() {
                    IRBasicType::INT | IRBasicType::FLOAT => {
                        if declare_type.get_dims_len() != ir_pos.len() {
                            let error = array_name_ident
                                .span
                                .make_error("right value refers to an array but dimensions is unmatched", None);
                            session.report_error(error);
                            return Err(());
                        }
                        IRNamedArrayEle::new(declare_type.get_irname(), declare_type.get_type(), ir_pos)
                    }
                    IRBasicType::VOID => panic!("ERROR"),
                };

                let suffix = parse_unary_suffix(&suffix, is_bool);
                if suffix.is_err() {
                    let error = Error::new("variable suffix error", None);
                    session.report_error(error);
                    return Err(());
                }
                declare_type.get_irname().ref_plus_1();
                Ok((before_compute, IRRValue::Basic(suffix.unwrap(), IRBasicExpr::ArrayEleRef(name_array_ele))))
            }
            UnaryExp::Invoke(suffix, invoke) => {
                let suffix = parse_unary_suffix(&suffix, is_bool);
                if suffix.is_err() {
                    let error = invoke.fun_name.span.make_error("invoke suffix error", None);
                    session.report_error(error);
                    return Err(());
                }
                let hir = invoke.to_hir(variable_layer, name_table, session, true);
                match hir {
                    Ok((pre, invoke)) => {
                        let invoke = IRRValue::Invoke(suffix.unwrap(), invoke);
                        Ok((pre, invoke))
                    }
                    Err(_) => Err(()),
                }
            }
            UnaryExp::Paren(suffix, expr) => {
                let suffix = parse_unary_suffix(&suffix, is_bool);
                if suffix.is_err() {
                    let error = Error::new("expr suffix error", None);
                    session.report_error(error);
                    return Err(());
                }
                let suffix = suffix.unwrap();
                // 这里已经被完全替换，理论上不可达
                let expr = expr.to_hir(variable_layer, name_table, session, is_bool, expected_type);
                let expr = match expr {
                    Ok(expr) => (expr.0, expr.1.combine_before_suffix(suffix)),
                    Err(_) => {
                        session.report_error(Error::new("expression error", None));
                        return Err(());
                    }
                };
                // let rvalue = match expr.1 {
                //     // fixme 想清楚，这里没有去处理前缀
                //     IRRValue::Invoke(_, a) => {
                //         IRRValue::Invoke(suffix.unwrap(), a)
                //     }
                //     IRRValue::Basic(_, b) => {
                //         IRRValue::Basic(suffix.unwrap(), b)
                //     }
                //     IRRValue::Math(_, a, b, c) => {
                //         IRRValue::Math(suffix.unwrap(), a, b, c)
                //     }
                //     IRRValue::Bool(_) => {
                //         // IRValue::Bool 是主动构造的，不可能出现在 AST 里
                //         // 而 主动构造的 IRRValue::Bool 只会直接以 HIR 的形式出现 ,
                //         // 即MathExpr不会去转成IRBool ， 而IRBool便也没有必要去转成IRRValue
                //         // 但是IRBool 可以转成 IRExpr
                //         panic!("NEVER HERE");
                //     }
                // };

                Ok(expr)
            }
            UnaryExp::None => {
                // 数组的第一维度没有出现，对于传递数组的情况，必须提前检测
                session.report_error(Error::new("array dimension miss", None));
                Err(())
            }
        };
    }
}

impl ArrayDefine {
    /// 该函数提供报错
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        is_global: bool,
    ) -> Result<(Vec<IRVarDefine>, IRArrayDefine), ()> {
        let is_const = self.is_const;
        let dims = self.dimensions;

        let mut ir_dim = vec![];
        let mut all_dim = 1;
        for single in dims {
            let ir_expr = single.to_ir_number(variable_layer, session);
            match ir_expr {
                Ok(ir) => {
                    // 这里完成了数组大小中可能不是整型的隐式转换
                    let dim = ir.to_const_usize();
                    ir_dim.push(dim);
                    all_dim *= dim;
                }
                Err(_) => {
                    let error = self.ident.span.make_error("array dimension error", None);
                    session.report_error(error);
                    return Err(());
                }
            }
        }
        if all_dim == 0 {
            let error = self.ident.span.make_error("array size = 0 , error dimension size ", None);
            session.report_error(error);
            return Err(());
        }
        let name = variable_layer.add_array(self.ident.sym, self.define_type.to_hir_type(), ir_dim.clone(), self.is_const, name_table);
        let mut typed_array = match name {
            Ok(name) => name,
            Err(_) => {
                let error = self.ident.span.make_error("symbol redeclare", None);
                session.report_error(error);
                return Err(());
            }
        };

        let mut pre_compute = vec![];
        let init = match self.init {
            None => IRArrayInit::new(),
            Some(init) => {
                if is_global {
                    let ret = init.to_gl_hir(&ir_dim, variable_layer, name_table, session, typed_array.get_type());
                    match ret {
                        Ok(ok) => ok,
                        Err(_) => {
                            return Err(());
                        }
                    }
                } else {
                    let ret = init.to_hir(&ir_dim, variable_layer, name_table, session, typed_array.get_type());
                    match ret {
                        Ok(mut ok) => {
                            pre_compute.append(&mut ok.0);
                            ok.1
                        }
                        Err(_) => {
                            return Err(());
                        }
                    }
                }
            }
        };
        Ok((
            pre_compute,
            IRArrayDefine::new(typed_array.get_irname(), typed_array.take_dims(), typed_array.get_type(), is_const, init),
        ))
    }
}

impl ArrayInit {
    fn read_init(
        dims: &[usize],
        init: ArrayInit,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        is_gl: bool,
        required_type: IRBasicType,
    ) -> Result<(Vec<IRVarDefine>, Vec<(usize, IRRValue)>), ()> {
        return match init {
            ArrayInit::Brace(init) => {
                let mut pre_compute = vec![];
                let mut ir_init = vec![];
                #[allow(unused)]
                    let mut pos_offset = 0;
                let mut base_offset = 0;
                let mut sub_layer_size = 1;
                if dims.len() > 1 {
                    for each in &dims[1..] {
                        sub_layer_size *= each;
                    }
                }

                for single in init {
                    match single {
                        ArrayInit::Brace(_) => {
                            pos_offset = sub_layer_size;
                        }
                        ArrayInit::None => {
                            pos_offset = sub_layer_size;
                        }
                        ArrayInit::Expr(_) => {
                            pos_offset = 1;
                        }
                    }
                    let ret = Self::read_init(&dims[1..], single, variable_layer, name_table, session, is_gl, required_type);
                    if ret.is_err() {
                        return Err(());
                    }
                    let mut ret = ret.unwrap();
                    pre_compute.append(&mut ret.0);
                    for (pos, ir_rvalue) in ret.1 {
                        ir_init.push((base_offset + pos, ir_rvalue));
                    }
                    base_offset += pos_offset;
                }
                Ok((pre_compute, ir_init))
            }
            ArrayInit::Expr(expr) => {
                if is_gl {
                    let ir_rvalue = expr.to_ir_number(variable_layer, session);
                    // fixme 注意这里初始化的时候没有检查类型
                    if ir_rvalue.is_err() {
                        return Err(());
                    }
                    let ret = ir_rvalue.unwrap();
                    Ok((vec![], vec![(0, IRRValue::Basic(IRExprPrefix::none(), IRBasicExpr::Literal(ret)))]))
                } else {
                    // Sysy支持这种情况吗 ？ ， 局部变量采用非compile time 初始化
                    let ir_rvalue = expr.to_hir_guess(variable_layer, name_table, session, false);
                    if ir_rvalue.is_err() {
                        return Err(());
                    }
                    let ret = ir_rvalue.unwrap();
                    Ok((ret.0, vec![(0, ret.2)]))
                }
            }
            ArrayInit::None => Ok((vec![], vec![])),
        };
    }

    fn to_hir(
        self,
        dimension: &Vec<usize>,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        reburied_type: IRBasicType,
    ) -> Result<(Vec<IRVarDefine>, IRArrayInit), ()> {
        if let ArrayInit::Expr(_) = self {
            session.report_error(Error::new("array init error", None));
            return Err(());
        }
        let ret = Self::read_init(dimension, self, variable_layer, name_table, session, false, reburied_type);
        if ret.is_err() {
            return Err(());
        }
        let mut array_init = IRArrayInit { init: vec![] };
        let ret = ret.unwrap();
        for single in ret.1 {
            array_init.init.push(single)
        }
        Ok((ret.0, array_init))
    }

    fn to_gl_hir(
        self,
        dimension: &Vec<usize>,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        reburied_type: IRBasicType,
    ) -> Result<IRArrayInit, ()> {
        let ret = Self::read_init(dimension, self, variable_layer, name_table, session, true, reburied_type);
        if ret.is_err() {
            return Err(());
        }
        let mut ir_init = IRArrayInit { init: vec![] };
        ir_init.init.append(&mut ret.unwrap().1);

        ir_init.init.sort_by(|a, b| {
            if a.0 > b.0 {
                return Ordering::Greater;
            } else if a.0 == b.0 {
                panic!("NEVER HERE")
            } else {
                return Ordering::Less;
            }
        });

        return Ok(ir_init);
    }
}

impl Assign {
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
    ) -> Result<(Vec<IRVarDefine>, IRAssign), ()> {
        return match self.to {
            LVarRef::VarRef(ident) => {
                let result = variable_layer.query_var(&ident.sym);
                match result {
                    VarQueryResult::Const => {
                        let error = ident.span.make_error("const value can't be assigned twice", None);
                        session.report_error(error);
                        return Err(());
                    }
                    VarQueryResult::Array => {
                        let error = ident.span.make_error("array redefine not support", None);
                        session.report_error(error);
                        return Err(());
                    }
                    VarQueryResult::NotFound => {
                        let error = ident.span.make_error("variable not found", None);
                        session.report_error(error);
                        return Err(());
                    }
                    VarQueryResult::Ok(_) => {}
                }; // 第一遍查询用来排查除错
                let from = self.from;
                let right_value = from.to_hir_guess(variable_layer, name_table, session, false);
                if right_value.is_err() {
                    return Err(());
                }
                let result = variable_layer.query_var_incremented(&ident.sym);
                match result {
                    VarQueryResult::Ok(mut var) => {
                        let (defs, _guess_type, right) = right_value.unwrap();
                        var.ref_plus_1();
                        match right.try_into_ir_number() {
                            None => {
                                let result = variable_layer.update_available_var(&ident.sym, None);
                                if result.is_err() {
                                    panic!("available update error")
                                }
                            }
                            Some(_) => {
                                // fixme 可优化,这里可以继续优化,如果右边是立即数,可以继续传播,注意一个前提，必须在没有IF,WHILE 分支的情况下
                                let result = variable_layer.update_available_var(&ident.sym, None);
                                if result.is_err() {
                                    panic!("available update error")
                                }
                            }
                        }
                        let assign = IRAssign {
                            left: IRLValue::Var(var),
                            right,
                        };
                        Ok((defs, assign))
                    }
                    _ => {
                        panic!("NEVER HERE");
                    }
                }
            }
            LVarRef::ArrayRef(array) => {
                let array_name_ident = array.array_name;
                let array_ref = variable_layer.query_array(&array_name_ident.sym);
                if array_ref.is_err() {
                    session.report_error(array_name_ident.span.make_error("missing array", None));
                    return Err(());
                }
                let pos = array.pos;
                let mut before_compute = vec![];
                let mut ir_pos = vec![];
                for single in pos {
                    // 虽然需求类型是 Int 但是如果返回Float怎么办
                    let expr = single.to_hir_guess(variable_layer, name_table, session, false);
                    let (mut defs, _expected_type, right) = match expr {
                        Ok(expr) => expr,
                        Err(_) => {
                            return Err(());
                        }
                    };
                    before_compute.append(&mut defs);
                    ir_pos.push(right);
                }
                let typed_array = array_ref.unwrap();
                let name_array_ele = match typed_array.get_type() {
                    IRBasicType::INT | IRBasicType::FLOAT => {
                        if typed_array.get_dims_len() != ir_pos.len() {
                            let error = array_name_ident
                                .span
                                .make_error("right value refers to an array but dimensions is unmatched", None);
                            session.report_error(error);
                            return Err(());
                        }
                        IRNamedArrayEle::new(typed_array.get_irname(), typed_array.get_type(), ir_pos)
                    }
                    IRBasicType::VOID => panic!("ERROR"),
                };

                let from = self.from;
                let right_value = from.to_hir_guess(variable_layer, name_table, session, false);
                if right_value.is_err() {
                    return Err(());
                }

                let (mut defs, _guess_type, rvalue) = right_value.unwrap();
                before_compute.append(&mut defs);
                let assign = IRAssign {
                    left: IRLValue::ArrayEle(name_array_ele),
                    right: rvalue,
                };
                Ok((before_compute, assign))
            }
        };
    }
}

impl RowDefines {
    /// 翻译局部的RowDefine
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
    ) -> Result<Vec<(Vec<IRVarDefine>, IRVarOrArrayDefine)>, ()> {
        let define_type = self.define_type;
        let is_const = self.is_const;
        let mut defs = vec![];
        for single in self.defs {
            match single {
                IdentWithInit::Var { ident, init } => {
                    // 复杂逻辑可优化
                    // 这里的逻辑有问题 首先对于数组大小的参数，必须是const，
                    // 所以对于to_gl_hir()，这个函数完成了两个任务，一级常量传播和数组的常数检测。
                    // 而数组在定义时候，其维度值的大小是已知的。所以必然不会产生pre_compute表达式。
                    let var_define = VarDefine {
                        is_const,
                        define_type: define_type.clone(),
                        ident: ident.clone(),
                        with_value: init,
                    };
                    let ret = var_define.to_hir(variable_layer, name_table, session);
                    if ret.is_err() {
                        let error = ident.span.make_error("variable define error", None);
                        session.report_error(error);
                        return Err(());
                    }
                    let def = ret.unwrap();
                    defs.push((def.0, IRVarOrArrayDefine::Var(def.1)))
                }
                IdentWithInit::Array { ident, dimensions, init } => {
                    let array_define = ArrayDefine {
                        is_const,
                        define_type: define_type.clone(),
                        ident,
                        dimensions,
                        init,
                    };
                    let def = array_define.to_hir(variable_layer, name_table, session, false);
                    if def.is_err() {
                        return Err(());
                    }
                    let def = def.unwrap();
                    defs.push((def.0, IRVarOrArrayDefine::Array(def.1)))
                }
            }
        }
        return Ok(defs);
    }
}

impl VarDefine {
    // 局部变量转化为IRVarDefine
    // 该函数提供报错
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
    ) -> Result<(Vec<IRVarDefine>, IRVarDefine), ()> {
        if self.is_const {
            let init = self.with_value;
            let init = match init {
                None => {
                    Some(self.define_type.make_ir_number(0.0))
                }
                Some(def) => {
                    let hir = def.to_ir_number(variable_layer, session);
                    if hir.is_err() {
                        return Err(());
                    }
                    Some(hir.unwrap())
                }
            };

            let var = variable_layer.add_any(self.ident.sym, self.define_type.to_hir_type(), name_table, init.clone(), true);
            if var.is_err() {
                // 这里要根据实际情况来看 , 虽然symbol redefine 足够简单，但是缺少了必要信息
                session.report_error(self.ident.span.make_error("symbol redefine", None));
                return Err(());
            }
            let init = match init {
                None => None,
                Some(number) => Some(IRRValue::Basic(IRExprPrefix::none(), IRBasicExpr::Literal(number))),
            };
            let var = var.unwrap();
            let var_def = IRVarDefine::new(var.copy_name(), var.get_type(), init, self.is_const, false);
            return Ok((vec![], var_def));
        }
        let mut before_compute = vec![];
        let init = self.with_value;
        let init = match init {
            None => {
                // 默认不初始化HIR
                None
            }
            Some(def) => {
                let hir = def.to_hir_guess(variable_layer, name_table, session, false);
                if hir.is_err() {
                    return Err(());
                }
                let (mut defs, _expected_type, rvalue) = hir.unwrap();
                before_compute.append(&mut defs);
                Some(rvalue)
            }
        };
        let var = variable_layer.add_any(self.ident.sym, self.define_type.to_hir_type(), name_table, None, false);
        if var.is_err() {
            // 这里要根据实际情况来看 , 虽然symbol redefine 足够简单，但是缺少了必要信息
            session.report_error(self.ident.span.make_error("symbol redefine", None));
            return Err(());
        }
        let var = var.unwrap();
        let var_def = IRVarDefine::new(var.copy_name(), var.get_type(), init, self.is_const, false);
        return Ok((before_compute, var_def));
    }

    /// # 全局变量翻译的唯一接口
    /// 全局变量转化为IRVarDefine
    fn to_gl_hir(self, variables_pool: &mut VariableLayer, name_table: &mut SymbolTable, session: &mut CompileSession) -> Result<IRVarDefine, ()> {
        let var = self;
        let init = var.with_value;
        let is_const = var.is_const;
        let define_type = var.define_type.to_hir_type();
        return match init {
            None => {
                let available = if is_const { Some(var.define_type.make_ir_number(0.0)) } else { None };
                let typed_var = variables_pool.add_any(
                    var.ident.sym,
                    var.define_type.to_hir_type(),
                    name_table,
                    // 默认值是 0 ,所以让他直接available即可
                    // 7 月 6 日修正,全局变量的available具有局限性，只能对于main函数，其他函数不行
                    available,
                    is_const,
                );

                if typed_var.is_err() {
                    session.report_error(var.ident.span.make_error("const value redefined", None));
                    return Err(());
                }
                let ret_var = typed_var.unwrap();
                Ok(
                    // init point
                    IRVarDefine::new(
                        ret_var.copy_name(),
                        ret_var.get_type(),
                        Some(IRRValue::Basic(
                            IRExprPrefix::none(),
                            IRBasicExpr::Literal(var.define_type.make_ir_number(0.0)),
                        )),
                        is_const,
                        false,
                    ),
                )
            }
            Some(expr) => {
                // 全局变量的初始值的右值必须是const的
                let ir_num = expr.to_ir_number(variables_pool, session);
                if ir_num.is_err() {
                    return Err(());
                }
                let init_number = ir_num.unwrap().check_change_type(define_type);
                let available = if is_const { Some(init_number.clone()) } else { None };
                let typed_var = variables_pool.add_any(var.ident.sym, define_type, name_table, available, is_const);
                if typed_var.is_err() {
                    session.report_error(var.ident.span.make_error("const value redefined", None));
                    return Err(());
                }
                let var = typed_var.unwrap();
                Ok(IRVarDefine::new(
                    var.copy_name(),
                    var.get_type(),
                    Some(IRRValue::Basic(IRExprPrefix::none(), IRBasicExpr::Literal(init_number))),
                    is_const,
                    false,
                ))
            }
        };
    }
}

impl Invoke {
    fn to_hir(
        self,
        variable_layer: &mut VariableLayer,
        name_table: &mut SymbolTable,
        session: &mut CompileSession,
        need_ret_val: bool,
    ) -> Result<(Vec<IRVarDefine>, IRInvoke), ()> {
        let an_fn = variable_layer.query_fn(&self.fun_name.sym);
        if an_fn.is_err() {
            session.report_error(self.fun_name.span.make_error("function not found", None));
            return Err(());
        }
        let fn_def = an_fn.unwrap();
        if need_ret_val {
            if fn_def.ret_type == BasicType::Void {
                session.report_error(self.fun_name.span.make_error("this function return void", None));
                return Err(());
            }
        }
        if self.params.len() != fn_def.params.len() {
            session.report_error(self.fun_name.span.make_error("function params number is unmatched", None));
            return Err(());
        }
        let mut pre_compute = vec![];
        let mut params = vec![];
        let mut index = 0;
        for act_param in self.params {
            let should_param = fn_def.params.get(index);
            if should_param.is_none() {
                session.report_error(self.fun_name.span.make_error("extra function param", None));
                return Err(());
            }
            let should_param = should_param.unwrap();

            match should_param {
                IRTypedParam::Var(var) => {
                    // fixme 传递过程没有做类型检查
                    let ret = act_param.to_hir_guess(variable_layer, name_table, session, false);
                    if ret.is_err() {
                        let error = self.fun_name.span.make_error("param passing to function is error 1", None);
                        session.report_error(error);
                        return Err(());
                    }
                    let (mut defs, _guess_type, right) = ret.unwrap();
                    pre_compute.append(&mut defs);
                    // 参数 的类型检查交给MIR
                    params.push(IRNamedParam::IRRValue(var.get_type(), right));
                }
                IRTypedParam::Array(array) => {
                    // _array 形式参数的名称，暂时没用 , 但是实际上，这里可以检查类型是否匹配
                    let named_array_result = act_param.to_named_array(variable_layer, name_table, session);
                    if named_array_result.is_err() {
                        let error = self.fun_name.span.make_error("param passing to function is error 0", None);
                        session.report_error(error);
                        return Err(());
                    }
                    let mut named_array = named_array_result.unwrap();
                    pre_compute.append(&mut named_array.0);
                    if array.get_type() != named_array.1.get_type() {
                        let error = self.fun_name.span.make_error("array type mismatch in passing param to function", None);
                        session.report_error(error);
                        return Err(());
                    }
                    params.push(IRNamedParam::IRNamedArray(named_array.1));
                }
            }
            index += 1;
        }

        let ir_invoke = IRInvoke {
            fun_name: fn_def.fn_name,
            return_type: fn_def.ret_type.to_hir_type(),
            variables: params,
            line_at: self.line_at,
        };
        return Ok((pre_compute, ir_invoke));
    }
}

impl IRNumber {
    fn to_const_usize(self) -> usize {
        match self {
            IRNumber::I32(num) => num as usize,
            IRNumber::F32(num) => {
                //fixme 危险情况，float转为usize
                num as usize
            }
        }
    }
}

impl BasicType {
    fn to_hir_type(&self) -> IRBasicType {
        return match self {
            BasicType::Void => IRBasicType::VOID,
            BasicType::Int => IRBasicType::INT,
            BasicType::Float => IRBasicType::FLOAT,
        };
    }
    /// 根据类型返回合适的IRNumber，如果是Int类型，则对f32进行强制转换
    fn make_ir_number(&self, num: f32) -> IRNumber {
        match self {
            BasicType::Float => IRNumber::F32(num),
            BasicType::Int => IRNumber::I32(num as i32),
            _ => {
                panic!("NOT SUPPORT")
            }
        }
    }
}

impl Symbol {
    fn parse_lit(&self, kind: &LiteralKind, session: &mut CompileSession) -> Result<IRNumber, ()> {
        match kind {
            LiteralKind::Int { base, empty_int: _ } => {
                return match base {
                    Base::Octal => {
                        let mut started = false;
                        let mut chars = VecDeque::new();
                        for char in self.string.chars() {
                            if !started {
                                if char != '0' {
                                    if char > '7' {
                                        return Err(());
                                    }
                                    chars.push_back(char);
                                    started = true;
                                }
                            } else {
                                if char > '7' {
                                    return Err(());
                                }
                                chars.push_back(char);
                            }
                        }
                        let mut base_num = 1;
                        let mut end = 0;
                        loop {
                            let num = chars.pop_back();
                            if num.is_none() {
                                break;
                            }
                            let num = num.unwrap().to_digit(8).unwrap() as i32;
                            end += num * base_num;
                            base_num = Base::Octal as i32 * base_num;
                        }
                        Ok(IRNumber::I32(end))
                    }
                    Base::Decimal => {
                        let number = self.string.parse();
                        if number.is_err() {
                            panic!("NEVER HERE")
                        }
                        let num: i32 = number.unwrap();
                        Ok(IRNumber::I32(num))
                    }
                    Base::Hexadecimal => {
                        let mut chars = VecDeque::new();
                        for (index, char) in self.string.chars().enumerate() {
                            if index == 0 || index == 1 {
                                continue; // 0x
                            }
                            chars.push_back(char);
                        }
                        let mut base_num = 1;
                        let mut end = 0;
                        let mut is_base_overflow = false;
                        loop {
                            let num = chars.pop_back();
                            if num.is_none() {
                                break;
                            }

                            if is_base_overflow {
                                session.report_warning(Warning::new(&format!("{} overflow", self.string), None))
                            }

                            let num = num.unwrap().to_digit(16).unwrap() as i32; // 字符在前面都是经过检查的，这里肯定是安全的
                            let ret = num * base_num;
                            end += ret;
                            let ret = (Base::Hexadecimal as i32).overflowing_mul(base_num); // 这里的问题，在release下会自己处理
                            base_num = ret.0;
                            if !is_base_overflow {
                                is_base_overflow = ret.1;
                            }
                        }
                        Ok(IRNumber::I32(end))
                    }
                };
            }
            LiteralKind::Float {
                base,
                empty_exponent: _,
                missing_exponent: _,
                missing_dot_after: _missing_dot_after,
            } => {
                match base {
                    Base::Octal => {
                        panic!("NEVER HERE")
                    }
                    Base::Decimal => {
                        let num: Result<f32, _> = self.string.parse();
                        if num.is_err() {
                            panic!("NEVER HERE")
                        }
                        return Ok(IRNumber::F32(num.unwrap()));
                    }
                    Base::Hexadecimal => {
                        let mut before_point = VecDeque::new();
                        let mut after_point = Vec::new();
                        let mut exponent = String::new();
                        let mut is_before_point = true;
                        let mut is_after_point = false;
                        let mut is_exponent = false;
                        let mut is_exponent_start = true;

                        let mut chars = self.string.chars();
                        let mut is_neg = false;
                        chars.next(); // 0
                        chars.next(); // x
                        for char in chars {
                            if is_before_point {
                                if char == 'p' {
                                    is_before_point = false;
                                    is_exponent = true;
                                    continue;
                                }
                                if char == '.' {
                                    is_before_point = false;
                                    is_after_point = true;
                                    continue;
                                }
                                before_point.push_back(char);
                                continue;
                            }
                            if is_after_point {
                                if char == 'p' || char == 'P' {
                                    is_after_point = false;
                                    is_exponent = true;
                                    continue;
                                }
                                after_point.push(char);
                                continue;
                            }
                            if is_exponent_start {
                                is_exponent_start = false;
                                if char == '-' {
                                    is_neg = true;
                                    continue;
                                }
                                if char == '+' {
                                    is_neg = false;
                                    continue;
                                }
                            }
                            if is_exponent {
                                exponent.push(char)
                            }
                        }
                        let mut first: i32 = 0;
                        let mut base = 1;
                        loop {
                            let ret = before_point.pop_back();
                            if ret.is_none() {
                                break;
                            }
                            first += ret.unwrap().to_digit(16).unwrap() as i32 * base;
                            base *= Base::Hexadecimal as i32;
                        }

                        let mut base = 1.0 / 16.0;
                        let mut center_all: f32 = 0.0;
                        for every in after_point {
                            center_all += every.to_digit(16).unwrap() as f32 * base;
                            base = base * 1.0 / 16.0;
                        }

                        let end: Result<u32, _> = exponent.parse();
                        if end.is_err() {
                            panic!("NEVER HERE")
                        }
                        let end = end.unwrap();
                        let end: f32 = 2_u32.pow(end) as f32;
                        let end_f32 = if is_neg {
                            (first as f32 + center_all) * 1.0 / end
                        } else {
                            (first as f32 + center_all) * end
                        };
                        return Ok(IRNumber::F32(end_f32));
                    }
                }
            }
        }
    }
}

impl MathOpe {
    fn to_hir_ope(&self) -> IRMathOpe {
        match self {
            MathOpe::Add => IRMathOpe::Add,
            MathOpe::Sub => IRMathOpe::Sub,
            MathOpe::Mul => IRMathOpe::Mul,
            MathOpe::Div => IRMathOpe::Div,
            MathOpe::Ram => IRMathOpe::Rem,
        }
    }
}

fn parse_unary_suffix(opes: &Vec<UnaryOp>, is_bool: bool) -> Result<IRExprPrefix, ()> {
    let mut wait_positive = true;
    let mut wait_negative = true;
    let mut wait_first = true;

    let mut is_bang = false;
    let mut is_bang_even = false; // 默认Bang是奇数次

    let mut is_negative = false;
    let mut is_negative_even = false;
    for ope in opes {
        if wait_first {
            match ope {
                UnaryOp::Positive => {
                    wait_positive = true;
                    wait_negative = true;
                }
                UnaryOp::Negative => {
                    wait_positive = true;
                    wait_negative = true;
                    is_negative = true;
                }
                UnaryOp::Bang => {
                    if !is_bool {
                        return Err(());
                    }
                    is_bang = true;
                }
                UnaryOp::Space => {
                    allow_nothing!();
                }
            }
            wait_first = false;
            continue;
        }
        match ope {
            UnaryOp::Positive => {
                if wait_positive {
                    wait_positive = true;
                    wait_negative = true;
                    continue;
                } else {
                    return Err(());
                }
            }
            UnaryOp::Negative => {
                if wait_negative {
                    wait_positive = true;
                    wait_negative = true;
                    if !is_bang {
                        if !is_negative {
                            is_negative = true;
                            is_negative_even = false;
                            continue;
                        }
                        if is_negative_even {
                            is_negative_even = false;
                        } else {
                            is_negative_even = true;
                        }
                    }
                    continue;
                } else {
                    return Err(());
                }
            }
            UnaryOp::Bang => {
                if !is_bool {
                    return Err(());
                }
                if is_bang {
                    if is_bang_even {
                        is_bang_even = false;
                    } else {
                        is_bang_even = true;
                    }
                } else {
                    is_bang = true;
                }
                continue;
            }
            UnaryOp::Space => {
                // 这里的最初的设计是 c-style 前缀，但是到了后面才注意到 Sysy因为不支持自增操作符，所以允许+-+-+-这样的操作，所以这个接口暂时就留着没用了
                wait_positive = true;
                wait_negative = true;
            }
        }
    }

    let first_suffix = if is_negative {
        if is_negative_even {
            IRFirstSuffix::None
        } else {
            IRFirstSuffix::Negative
        }
    } else {
        IRFirstSuffix::None
    };

    let second_suffix = if is_bang {
        if is_bang_even {
            IRSecondSuffix::EvenBang
        } else {
            IRSecondSuffix::OddBang
        }
    } else {
        IRSecondSuffix::None
    };

    return Ok(IRExprPrefix::new(first_suffix, second_suffix));
}

/// # 提供错误信息
fn check_lit(literal: &Literal, session: &mut CompileSession) -> Result<IRNumber, ()> {
    let kind = &literal.kind;
    match kind {
        LiteralKind::Int { base: _, empty_int } => {
            if *empty_int {
                let error = literal.span.make_error("illegal literal , number is empty", None);
                session.report_error(error);
                return Err(());
            }
            let number = literal.sym.parse_lit(kind, session);
            if number.is_err() {
                let error = literal.span.make_error("illegal literal", None);
                session.report_error(error);
                return Err(());
            }
            return number;
        }
        LiteralKind::Float {
            base,
            empty_exponent,
            missing_exponent,
            missing_dot_after: _missing_dot_after,
        } => {
            if *empty_exponent && !*missing_exponent {
                // only p or e appear
                let error = literal.span.make_error("error float literal , missing exponent", None);
                session.report_error(error);
                return Err(());
            }
            if *base == Base::Hexadecimal && *missing_exponent {
                let error = literal.span.make_error("error float hex literal ", None);
                session.report_error(error);
                return Err(());
            }
            match base {
                Base::Octal => {
                    panic!("NEVER HERE")
                }
                Base::Decimal | Base::Hexadecimal => {
                    let number = literal.sym.parse_lit(kind, session);
                    if number.is_err() {
                        let error = literal.span.make_error("illegal literal", None);
                        session.report_error(error);
                    }
                    return number;
                }
            }
            // FLOAT
        }
    }
}

fn stmts_to_hir(
    stmts: Stmts,
    variable_layer: &mut VariableLayer,
    name_table: &mut SymbolTable,
    session: &mut CompileSession,
    in_while: bool,
    expect_ret_type: Option<IRBasicType>,
) -> Result<Vec<IRStatement>, ()> {
    let mut ir_stmts = vec![];
    for stmt in stmts {
        match stmt {
            Stmt::IfElse(branches) => {
                let ret = branches.to_hir(variable_layer, name_table, session, in_while, expect_ret_type);
                if ret.is_err() {
                    return Err(());
                }
                ir_stmts.push(IRStatement::Branches(ret.unwrap()))
            }
            Stmt::While(circulate) => {
                let ir_while = circulate.to_hir(variable_layer, name_table, session, expect_ret_type);
                if ir_while.is_err() {
                    return Err(());
                }
                ir_stmts.push(IRStatement::While(ir_while.unwrap()))
            }
            Stmt::Continue => {
                if in_while {
                    ir_stmts.push(IRStatement::Continue);
                    continue;
                }
                // 函数体内 错误
                session.report_error(Error::new("'continue' are now allowed in here", None));
                return Err(());
            }
            Stmt::Break => {
                if in_while {
                    ir_stmts.push(IRStatement::Break);
                    continue;
                }
                session.report_error(Error::new("'break' are now allowed in here", None));
                return Err(());
            }
            Stmt::Return(ret) => {
                // 返回值类型没有检查
                match ret.return_value {
                    None => {
                        if expect_ret_type.is_none() {
                            panic!("NEVER HERE")
                        }
                        let ret_type = expect_ret_type.unwrap();
                        if ret_type != IRBasicType::VOID {
                            session.report_error(Error::new("the non-void function can't return nothing", None));
                            return Err(());
                        }
                        ir_stmts.push(IRStatement::Return(None))
                    }
                    Some(expr) => {
                        if expect_ret_type.is_none() {
                            panic!("NEVER HERE")
                        }
                        let ret_type = expect_ret_type.unwrap();
                        if ret_type == IRBasicType::VOID {
                            session.report_error(Error::new(
                                "the void function can't return any value",
                                make_advise("try remove the expression after the 'return'"),
                            ));
                            return Err(());
                        }
                        let expr = expr.to_hir_guess(variable_layer, name_table, session, false);
                        if expr.is_err() {
                            return Err(());
                        }
                        let (defs, _guess_type, right) = expr.unwrap();
                        for i in defs {
                            ir_stmts.push(IRStatement::VarDef(i))
                        }
                        // return 的类型检查交给MIR
                        ir_stmts.push(IRStatement::Return(Some(right)));
                    }
                }
            }
            Stmt::Assign(assign) => {
                let assign = assign.to_hir(variable_layer, name_table, session);
                if assign.is_err() {
                    return Err(());
                }
                let assign = assign.unwrap();
                for x in assign.0 {
                    ir_stmts.push(IRStatement::VarDef(x));
                }
                ir_stmts.push(IRStatement::Assign(assign.1))
            }
            Stmt::VarDefine(define) => {
                let def = define.to_hir(variable_layer, name_table, session);
                if def.is_err() {
                    return Err(());
                }
                let def = def.unwrap();
                for single in def.0 {
                    ir_stmts.push(IRStatement::VarDef(single))
                }
                ir_stmts.push(IRStatement::VarDef(def.1))
            }
            Stmt::ArrayDefine(define) => {
                let hir_array = define.to_hir(variable_layer, name_table, session, false);
                if hir_array.is_err() {
                    return Err(());
                }
                let array_define = hir_array.unwrap();
                for define in array_define.0 {
                    ir_stmts.push(IRStatement::VarDef(define))
                }
                ir_stmts.push(IRStatement::ArrDef(array_define.1))
            }
            Stmt::RowDefines(row_def) => {
                let defs = row_def.to_hir(variable_layer, name_table, session);
                if defs.is_err() {
                    return Err(());
                }
                let def = defs.unwrap();
                for sing_def in def {
                    match sing_def.1 {
                        IRVarOrArrayDefine::Var(def) => {
                            for before_def in sing_def.0 {
                                ir_stmts.push(IRStatement::VarDef(before_def));
                            }
                            ir_stmts.push(IRStatement::VarDef(def));
                        }
                        IRVarOrArrayDefine::Array(def) => {
                            for before_def in sing_def.0 {
                                ir_stmts.push(IRStatement::VarDef(before_def));
                            }
                            ir_stmts.push(IRStatement::ArrDef(def));
                        }
                    }
                }
            }
            Stmt::Invoke(invoke) => {
                let invoke = invoke.to_hir(variable_layer, name_table, session, false);
                match invoke {
                    Ok((pre, invoke)) => {
                        for stat in pre {
                            ir_stmts.push(IRStatement::VarDef(stat))
                        }
                        ir_stmts.push(IRStatement::FunInvoke(invoke))
                    }
                    Err(_) => {
                        return Err(());
                        //panic!("invoke translate error")
                    }
                }
            }
            Stmt::Block(stmt) => {
                let mut new_variable_layer = variable_layer.new_nested_layer();
                let stmts = stmts_to_hir(stmt, &mut new_variable_layer, name_table, session, in_while, expect_ret_type);
                if stmts.is_err() {
                    return Err(());
                }
                let stmt = IRStatement::Block(stmts.unwrap());
                ir_stmts.push(stmt);
                new_variable_layer.drop()
            }
            Stmt::Expr(expr) => {
                let ret = expr.to_hir(variable_layer, name_table, session);
                if ret.is_err() {
                    return Err(());
                }
                let stmt = ret.unwrap();
                ir_stmts.push(IRStatement::EmptyExpr(stmt));
            }
            Stmt::None => {
                allow_nothing!();
            }
        }
    }
    return Ok(ir_stmts);
}

pub fn new_temp_var(symbol_table: &mut SymbolTable, right: IRRValue, basic_type: IRBasicType) -> (IRVarDefine, IRTypedVar) {
    let temp_var_sym = symbol_table.add_symbol(AnySymbol::new_rand_underscore("te"));
    let temp_var = IRName::from_ref_zero_sub(temp_var_sym);
    let temp_var_define = IRVarDefine::new(temp_var.clone(), basic_type, Some(right), false, true);
    let mut named_temp_var = IRNamedVar::new(temp_var, basic_type);
    named_temp_var.set_temp();
    (temp_var_define, named_temp_var)
}
