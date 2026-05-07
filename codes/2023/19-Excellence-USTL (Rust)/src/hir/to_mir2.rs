use crate::allow_nothing;
use crate::ast::table::{AnySymbol, SymbolTable};
use crate::compiler::CompileOptions;
use crate::hir::hir::{
    HirModule, HirUnit, IRArrayDefine, IRAssign, IRBasicExpr, IRBasicType, IRBool, IRBoolExpr, IRBoolOrVarDefine, IRBoolVarDefine, IRBranches,
    IRExpr, IRExprPrefix, IRExternalFun, IRFirstSuffix, IRFunDefine, IRIf, IRInvoke, IRLValue, IRNamedArray, IRNamedArrayEle, IRNamedParam, IRNumber,
    IRRValue, IRRelSource, IRSecondSuffix, IRStatement, IRStatements, IRTypedParam, IRVarDefine, IRVarOrArrayDefine, IRWhile,
};
use crate::mir::cfg::{CFGBasicBlock, CFGBasicBlockRef, CFGFun, CFG};
use crate::mir::MIRVarActiveType::{Normal, Temp};
use crate::mir::{
    MIRArrayDef, MIRArrayEleCopyAssign, MIRArrayEleRef, MIRArrayInit, MIRBool, MIRBoolCmpAssign, MIRBoolLRef, MIRCmpExpr, MIRCmpRight, MIRCondJump,
    MIRGlDef, MIRInvoke, MIRJump, MIRLabel, MIRLit, MIRMathCvt, MIRMathLRef, MIRMathOper, MIRMathPrefix, MIRMathRight, MIRName, MIRNamedArrayParam,
    MIRNamedParam, MIRRefPool, MIRReturn, MIRRight, MIRStmt, MIRType, MIRUnit, MIRVarActiveType, MIRVarComputeAssign, MIRVarCopyAssign, MIRVarRef,
};
use crate::optimizer::OptLevel;
use crate::session::CompileSession;
use std::collections::VecDeque;

pub struct MIRTranSlate2<'a> {
    ref_pool: &'a mut MIRRefPool,
    symbol_table: &'a mut SymbolTable,
    session: &'a mut CompileSession,

    continue_tags: VecDeque<MIRLabel>,
    break_tags: VecDeque<MIRLabel>,
    current_ret_type: IRBasicType,
}

impl<'a> MIRTranSlate2<'a> {
    pub fn parse_hir(hir_module: HirModule, ref_pool: &'a mut MIRRefPool, symbol_table: &mut SymbolTable, session: &mut CompileSession) -> CFG {
        let mut translater = MIRTranSlate2 {
            ref_pool,
            symbol_table,
            session,
            continue_tags: Default::default(),
            break_tags: Default::default(),
            current_ret_type: IRBasicType::VOID,
        };
        translater.ir_units_to_mir(hir_module)
    }

    pub fn ir_units_to_mir(&mut self, hir_module: HirModule) -> CFG {
        let mut cfg = CFG::new();
        for unit in hir_module.units {
            match unit {
                HirUnit::GLVarDefine(mut var_def) => {
                    let is_const = var_def.is_const();
                    if is_const {
                        let name = var_def.get_name();
                        if name.get_all_ref_num() == 0 {
                            continue;
                        }
                    }

                    let sym_ref = var_def.get_name().get_sym_ref();
                    let var_ref = self.ref_pool.add_var_by_hir_type(sym_ref, var_def.get_type(), MIRVarActiveType::Global);
                    let def = MIRGlDef::Var(var_def.is_const(), var_ref, var_def.take_init().unwrap().into_ir_number());
                    cfg.add_gldef(def);
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
                    cfg.add_gldef(MIRGlDef::Array(array_def.is_const(), mir_array_ref, mir_init));
                }
                HirUnit::GlDefines(defs) => {
                    for single in defs {
                        match single {
                            IRVarOrArrayDefine::Var(mut var_def) => {
                                let is_const = var_def.is_const();
                                if is_const {
                                    let name = var_def.get_name();
                                    if name.get_all_ref_num() == 0 {
                                        continue;
                                    }
                                }
                                let sym_ref = var_def.get_name().get_sym_ref();
                                let var_ref = self.ref_pool.add_var_by_hir_type(sym_ref, var_def.get_type(), MIRVarActiveType::Global);
                                let def = MIRGlDef::Var(var_def.is_const(), var_ref, var_def.take_init().unwrap().into_ir_number());
                                cfg.add_gldef(def);
                            }
                            IRVarOrArrayDefine::Array(mut array_def) => {
                                let sym_ref = array_def.get_name().get_sym_ref();
                                let var_ref =
                                    self.ref_pool
                                        .add_array(sym_ref, array_def.copy_dims(), array_def.compute_ele_num(), array_def.get_type());
                                let inits = array_def.take_init();
                                let mut mir_init = MIRArrayInit { inits: vec![] };
                                for (index, init) in inits.init {
                                    let init = init.into_ir_number();
                                    mir_init.inits.push((index, init));
                                }
                                cfg.add_gldef(MIRGlDef::Array(array_def.is_const(), var_ref, mir_init));
                            }
                        }
                    }
                }
                HirUnit::FunDefine(fun_def) => {
                    let cfg_fun = self.ir_fun_to_cfg_unit(fun_def);
                    cfg.add_fun(cfg_fun);
                }
                HirUnit::ExternalFun(external) => self.ir_external_fun_to_mir(external),
            }
        }
        cfg
    }

