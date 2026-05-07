use crate::allow_nothing;
use crate::ast::table::{AnySymbol, SymbolTable};
use crate::compiler::CompileOptions;
use crate::hir::hir::{
    HirModule, HirUnit, IRArrayDefine, IRAssign, IRBasicExpr, IRBasicType, IRBool, IRBoolExpr, IRBoolOpe, IRBoolOrVarDefine, IRBoolVarDefine,
    IRBranches, IRExprPrefix, IRExternalFun, IRFirstSuffix, IRFunDefine, IRInvoke, IRLValue, IRMathOpe, IRNamedArray, IRNamedArrayEle, IRNamedParam,
    IRNumber, IRRValue, IRRelSource, IRSecondSuffix, IRStatement, IRStatements, IRTypedParam, IRVarDefine, IRVarOrArrayDefine, IRWhile,
};
use crate::mir::MIRVarActiveType::{Normal, Temp};
use crate::mir::{MIRArrayDef, MIRArrayEleCopyAssign, MIRArrayEleRef, MIRArrayInit, MIRBlock, MIRBool, MIRBoolCmpAssign, MIRBoolCopyAssign, MIRBoolLRef, MIRBoolOper, MIRCmpExpr, MIRCmpRight, MIRCondJump, MIRFun, MIRGlDef, MIRInvoke, MIRJump, MIRLabel, MIRLit, MIRMathCvt, MIRMathLRef, MIRMathOper, MIRMathPrefix, MIRMathRight, MIRModule, MIRName, MIRNamedArrayParam, MIRNamedParam, MIRRefPool, MIRReturn, MIRRight, MIRStmt, MIRType, MIRUnit, MIRVarActiveType, MIRVarComputeAssign, MIRVarCopyAssign, MIRVarRef};
use crate::optimizer::OptLevel::O1;
use crate::session::CompileSession;
use std::collections::{HashMap, HashSet, VecDeque};
use std::{mem, vec};
use std::vec::IntoIter;
use crate::util::Kotlin;

#[derive(Eq, PartialEq, Debug, Hash, Copy, Clone)]
enum VariableEnv {
    #[allow(unused)]
    Unknown,
    InFun,
    InWhileCond,
    InWhile,
}

pub struct MIRTranslate<'a> {
    ref_pool: MIRRefPool,
    symbol_table: SymbolTable,
    now_continue_tag: VecDeque<MIRLabel>,
    now_break_tag: VecDeque<MIRLabel>,
    now_ret_type: IRBasicType,
    env_stack: VecDeque<VariableEnv>,
    session: &'a mut CompileSession,

    current_actives: HashMap<MIRVarRef, usize>, // 定义来自上一个环境但是当前环境活跃
    before_actives:HashMap<MIRVarRef,usize>,
    current_defines:HashSet<MIRVarRef>,         // 在当前环境下定义
    global_defines:HashSet<MIRVarRef>,          // 全局定义

    current_lazy_drop: HashSet<MIRVarRef>,
}

impl<'a> MIRTranslate<'a> {
    pub fn parse_module(
        hir_module: HirModule,
        ref_pool: MIRRefPool,
        symbol_pool: SymbolTable,
        session: &mut CompileSession,
    ) -> (MIRModule, MIRRefPool, SymbolTable) {
        let mut translate = MIRTranslate {
            ref_pool,
            symbol_table: symbol_pool,
            now_continue_tag: VecDeque::new(),
            now_break_tag: VecDeque::new(),
            now_ret_type: IRBasicType::VOID,
            env_stack: Default::default(),
            current_actives: Default::default(),
            before_actives: Default::default(),
            current_defines: Default::default(),
            global_defines: Default::default(),
            current_lazy_drop: Default::default(),
            session,
        };
        let mut mir_module = MIRModule::new();
        for hir_unit in hir_module.units {
            let mir_units = translate.ir_unit_mir(hir_unit);
            for unit in mir_units {
                mir_module.add_unit(unit);
            }
        }
        (mir_module, translate.ref_pool, translate.symbol_table)
    }

    fn ir_unit_mir(&mut self, unit: HirUnit) -> Vec<MIRUnit> {
        return match unit {
            HirUnit::GLVarDefine(mut var_def) => {
                let is_const = var_def.is_const();
                if is_const {
                    let name = var_def.get_name();
                    if name.get_all_ref_num() != 0 {
                        panic!("ERROR CONST REF NUMBER > 0")
                    } else {
                        return vec![];
                    }
                }

                let sym_ref = var_def.get_name().get_sym_ref();
                let var_ref = self.ref_pool.add_var_by_hir_type(sym_ref, var_def.get_type(), MIRVarActiveType::Global);
                let def = MIRGlDef::Var(var_def.is_const(), var_ref, var_def.take_init().unwrap().into_ir_number());
                vec![MIRUnit::Def(def)]
            }
            HirUnit::GLArrayDefine(mut array_def) => {
                let sym_ref = array_def.get_name().get_sym_ref();
                let mir_array_ref = self
                    .ref_pool
                    .add_array(sym_ref, array_def.copy_dims(), array_def.compute_ele_num(), array_def.get_type());
                let inits = array_def.take_init();
                let mut mir_init = MIRArrayInit { inits: vec![] };
                for (index, init) in inits.init {
                    let init = init.into_ir_number();
                    mir_init.inits.push((index, init));
                }
                vec![MIRUnit::Def(MIRGlDef::Array(array_def.is_const(), mir_array_ref, mir_init))]
            }
            HirUnit::GlDefines(defs) => {
                let mut def_units = vec![];
                for single in defs {
                    match single {
                        IRVarOrArrayDefine::Var(mut var_def) => {
                            let is_const = var_def.is_const();
                            if is_const {
                                let name = var_def.get_name();
                                if name.get_all_ref_num() != 0 {
                                    panic!("ERROR CONST REF NUMBER > 0")
                                } else {
                                    return vec![];
                                }
                            }
                            let sym_ref = var_def.get_name().get_sym_ref();
                            let var_ref = self.ref_pool.add_var_by_hir_type(sym_ref, var_def.get_type(), MIRVarActiveType::Global);
                            let def = MIRGlDef::Var(var_def.is_const(), var_ref, var_def.take_init().unwrap().into_ir_number());
                            def_units.push(MIRUnit::Def(def));
                        }
                        IRVarOrArrayDefine::Array(mut array_def) => {
                            let sym_ref = array_def.get_name().get_sym_ref();
                            let var_ref = self
                                .ref_pool
                                .add_array(sym_ref, array_def.copy_dims(), array_def.compute_ele_num(), array_def.get_type());
                            let inits = array_def.take_init();
                            let mut mir_init = MIRArrayInit { inits: vec![] };
                            for (index, init) in inits.init {
                                let init = init.into_ir_number();
                                mir_init.inits.push((index, init));
                            }
                            def_units.push(MIRUnit::Def(MIRGlDef::Array(array_def.is_const(), var_ref, mir_init)));
                        }
                    }
                }
                def_units
            }
            HirUnit::FunDefine(fun_def) => {
                vec![MIRUnit::Fun(self.ir_fun_mir(fun_def))]
            }
            HirUnit::ExternalFun(external_fun) => {
                self.ir_external_fun_to_mir(external_fun);
                vec![]
            }
        };
    }

    /// 将RelSource转为MirBoolRight
    fn ir_rel_source_to_mir(&mut self, rel_source: IRRelSource) -> (Vec<MIRStmt>, MIRType, MIRCmpRight) {
        match rel_source {
            IRRelSource::Literal(lit) => {
                (vec![], lit.get_type().get_correspond_mir_type(), MIRCmpRight::Lit(lit))
            }
            IRRelSource::Var(var) => {
                let var = self.ref_pool.get_lvar_ref(var.get_name_sym_ref());
                self.increase_active(var);
                (vec![], var.get_type().clone(), MIRCmpRight::MathL(MIRMathLRef::Var(var)))
            }
            IRRelSource::ArrayEleRef(ele) => {
                let (bef_computes, ele) = self.ir_name_array_ele_to_mir(ele);
                // todo 这里没有处理类型
                (bef_computes, ele.get_type(), MIRCmpRight::MathL(MIRMathLRef::ArrayEle(ele)))
            }
            IRRelSource::BasicBool(basic) => {
                let var = self.ref_pool.get_lvar_ref(basic.get_name_sym_ref());
                self.increase_active(var);
                (vec![], MIRType::Bool, MIRCmpRight::CmpL(MIRBoolLRef::new(var)))
            }
        }
    }