    pub(crate) fn ir_fun_to_cfg_unit(&mut self, mut fun_def: IRFunDefine) -> CFGFun {
        let fn_name_symbol_ref = fun_def.get_name().get_sym_ref();
        self.current_ret_type = fun_def.get_ret_type();
        let fun_name = self.ref_pool.add_fn(fn_name_symbol_ref);

        let mut cfg_fun = CFGFun::new(fun_name);
        cfg_fun.ret_type = self.current_ret_type.get_correspond_mir_type();

        let params = fun_def.take_params();
        for param in params {
            match param {
                IRTypedParam::Var(var) => {
                    let sym_ref = var.get_name_sym_ref();
                    let mir_var = self.ref_pool.add_var_by_hir_type(sym_ref.clone(), var.get_type(), Normal);
                    cfg_fun.add_var_param(mir_var);
                }
                IRTypedParam::Array(mut array) => {
                    let size = array.get_array_size();
                    let array = self
                        .ref_pool
                        .add_array(array.get_irname().get_sym_ref(), array.take_dims(), size, array.get_type());
                    cfg_fun.add_array_param(array)
                }
            }
        }
        let label = self.new_mir_label_or_name(".L_block_");
        let mut current_cfg_block = CFGBasicBlock::new(label);

        let stmts = fun_def.take_stmts();
        let mut now_block_ref: Option<_> = Option::<CFGBasicBlockRef>::None;
        for stmt in stmts {
            match stmt {
                IRStatement::VarDef(var_def) => {
                    self.ir_var_def_to_mir(var_def, &mut current_cfg_block);
                }
                IRStatement::ArrDef(array_def) => {
                    self.ir_array_def_to_mir(array_def, &mut current_cfg_block);
                }
                IRStatement::Assign(assign) => {
                    self.ir_assign_to_mir(assign, &mut current_cfg_block);
                }
                IRStatement::FunInvoke(invoke) => {
                    let mir_invoke = self.ir_invoke_to_mir(invoke, &mut current_cfg_block);
                    current_cfg_block.add_stmt(MIRStmt::Invoke(mir_invoke));
                }
                IRStatement::Branches(branches) => {
                    let last_ref = match now_block_ref {
                        None => cfg_fun.push_block(current_cfg_block),
                        Some(block_ref) => {
                            cfg_fun.push_block_with_ref(block_ref.clone(), current_cfg_block);
                            block_ref
                        }
                    };

                    let exit_label = self.new_mir_label_or_name(".L_after_branches_");
                    let next_block_ref = cfg_fun.pre_acquire_block_ref(exit_label);
                    now_block_ref = Some(next_block_ref.clone());
                    // parse branches
                    self.ir_branches_to_cfg(branches, &mut cfg_fun, last_ref, next_block_ref);
                    current_cfg_block = CFGBasicBlock::new(exit_label);
                }
                IRStatement::Block(block) => {
                    let last_ref = match now_block_ref {
                        None => cfg_fun.push_block(current_cfg_block),
                        Some(block_ref) => {
                            cfg_fun.push_block_with_ref(block_ref.clone(), current_cfg_block);
                            block_ref
                        }
                    };
                    let body_label = self.new_mir_label_or_name(".L_block_body");
                    let next_label = self.new_mir_label_or_name(".L_after_blocl_");
                    let next_block_ref = cfg_fun.pre_acquire_block_ref(next_label);
                    self.ir_stmt_to_cfg(block, &mut cfg_fun, body_label, &last_ref, &next_block_ref, None);
                    now_block_ref = Some(next_block_ref.clone());
                    current_cfg_block = CFGBasicBlock::new(next_label);
                }
                IRStatement::While(while_loop) => {
                    let last_ref = match now_block_ref {
                        None => cfg_fun.push_block(current_cfg_block),
                        Some(block_ref) => {
                            cfg_fun.push_block_with_ref(block_ref.clone(), current_cfg_block);
                            block_ref
                        }
                    };
                    let exit_label = self.new_mir_label_or_name(".L_after_while_");
                    let next_block_ref = cfg_fun.pre_acquire_block_ref(exit_label);
                    self.ir_while_to_cfg(while_loop, &mut cfg_fun, last_ref, next_block_ref.clone());
                    now_block_ref = Some(next_block_ref);
                    current_cfg_block = CFGBasicBlock::new(exit_label);
                }
                IRStatement::Return(ret) => match ret {
                    None => current_cfg_block.add_stmt(MIRStmt::Return(MIRReturn::just_return())),
                    Some(expr) => {
                        let math_right = self.ir_rvalue_to_mir(expr, self.current_ret_type, &mut current_cfg_block);
                        let right = self.create_or_unwrap_right_from_mir_math_right(math_right, &mut current_cfg_block);
                        let lref = self.create_or_unwrap_lref_from_mir_right(right, &mut current_cfg_block);
                        current_cfg_block.add_stmt(MIRStmt::Return(MIRReturn::var_return(lref)))
                    }
                },

                // --- the below code can never reach ---
                IRStatement::EmptyExpr(_) => {
                    allow_nothing!();
                }
                IRStatement::Continue => {
                    panic!("NEVER HERE")
                }
                IRStatement::Break => {
                    panic!("NEVER HERE")
                }
                IRStatement::BoolDef(_) => {
                    panic!("NEVER HERE")
                }
                IRStatement::BoolAssign(_) => {
                    panic!("NEVER HERE")
                }
            }
        }
        match now_block_ref {
            None => {
                cfg_fun.push_block(current_cfg_block);
            }
            Some(basic_block_ref) => {
                cfg_fun.push_block_with_ref(basic_block_ref, current_cfg_block);
            }
        }
        cfg_fun
    }

    // ir 系列函数如果不产生新块，那么将当前块作为参数传递 , 并且以to_mir 为后缀
    // 如果产生，特殊处理 函数以 to_cfg 为后缀

    /// 会处理上之间的CFG联系
    fn ir_stmt_to_cfg(
        &mut self,
        stmts: IRStatements,
        cfg_fun: &mut CFGFun,
        body_label: MIRLabel,
        last_block: &CFGBasicBlockRef,
        next_block: &CFGBasicBlockRef,
        final_jump_next: Option<CFGBasicBlockRef>,
    ) {
        let current_label = body_label;
        let mut now_block_ref: Option<_> = Option::<CFGBasicBlockRef>::None;
        let mut current_block = CFGBasicBlock::new(current_label);
        let mut is_first_block_linked = false;
        for stmt in stmts {
            match stmt {
                IRStatement::VarDef(def) => {
                    self.ir_var_def_to_mir(def, &mut current_block);
                }
                IRStatement::BoolDef(def) => {
                    self.ir_bool_def_to_mir(def, &mut current_block);
                }
                IRStatement::ArrDef(def) => {
                    self.ir_array_def_to_mir(def, &mut current_block);
                }
                IRStatement::Assign(assign) => {
                    self.ir_assign_to_mir(assign, &mut current_block);
                }
                IRStatement::Branches(branches) => {
                    let last_ref = match now_block_ref {
                        None => cfg_fun.push_block(current_block),
                        Some(block_ref) => {
                            cfg_fun.push_block_with_ref(block_ref.clone(), current_block);
                            block_ref
                        }
                    };

                    let exit_label = self.new_mir_label_or_name(".L_after_branches_");
                    let next_block_ref = cfg_fun.pre_acquire_block_ref(exit_label);
                    now_block_ref = Some(next_block_ref.clone());
                    // parse branches
                    self.ir_branches_to_cfg(branches, cfg_fun, last_ref, next_block_ref);
                    current_block = CFGBasicBlock::new(exit_label);
                }
                IRStatement::FunInvoke(invoke) => {
                    let invoke = self.ir_invoke_to_mir(invoke, &mut current_block);
                    current_block.add_stmt(MIRStmt::Invoke(invoke))
                }
                IRStatement::Block(block) => {
                    let last_ref = match now_block_ref {
                        None => cfg_fun.push_block(current_block),
                        Some(block_ref) => {
                            cfg_fun.push_block_with_ref(block_ref.clone(), current_block);
                            block_ref
                        }
                    };
                    let body_label = self.new_mir_label_or_name(".L_block_body");
                    let next_label = self.new_mir_label_or_name(".L_after_block_");
                    let next_block_ref = cfg_fun.pre_acquire_block_ref(next_label);
                    self.ir_stmt_to_cfg(block, cfg_fun, body_label, &last_ref, &next_block_ref, None);
                    now_block_ref = Some(next_block_ref.clone());
                    current_block = CFGBasicBlock::new(next_label);
                }
                IRStatement::While(ir_while) => {
                    let last_ref = match now_block_ref {
                        None => cfg_fun.push_block(current_block),
                        Some(block_ref) => {
                            cfg_fun.push_block_with_ref(block_ref.clone(), current_block);
                            block_ref
                        }
                    };

                    let next_label = self.new_mir_label_or_name(".L_after_while_");
                    let next_block_ref = cfg_fun.pre_acquire_block_ref(next_label);
                    self.ir_while_to_cfg(ir_while, cfg_fun, last_ref, next_block_ref.clone());
                    now_block_ref = Some(next_block_ref);
                    current_block = CFGBasicBlock::new(next_label);
                }

                IRStatement::Continue => {
                    let tag = self.continue_tags.back();
                    match tag {
                        None => {
                            panic!("NEVER HERE")
                        }
                        Some(label) => current_block.add_stmt(MIRStmt::Jump(MIRJump::new(label.clone()))),
                    }
                }
                IRStatement::Break => {
                    let tag = self.break_tags.back();
                    match tag {
                        None => {
                            panic!("NEVER HERE")
                        }
                        Some(label) => current_block.add_stmt(MIRStmt::Jump(MIRJump::new(label.clone()))),
                    }
                }
                IRStatement::Return(ret) => match ret {
                    None => current_block.add_stmt(MIRStmt::Return(MIRReturn::just_return())),
                    Some(expr) => {
                        let right = self.ir_rvalue_to_mir(expr, self.current_ret_type, &mut current_block);
                        let right = self.create_or_unwrap_right_from_mir_math_right(right, &mut current_block);
                        let lref = self.create_or_unwrap_lref_from_mir_right(right, &mut current_block);
                        current_block.add_stmt(MIRStmt::Return(MIRReturn::var_return(lref)))
                    }
                },
                IRStatement::BoolAssign(_) => {
                    panic!("NEVER HERE")
                }
                IRStatement::EmptyExpr(_) => {
                    allow_nothing!();
                }
            }
        }
        if let Some(jump) =  final_jump_next {
            let next_label = cfg_fun.get_block_label(&jump);
            current_block.add_stmt(MIRStmt::Jump(MIRJump::new(next_label)))
        }
        // 值得注意的是，如果第一块已经被确定，当前块的上链接一定是没有问题的
        match now_block_ref {
            None => {
                let new_block_ref = cfg_fun.push_block(current_block);
                if is_first_block_linked {
                    // 向下链接需要处理
                    cfg_fun.link(new_block_ref, next_block.clone());
                } else {
                    // 如果第一块没有链接,那么这就是第一块
                    cfg_fun.link(last_block.clone(), new_block_ref.clone());
                    cfg_fun.link(new_block_ref, next_block.clone());
                }
            }
            Some(block_ref) => {
                cfg_fun.push_block_with_ref(block_ref.clone(), current_block);
                if is_first_block_linked {
                    // 如果第一块链接了
                    cfg_fun.link(block_ref, next_block.clone());
                } else {
                    // 如果第一块没有链接,那么这就是第一块
                    cfg_fun.link(last_block.clone(), block_ref.clone());
                    cfg_fun.link(block_ref, next_block.clone());
                }
            }
        }
    }

    fn ir_while_to_cfg(&mut self, ir_while: IRWhile, cfg_fun: &mut CFGFun, last_block: CFGBasicBlockRef, next_block_ref: CFGBasicBlockRef) {
        let cond = ir_while.condition;
        let enter_cond = self.new_mir_label_or_name(".L_while_cond_");
        let body_label = self.new_mir_label_or_name(".L_while_body_");
        let next_label = cfg_fun.get_block_label(&next_block_ref);

        let mut cond_block = CFGBasicBlock::new(enter_cond);

        self.break_tags.push_back(next_label);
        self.continue_tags.push_back(enter_cond);

        self.ir_bool_expr_to_mir(cond, &mut cond_block, next_label, body_label);
        let cond_block_ref = cfg_fun.push_block(cond_block);
        cfg_fun.link(last_block, cond_block_ref.clone());

        self.ir_stmt_to_cfg(ir_while.statements, cfg_fun, body_label, &cond_block_ref, &next_block_ref, Some(cond_block_ref.clone()));

        self.break_tags.pop_back();
        self.continue_tags.pop_back();
    }

    fn ir_branches_to_cfg(&mut self, ir_branches: IRBranches, cfg_fun: &mut CFGFun, last_block: CFGBasicBlockRef, exit_block: CFGBasicBlockRef) {
        let first = ir_branches.first_if;
        cfg_fun.new_layer(); // 进入新的一层
        let branch_num = ir_branches.elif.len();
        let mut next_block_ref: CFGBasicBlockRef = match ir_branches.elif.front() {
            None => match ir_branches.last_else {
                None => exit_block.clone(),
                Some(_) => cfg_fun.pre_acquire_block_ref(self.new_mir_label_or_name(".L_else_")),
            },
            Some(_) => cfg_fun.pre_acquire_block_ref(self.new_mir_label_or_name(".L_cond_block_")),
        };
        let mut current_source_block_ref = last_block;
        let mut current_cond_block_ref = cfg_fun.pre_acquire_block_ref(self.new_mir_label_or_name(".L_cond_block_"));
        self.ir_cond_stmts_to_cfg(
            first,
            cfg_fun,
            &current_cond_block_ref,
            &current_source_block_ref,
            &next_block_ref,
            &exit_block,
        );
        for (index, branch) in ir_branches.elif.into_iter().enumerate() {
            if index == branch_num - 1 {
                // branch_num >= 1
                // 最后一块
                current_source_block_ref = current_cond_block_ref;
                current_cond_block_ref = next_block_ref.clone();
                next_block_ref = match ir_branches.last_else {
                    None => exit_block.clone(),
                    Some(_) => cfg_fun.pre_acquire_block_ref(self.new_mir_label_or_name(".L_else_")),
                };
                self.ir_cond_stmts_to_cfg(
                    branch,
                    cfg_fun,
                    &current_cond_block_ref,
                    &current_source_block_ref,
                    &next_block_ref,
                    &exit_block,
                );
            } else {
                // 还有下一个分支
                current_source_block_ref = current_cond_block_ref;
                current_cond_block_ref = next_block_ref.clone();
                next_block_ref = cfg_fun.pre_acquire_block_ref(self.new_mir_label_or_name(".L_cond_block_"));
                self.ir_cond_stmts_to_cfg(
                    branch,
                    cfg_fun,
                    &current_cond_block_ref,
                    &current_source_block_ref,
                    &next_block_ref,
                    &exit_block,
                );
            }
        } // 如果elif分支的话，这里直接退出

        match ir_branches.last_else {
            None => {
                allow_nothing!();
            }
            Some(else_branch) => {
                let else_body_label = cfg_fun.get_block_label(&next_block_ref);
                self.ir_stmt_to_cfg(else_branch, cfg_fun, else_body_label, &current_cond_block_ref, &exit_block, None);
            }
        }
        cfg_fun.exit_layer();
    }