    fn ir_bool_def_to_mir(&mut self, bool_def: IRBoolVarDefine) -> Vec<MIRStmt> {
        let mut stmts = vec![];
        let active_type = if bool_def.is_temp() {
            Temp
        } else {
            Normal
        };
        let left_var = self
            .ref_pool
            .add_var_by_mir_type(bool_def.get_name().get_sym_ref(), MIRType::Bool, active_type);
        match bool_def.init {
            IRBool::Basic(_basic) => {
                // let (mut stmts1, source_type, right) = self.ir_rel_source_to_mir(basic);
                // stmts.append(&mut stmts1);
                // // 这里的实现似乎有问题 ， 右值完全可以是float 啊
                // let stmt = MIRBoolCopyAssign::new(true, false, MIRBoolLRef::new(left_var), right);
                // stmts.push(MIRStmt::BoolCopyAssign(stmt));
                panic!("NEVER HERE");
            }
            IRBool::Rel(a, oper, c) => {
                let (mut stmts1, source_type1, right1) = self.ir_rel_source_to_mir(a);
                stmts.append(&mut stmts1);
                let (mut stmts2, source_type2, right2) = self.ir_rel_source_to_mir(c);
                stmts.append(&mut stmts2);
                if source_type1 != MIRType::Float && source_type2 != MIRType::Float
                    || (source_type1 == MIRType::Float && source_type2 == MIRType::Float)
                {
                    let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), right2);
                    let stmt = MIRBoolCmpAssign::new(true, false, MIRBoolLRef::new(left_var), cmp_expr);
                    stmts.push(MIRStmt::BoolCmpAssign(stmt));
                } else if source_type1 == MIRType::Float && source_type2 != MIRType::Float {
                    let (stmt, var2_ref) = self.mir_cmp_right_into_var_ref(right2);
                    if stmt.is_some() {
                        stmts.push(stmt.unwrap());
                    }
                    let new_var2 = self.new_var_by_env_mir(MIRType::Float);
                    let new_cvt = self.new_mir_cvt(true, new_var2, var2_ref);
                    stmts.push(new_cvt);
                    let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), MIRCmpRight::MathL(MIRMathLRef::Var(new_var2)));
                    stmts.push(MIRStmt::BoolCmpAssign(MIRBoolCmpAssign::new(
                        true,
                        false,
                        MIRBoolLRef::new(left_var),
                        cmp_expr,
                    )));
                } else if source_type1 != MIRType::Float && source_type2 == MIRType::Float {
                    let (stmt, var1_ref) = self.mir_cmp_right_into_var_ref(right1);
                    if stmt.is_some() {
                        stmts.push(stmt.unwrap());
                    }
                    let new_var1 = self.new_var_by_env_mir(MIRType::Float);
                    let new_cvt = self.new_mir_cvt(true, new_var1, var1_ref);
                    stmts.push(new_cvt);
                    let cmp_expr = MIRCmpExpr::new(MIRCmpRight::MathL(MIRMathLRef::Var(new_var1)), oper.get_correspond_mir_type(), right2);
                    stmts.push(MIRStmt::BoolCmpAssign(MIRBoolCmpAssign::new(
                        true,
                        false,
                        MIRBoolLRef::new(left_var),
                        cmp_expr,
                    )));
                }
            }
        }
        stmts
    }

    fn ir_var_def_to_mir(&mut self, mut var_def: IRVarDefine) -> Vec<MIRStmt> {
        let mut current_block = vec![];
        let sym_ref = var_def.get_name().get_sym_ref();
        let active_type =
            if var_def.is_temp() {
                Temp
            } else {
                Normal
            };
        let mir_var_ref = self.ref_pool.add_var_by_hir_type(sym_ref, var_def.get_type(), active_type);
        self.increase_active(mir_var_ref);
        let mir_var_init = var_def.take_init();
        match mir_var_init {
            None => {
                panic!("HIR ERROR")
            }
            Some(init) => {
                let ret = self.ir_rvalue_to_mir(init, var_def.get_type());
                let assign = MIRVarCopyAssign::new(true, mir_var_ref, ret.1);
                let stmt = MIRStmt::MathVarCopyAssign(assign);
                for stmt in ret.0 {
                    current_block.push(stmt);
                }
                current_block.push(stmt)
            }
        }
        current_block
    }

    fn ir_assign_mir(&mut self, assign: IRAssign) -> Vec<MIRStmt> {
        let left = assign.left;
        return match left {
            IRLValue::Var(var) => {
                // opt 基本死代码消除
                if self.session.get_opt_level() >= O1 {
                    if var.get_name().get_all_ref_num() == 0 {
                        return vec![];
                    }
                }
                let (mut stmts, right) = self.ir_rvalue_to_mir(assign.right, var.get_type());
                let dest = self.ref_pool.get_lvar_ref(var.get_name_sym_ref());
                self.increase_active(dest);
                stmts.push(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(false, dest, right)));
                stmts
            }
            IRLValue::ArrayEle(array_ele) => {
                let (mut stmts, right) = self.ir_rvalue_to_mir(assign.right, array_ele.get_type());
                // 可优化，这里的右值 ？？？
                let mut mir = self.ir_name_array_ele_to_mir(array_ele);
                stmts.append(&mut mir.0);
                stmts.push(MIRStmt::ArrayEleAssign(MIRArrayEleCopyAssign::new(
                    mir.1,
                    right,
                )));
                stmts
            }
        };
    }

    fn ir_fun_mir(&mut self, mut fun_def: IRFunDefine) -> MIRFun {
        self.env_stack.push_back(VariableEnv::InFun); // 指定当前环境为在函数内部
        let any_symbol_ref = fun_def.get_name().get_sym_ref();
        self.now_ret_type = fun_def.get_ret_type();
        let fun_name = self.ref_pool.add_fn(any_symbol_ref);
        let mut mir_fun = MIRFun::new(fun_name);
        if self.now_ret_type != IRBasicType::VOID {
            mir_fun.set_ret_type(self.now_ret_type.get_correspond_mir_type());
        }
        let params = fun_def.take_params();
        for param in params {
            match param {
                IRTypedParam::Var(var) => {
                    let sym_ref = var.get_name_sym_ref();
                    let mir_var = self.ref_pool.add_var_by_hir_type(sym_ref.clone(), var.get_type(), Normal);
                    mir_fun.add_var_param(mir_var);
                }
                IRTypedParam::Array(mut array) => {
                    let size = array.get_array_size();
                    let array = self
                        .ref_pool
                        .add_array(array.get_irname().get_sym_ref(), array.take_dims(), size, array.get_type());
                    mir_fun.add_array_param(array)
                }
            }
        }

        let stmts = fun_def.take_stmts();
        let label = self.new_mir_label_or_name(".L_block_");
        let mut current_block = MIRBlock::new(label);
        for stmt in stmts {
            match stmt {
                IRStatement::VarDef(var_def) => {
                    if self.session.get_opt_level() >= O1 {
                        if var_def.get_name().get_all_ref_num() == 0 {
                            continue; // opt 基本死代码消除
                        }
                    }
                    let stmts = self.ir_var_def_to_mir(var_def);
                    current_block.append(stmts);
                }
                IRStatement::ArrDef(array_def) => {
                    let (stmts, def) = self.ir_array_def_to_mir(array_def);
                    current_block.append(stmts);
                    current_block.add_stmt(MIRStmt::ArrayDef(def))
                }
                IRStatement::Assign(assign) => {
                    let stmts = self.ir_assign_mir(assign);
                    current_block.append(stmts);
                }
                IRStatement::EmptyExpr(_) => {
                    // 注意 10 + invoke_affect_global_var();  样例没出现
                    // drop
                }
                IRStatement::Block(block) => {
                    if current_block.size() > 0 {
                        mir_fun.add_block(current_block);
                        let label = self.new_mir_label_or_name(".L_in_block_");
                        let blocks = self.ir_block_to_mir(block, label);
                        for block in blocks {
                            mir_fun.add_block(block)
                        }
                    } else {
                        let blocks = self.ir_block_to_mir(block, current_block.copy_name());
                        for block in blocks {
                            mir_fun.add_block(block)
                        }
                    }

                    let label = self.new_mir_label_or_name(".L_after_block_");
                    current_block = MIRBlock::new(label);
                }
                IRStatement::While(while_loop) => {
                    if current_block.size() > 0 {
                        mir_fun.add_block(current_block);
                        let label = self.new_mir_label_or_name(".L_while_start_");
                        let next_label = self.new_mir_label_or_name(".L_after_while_");
                        current_block = MIRBlock::new(next_label);
                        let blocks = self.ir_while_to_mir(while_loop, label, next_label);
                        for block in blocks {
                            mir_fun.add_block(block)
                        }
                    } else {
                        let next_label = self.new_mir_label_or_name(".L_after_while_");
                        let blocks = self.ir_while_to_mir(while_loop, current_block.copy_name(), next_label);
                        current_block = MIRBlock::new(next_label);
                        for block in blocks {
                            mir_fun.add_block(block)
                        }
                    }
                }
                IRStatement::Branches(branches) => {
                    if current_block.size() > 0 {
                        mir_fun.add_block(current_block);
                        let label = self.new_mir_label_or_name(".L_enter_cond_");
                        let exit_label = self.new_mir_label_or_name(".L_after_branches_");
                        let blocks = self.ir_branches_to_mir(branches, label, exit_label);
                        current_block = MIRBlock::new(exit_label);
                        for block in blocks {
                            mir_fun.add_block(block);
                        }
                    } else {
                        let exit_label = self.new_mir_label_or_name(".L_after_branches_");
                        let blocks = self.ir_branches_to_mir(branches, current_block.copy_name(), exit_label);
                        current_block = MIRBlock::new(exit_label);
                        for block in blocks {
                            mir_fun.add_block(block);
                        }
                    }
                }
                IRStatement::FunInvoke(invoke) => {
                    let ret = self.ir_invoke_to_mir(invoke);
                    current_block.append(ret.0);
                    current_block.add_stmt(MIRStmt::Invoke(ret.1))
                }
                IRStatement::Return(ret) => match ret {
                    None => current_block.add_stmt(MIRStmt::Return(MIRReturn::just_return())),
                    Some(expr) => {
                        let (stmts, math_right) = self.ir_rvalue_to_mir(expr, self.now_ret_type);
                        for stmt in stmts {
                            current_block.add_stmt(stmt);
                        }
                        let (stmt, lref) = self.mir_math_right_into_lref(math_right);
                        match stmt {
                            None => {
                                allow_nothing!();
                            }
                            Some(stmt) => current_block.add_stmt(stmt),
                        }
                        current_block.add_stmt(MIRStmt::Return(MIRReturn::var_return(lref)))
                    }
                },
                IRStatement::BoolDef(_) => {
                    panic!("NEVER HERE")
                }
                IRStatement::BoolAssign(_) => {
                    // bool assign can only appear in branch's pre_computed defines stmt
                    panic!("NEVER HERE")
                }
                IRStatement::Continue => {
                    panic!("NEVER HERE")
                }
                IRStatement::Break => {
                    panic!("NEVER HERE")
                }
            }
        }
        mir_fun.add_block(current_block); // 最后一块直接加入,有时候是while的出口
        let env_result = self.env_stack.pop_back();
        match env_result {
            None => {
                panic!("退出函数的时候环境异常，无环境")
            }
            Some(env) => {
                if env != VariableEnv::InFun {
                    println!("{:?}", self.env_stack);
                    panic!("退出函数的时候环境异常，环境不匹配 {:?}", env)
                }
            }
        }
        mir_fun
    }

    fn ir_invoke_to_mir(&mut self, ir_invoke: IRInvoke) -> (Vec<MIRStmt>, MIRInvoke) {
        let name = self.ref_pool.get_fn_name(&ir_invoke.fun_name.get_sym_ref());
        let fun_name_str = self.symbol_table.get_symbol(&ir_invoke.fun_name.get_sym_ref());
        if fun_name_str.get_string() == "starttime".to_string() {
            let symbol_ref = self.symbol_table.get_symbol_ref(&AnySymbol::new("_sysy_starttime")).unwrap();
            let mir_ref = self.ref_pool.get_fn_name(&symbol_ref);
            let new_invoke = MIRInvoke::new(mir_ref, vec![MIRNamedParam::Right(MIRRight::Lit(MIRLit::I32(ir_invoke.line_at as i32)))], MIRType::Void);
            return (vec![], new_invoke);
        } else if fun_name_str.get_string() == "stoptime".to_string() {
            let symbol_ref = self.symbol_table.get_symbol_ref(&AnySymbol::new("_sysy_stoptime")).unwrap();
            let mir_ref = self.ref_pool.get_fn_name(&symbol_ref);
            let new_invoke = MIRInvoke::new(mir_ref, vec![MIRNamedParam::Right(MIRRight::Lit(MIRLit::I32(ir_invoke.line_at as i32)))], MIRType::Void);
            return (vec![], new_invoke);
        }
        let mut stmts = vec![];
        let mut params = vec![];
        for param in ir_invoke.variables {
            match param {
                IRNamedParam::IRRValue(expected, actual) => {
                    let mut mir_right = self.ir_rvalue_to_mir(actual, expected);
                    stmts.append(&mut mir_right.0);
                    if mir_right.1.is_support_unwrap() {
                        let right = mir_right.1.into_right_uncheck();
                        params.push(MIRNamedParam::Right(right));
                    } else {
                        // 堆叠中间变量
                        let lref = self.new_var_by_env_mir(mir_right.1.math.get_type()); //solve
                        stmts.push(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(
                            true,
                            lref,
                            mir_right.1,
                        )));
                        params.push(MIRNamedParam::Right(MIRRight::VarRef(lref)))
                    }
                }
                IRNamedParam::IRNamedArray(ir_name_array) => {
                    let (mut pre_stmts, array) = self.ir_name_array_to_mir(ir_name_array);
                    stmts.append(&mut pre_stmts);
                    params.push(MIRNamedParam::Array(array))
                }
            }
        }
        let mir = MIRInvoke::new(name, params, ir_invoke.return_type.get_correspond_mir_type());
        (stmts, mir)
    }

    /// 将表达式转换为 MIR右值，同时做类型转换
    fn ir_rvalue_to_mir(&mut self, rvalue: IRRValue, expected_type: IRBasicType) -> (Vec<MIRStmt>, MIRMathRight) {
        match rvalue {
            IRRValue::Invoke(suffix, invoke) => {
                let invoke_ret_type = invoke.return_type;
                let mut mir_invoke = self.ir_invoke_to_mir(invoke);
                return if suffix.is_none() && expected_type == invoke_ret_type {
                    (mir_invoke.0, MIRMathRight::new(MIRMathPrefix::None, MIRRight::Invoke(mir_invoke.1)))
                } else {
                    let var = self.new_var_by_env_hir(invoke_ret_type);
                    let assign = MIRVarCopyAssign::new(
                        true,
                        var.clone(),
                        MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::Invoke(mir_invoke.1)),
                    );
                    let ir_stmt = MIRStmt::MathVarCopyAssign(assign);

                    mir_invoke.0.push(ir_stmt);

                    let var = if invoke_ret_type == expected_type {
                        var
                    } else {
                        let new_var = self.new_var_by_env_hir(expected_type);
                        let math_cvt = MIRMathCvt::new(true, MIRMathLRef::Var(new_var), MIRMathLRef::Var(var));
                        let stmt = MIRStmt::MathCvt(math_cvt);
                        mir_invoke.0.push(stmt);
                        new_var
                    };

                    (mir_invoke.0, MIRMathRight::new(MIRMathPrefix::None, MIRRight::VarRef(var)))
                };
            }
            IRRValue::Basic(prefix, basic) => {
                return match basic {
                    IRBasicExpr::Literal(lit) => {
                        let mut stmts = vec![];
                        if lit.get_type() != expected_type {
                            let old_var = self.new_var_by_env_hir(lit.get_type());
                            let assign = MIRVarCopyAssign::new(
                                true,
                                old_var,
                                MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::Lit(lit)),
                            );
                            let stmt = MIRStmt::MathVarCopyAssign(assign);
                            stmts.push(stmt);
                            let new_var = self.new_var_by_env_hir(expected_type);
                            let stmt = self.new_mir_cvt(true, new_var, old_var);
                            stmts.push(stmt);
                            (stmts, MIRMathRight::new(MIRMathPrefix::None, MIRRight::VarRef(new_var)))
                        } else {
                            (stmts, MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::Lit(lit)))
                        }
                    }
                    IRBasicExpr::Var(var) => {
                        let lvar = self.ref_pool.get_lvar_ref(var.get_name_sym_ref());
                        self.increase_active(lvar);
                        if var.get_type() != expected_type {
                            let new_var = self.new_var_by_env_hir(expected_type);
                            let cvt_stmt = self.new_mir_cvt(true, new_var, lvar);
                            (
                                vec![cvt_stmt],
                                MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::VarRef(new_var)),
                            )
                        } else {
                            (vec![], MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::VarRef(lvar)))
                        }
                    }
                    IRBasicExpr::ArrayEleRef(array_ele) => {
                        let array_type = array_ele.get_type();
                        let mut mir_array_ele = self.ir_name_array_ele_to_mir(array_ele);
                        if array_type != expected_type {
                            let new_var = self.new_var_by_env_hir(expected_type);
                            let old_var = self.new_var_by_env_hir(array_type);
                            let assign = MIRVarCopyAssign::new(
                                true,
                                old_var,
                                MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::ArrEleRef(mir_array_ele.1)),
                            );
                            let stmt = MIRStmt::MathVarCopyAssign(assign);
                            let cvt_stmt = self.new_mir_cvt(true, new_var, old_var);
                            mir_array_ele.0.push(stmt);
                            mir_array_ele.0.push(cvt_stmt);
                            (mir_array_ele.0, MIRMathRight::new(MIRMathPrefix::None, MIRRight::VarRef(new_var)))
                        } else {
                            (
                                mir_array_ele.0,
                                MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::ArrEleRef(mir_array_ele.1)),
                            )
                        }
                    }
                };
            }
            IRRValue::Math(suffix, a, b, c) => {
                let (mut a_stmt, mir_left) = self.ir_basic_expr_to_mir(a);
                let (mut c_stmt, mir_right) = self.ir_basic_expr_to_mir(c);
                let mir_oper = b.get_correspond_mir_type();
                let mut stmts = vec![];
                stmts.append(&mut a_stmt);
                stmts.append(&mut c_stmt);
                let left_kind = mir_left.get_type();
                let right_kind = mir_right.get_type();
                if left_kind != right_kind {
                    //  如果类型不相等，那么只能转成Float，然后再做出运算，最后再转向期望类型
                    let mir_left = if left_kind == MIRType::Float {
                        mir_left
                    } else {
                        let old_var = self.new_var_by_env_hir(IRBasicType::INT);
                        let assign = MIRVarCopyAssign::new(true, old_var, MIRMathRight::new(MIRMathPrefix::None, mir_left));
                        let stmt = MIRStmt::MathVarCopyAssign(assign);
                        stmts.push(stmt);
                        let new_var = self.new_var_by_env_hir(IRBasicType::FLOAT);
                        let stmt = self.new_mir_cvt(true, new_var, old_var);
                        stmts.push(stmt);
                        MIRRight::VarRef(new_var)
                    };

                    let mir_right = if right_kind == MIRType::Float {
                        mir_right
                    } else {
                        let old_var = self.new_var_by_env_hir(IRBasicType::INT);
                        let assign = MIRVarCopyAssign::new(true, old_var, MIRMathRight::new(MIRMathPrefix::None, mir_right));
                        let stmt = MIRStmt::MathVarCopyAssign(assign);
                        stmts.push(stmt);
                        let new_var = self.new_var_by_env_hir(IRBasicType::FLOAT);
                        let stmt = self.new_mir_cvt(true, new_var, old_var);
                        stmts.push(stmt);
                        MIRRight::VarRef(new_var)
                    };

                    let left_var = self.mir_right_into_lref(mir_left);
                    if left_var.0.is_some() {
                        stmts.push(left_var.0.unwrap())
                    }
                    let right_var = self.mir_right_into_lref(mir_right);
                    if right_var.0.is_some() {
                        stmts.push(right_var.0.unwrap())
                    }

                    let new_var = self.new_var_by_env_hir(IRBasicType::FLOAT);
                    let compute_assign = MIRVarComputeAssign::new(true, new_var, left_var.1, mir_oper, right_var.1);
                    let result = MIRStmt::MathVarComputeAssign(compute_assign);
                    stmts.push(result);

                    return if expected_type != IRBasicType::FLOAT {
                        let new_cvt_var = self.new_var_by_env_hir(expected_type);
                        let stmt = self.new_mir_cvt(true, new_cvt_var, new_var);
                        stmts.push(stmt);
                        (stmts, MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::VarRef(new_cvt_var)))
                    } else {
                        (stmts, MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::VarRef(new_var)))
                    };
                } else {
                    let result_var = self.new_var_by_env_mir(left_kind);
                    let is_process = if self.session.is_opt_enable(CompileOptions::HirLitExprPreComp) {
                        match &mir_left {
                            MIRRight::Lit(lit1) => {
                                match &mir_right {
                                    MIRRight::Lit(lit2) => {
                                        let result = match mir_oper {
                                            MIRMathOper::Add => lit1 + lit2,
                                            MIRMathOper::Sub => lit1 - lit2,
                                            MIRMathOper::Mul => lit1 * lit2,
                                            MIRMathOper::Div => lit1 / lit2,
                                            MIRMathOper::Rem => lit1 % lit2,
                                        };
                                        let copy_assign = MIRVarCopyAssign::new(
                                            true,
                                            result_var,
                                            MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(result)),
                                        );
                                        stmts.push(MIRStmt::MathVarCopyAssign(copy_assign));
                                        true
                                    }
                                    _ => {
                                        // 无法优化
                                        false
                                    }
                                }
                            }
                            _ => {
                                // 无法优化
                                false
                            }
                        }
                    } else {
                        // 无法优化
                        false
                    };
                    if !is_process {
                        let left_var = self.mir_right_into_lref(mir_left);
                        if left_var.0.is_some() {
                            stmts.push(left_var.0.unwrap());
                        }
                        let right_var = self.mir_right_into_lref(mir_right);
                        if right_var.0.is_some() {
                            stmts.push(right_var.0.unwrap());
                        }
                        let compute_assign = MIRVarComputeAssign::new(true, result_var, left_var.1, mir_oper, right_var.1);
                        let result = MIRStmt::MathVarComputeAssign(compute_assign);
                        stmts.push(result);
                    }
                    return if expected_type.get_correspond_mir_type() == right_kind {
                        (stmts, MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::VarRef(result_var)))
                    } else {
                        let new_var = self.new_var_by_env_hir(expected_type);
                        let stmt = self.new_mir_cvt(true, new_var, result_var);
                        stmts.push(stmt);
                        (stmts, MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::VarRef(new_var)))
                    };
                }
            }
            IRRValue::Bool(_) => {
                panic!("NEVER HERE")
            }
        }
    }

    fn ir_name_array_to_mir(&mut self, mut array: IRNamedArray) -> (Vec<MIRStmt>, MIRNamedArrayParam) {
        let actual_dims = array.take_dims();
        let mut stmts = vec![];
        let typed_array = self.ref_pool.get_larray_ref(&array.get_name().get_sym_ref());
        let typed_dims = typed_array.get_dims();
        return if actual_dims.len() == 0 {
            let a_var = self.new_var_by_env_hir(IRBasicType::INT);
            let copy_assign = MIRVarCopyAssign::new(
                true,
                a_var,
                MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(MIRLit::I32(0))),
            );
            let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
            stmts.push(stmt);
            (stmts, MIRNamedArrayParam::new(typed_array.copy_name(), a_var, typed_array.get_type()))
        } else {
            let (stmts, array_offset_var) = self.ir_compute_array_offset(actual_dims, typed_dims);
            let stmt = MIRNamedArrayParam::new(typed_array.copy_name(), array_offset_var, typed_array.get_type());
            (stmts, stmt)
        };
    }

    fn ir_name_array_ele_to_mir(&mut self, mut array_ele: IRNamedArrayEle) -> (Vec<MIRStmt>, MIRArrayEleRef) {
        let actual_dims = array_ele.take_dims();
        let typed_array = self.ref_pool.get_larray_ref(array_ele.get_name_sym_ref());
        let typed_dims = typed_array.get_dims();
        if typed_dims.len() > 1 {
            let (stmts, array_offset_var) = self.ir_compute_array_offset(actual_dims, typed_dims);
            let ele_ref = self.ref_pool.create_array_ele_ref(typed_array.copy_name(), typed_array.get_type(), array_offset_var);
            (stmts, ele_ref)
        } else {
            let mut stmts = vec![];
            let mut dims = actual_dims;
            let dim = dims.pop();

            match dim {
                None => {
                    // 这里仅仅是作为元素的转化，传递数组时候的计算不在这里完成
                    panic!("NEVER HERE")
                }
                Some(dim) => {
                    let (mut defs, right) = self.ir_rvalue_to_mir(dim, IRBasicType::INT); // 取出本维大小
                    stmts.append(&mut defs);
                    let var_opt = right.try_unwraped_to_var();
                    return match var_opt {
                        None => {
                            let a_var = self.new_var_by_env_hir(IRBasicType::INT);
                            let copy_assign = MIRVarCopyAssign::new(true, a_var, right);
                            let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
                            stmts.push(stmt);
                            let ele_ref = self.ref_pool.create_array_ele_ref(typed_array.copy_name(), typed_array.get_type(), a_var);
                            (stmts, ele_ref)
                        }
                        Some(var) => {
                            let ele_ref = self.ref_pool.create_array_ele_ref(typed_array.copy_name(), typed_array.get_type(), var);
                            (stmts, ele_ref)
                        }
                    };
                }
            }
        }
    }

    fn ir_compute_array_offset(&mut self, actual_dims: Vec<IRRValue>, typed_dims: &Vec<usize>) -> (Vec<MIRStmt>, MIRVarRef) {
        let mut typed_dims_unit = VecDeque::new();
        let mut index = typed_dims.len() - 1;
        let mut now_dim_len = 1;
        loop {
            typed_dims_unit.push_front(now_dim_len);
            if index != 0 {
                now_dim_len *= *typed_dims.get(index).unwrap(); // 安全
                index -= 1;
            } else {
                break;
            }
        }

        if self.session.is_opt_enable(CompileOptions::HirConstArrDimPreComp) {
            // opt 这里的有点缺陷，只能计算全部，不能计算一半
            let mut is_success = false;
            let mut now_dim_size :usize;
            let mut all_offset = IRNumber::I32(0);
            for (index, irrvalue) in actual_dims.iter().enumerate() {
                let result = irrvalue.try_into_ir_number();
                match result {
                    None => break,
                    Some(dim) => {
                        if index == typed_dims.len() - 1 {
                            all_offset = all_offset + dim;
                            is_success = true;
                        } else {
                            now_dim_size = typed_dims_unit.get(index).unwrap().clone();
                            let this_offset = IRNumber::I32(now_dim_size as i32) * dim;
                            all_offset = all_offset + this_offset;
                        }
                    }
                }
            }
            if is_success {
                let array_offset_var = self.new_var_by_env_hir(IRBasicType::INT);
                let def = MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(
                    true,
                    array_offset_var,
                    MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(all_offset)),
                ));
                return (vec![def], array_offset_var);
            }
        }

        let mut stmts: Vec<MIRStmt> = vec![];
        let mut array_offset_var = self.new_var_by_env_hir(IRBasicType::INT); //  特殊情况
        array_offset_var.active_type = Normal;
        let zero_math_right_0 = MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(IRNumber::I32(0)));
        let zero_math_right_1 = MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(IRNumber::I32(0)));
        let zero_math_right_2 = MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(IRNumber::I32(0)));
        let mut a_var = self.new_var_by_env_hir(IRBasicType::INT); // 特殊情况
        a_var.active_type = Normal;
        let mut c_var = self.new_var_by_env_hir(IRBasicType::INT); // 特殊情况
        c_var.active_type = Normal;
        let offset_init_assign = MIRVarCopyAssign::new(true, array_offset_var, zero_math_right_0);
        let a_init_assign = MIRVarCopyAssign::new(true, a_var, zero_math_right_1);
        let c_init_assign = MIRVarCopyAssign::new(true, c_var, zero_math_right_2);
        let offset_stmt = MIRStmt::MathVarCopyAssign(offset_init_assign);
        let a_stmt = MIRStmt::MathVarCopyAssign(a_init_assign);
        let c_stmt = MIRStmt::MathVarCopyAssign(c_init_assign);
        stmts.push(offset_stmt);
        stmts.push(a_stmt);
        stmts.push(c_stmt);
        let mut now_dim_size: usize;
        for (index, ir_rvalue) in actual_dims.into_iter().enumerate() {
            if index == typed_dims.len() - 1 {
                // 最后一个维度
                let (mut pre_stmts, right) = self.ir_rvalue_to_mir(ir_rvalue, IRBasicType::INT);
                let unwrap_result = right.try_unwraped_to_var();
                let part_a_var = match unwrap_result {
                    None => {
                        // 这种情况下 a_var 需要使用
                        let a_assign = MIRVarCopyAssign::new(false, a_var, right);
                        pre_stmts.push(MIRStmt::MathVarCopyAssign(a_assign));
                        a_var
                    }
                    Some(unwrap) => {
                        // 直接使用 var
                        unwrap
                    }
                };
                let assign = MIRVarComputeAssign::new(
                    false,
                    array_offset_var,
                    MIRMathLRef::Var(array_offset_var),
                    MIRMathOper::Add,
                    MIRMathLRef::Var(part_a_var),
                );
                pre_stmts.push(MIRStmt::MathVarComputeAssign(assign));
                stmts.append(&mut pre_stmts);
                break;
            } else {
                now_dim_size = typed_dims_unit.get(index).unwrap().clone();
                let (mut pre_stmts, right) = self.ir_rvalue_to_mir(ir_rvalue, IRBasicType::INT);
                let unwrap_result = right.try_unwraped_to_var();
                let part_a_var = match unwrap_result {
                    None => {
                        // 这种情况下 a_var 需要使用
                        let a_assign = MIRVarCopyAssign::new(false, a_var, right);
                        pre_stmts.push(MIRStmt::MathVarCopyAssign(a_assign));
                        a_var
                    }
                    Some(unwrap) => {
                        // 直接使用 var
                        unwrap
                    }
                };
                let c_assign = MIRVarCopyAssign::new(
                    false,
                    c_var,
                    MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(MIRLit::I32(now_dim_size as i32))),
                );
                pre_stmts.push(MIRStmt::MathVarCopyAssign(c_assign));

                let a_assign = MIRVarComputeAssign::new(
                    false,
                    a_var,
                    MIRMathLRef::Var(part_a_var),
                    MIRMathOper::Mul,
                    MIRMathLRef::Var(c_var),
                );
                pre_stmts.push(MIRStmt::MathVarComputeAssign(a_assign));
                let offset_assign = MIRVarComputeAssign::new(
                    false,
                    array_offset_var,
                    MIRMathLRef::Var(array_offset_var),
                    MIRMathOper::Add,
                    MIRMathLRef::Var(a_var),
                );
                pre_stmts.push(MIRStmt::MathVarComputeAssign(offset_assign));
                stmts.append(&mut pre_stmts);
            }
        }
        (stmts, array_offset_var)
    }

    fn ir_array_def_to_mir(&mut self, mut array_def: IRArrayDefine) -> (Vec<MIRStmt>, MIRArrayDef) {
        let sym = array_def.get_name().get_sym_ref();
        let ele_num = array_def.compute_ele_num();
        let array_ref = self.ref_pool.add_array(sym, array_def.take_dims(), ele_num, array_def.get_type());
        let init = array_def.take_init();
        let mut inits = vec![];
        let mut all_stmts = vec![];
        for (pos, expr) in init.init {
            let (mut stmts, right) = self.ir_rvalue_to_mir(expr, array_def.get_type());
            let (stmt, right) = self.mir_math_right_into_right(right);
            if stmt.is_some() {
                stmts.push(stmt.unwrap());
            }
            all_stmts.append(&mut stmts);
            inits.push((pos, right))
        }
        (all_stmts, MIRArrayDef::new(array_def.is_const(), array_ref, inits))
    }

    fn ir_basic_expr_to_mir(&mut self, basic_expr: IRBasicExpr) -> (Vec<MIRStmt>, MIRRight) {
        return match basic_expr {
            IRBasicExpr::Literal(lit) => (vec![], MIRRight::Lit(lit)),
            IRBasicExpr::Var(var) => {
                let var = self.ref_pool.get_lvar_ref(var.get_name_sym_ref());
                self.increase_active(var);
                (vec![], MIRRight::VarRef(var))
            }
            IRBasicExpr::ArrayEleRef(array_ele) => {
                let ret = self.ir_name_array_ele_to_mir(array_ele);
                (ret.0, MIRRight::ArrEleRef(ret.1))
            }
        };
    }

    fn ir_prefix_to_mir(&self, suffix: IRExprPrefix) -> MIRMathPrefix {
        return match suffix.get_first() {
            IRFirstSuffix::None => match suffix.get_second() {
                IRSecondSuffix::None => {
                    MIRMathPrefix::None
                }
                IRSecondSuffix::EvenBang => {
                    MIRMathPrefix::BangBang
                }
                IRSecondSuffix::OddBang => {
                    MIRMathPrefix::Bang
                }
            },
            IRFirstSuffix::Negative => match suffix.get_second() {
                IRSecondSuffix::None => {
                    MIRMathPrefix::Neg
                }
                IRSecondSuffix::EvenBang => {
                    MIRMathPrefix::NegBangBang
                }
                IRSecondSuffix::OddBang => {
                    MIRMathPrefix::NegBang
                }
            },
        }
    }

    fn ir_external_fun_to_mir(&mut self, external_fun: IRExternalFun) {
        self.ref_pool.add_fn(external_fun.name.get_sym_ref());
    }

    fn ir_while_to_mir(&mut self, while_loop: IRWhile, start_label: MIRLabel, next_block_label: MIRLabel) -> VecDeque<MIRBlock> {
        let before_active_state = self.current_actives.clone(); // save the before active state
        let mut actives = VecDeque::new();
        for active in before_active_state {
            actives.push_back(active.0.copy_name());
        }
        self.current_actives.clear(); // 清理活跃状态
        self.env_stack.push_back(VariableEnv::InWhile);
        let cond = while_loop.condition;
        let mut blocks = VecDeque::new();
        blocks.push_back(
            MIRBlock::new(MIRName::special())
                .apply_once(|mut it| {
                    it.add_stmt(MIRStmt::Sync(actives));
                    it
                }));
        let loop_cond = start_label;
        let enter_loop = self.new_mir_label_or_name(".L_enter_while_");
        let exit_loop = next_block_label;
        self.env_stack.push_back(VariableEnv::InWhileCond); // 环境进入
        let cond_blocks = self.ir_bool_expr_to_mir(cond, loop_cond, enter_loop, exit_loop);
        for cond_block in cond_blocks {
            blocks.push_back(cond_block)
        }
        let result_env = self.env_stack.pop_back(); // 环境退出
        match result_env {
            None => {
                panic!("变量环境异常")
            }
            Some(env) => {
                if env != VariableEnv::InWhileCond {
                    panic!("变量环境异常 {:?}", env);
                }
            }
        }
        self.now_break_tag.push_back(exit_loop);
        self.now_continue_tag.push_back(loop_cond);

        let mut stmts = while_loop.statements.into_iter();
        let mut label = enter_loop;
        loop {
            let (stop_stmt, after_stmts, mut block) = self.ir_normal_stmt_to_block(label, stmts);
            match stop_stmt {
                None => {
                    block.add_stmt(MIRStmt::Jump(MIRJump::new(loop_cond)));
                    // block.add_stmt(MIRStmt::Label(exit_loop)); // while 的假出口
                    blocks.push_back(block);
                    break;
                }
                Some(stmt) => {
                    match stmt {
                        IRStatement::Continue => {
                            let now_continue_tag = self.now_continue_tag.back();
                            if now_continue_tag.is_some() {
                                let now_continue = now_continue_tag.unwrap();
                                block.add_stmt(MIRStmt::Jump(MIRJump::new(now_continue.clone())));
                                blocks.push_back(block);
                            } else {
                                panic!("NEVER HERE")
                            }
                            label = self.new_mir_label_or_name(".L_in_while_block_");
                            stmts = after_stmts;
                        }
                        IRStatement::Break => {
                            let now_break_tag = self.now_break_tag.back();
                            if now_break_tag.is_some() {
                                let now_break = now_break_tag.unwrap();
                                block.add_stmt(MIRStmt::Jump(MIRJump::new(now_break.clone())));
                                blocks.push_back(block);
                            } else {
                                panic!("NEVER HERE")
                            }
                            label = self.new_mir_label_or_name(".L_in_while_block_");
                            stmts = after_stmts;
                        }
                        // todo miss return stmt handle
                        // 不在处理范围之内
                        _ => {
                            let enter_label = if blocks.len() == 0 {
                                // while语句的第一句就是特殊语句，就得提供enter_label
                                Some(block.get_name().clone())
                            } else {
                                blocks.push_back(block);
                                None
                            };
                            let next_label = self.new_mir_label_or_name(".L_in_while_block_");
                            let mut new_blocks = self.ir_handle_branches_block_while_to_mir(stmt, enter_label, next_label);
                            blocks.append(&mut new_blocks);
                            stmts = after_stmts;
                            label = next_label;
                        }
                    }
                }
            }
        }
        let while_active_state = mem::replace(&mut self.current_actives, HashMap::new());
        let mut actives = VecDeque::new();
        for active in while_active_state {
            actives.push_back(active.0.copy_name());
        }
        blocks.push_back(
            MIRBlock::new(MIRName::special())
                .apply_once(|mut it| {
                    it.add_stmt(MIRStmt::Sync(actives));
                    it
                }));
        self.now_break_tag.pop_back();
        self.now_continue_tag.pop_back();
        let result_env = self.env_stack.pop_back();
        match result_env {
            None => {
                panic!("变量环境异常")
            }
            Some(env) => {
                if env != VariableEnv::InWhile {
                    panic!("变量环境异常 {:?}", env);
                }
            }
        }
        blocks
    }

    fn ir_branches_to_mir(&mut self, branches: IRBranches, enter_label: MIRLabel, exit_label: MIRLabel) -> VecDeque<MIRBlock> {
        let mut blocks = VecDeque::new();
        let mut is_first_enter_context_block = true;

        let mut else_ifs = branches.elif;
        let mut else_branch = branches.last_else;
        let is_else_exists = else_branch.is_some();
        let mut now_elif_branch = 0;
        let all_elif_branch = else_ifs.len();

        let enter_cond_label = enter_label;
        let all_exit = exit_label;
        let mut true_exit = self.new_mir_label_or_name(".L_true_exit_");

        // 如果没有else分支并且elif分支总数等于0，那么这里的假分支入口是完全出口
        let mut next_enter_port_or_false_exit = if !is_else_exists && all_elif_branch == 0 {
            all_exit
        } else {
            self.new_mir_label_or_name(".L_false_exit")
        };

        let cond = self.ir_bool_expr_to_mir(branches.first_if.condition, enter_cond_label, true_exit, next_enter_port_or_false_exit);
        for block in cond {
            blocks.push_back(block);
        }

        let mut stmts = branches.first_if.statements.into_iter();

        let mut is_now_branch_first_deal = true;
        let mut block_label = true_exit;

        loop {
            let (last_stmt, after_stmts, mut block) = self.ir_normal_stmt_to_block(block_label, stmts);
            if is_now_branch_first_deal {
                block.set_name(true_exit);
                is_now_branch_first_deal = false;
            }
            match last_stmt {
                None => {
                    // 当前分支的语句已经处理结束
                    let next_branch = else_ifs.pop_front();
                    match next_branch {
                        None => {
                            // 所有else-if 分支都已经结束，翻译else分支加入完全出口
                            match else_branch.take() {
                                None => {
                                    // block.add_stmt(MIRStmt::Label(all_exit));
                                    // 最后一个假出口默认替换为完全出口
                                    // if !is_else_exists {
                                    //     // 如果没有else 分支 ，还要处理最后一个假出口
                                    //     block.add_stmt(MIRStmt::Label(next_enter_port_or_false_exit));
                                    // }
                                    blocks.push_back(block);
                                    break;
                                }
                                Some(else_bran) => {
                                    // 最后一个if分支跳转到完全出口
                                    block.add_stmt(MIRStmt::Jump(MIRJump::new(all_exit)));
                                    blocks.push_back(block);
                                    stmts = else_bran.into_iter();

                                    true_exit = next_enter_port_or_false_exit; // 对else 分支的入口重命名
                                    is_now_branch_first_deal = true;
                                    // block_label = self.new_mir_label_or_name(".L_else_block_");  因为是第一次处理当分支，所以不用管block_label
                                    is_first_enter_context_block = true;
                                }
                            }
                        }
                        Some(branch) => {
                            // 跳转到完全出口
                            block.add_stmt(MIRStmt::Jump(MIRJump::new(all_exit)));
                            blocks.push_back(block);
                            // 后面还有分支需要处理
                            let new_false_exit = if now_elif_branch == all_elif_branch - 1 && !is_else_exists {
                                //如果是最后一个elif分支,并且else分支不存在,那么假出口就是完全出口
                                all_exit
                            } else {
                                now_elif_branch += 1;
                                self.new_mir_label_or_name(".L_false_exit_")
                            };

                            let new_true_exit = self.new_mir_label_or_name(".L_true_exit_");
                            let cond = self.ir_bool_expr_to_mir(branch.condition, next_enter_port_or_false_exit, new_true_exit, new_false_exit);
                            for block in cond {
                                blocks.push_back(block);
                            }
                            next_enter_port_or_false_exit = new_false_exit;
                            true_exit = new_true_exit;
                            stmts = branch.statements.into_iter();
                            is_now_branch_first_deal = true;
                            // block_label = self.new_mir_label_or_name(".L_after_cond_block_"); 因为是第一次处理当分支，所以不用管block_label
                            is_first_enter_context_block = true;
                        }
                    }
                }
                Some(stmt) => {
                    // 当前分支遇到了特殊语句
                    let next_block_label = self.new_mir_label_or_name(".L_in_block_after_");

                    let is_use_first_empty_block = if is_first_enter_context_block && block.size() == 0 {
                        true
                    } else {
                        false
                    };

                    let (block, mut new_blocks) = self.ir_handle_special_single_stmt_to_mir(block, stmt, is_use_first_empty_block, next_block_label);
                    blocks.push_back(block);
                    blocks.append(&mut new_blocks);
                    stmts = after_stmts;
                    block_label = next_block_label;
                    if is_first_enter_context_block {
                        is_first_enter_context_block = false;
                    }
                }
            }
        }
        blocks
    }

    /// MIR 保证比较的时候类型是一致的
    fn ir_bool_expr_to_mir(&mut self, cond: IRBoolExpr, first_block_label: MIRLabel, true_exit: MIRLabel, false_exit: MIRLabel) -> Vec<MIRBlock> {
        return match cond {
            IRBoolExpr::Bool(bool_defs, ir_bool) => {
                let mut current_cond_block = MIRBlock::new(first_block_label);
                for single_def in bool_defs {
                    match single_def {
                        IRBoolOrVarDefine::VarDef(var) => {
                            let codes = self.ir_var_def_to_mir(var);
                            current_cond_block.append(codes);
                        }
                        IRBoolOrVarDefine::BoolDef(bool) => {
                            let codes = self.ir_bool_def_to_mir(bool);
                            current_cond_block.append(codes);
                        }
                    }
                }

                match ir_bool {
                    IRBool::Basic(basic) => {
                        let (stmts, _, right) = self.ir_rel_source_to_mir(basic);
                        for stmt in stmts {
                            current_cond_block.add_stmt(stmt);
                        }
                        let cond = MIRBool::BoolRef(right);
                        let jump = MIRCondJump::new(cond, false_exit, Some(true_exit));
                        current_cond_block.add_stmt(MIRStmt::CondJump(jump));
                    }
                    IRBool::Rel(a, oper, c) => {
                        let (stmts1, source_type1, right1) = self.ir_rel_source_to_mir(a);
                        current_cond_block.append(stmts1);
                        let (stmts2, source_type2, right2) = self.ir_rel_source_to_mir(c);
                        current_cond_block.append(stmts2);
                        if source_type1 != MIRType::Float && source_type2 != MIRType::Float {
                            let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), right2);
                            let cmp = MIRBool::Cmp(cmp_expr);
                            current_cond_block.add_stmt(MIRStmt::CondJump(MIRCondJump::new(cmp, false_exit, Some(true_exit))))
                        } else {
                            if source_type1 == MIRType::Float && source_type2 != MIRType::Float {
                                let new_source2 = self.new_var_by_env_mir(MIRType::Float);
                                let (stmt, lref) = self.mir_cmp_right_into_var_ref(right2);
                                if stmt.is_some() {
                                    current_cond_block.add_stmt(stmt.unwrap());
                                }
                                let stmt = self.new_mir_cvt(true, new_source2, lref);
                                current_cond_block.add_stmt(stmt);
                                let cmp_expr =
                                    MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), MIRCmpRight::MathL(MIRMathLRef::Var(new_source2)));
                                let cmp = MIRBool::Cmp(cmp_expr);
                                current_cond_block.add_stmt(MIRStmt::CondJump(MIRCondJump::new(cmp, false_exit, Some(true_exit))))
                            } else if source_type1 != MIRType::Float && source_type2 == MIRType::Float {
                                // source_type2 -> MIRType::FLOAT
                                let new_source1 = self.new_var_by_env_mir(MIRType::Float);
                                let (stmt, lref) = self.mir_cmp_right_into_var_ref(right1);
                                if stmt.is_some() {
                                    current_cond_block.add_stmt(stmt.unwrap());
                                }
                                let stmt = self.new_mir_cvt(true, new_source1, lref);
                                current_cond_block.add_stmt(stmt);
                                let cmp_expr =
                                    MIRCmpExpr::new(MIRCmpRight::MathL(MIRMathLRef::Var(new_source1)), oper.get_correspond_mir_type(), right2);
                                let cmp = MIRBool::Cmp(cmp_expr);
                                current_cond_block.add_stmt(MIRStmt::CondJump(MIRCondJump::new(cmp, false_exit, Some(true_exit))))
                            } else {
                                let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), right2);
                                let cmp = MIRBool::Cmp(cmp_expr);
                                current_cond_block.add_stmt(MIRStmt::CondJump(MIRCondJump::new(cmp, false_exit, Some(true_exit))))
                            }
                        }
                    }
                }
                vec![current_cond_block]
            }
            IRBoolExpr::And(ands) => {
                let mut all_blocks = vec![];
                let mut last_true_exit = first_block_label;
                let mut next_true_exit = self.new_mir_label_or_name(".L_cond_");
                let mut index = 0;
                let last_index = ands.len() - 1;
                for and in ands {
                    let mut blocks = self.ir_bool_expr_to_mir(and, last_true_exit, next_true_exit, false_exit);
                    all_blocks.append(&mut blocks);
                    last_true_exit = next_true_exit;
                    if index == last_index - 1 {
                        next_true_exit = true_exit; // 最后一块跳出到真出口
                    } else {
                        next_true_exit = self.new_mir_label_or_name(".L_cond_");
                    }
                    index += 1;
                }
                all_blocks
            }
            IRBoolExpr::Or(ors) => {
                let mut all_blocks = vec![];
                let mut last_false_exit = first_block_label;
                let mut next_false_exit = self.new_mir_label_or_name(".L_cond_");
                let mut index = 0;
                let last_index = ors.len() - 1;
                for and in ors {
                    let mut blocks = self.ir_bool_expr_to_mir(and, last_false_exit, true_exit, next_false_exit);
                    all_blocks.append(&mut blocks);
                    last_false_exit = next_false_exit;
                    if index == last_index - 1 {
                        next_false_exit = false_exit; // 最后一块跳出到假出口
                    } else {
                        next_false_exit = self.new_mir_label_or_name(".L_cond_");
                    }
                    index += 1;
                }
                all_blocks
            }
        };
    }

    /// 处理分支或者循环或者块 ，产生新的基本块
    fn ir_handle_branches_block_while_to_mir(
        &mut self,
        stmt: IRStatement,
        enter_label: Option<MIRLabel>,
        next_block_label: MIRLabel,
    ) -> VecDeque<MIRBlock> {
        match stmt {
            IRStatement::Branches(branches) => {
                let label = match enter_label {
                    None => self.new_mir_label_or_name(".L_enter_cond_"),
                    Some(label) => label,
                };
                return self.ir_branches_to_mir(branches, label, next_block_label);
            }
            IRStatement::Block(block) => {
                let label = match enter_label {
                    None => self.new_mir_label_or_name(".L_enter_block_"),
                    Some(label) => label,
                };
                return self.ir_block_to_mir(block, label);
            }
            IRStatement::While(while_loop) => {
                let label = match enter_label {
                    None => self.new_mir_label_or_name(".L_enter_while_"),
                    Some(label) => label,
                };
                return self.ir_while_to_mir(while_loop, label, next_block_label);
            }
            _ => {
                println!("{:?}", stmt);
                panic!("NEVER HERE")
            }
        }
    }

    fn ir_handle_special_single_stmt_to_mir(
        &mut self,
        mut current_block: MIRBlock,
        stmt: IRStatement,
        // 当current_block为空并且该参数为true的时候，则翻译出来的第一块使用current_block的label
        is_use_first_empty: bool,
        next_block_label: MIRLabel,
    ) -> (MIRBlock, VecDeque<MIRBlock>) {
        match stmt {
            IRStatement::Continue => {
                let now_continue_tag = self.now_continue_tag.back();
                if now_continue_tag.is_some() {
                    let now_continue = now_continue_tag.unwrap();
                    current_block.add_stmt(MIRStmt::Jump(MIRJump::new(now_continue.clone())));
                } else {
                    panic!("NEVER HERE")
                }
                return (current_block, VecDeque::new());
            }
            IRStatement::Break => {
                let now_break_tag = self.now_break_tag.back();
                if now_break_tag.is_some() {
                    let now_break = now_break_tag.unwrap();
                    current_block.add_stmt(MIRStmt::Jump(MIRJump::new(now_break.clone())));
                } else {
                    panic!("NEVER HERE")
                }
                return (current_block, VecDeque::new());
            }
            IRStatement::Return(ret) => {
                match ret {
                    None => current_block.add_stmt(MIRStmt::Return(MIRReturn::just_return())),
                    Some(expr) => {
                        let (stmts, math_right) = self.ir_rvalue_to_mir(expr, self.now_ret_type);
                        current_block.append(stmts);
                        let (stmt, lref) = self.mir_math_right_into_lref(math_right);
                        match stmt {
                            None => {
                                allow_nothing!();
                            }
                            Some(stmt) => current_block.add_stmt(stmt),
                        }
                        current_block.add_stmt(MIRStmt::Return(MIRReturn::var_return(lref)));
                    }
                }
                return (current_block, VecDeque::new());
            }
            _ => {
                let enter_block_label = if is_use_first_empty {
                    if current_block.size() != 0 {
                        panic!("NEVER HERE")
                    }
                    Some(current_block.get_name().clone())
                } else {
                    None
                };
                let mut new_blocks = self.ir_handle_branches_block_while_to_mir(stmt, enter_block_label, next_block_label);
                let first_block = if is_use_first_empty {
                    new_blocks.pop_front().unwrap()
                } else {
                    current_block
                };
                return (first_block, new_blocks);
            }
        }
    }

    fn ir_block_to_mir(&mut self, block: IRStatements, enter_label: MIRLabel) -> VecDeque<MIRBlock> {
        let mut stmts = block.into_iter();
        let mut blocks = VecDeque::new();
        let mut label = enter_label;
        loop {
            let (stop_stmt, after_stmts, block) = self.ir_normal_stmt_to_block(label, stmts);
            match stop_stmt {
                None => {
                    blocks.push_back(block);
                    break;
                }
                Some(stmt) => {
                    let next_block_label = self.new_mir_label_or_name(".L_in_block_");
                    let is_use_first_empty = if blocks.len() == 0 && block.size() == 0 { true } else { false };
                    let (block, mut new_blocks) = self.ir_handle_special_single_stmt_to_mir(block, stmt, is_use_first_empty, next_block_label);
                    blocks.push_back(block);
                    blocks.append(&mut new_blocks);
                    stmts = after_stmts;
                    label = next_block_label;
                }
            }
        }
        return blocks;
    }

    /// 只处理基本语句，涉及到分支，跳转，块，等能产生新块的语句，则停止并返回停止语句和迭代器
    fn ir_normal_stmt_to_block(
        &mut self,
        block_label: MIRLabel,
        mut stmts: IntoIter<IRStatement>,
    ) -> (Option<IRStatement>, IntoIter<IRStatement>, MIRBlock) {
        let mut current_block = MIRBlock::new(block_label);
        let mut last_stmt = None;
        loop {
            let stmt = stmts.next();
            if stmt.is_none() {
                break;
            } else {
                let stmt = stmt.unwrap();
                match stmt {
                    IRStatement::VarDef(def) => {
                        let stmts = self.ir_var_def_to_mir(def);
                        current_block.append(stmts);
                    }
                    IRStatement::ArrDef(array_def) => {
                        let (stmts, def) = self.ir_array_def_to_mir(array_def);
                        current_block.append(stmts);
                        current_block.add_stmt(MIRStmt::ArrayDef(def))
                    }
                    IRStatement::Assign(assign) => {
                        let stmts = self.ir_assign_mir(assign);
                        current_block.append(stmts);
                    }
                    IRStatement::EmptyExpr(_) => {
                        // todo empty drop
                    }
                    IRStatement::FunInvoke(invoke) => {
                        let ret = self.ir_invoke_to_mir(invoke);
                        current_block.append(ret.0);
                        current_block.add_stmt(MIRStmt::Invoke(ret.1))
                    }
                    IRStatement::BoolAssign(_) => {
                        panic!("NEVER HERE")
                    }
                    IRStatement::BoolDef(_) => {
                        panic!("NEVER HERE")
                    }

                    // if , block , while , continue , break , return
                    _ => {
                        last_stmt = Some(stmt);
                        break;
                    }
                }
            }
        }

        return if last_stmt.is_none() {
            (None, stmts, current_block)
        } else {
            (last_stmt, stmts, current_block)
        };
    }

    fn new_var_by_env_hir(&mut self, basic_type: IRBasicType) -> MIRVarRef {
        let new_sym = self.symbol_table.add_symbol(AnySymbol::new_rand_underscore("te_mir"));
        let now_var_type = match self.get_now_env() {
            VariableEnv::Unknown => Temp,
            VariableEnv::InFun => Temp,
            VariableEnv::InWhile => Temp,
            VariableEnv::InWhileCond => Temp,
        };
        let c_var = self.ref_pool.add_var_by_hir_type(new_sym, basic_type, now_var_type);
        c_var
    }

    fn new_var_by_env_mir(&mut self, basic_type: MIRType) -> MIRVarRef {
        let new_sym = self.symbol_table.add_symbol(AnySymbol::new_rand_underscore("te_mir"));
        let now_var_type = match self.get_now_env() {
            VariableEnv::Unknown => Temp,
            VariableEnv::InFun => Temp,
            VariableEnv::InWhile => Temp,
            VariableEnv::InWhileCond => Temp,
        };
        let c_var = self.ref_pool.add_var_by_mir_type(new_sym, basic_type, now_var_type);
        c_var
    }

    fn new_mir_cvt(&self, is_def: bool, new_var: MIRVarRef, old_var: MIRVarRef) -> MIRStmt {
        let cvt = MIRMathCvt::new(is_def, MIRMathLRef::Var(new_var), MIRMathLRef::Var(old_var));
        let stmt = MIRStmt::MathCvt(cvt);
        stmt
    }

    fn new_mir_label_or_name(&mut self, prefix: &str) -> MIRLabel {
        let label_symbol = AnySymbol::new_rand(prefix);
        let sym_ref = self.symbol_table.add_symbol(label_symbol);
        let label = self.ref_pool.add_label(sym_ref);
        label
    }

    fn mir_right_into_lref(&mut self, mir_right: MIRRight) -> (Option<MIRStmt>, MIRMathLRef) {
        let right_type = mir_right.get_type();
        match mir_right {
            MIRRight::Lit(_) => {
                let var = self.new_var_by_env_mir(right_type);
                let copy_assign = MIRVarCopyAssign::new(true, var, MIRMathRight::new(MIRMathPrefix::None, mir_right));
                let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
                (Some(stmt), MIRMathLRef::Var(var))
            }
            MIRRight::VarRef(var) => (None, MIRMathLRef::Var(var)),
            MIRRight::ArrEleRef(arr) => (None, MIRMathLRef::ArrayEle(arr)),
            MIRRight::CmpRef(_) => {
                let var = self.new_var_by_env_mir(MIRType::Bool);
                let copy_assign = MIRVarCopyAssign::new(true, var, MIRMathRight::new(MIRMathPrefix::None, mir_right));
                let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
                (Some(stmt), MIRMathLRef::Var(var))
            }
            MIRRight::Invoke(invoke) => {
                let var = self.new_var_by_env_mir(invoke.get_ret_type());
                let mir_right = MIRRight::Invoke(invoke);
                let mir_math_right = MIRMathRight::new(MIRMathPrefix::None, mir_right);
                let copy_assign = MIRVarCopyAssign::new(true, var, mir_math_right);
                let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
                (Some(stmt), MIRMathLRef::Var(var))
            }
        }
    }

    fn mir_cmp_right_into_var_ref(&mut self, mir_right: MIRCmpRight) -> (Option<MIRStmt>, MIRVarRef) {
        return match mir_right {
            MIRCmpRight::MathL(mathl) => match mathl {
                MIRMathLRef::Var(var) => (None, var),
                MIRMathLRef::ArrayEle(ele) => {
                    let new_var = self.new_var_by_env_mir(ele.get_type());
                    (
                        Some(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(
                            true,
                            new_var,
                            MIRMathRight::new(MIRMathPrefix::None, MIRRight::ArrEleRef(ele)),
                        ))),
                        new_var,
                    )
                }
            },
            MIRCmpRight::Lit(lit) => {
                let new_var = self.new_var_by_env_mir(lit.get_type().get_correspond_mir_type());
                (
                    Some(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(
                        true,
                        new_var,
                        MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(lit)),
                    ))),
                    new_var,
                )
            }
            MIRCmpRight::CmpL(cmpl) => (None, cmpl.get_var()),
        };
    }

    fn mir_math_right_into_right(&mut self, right: MIRMathRight) -> (Option<MIRStmt>, MIRRight) {
        match right.get_prefix() {
            MIRMathPrefix::None => {
                return (None, right.math);
            }
            _ => {}
        }
        //
        let new_sym = self.symbol_table.add_symbol(AnySymbol::new_rand_underscore("te_mir"));
        let c_var = self.ref_pool.add_var_by_mir_type(new_sym, right.get_right().get_type(), Temp);
        let stmt = MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(true, c_var, right));
        return (Some(stmt), MIRRight::VarRef(c_var));
    }

    fn mir_math_right_into_lref(&mut self, mir_math_right: MIRMathRight) -> (Option<MIRStmt>, MIRMathLRef) {
        match mir_math_right.get_prefix() {
            MIRMathPrefix::None => {
                let ret = self.mir_right_into_lref(mir_math_right.math);
                return match ret.0 {
                    None => (None, ret.1),
                    Some(stmt) => (Some(stmt), ret.1),
                };
            }
            _ => {}
        }
        //
        let new_sym = self.symbol_table.add_symbol(AnySymbol::new_rand_underscore("te_mir"));
        let c_var = self.ref_pool.add_var_by_mir_type(new_sym, mir_math_right.get_right().get_type(), Temp);
        let stmt = MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(true, c_var, mir_math_right));
        return (Some(stmt), MIRMathLRef::Var(c_var));
    }

    fn get_now_env(&self) -> VariableEnv {
        self.env_stack.back().unwrap().clone()
    }
    // 客观事实是 只要做出来就能达成目标，希望是逐渐出来的，不是一蹴而就的
    // 从长远角度考虑，难道以后你不学习其他编译器技术吗？
    fn increase_active(&mut self, var: MIRVarRef) {
        if var.active_type == Temp {
            return;
        }

    }
}