    fn ir_cond_stmts_to_cfg(
        &mut self,
        ir_if: IRIf,
        cfg_fun: &mut CFGFun,
        cond_block_ref: &CFGBasicBlockRef,
        source_block_ref: &CFGBasicBlockRef,
        next_block_ref: &CFGBasicBlockRef,
        exit_block_ref: &CFGBasicBlockRef,
    ) {
        let label = self.new_mir_label_or_name(".L_cond_block_");
        let mut cond_block = CFGBasicBlock::new(label);

        let body_label = self.new_mir_label_or_name(".L_branch_body_");

        let cond_expr = ir_if.condition;
        let body_stmts = ir_if.statements;

        let next_label = cfg_fun.get_block_label(next_block_ref); // 下一个分支或者出口
                                                                  // let exit_label = cfg_fun.get_block_label(exit_block_ref); // 出口

        self.ir_bool_expr_to_mir(cond_expr, &mut cond_block, next_label, body_label);
        cfg_fun.push_block_with_ref(cond_block_ref.clone(), cond_block);

        cfg_fun.link(source_block_ref.clone(), cond_block_ref.clone()); // 向上连接

        // 条件体执行完成后需要跳到完全出口
        self.ir_stmt_to_cfg(body_stmts, cfg_fun, body_label, &cond_block_ref, exit_block_ref, Some(exit_block_ref.clone()));
    }

    /// 与 to_mir 不同,to_mir2 一个BoolExpr 只能产生一个基本块 ， 基本块内部可以跳转
    fn ir_bool_expr_to_mir(&mut self, bool_expr: IRBoolExpr, target_block: &mut CFGBasicBlock, false_exit: MIRLabel, true_exit: MIRLabel) {
        match bool_expr {
            IRBoolExpr::Bool(defs, bool) => {
                for single_def in defs {
                    match single_def {
                        IRBoolOrVarDefine::VarDef(var) => {
                            self.ir_var_def_to_mir(var, target_block);
                        }
                        IRBoolOrVarDefine::BoolDef(bool) => {
                            self.ir_bool_def_to_mir(bool, target_block);
                        }
                    }
                }
                self.ir_bool_to_mir(bool, target_block, false_exit, true_exit);
            }
            IRBoolExpr::And(ands) => {
                // len >= 2
                let len = ands.len();
                let mut next_true_exit = self.new_mir_label_or_name(&format!(".L_cond{}_", 0));
                for (index, bool_expr) in ands.into_iter().enumerate() {
                    self.ir_bool_expr_to_mir(bool_expr, target_block, false_exit, next_true_exit);
                    if index != len - 1 {
                        target_block.add_stmt(MIRStmt::Label(next_true_exit)); // 注意, 与to_mir1 不同，这里采用Label内嵌的方式完成
                    }
                    if index == len - 2 {
                        // 最后一块跳转到真出口
                        next_true_exit = true_exit;
                    } else {
                        next_true_exit = self.new_mir_label_or_name(&format!(".L_cond{}_", index));
                    }
                }
            }
            IRBoolExpr::Or(ors) => {
                let len = ors.len();
                let mut next_false_exit = self.new_mir_label_or_name(&format!(".L_cond{}_", 0));
                for (index, bool_expr) in ors.into_iter().enumerate() {
                    self.ir_bool_expr_to_mir(bool_expr, target_block, next_false_exit, true_exit);
                    if index != len - 1 {
                        target_block.add_stmt(MIRStmt::Label(next_false_exit)); // 注意, 与to_mir1 不同，这里采用Label内嵌的方式完成
                    }
                    if index == len - 2 {
                        // 最后一块跳转到假出口
                        next_false_exit = false_exit;
                    } else {
                        next_false_exit = self.new_mir_label_or_name(&format!(".L_cond{}_", index));
                    }
                }
            }
        }
    }

    fn ir_bool_to_mir(&mut self, ir_bool: IRBool, target_block: &mut CFGBasicBlock, false_exit: MIRLabel, true_exit: MIRLabel) {
        match ir_bool {
            IRBool::Basic(basic) => {
                let (_, right) = self.ir_rel_source_to_mir(basic, target_block);
                let cond = MIRBool::BoolRef(right);
                let jump = MIRCondJump::new(cond, false_exit, Some(true_exit));
                target_block.add_stmt(MIRStmt::CondJump(jump));
            }
            IRBool::Rel(a, oper, c) => {
                let (source_type1, right1) = self.ir_rel_source_to_mir(a, target_block);
                let (source_type2, right2) = self.ir_rel_source_to_mir(c, target_block);
                if source_type1 != MIRType::Float && source_type2 != MIRType::Float {
                    let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), right2);
                    let cmp = MIRBool::Cmp(cmp_expr);
                    target_block.add_stmt(MIRStmt::CondJump(MIRCondJump::new(cmp, false_exit, Some(true_exit))))
                } else {
                    if source_type1 == MIRType::Float && source_type2 != MIRType::Float {
                        let new_source2 = self.new_temp_var_by_mir(MIRType::Float);
                        let lref = self.create_or_unwrap_var_from_mir_cmp_right(right2, target_block);
                        let stmt = self.new_mir_cvt(true, new_source2, lref);
                        target_block.add_stmt(stmt);
                        let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), MIRCmpRight::MathL(MIRMathLRef::Var(new_source2)));
                        let cmp = MIRBool::Cmp(cmp_expr);
                        target_block.add_stmt(MIRStmt::CondJump(MIRCondJump::new(cmp, false_exit, Some(true_exit))))
                    } else if source_type1 != MIRType::Float && source_type2 == MIRType::Float {
                        // source_type2 -> MIRType::FLOAT
                        let new_source1 = self.new_temp_var_by_mir(MIRType::Float);
                        let lref = self.create_or_unwrap_var_from_mir_cmp_right(right1, target_block);

                        let stmt = self.new_mir_cvt(true, new_source1, lref);
                        target_block.add_stmt(stmt);
                        let cmp_expr = MIRCmpExpr::new(MIRCmpRight::MathL(MIRMathLRef::Var(new_source1)), oper.get_correspond_mir_type(), right2);
                        let cmp = MIRBool::Cmp(cmp_expr);
                        target_block.add_stmt(MIRStmt::CondJump(MIRCondJump::new(cmp, false_exit, Some(true_exit))))
                    } else {
                        let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), right2);
                        let cmp = MIRBool::Cmp(cmp_expr);
                        target_block.add_stmt(MIRStmt::CondJump(MIRCondJump::new(cmp, false_exit, Some(true_exit))))
                    }
                }
            }
        }
    }

    fn ir_rel_source_to_mir(&mut self, rel_source: IRRelSource, target_block: &mut CFGBasicBlock) -> (MIRType, MIRCmpRight) {
        match rel_source {
            IRRelSource::Literal(lit) => (lit.get_type().get_correspond_mir_type(), MIRCmpRight::Lit(lit)),
            IRRelSource::Var(var) => {
                let var = self.ref_pool.get_lvar_ref(var.get_name_sym_ref());
                (var.get_type().clone(), MIRCmpRight::MathL(MIRMathLRef::Var(var)))
            }
            IRRelSource::ArrayEleRef(ele) => {
                let ele = self.ir_name_array_ele_to_mir(ele, target_block);
                (ele.get_type(), MIRCmpRight::MathL(MIRMathLRef::ArrayEle(ele)))
            }
            IRRelSource::BasicBool(basic) => {
                let var = self.ref_pool.get_lvar_ref(basic.get_name_sym_ref());
                (MIRType::Bool, MIRCmpRight::CmpL(MIRBoolLRef::new(var)))
            }
        }
    }

    fn ir_bool_def_to_mir(&mut self, bool_def: IRBoolVarDefine, target_block: &mut CFGBasicBlock) {
        let active_type = if bool_def.is_temp() { Temp } else { Normal };
        let left_var = self
            .ref_pool
            .add_var_by_mir_type(bool_def.get_name().get_sym_ref(), MIRType::Bool, active_type);
        match bool_def.init {
            IRBool::Basic(_basic) => {
                panic!("NEVER HERE");
            }
            IRBool::Rel(a, oper, c) => {
                let (source_type1, right1) = self.ir_rel_source_to_mir(a, target_block);
                let (source_type2, right2) = self.ir_rel_source_to_mir(c, target_block);
                if source_type1 != MIRType::Float && source_type2 != MIRType::Float
                    || (source_type1 == MIRType::Float && source_type2 == MIRType::Float)
                {
                    let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), right2);
                    let stmt = MIRBoolCmpAssign::new(true, false, MIRBoolLRef::new(left_var), cmp_expr);
                    target_block.add_stmt(MIRStmt::BoolCmpAssign(stmt));
                } else if source_type1 == MIRType::Float && source_type2 != MIRType::Float {
                    let var2_ref = self.create_or_unwrap_var_from_mir_cmp_right(right2, target_block);
                    let new_var2 = self.new_temp_var_by_mir(MIRType::Float);
                    let new_cvt = self.new_mir_cvt(true, new_var2, var2_ref);
                    target_block.add_stmt(new_cvt);
                    let cmp_expr = MIRCmpExpr::new(right1, oper.get_correspond_mir_type(), MIRCmpRight::MathL(MIRMathLRef::Var(new_var2)));
                    target_block.add_stmt(MIRStmt::BoolCmpAssign(MIRBoolCmpAssign::new(
                        true,
                        false,
                        MIRBoolLRef::new(left_var),
                        cmp_expr,
                    )));
                } else if source_type1 != MIRType::Float && source_type2 == MIRType::Float {
                    let var1_ref = self.create_or_unwrap_var_from_mir_cmp_right(right1, target_block);
                    let new_var1 = self.new_temp_var_by_mir(MIRType::Float);
                    let new_cvt = self.new_mir_cvt(true, new_var1, var1_ref);
                    target_block.add_stmt(new_cvt);
                    let cmp_expr = MIRCmpExpr::new(MIRCmpRight::MathL(MIRMathLRef::Var(new_var1)), oper.get_correspond_mir_type(), right2);
                    target_block.add_stmt(MIRStmt::BoolCmpAssign(MIRBoolCmpAssign::new(
                        true,
                        false,
                        MIRBoolLRef::new(left_var),
                        cmp_expr,
                    )));
                }
            }
        }
    }

    fn ir_var_def_to_mir(&mut self, mut var_def: IRVarDefine, current_block: &mut CFGBasicBlock) {
        let sym_ref = var_def.get_name().get_sym_ref();
        let active_type = if var_def.is_temp() { Temp } else { Normal };
        let mir_var_ref = self.ref_pool.add_var_by_hir_type(sym_ref, var_def.get_type(), active_type);
        let mir_var_init = var_def.take_init();
        match mir_var_init {
            None => {
                current_block.add_stmt(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::just_def(mir_var_ref)))
            }
            Some(init) => {
                let ret = self.ir_rvalue_to_mir(init, var_def.get_type(), current_block);
                let assign = MIRVarCopyAssign::new(true, mir_var_ref, ret);
                let stmt = MIRStmt::MathVarCopyAssign(assign);
                current_block.add_stmt(stmt)
            }
        }
    }

    fn ir_rvalue_to_mir(&mut self, rvalue: IRRValue, dest_type: IRBasicType, target_block: &mut CFGBasicBlock) -> MIRMathRight {
        match rvalue {
            IRRValue::Invoke(suffix, invoke) => {
                let invoke_ret_type = invoke.return_type;
                let mir_invoke = self.ir_invoke_to_mir(invoke, target_block);
                return if suffix.is_none() && dest_type == invoke_ret_type {
                    MIRMathRight::new(MIRMathPrefix::None, MIRRight::Invoke(mir_invoke))
                } else {
                    let var = self.new_temp_var_by_hir(invoke_ret_type);
                    let assign = MIRVarCopyAssign::new(
                        true,
                        var.clone(),
                        MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::Invoke(mir_invoke)),
                    );
                    let ir_stmt = MIRStmt::MathVarCopyAssign(assign);
                    target_block.add_stmt(ir_stmt);
                    let var = if invoke_ret_type == dest_type {
                        var
                    } else {
                        let new_var = self.new_temp_var_by_hir(dest_type);
                        let math_cvt = MIRMathCvt::new(true, MIRMathLRef::Var(new_var), MIRMathLRef::Var(var));
                        let stmt = MIRStmt::MathCvt(math_cvt);
                        target_block.add_stmt(stmt);
                        new_var
                    };
                    MIRMathRight::new(MIRMathPrefix::None, MIRRight::VarRef(var))
                };
            }
            IRRValue::Basic(prefix, basic) => {
                return match basic {
                    IRBasicExpr::Literal(lit) => {
                        if lit.get_type() != dest_type {
                            let old_var = self.new_temp_var_by_hir(lit.get_type());
                            let assign = MIRVarCopyAssign::new(true, old_var, MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::Lit(lit)));
                            let stmt = MIRStmt::MathVarCopyAssign(assign);
                            target_block.add_stmt(stmt);
                            let new_var = self.new_temp_var_by_hir(dest_type);
                            let stmt = self.new_mir_cvt(true, new_var, old_var);
                            target_block.add_stmt(stmt);
                            MIRMathRight::new(MIRMathPrefix::None, MIRRight::VarRef(new_var))
                        } else {
                            MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::Lit(lit))
                        }
                    }
                    IRBasicExpr::Var(var) => {
                        let lvar = self.ref_pool.get_lvar_ref(var.get_name_sym_ref());
                        if var.get_type() != dest_type {
                            let new_var = self.new_temp_var_by_hir(dest_type);
                            let cvt_stmt = self.new_mir_cvt(true, new_var, lvar);
                            target_block.add_stmt(cvt_stmt);
                            MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::VarRef(new_var))
                        } else {
                            MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::VarRef(lvar))
                        }
                    }
                    IRBasicExpr::ArrayEleRef(array_ele) => {
                        let array_type = array_ele.get_type();
                        let mut mir_array_ele = self.ir_name_array_ele_to_mir(array_ele, target_block);
                        if array_type != dest_type {
                            let new_var = self.new_temp_var_by_hir(dest_type);
                            let old_var = self.new_temp_var_by_hir(array_type);
                            let assign = MIRVarCopyAssign::new(
                                true,
                                old_var,
                                MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::ArrEleRef(mir_array_ele)),
                            );
                            let stmt = MIRStmt::MathVarCopyAssign(assign);
                            let cvt_stmt = self.new_mir_cvt(true, new_var, old_var);
                            target_block.add_stmt(stmt);
                            target_block.add_stmt(cvt_stmt);
                            MIRMathRight::new(MIRMathPrefix::None, MIRRight::VarRef(new_var))
                        } else {
                            MIRMathRight::new(self.ir_prefix_to_mir(prefix), MIRRight::ArrEleRef(mir_array_ele))
                        }
                    }
                };
            }
            IRRValue::Math(suffix, a, b, c) => {
                let mir_left = self.ir_basic_expr_to_mir(a, target_block);
                let mir_right = self.ir_basic_expr_to_mir(c, target_block);
                let mir_oper = b.get_correspond_mir_type();
                let left_kind = mir_left.get_type();
                let right_kind = mir_right.get_type();
                if left_kind != right_kind {
                    //  如果类型不相等，那么只能转成Float，然后再做出运算，最后再转向期望类型
                    let mir_left = if left_kind == MIRType::Float {
                        mir_left
                    } else {
                        let old_var = self.new_temp_var_by_hir(IRBasicType::INT);
                        let assign = MIRVarCopyAssign::new(true, old_var, MIRMathRight::new(MIRMathPrefix::None, mir_left));
                        let stmt = MIRStmt::MathVarCopyAssign(assign);
                        target_block.add_stmt(stmt);
                        let new_var = self.new_temp_var_by_hir(IRBasicType::FLOAT);
                        let stmt = self.new_mir_cvt(true, new_var, old_var);
                        target_block.add_stmt(stmt);
                        MIRRight::VarRef(new_var)
                    };
                    let mir_right = if right_kind == MIRType::Float {
                        mir_right
                    } else {
                        let old_var = self.new_temp_var_by_hir(IRBasicType::INT);
                        let assign = MIRVarCopyAssign::new(true, old_var, MIRMathRight::new(MIRMathPrefix::None, mir_right));
                        let stmt = MIRStmt::MathVarCopyAssign(assign);
                        target_block.add_stmt(stmt);
                        let new_var = self.new_temp_var_by_hir(IRBasicType::FLOAT);
                        let stmt = self.new_mir_cvt(true, new_var, old_var);
                        target_block.add_stmt(stmt);
                        MIRRight::VarRef(new_var)
                    };
                    let left_var = self.create_or_unwrap_lref_from_mir_right(mir_left, target_block);
                    let right_var = self.create_or_unwrap_lref_from_mir_right(mir_right, target_block);
                    let new_var = self.new_temp_var_by_hir(IRBasicType::FLOAT);
                    let compute_assign = MIRVarComputeAssign::new(true, new_var, left_var, mir_oper, right_var);
                    let result = MIRStmt::MathVarComputeAssign(compute_assign);
                    target_block.add_stmt(result);
                    return if dest_type != IRBasicType::FLOAT {
                        let new_cvt_var = self.new_temp_var_by_hir(dest_type);
                        let stmt = self.new_mir_cvt(true, new_cvt_var, new_var);
                        target_block.add_stmt(stmt);
                        MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::VarRef(new_cvt_var))
                    } else {
                        MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::VarRef(new_var))
                    };
                } else {
                    let result_var = self.new_temp_var_by_mir(left_kind);
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
                                        let copy_assign =
                                            MIRVarCopyAssign::new(true, result_var, MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(result)));
                                        target_block.add_stmt(MIRStmt::MathVarCopyAssign(copy_assign));
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
                        let left_var = self.create_or_unwrap_lref_from_mir_right(mir_left, target_block);
                        let right_var = self.create_or_unwrap_lref_from_mir_right(mir_right, target_block);
                        let compute_assign = MIRVarComputeAssign::new(true, result_var, left_var, mir_oper, right_var);
                        let result = MIRStmt::MathVarComputeAssign(compute_assign);
                        target_block.add_stmt(result);
                    }
                    return if dest_type.get_correspond_mir_type() == right_kind {
                        MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::VarRef(result_var))
                    } else {
                        let new_var = self.new_temp_var_by_hir(dest_type);
                        let stmt = self.new_mir_cvt(true, new_var, result_var);
                        target_block.add_stmt(stmt);
                        MIRMathRight::new(self.ir_prefix_to_mir(suffix), MIRRight::VarRef(new_var))
                    };
                }
            }
            IRRValue::Bool(_) => {
                panic!("NEVER HERE")
            }
        }
    }

    fn ir_invoke_to_mir(&mut self, ir_invoke: IRInvoke, current_block: &mut CFGBasicBlock) -> MIRInvoke {
        let name = self.ref_pool.get_fn_name(&ir_invoke.fun_name.get_sym_ref());
        let fun_name_str = self.symbol_table.get_symbol(&ir_invoke.fun_name.get_sym_ref());
        if fun_name_str.get_string() == "starttime".to_string() {
            let symbol_ref = self.symbol_table.get_symbol_ref(&AnySymbol::new("_sysy_starttime")).unwrap();
            let mir_ref = self.ref_pool.get_fn_name(&symbol_ref);
            let new_invoke = MIRInvoke::new(
                mir_ref,
                vec![MIRNamedParam::Right(MIRRight::Lit(MIRLit::I32(ir_invoke.line_at as i32)))],
                MIRType::Void,
            );
            return new_invoke;
        } else if fun_name_str.get_string() == "stoptime".to_string() {
            let symbol_ref = self.symbol_table.get_symbol_ref(&AnySymbol::new("_sysy_stoptime")).unwrap();
            let mir_ref = self.ref_pool.get_fn_name(&symbol_ref);
            let new_invoke = MIRInvoke::new(
                mir_ref,
                vec![MIRNamedParam::Right(MIRRight::Lit(MIRLit::I32(ir_invoke.line_at as i32)))],
                MIRType::Void,
            );
            return new_invoke;
        }
        let mut params = vec![];
        for param in ir_invoke.variables {
            match param {
                IRNamedParam::IRRValue(expected, actual) => {
                    let mir_right = self.ir_rvalue_to_mir(actual, expected, current_block);
                    if mir_right.is_support_unwrap() {
                        let right = mir_right.into_right_uncheck();
                        params.push(MIRNamedParam::Right(right));
                    } else {
                        let lref = self.new_temp_var_by_mir(mir_right.math.get_type());
                        current_block.add_stmt(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(true, lref, mir_right)));
                        params.push(MIRNamedParam::Right(MIRRight::VarRef(lref)))
                    }
                }
                IRNamedParam::IRNamedArray(ir_name_array) => {
                    let array = self.ir_name_array_to_mir(ir_name_array, current_block);
                    params.push(MIRNamedParam::Array(array))
                }
            }
        }
        let mir = MIRInvoke::new(name, params, ir_invoke.return_type.get_correspond_mir_type());
        // current_block.add_stmt()
        mir
    }

    fn ir_name_array_ele_to_mir(&mut self, mut array_ele: IRNamedArrayEle, target_block: &mut CFGBasicBlock) -> MIRArrayEleRef {
        let actual_dims = array_ele.take_dims();
        let typed_array = self.ref_pool.get_larray_ref(array_ele.get_name_sym_ref());
        let typed_dims = typed_array.get_dims();
        if typed_dims.len() > 1 {
            let array_offset_var = self.ir_compute_array_offset(actual_dims, typed_dims, target_block);
            let ele_ref = self
                .ref_pool
                .create_array_ele_ref(typed_array.copy_name(), typed_array.get_type(), array_offset_var);
            ele_ref
        } else {
            let mut dims = actual_dims;
            let dim = dims.pop();
            match dim {
                None => {
                    // 这里仅仅是作为元素的转化，传递数组时候的计算不在这里完成
                    panic!("NEVER HERE")
                }
                Some(dim) => {
                    let right = self.ir_rvalue_to_mir(dim, IRBasicType::INT, target_block); // 取出本维大小
                    let var_opt = right.try_unwraped_to_var();
                    return match var_opt {
                        None => {
                            let a_var = self.new_temp_var_by_hir(IRBasicType::INT);
                            let copy_assign = MIRVarCopyAssign::new(true, a_var, right);
                            let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
                            target_block.add_stmt(stmt);
                            let ele_ref = self.ref_pool.create_array_ele_ref(typed_array.copy_name(), typed_array.get_type(), a_var);
                            ele_ref
                        }
                        Some(var) => {
                            let ele_ref = self.ref_pool.create_array_ele_ref(typed_array.copy_name(), typed_array.get_type(), var);
                            ele_ref
                        }
                    };
                }
            }
        }
    }

    fn ir_name_array_to_mir(&mut self, mut array: IRNamedArray, target_block: &mut CFGBasicBlock) -> MIRNamedArrayParam {
        let actual_dims = array.take_dims();
        let typed_array = self.ref_pool.get_larray_ref(&array.get_name().get_sym_ref());
        let typed_dims = typed_array.get_dims();
        return if actual_dims.len() == 0 {
            let a_var = self.new_temp_var_by_hir(IRBasicType::INT);
            let copy_assign = MIRVarCopyAssign::new(true, a_var, MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(MIRLit::I32(0))));
            let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
            target_block.add_stmt(stmt);
            MIRNamedArrayParam::new(typed_array.copy_name(), a_var, typed_array.get_type())
        } else {
            let array_offset_var = self.ir_compute_array_offset(actual_dims, typed_dims, target_block);
            let stmt = MIRNamedArrayParam::new(typed_array.copy_name(), array_offset_var, typed_array.get_type());
            stmt
        };
    }

    fn ir_basic_expr_to_mir(&mut self, basic_expr: IRBasicExpr, target_block: &mut CFGBasicBlock) -> MIRRight {
        return match basic_expr {
            IRBasicExpr::Literal(lit) => MIRRight::Lit(lit),
            IRBasicExpr::Var(var) => {
                let var = self.ref_pool.get_lvar_ref(var.get_name_sym_ref());
                MIRRight::VarRef(var)
            }
            IRBasicExpr::ArrayEleRef(array_ele) => {
                let ret = self.ir_name_array_ele_to_mir(array_ele, target_block);
                MIRRight::ArrEleRef(ret)
            }
        };
    }

    fn ir_compute_array_offset(&mut self, actual_dims: Vec<IRRValue>, typed_dims: &Vec<usize>, target_block: &mut CFGBasicBlock) -> MIRVarRef {
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
            let mut now_dim_size: usize;
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
                let array_offset_var = self.new_temp_var_by_hir(IRBasicType::INT);
                let def = MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(
                    true,
                    array_offset_var,
                    MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(all_offset)),
                ));
                target_block.add_stmt(def);
                return array_offset_var;
            }
        }

        let mut array_offset_var = self.new_temp_var_by_hir(IRBasicType::INT); //  特殊情况
        array_offset_var.active_type = Normal;
        let zero_math_right_0 = MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(IRNumber::I32(0)));
        let zero_math_right_1 = MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(IRNumber::I32(0)));
        let zero_math_right_2 = MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(IRNumber::I32(0)));
        let mut a_var = self.new_temp_var_by_hir(IRBasicType::INT); // 特殊情况
        a_var.active_type = Normal;
        let mut c_var = self.new_temp_var_by_hir(IRBasicType::INT); // 特殊情况
        c_var.active_type = Normal;
        let offset_init_assign = MIRVarCopyAssign::new(true, array_offset_var, zero_math_right_0);
        let a_init_assign = MIRVarCopyAssign::new(true, a_var, zero_math_right_1);
        let c_init_assign = MIRVarCopyAssign::new(true, c_var, zero_math_right_2);
        let offset_stmt = MIRStmt::MathVarCopyAssign(offset_init_assign);
        let a_stmt = MIRStmt::MathVarCopyAssign(a_init_assign);
        let c_stmt = MIRStmt::MathVarCopyAssign(c_init_assign);
        target_block.add_stmt(offset_stmt);
        target_block.add_stmt(a_stmt);
        target_block.add_stmt(c_stmt);
        let mut now_dim_size: usize;
        for (index, ir_rvalue) in actual_dims.into_iter().enumerate() {
            if index == typed_dims.len() - 1 {
                // 最后一个维度
                let right = self.ir_rvalue_to_mir(ir_rvalue, IRBasicType::INT, target_block);
                let unwrap_result = right.try_unwraped_to_var();
                let part_a_var = match unwrap_result {
                    None => {
                        // 这种情况下 a_var 需要使用
                        let a_assign = MIRVarCopyAssign::new(false, a_var, right);
                        target_block.add_stmt(MIRStmt::MathVarCopyAssign(a_assign));
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
                target_block.add_stmt(MIRStmt::MathVarComputeAssign(assign));
                break;
            } else {
                now_dim_size = typed_dims_unit.get(index).unwrap().clone();
                let right = self.ir_rvalue_to_mir(ir_rvalue, IRBasicType::INT, target_block);
                let unwrap_result = right.try_unwraped_to_var();
                let part_a_var = match unwrap_result {
                    None => {
                        // 这种情况下 a_var 需要使用
                        let a_assign = MIRVarCopyAssign::new(false, a_var, right);
                        target_block.add_stmt(MIRStmt::MathVarCopyAssign(a_assign));
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
                target_block.add_stmt(MIRStmt::MathVarCopyAssign(c_assign));

                let a_assign = MIRVarComputeAssign::new(false, a_var, MIRMathLRef::Var(part_a_var), MIRMathOper::Mul, MIRMathLRef::Var(c_var));
                target_block.add_stmt(MIRStmt::MathVarComputeAssign(a_assign));
                let offset_assign = MIRVarComputeAssign::new(
                    false,
                    array_offset_var,
                    MIRMathLRef::Var(array_offset_var),
                    MIRMathOper::Add,
                    MIRMathLRef::Var(a_var),
                );
                target_block.add_stmt(MIRStmt::MathVarComputeAssign(offset_assign));
            }
        }
        array_offset_var
    }

    fn ir_prefix_to_mir(&self, suffix: IRExprPrefix) -> MIRMathPrefix {
        return match suffix.get_first() {
            IRFirstSuffix::None => match suffix.get_second() {
                IRSecondSuffix::None => MIRMathPrefix::None,
                IRSecondSuffix::EvenBang => MIRMathPrefix::BangBang,
                IRSecondSuffix::OddBang => MIRMathPrefix::Bang,
            },
            IRFirstSuffix::Negative => match suffix.get_second() {
                IRSecondSuffix::None => MIRMathPrefix::Neg,
                IRSecondSuffix::EvenBang => MIRMathPrefix::NegBangBang,
                IRSecondSuffix::OddBang => MIRMathPrefix::NegBang,
            },
        };
    }

    fn ir_assign_to_mir(&mut self, assign: IRAssign, target_block: &mut CFGBasicBlock) {
        let left = assign.left;
        return match left {
            IRLValue::Var(var) => {
                // opt 基本死代码消除
                if self.session.get_opt_level() >= OptLevel::O1 {
                    if var.get_name().get_all_ref_num() == 0 {
                        return;
                    }
                }
                let right = self.ir_rvalue_to_mir(assign.right, var.get_type(), target_block);
                let dest = self.ref_pool.get_lvar_ref(var.get_name_sym_ref());
                target_block.add_stmt(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(false, dest, right)));
            }
            IRLValue::ArrayEle(array_ele) => {
                let right = self.ir_rvalue_to_mir(assign.right, array_ele.get_type(), target_block);
                // 可优化，这里的右值 ？？？
                let mir = self.ir_name_array_ele_to_mir(array_ele, target_block);
                target_block.add_stmt(MIRStmt::ArrayEleAssign(MIRArrayEleCopyAssign::new(mir, right)));
            }
        };
    }

    fn ir_array_def_to_mir(&mut self, mut array_def: IRArrayDefine, target_block: &mut CFGBasicBlock) {
        let sym = array_def.get_name().get_sym_ref();
        let ele_num = array_def.compute_ele_num();
        let array_ref = self.ref_pool.add_array(sym, array_def.take_dims(), ele_num, array_def.get_type());
        let init = array_def.take_init();
        let mut inits = vec![];

        for (pos, expr) in init.init {
            let right = self.ir_rvalue_to_mir(expr, array_def.get_type(), target_block);
            let right = self.create_or_unwrap_right_from_mir_math_right(right, target_block);
            inits.push((pos, right))
        }
        target_block.add_stmt(MIRStmt::ArrayDef(MIRArrayDef::new(array_def.is_const(), array_ref, inits)));
    }

    fn ir_external_fun_to_mir(&mut self, external_fun: IRExternalFun) {
        self.ref_pool.add_fn(external_fun.name.get_sym_ref());
    }

    fn create_or_unwrap_var_from_mir_cmp_right(&mut self, mir_right: MIRCmpRight, target_block: &mut CFGBasicBlock) -> MIRVarRef {
        return match mir_right {
            MIRCmpRight::MathL(mathl) => match mathl {
                MIRMathLRef::Var(var) => var,
                MIRMathLRef::ArrayEle(ele) => {
                    let new_var = self.new_temp_var_by_mir(ele.get_type());
                    target_block.add_stmt(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(
                        true,
                        new_var,
                        MIRMathRight::new(MIRMathPrefix::None, MIRRight::ArrEleRef(ele)),
                    )));
                    new_var
                }
            },
            MIRCmpRight::Lit(lit) => {
                let new_var = self.new_temp_var_by_mir(lit.get_type().get_correspond_mir_type());
                target_block.add_stmt(MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(
                    true,
                    new_var,
                    MIRMathRight::new(MIRMathPrefix::None, MIRRight::Lit(lit)),
                )));
                new_var
            }
            MIRCmpRight::CmpL(cmpl) => cmpl.get_var(),
        };
    }

    fn create_or_unwrap_lref_from_mir_right(&mut self, mir_right: MIRRight, target_block: &mut CFGBasicBlock) -> MIRMathLRef {
        let right_type = mir_right.get_type();
        match mir_right {
            MIRRight::Lit(_) => {
                let var = self.new_temp_var_by_mir(right_type);
                let copy_assign = MIRVarCopyAssign::new(true, var, MIRMathRight::new(MIRMathPrefix::None, mir_right));
                let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
                target_block.add_stmt(stmt);
                MIRMathLRef::Var(var)
            }
            MIRRight::VarRef(var) => MIRMathLRef::Var(var),
            MIRRight::ArrEleRef(arr) => MIRMathLRef::ArrayEle(arr),
            MIRRight::CmpRef(_) => {
                let var = self.new_temp_var_by_mir(MIRType::Bool);
                let copy_assign = MIRVarCopyAssign::new(true, var, MIRMathRight::new(MIRMathPrefix::None, mir_right));
                let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
                target_block.add_stmt(stmt);
                MIRMathLRef::Var(var);
                todo!("NEVER HERE")
            }
            MIRRight::Invoke(invoke) => {
                let var = self.new_temp_var_by_mir(invoke.get_ret_type());
                let mir_right = MIRRight::Invoke(invoke);
                let mir_math_right = MIRMathRight::new(MIRMathPrefix::None, mir_right);
                let copy_assign = MIRVarCopyAssign::new(true, var, mir_math_right);
                let stmt = MIRStmt::MathVarCopyAssign(copy_assign);
                target_block.add_stmt(stmt);
                MIRMathLRef::Var(var)
            }
        }
    }

    fn create_or_unwrap_right_from_mir_math_right(&mut self, right: MIRMathRight, target_block: &mut CFGBasicBlock) -> MIRRight {
        match right.get_prefix() {
            MIRMathPrefix::None => {
                return right.math;
            }
            _ => {}
        }
        let new_sym = self.symbol_table.add_symbol(AnySymbol::new_rand_underscore("te_mir"));
        let c_var = self.ref_pool.add_var_by_mir_type(new_sym, right.get_right().get_type(), Temp);
        let stmt = MIRStmt::MathVarCopyAssign(MIRVarCopyAssign::new(true, c_var, right));
        target_block.add_stmt(stmt);
        return MIRRight::VarRef(c_var);
    }

    fn new_mir_cvt(&self, is_def: bool, new_var: MIRVarRef, old_var: MIRVarRef) -> MIRStmt {
        let cvt = MIRMathCvt::new(is_def, MIRMathLRef::Var(new_var), MIRMathLRef::Var(old_var));
        let stmt = MIRStmt::MathCvt(cvt);
        stmt
    }

    fn new_temp_var_by_hir(&mut self, basic_type: IRBasicType) -> MIRVarRef {
        let new_sym = self.symbol_table.add_symbol(AnySymbol::new_rand_underscore("te_mir"));
        let c_var = self.ref_pool.add_var_by_hir_type(new_sym, basic_type, Temp);
        c_var
    }

    fn new_temp_var_by_mir(&mut self, basic_type: MIRType) -> MIRVarRef {
        let new_sym = self.symbol_table.add_symbol(AnySymbol::new_rand_underscore("te_mir"));
        let c_var = self.ref_pool.add_var_by_mir_type(new_sym, basic_type, Temp);
        c_var
    }

    fn new_mir_label_or_name(&mut self, prefix: &str) -> MIRLabel {
        let label_symbol = AnySymbol::new_rand(prefix);
        let sym_ref = self.symbol_table.add_symbol(label_symbol);
        let label = self.ref_pool.add_label(sym_ref);
        label
    }
}