impl IRMathOpe {
    pub fn get_correspond_mir_type(&self) -> MIRMathOper {
        match self {
            IRMathOpe::Add => MIRMathOper::Add,
            IRMathOpe::Sub => MIRMathOper::Sub,
            IRMathOpe::Mul => MIRMathOper::Mul,
            IRMathOpe::Div => MIRMathOper::Div,
            IRMathOpe::Rem => MIRMathOper::Rem,
        }
    }
}

impl IRBoolOpe {
    pub fn get_correspond_mir_type(&self) -> MIRBoolOper {
        match self {
            IRBoolOpe::Less => MIRBoolOper::Less,
            IRBoolOpe::Greater => MIRBoolOper::Greater,
            IRBoolOpe::GreaterOrEqual => MIRBoolOper::GreaterEqual,
            IRBoolOpe::LessOrEqual => MIRBoolOper::LessEqual,
            IRBoolOpe::Equal => MIRBoolOper::Equal,
            IRBoolOpe::NotEqual => MIRBoolOper::NotEqual,
            IRBoolOpe::And => MIRBoolOper::And,
            IRBoolOpe::Or => MIRBoolOper::Or,
        }
    }
}

impl IRBasicType {
    pub fn get_correspond_mir_type(&self) -> MIRType {
        match self {
            IRBasicType::INT => MIRType::Int,
            IRBasicType::FLOAT => MIRType::Float,
            IRBasicType::VOID => MIRType::Void,
        }
    }
}
