use crate::allow_nothing;
use crate::asm::{get_reg, DetailedRegisterKind, ISABackend, RegisterKind};
use crate::ast::table::{AnySymbol, AnySymbolRef, SymbolTable};
use crate::hir::hir::{IRBasicType, IRNumber};
use crate::lir::lir::{
    HRegister, ImmediateBits, LirStatement, RegOrImm, RegWithOff, RegWithOffOrImm, WidthType, ADDINS, BEQINS, BGEINS, BLTINS, BNEINS, CVTINS, DIVINS,
    FSEQINS, FSLEINS, IMM, LOADINS, MOVINS, MULINS, NEGINS, REMINS, RWOOI, SLTINS, STOREINS, SUBINS,
};
use crate::lir::register_helper2::RegisterHelper2;
use crate::lir::session::Session;
use crate::mir::{
    MIRBool, MIRBoolCmpAssign, MIRBoolCopyAssign, MIRBoolOper, MIRCmpRight, MIRCondJump, MIRFun, MIRGlDef, MIRInvoke, MIRMathCvt, MIRMathLRef,
    MIRMathOper, MIRMathPrefix, MIRName, MIRNamedParam, MIRRefPool, MIRRight, MIRStmt, MIRType, MIRTypedParam, MIRVarRef,
};
use crate::util::MutableRef;
use std::collections::VecDeque;

pub struct MIRFunTranslate2<'a> {
    ref_pool: &'a mut MIRRefPool,
    symbol_table: &'a mut SymbolTable,
    session: MutableRef<Session>,
    register_helper: &'a mut RegisterHelper2,
    isa_backend: &'a dyn ISABackend,
    just_ret: bool,
    fun_name: Option<MIRName>,
    is_main: bool,
}

impl<'a> MIRFunTranslate2<'a> {
    pub(crate) fn create(
        ref_pool: &'a mut MIRRefPool,
        symbol_table: &'a mut SymbolTable,
        register_helper: &'a mut RegisterHelper2,
        session: MutableRef<Session>,
        isa_backend:&'a dyn ISABackend
    ) -> Self {
        Self {
            ref_pool,
            symbol_table,
            session,
            register_helper,
            isa_backend: isa_backend,
            just_ret: false,
            // 安全高于一切，复杂不是代价
            fun_name: None,
            is_main: false,
        }
    }

    pub fn translate_gldef(&mut self, def: MIRGlDef) {
        match def {
            MIRGlDef::Var(is_const, var, init) => {
                if is_const {
                    panic!("ERROR CONST NOT ALLOWED")
                }
                self.register_helper.alloc_global_var(var, init, self.ref_pool, self.symbol_table);
            }
            MIRGlDef::Array(is_const, array_ref, init) => {
                self.register_helper
                    .alloc_global_array(&array_ref, init, is_const, self.ref_pool, self.symbol_table);
            }
        }
    }

    pub fn translate_fun(&mut self, mut fun: MIRFun, regs: Vec<HRegister>) {
        self.register_helper.reset(regs);
        let fun_name = fun.get_name().clone();
        self.fun_name = Some(fun_name);
        let fun_blocks = fun.take_blocks();
        let params = fun.params;
        let fun_name_ref = self.ref_pool.get_sym_ref(&fun_name);
        let fun_name_str = self.symbol_table.get_symbol(fun_name_ref).get_string();
        let start_tag_symbol = self
            .symbol_table
            .add_symbol(AnySymbol::new(&format!(".SYSYC_{}_fun_start", fun_name_str)));
        if fun_name_str == "main" {
            self.is_main = true;
        } else {
            self.is_main = false;
        }
        self.session.borrow_mut().enter_fun(fun_name_str.clone(), start_tag_symbol);

        let mut normal_reg_at: i32 = 0;
        let mut float_reg_at: i32 = 0;
        let mut exceed = VecDeque::new();
        for single in params {
            match single {
                MIRTypedParam::Var(var) => {
                    let reg = match var.get_type() {
                        MIRType::Int => {
                            if normal_reg_at >= self.isa_backend.preg_num() as i32 {
                                exceed.push_back(MIRTypedParam::Var(var));
                                continue;
                            }
                            let reg = get_reg(self.isa_backend, DetailedRegisterKind::Param, normal_reg_at as u8);
                            normal_reg_at += 1;
                            reg
                        }
                        MIRType::Float => {
                            if float_reg_at >= self.isa_backend.fpreg_num() as i32 {
                                exceed.push_back(MIRTypedParam::Var(var));
                                continue;
                            }
                            let reg = get_reg(self.isa_backend, DetailedRegisterKind::FParam, float_reg_at as u8);
                            float_reg_at += 1;
                            reg
                        }
                        MIRType::Bool => {
                            panic!("NEVER HERE")
                        }
                        MIRType::Void => {
                            panic!("NEVER HERE")
                        }
                    };
                    self.register_helper.alloc_var_with_specified_reg(&var, reg.clone());
                }
                MIRTypedParam::Array(array) => {
                    if normal_reg_at >= self.isa_backend.preg_num() as i32 {
                        exceed.push_back(MIRTypedParam::Array(array));
                        continue;
                    }
                    let reg = get_reg(self.isa_backend, DetailedRegisterKind::Param, normal_reg_at as u8);
                    normal_reg_at += 1;
                    self.register_helper.add_array_with_addr_reg(&array, reg);
                }
            }
        }
        if exceed.len() > 0 {
            let mut now_param_stack_at = 0;
            // clang 采用的策略是把 sp -> reg
            // 这里我是直接使用 sp
            for param in exceed {
                match param {
                    MIRTypedParam::Var(var) => {
                        let addr = RegWithOff::suggest_fp_top_based(HRegister::fp, now_param_stack_at);
                        self.register_helper.add_var_with_specified_addr(&var, addr);
                    }
                    MIRTypedParam::Array(array) => {
                        let addr = RegWithOff::suggest_fp_top_based(HRegister::fp, now_param_stack_at);
                        self.register_helper.add_array_with_addr(&array, addr);
                    }
                }
                now_param_stack_at += 8;
            }
        }

        for mut block in fun_blocks {
            if block.get_name() != &MIRName::special() {
                let label_symbol = self.ref_pool.get_sym_ref(block.get_name());
                self.session.borrow_mut().push(LirStatement::Label(label_symbol.clone()));
            }
            let stmts = block.take_stmts();
            for stmt in stmts.into_iter() {
                self.translate_stmt(stmt)
            }
        }
        self.session
            .borrow_mut()
            .exit_fun(fun_name_str, self.register_helper.get_stack_size(), !self.just_ret, self.symbol_table);
    }

    pub fn process_prefix(&mut self, prefix: MIRMathPrefix, rd: HRegister, rs: HRegister) {
        match prefix {
            MIRMathPrefix::None => {
                if rd != rs {
                    self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: rd, 1: rs }))
                }
            }
            MIRMathPrefix::Neg => self.session.borrow_mut().push(LirStatement::Neg(NEGINS { 0: rd, 1: rs })),
            MIRMathPrefix::Bang => self.session.borrow_mut().push(LirStatement::Slt(SLTINS {
                0: rd,
                1: rs,
                2: RegOrImm::Imm(IMM::ImmI12(1)),
            })),
            MIRMathPrefix::BangBang => {
                self.session.borrow_mut().push(LirStatement::Slt(SLTINS {
                    0: rd,
                    1: HRegister::zero,
                    2: RegOrImm::Reg(rs),
                }));
            }
            MIRMathPrefix::NegBang => {
                self.session.borrow_mut().push(LirStatement::Slt(SLTINS {
                    0: rd,
                    1: HRegister::zero,
                    2: RegOrImm::Reg(rs),
                }));
                self.session.borrow_mut().push(LirStatement::Add(ADDINS {
                    0: WidthType::Word,
                    1: rd,
                    2: rs,
                    3: RegOrImm::Imm(ImmediateBits::ImmI12(-1)),
                }));
            }
            MIRMathPrefix::NegBangBang => {
                /*
                    seqz	a0, a0
                    addiw	a0, a0, -1
                */
                self.session.borrow_mut().push(LirStatement::Slt(SLTINS {
                    0: rd,
                    1: rs,
                    2: RegOrImm::Imm(IMM::ImmI12(1)),
                }));
                self.session.borrow_mut().push(LirStatement::Add(ADDINS {
                    0: WidthType::DoubleWord,
                    1: rd,
                    2: rs,
                    3: RegOrImm::Imm(IMM::ImmI12(-1)),
                }))
            }
        }
    }

    pub fn translate_stmt(&mut self, stmt: MIRStmt) {
        match stmt {
            MIRStmt::MathVarCopyAssign(copy_assign) => {
                let is_def = copy_assign.is_def();
                let var = copy_assign.left;
                let math_right = copy_assign.right;
                let reg;
                let math_right = match math_right {
                    None => {
                        self.register_helper.add_var_wait_assign(&var);
                        return;
                    }
                    Some(right) => {
                        right
                    }
                };
                match math_right.math {
                    MIRRight::Lit(lit) => {
                        let target_reg = if is_def {
                            self.register_helper.alloc_var_new(&var)
                        } else {
                            self.register_helper.load_var_for_save(&var)
                        };

                        let prefix = math_right.prefix;
                        let lit_type = lit.get_type();
                        if lit_type != IRBasicType::FLOAT {
                            let num = lit.prefix_integer_into_imm_i32(prefix);
                            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                0: WidthType::Word,
                                1: target_reg,
                                2: RWOOI::Imm(IMM::ImmWordI32(num)),
                            }));
                        } else {
                            let num = lit.prefix_float_into_ieee754_u32(prefix);
                            let temp_rag = self.register_helper.alloc_reg(MIRType::Int);
                            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                0: WidthType::Word,
                                1: temp_rag,
                                2: RWOOI::Imm(IMM::ImmWordU32(num)),
                            }));
                            self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: target_reg, 1: temp_rag }));
                            self.register_helper.drop_reg(&temp_rag);
                        }
                        self.register_helper.save_var_to_addr(&var, is_def);
                        return;
                    }
                    MIRRight::VarRef(var_ref) => {
                        let is_def = is_def;
                        let target_reg = if is_def {
                            self.register_helper.alloc_var_new(&var)
                        } else {
                            self.register_helper.load_var_for_save(&var)
                        };
                        let a = self.register_helper.load_var_new(&var_ref);
                        self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: target_reg, 1: a }));
                        self.register_helper.drop_var_one_time(&var_ref);
                        reg = target_reg;
                    }
                    MIRRight::ArrEleRef(ele_ref) => {
                        let target_reg = if is_def {
                            self.register_helper.alloc_var_new(&var)
                        } else {
                            self.register_helper.load_var_for_save(&var)
                        };
                        let a = self.register_helper.load_array_ele_new(&ele_ref, false, true);
                        self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: target_reg, 1: a }));
                        self.register_helper.drop_reg(&a);
                        reg = target_reg;
                    }
                    MIRRight::Invoke(invoke) => {
                        let ret = self.translate_invoke(invoke);
                        let target_reg = if is_def {
                            self.register_helper.alloc_var_new(&var)
                        } else {
                            self.register_helper.load_var_for_save(&var)
                        };
                        let ret_reg = ret.unwrap();
                        self.process_prefix(math_right.prefix, target_reg, ret_reg);
                        self.register_helper.save_var_to_addr(&var, is_def);
                        self.register_helper.grant_free(ret_reg);
                        return;
                    }
                    MIRRight::CmpRef(_) => {
                        panic!("NEVER HERE")
                    }
                }
                self.process_prefix(math_right.prefix, reg, reg);
                self.register_helper.save_var_to_addr(&var, is_def);
                self.just_ret = false;
            }
            MIRStmt::ArrayEleAssign(array_ele_assign) => {
                let left_ele = array_ele_assign.left;
                // 这里的写法存在问题，应该先判断右边，可以参考一下上面的写法
                let math_right = array_ele_assign.right;
                let mut tar_reg = HRegister::zero;
                if let MIRRight::Invoke(_) = math_right.math {
                    allow_nothing!();
                } else {
                    tar_reg = self.register_helper.load_array_ele_new(&left_ele, true, false);
                }
                match math_right.math {
                    MIRRight::Lit(lit) => {
                        let prefix = math_right.prefix;
                        let lit_type = lit.get_type();
                        if lit_type != IRBasicType::FLOAT {
                            let num = lit.prefix_integer_into_imm_i32(prefix);
                            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                0: WidthType::Word,
                                1: tar_reg,
                                2: RWOOI::Imm(IMM::ImmWordI32(num)),
                            }));
                        } else {
                            let num = lit.prefix_float_into_ieee754_u32(prefix);
                            let temp_rag = self.register_helper.alloc_reg(MIRType::Int);
                            self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                0: WidthType::Word,
                                1: temp_rag,
                                2: RWOOI::Imm(IMM::ImmWordU32(num)),
                            }));
                            self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: tar_reg, 1: temp_rag }));
                            self.register_helper.drop_reg(&temp_rag);
                        }
                        self.register_helper.save_array_ele_to_addr(&left_ele);
                        return;
                    }
                    MIRRight::VarRef(var) => {
                        let a = self.register_helper.load_var_new(&var);
                        self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: tar_reg, 1: a }));
                        self.register_helper.drop_var_one_time(&var);
                    }
                    MIRRight::ArrEleRef(ele) => {
                        let ret = self.register_helper.load_array_ele_new(&ele, false, true);
                        self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: tar_reg, 1: ret }));
                        self.register_helper.drop_reg(&ret);
                    }
                    MIRRight::Invoke(invoke) => {
                        let ret = self.translate_invoke(invoke);
                        let ret_reg = ret.unwrap();
                        tar_reg = self.register_helper.load_array_ele_new(&left_ele, true, false);
                        self.process_prefix(math_right.prefix, tar_reg, ret_reg);
                        self.register_helper.save_array_ele_to_addr(&left_ele);
                        self.register_helper.grant_free(ret_reg);
                        return;
                    }
                    MIRRight::CmpRef(_) => {
                        panic!("NEVER HERE")
                    }
                }
                self.process_prefix(math_right.prefix, tar_reg, tar_reg);
                self.register_helper.save_array_ele_to_addr(&left_ele);
                self.just_ret = false;
            }
            MIRStmt::MathVarComputeAssign(compute_assign) => {
                let first = compute_assign.get_first();
                let oper = compute_assign.get_oper();
                let second = compute_assign.get_second();
                let var = compute_assign.get_left();
                let is_def = compute_assign.is_def();
                let target_reg = if is_def {
                    self.register_helper.alloc_var_new(var)
                } else {
                    self.register_helper.load_var_for_save(var)
                };
                let mut first_reg_is_bind = true;
                let first_reg = match first {
                    MIRMathLRef::Var(var) => self.register_helper.load_var_new(var),
                    MIRMathLRef::ArrayEle(array_ele) => {
                        first_reg_is_bind = false;
                        self.register_helper.load_array_ele_new(array_ele, false, true)
                    }
                };
                let mut second_reg_is_bind = true;
                let second_reg = match second {
                    MIRMathLRef::Var(var) => self.register_helper.load_var_new(var),
                    MIRMathLRef::ArrayEle(array_ele) => {
                        second_reg_is_bind = false;
                        self.register_helper.load_array_ele_new(array_ele, false, true)
                    }
                };
                match oper {
                    MIRMathOper::Add => {
                        self.session.borrow_mut().push(LirStatement::Add(ADDINS {
                            0: WidthType::Word,
                            1: target_reg,
                            2: first_reg,
                            3: RegOrImm::Reg(second_reg),
                        }));
                    }
                    MIRMathOper::Sub => {
                        self.session.borrow_mut().push(LirStatement::Sub(SUBINS {
                            0: WidthType::Word,
                            1: target_reg,
                            2: first_reg,
                            3: RegOrImm::Reg(second_reg),
                        }));
                    }
                    MIRMathOper::Mul => {
                        self.session.borrow_mut().push(LirStatement::Mul(MULINS {
                            0: target_reg,
                            1: first_reg,
                            2: second_reg,
                        }));
                    }
                    MIRMathOper::Div => {
                        self.session.borrow_mut().push(LirStatement::Div(DIVINS {
                            0: target_reg,
                            1: first_reg,
                            2: second_reg,
                        }));
                    }
                    MIRMathOper::Rem => {
                        self.session.borrow_mut().push(LirStatement::Rem(REMINS {
                            0: target_reg,
                            1: first_reg,
                            2: second_reg,
                        }));
                    }
                }

                if first_reg_is_bind {
                    self.register_helper.drop_var_one_time(first.unwrap_get_var_lref_uncheck());
                } else {
                    self.register_helper.drop_reg(&first_reg);
                }
                if second_reg_is_bind {
                    self.register_helper.drop_var_one_time(second.unwrap_get_var_lref_uncheck());
                } else {
                    self.register_helper.drop_reg(&second_reg);
                }
                self.register_helper.save_var_to_addr(var, is_def);
                self.just_ret = false;
            }
            MIRStmt::BoolCmpAssign(assign) => self.translate_bool_cmp_assign(assign),
            MIRStmt::ArrayDef(array) => {
                self.register_helper.alloc_array(&array.array_ref, &mut self.symbol_table);
                let init = array.init;
                for (pos, init) in init {
                    match init {
                        MIRRight::Lit(lit) => {
                            let temp_init_reg = self.register_helper.alloc_reg(array.array_ref.get_type());
                            let lit_type = lit.get_type();
                            if lit_type != IRBasicType::FLOAT {
                                let num = lit.integer_into_imm_i32();
                                self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                    0: WidthType::Word,
                                    1: temp_init_reg,
                                    2: RWOOI::Imm(IMM::ImmWordI32(num)),
                                }));
                            } else {
                                let num = lit.float_into_ieee754_u32();
                                let temp_rag_a = self.register_helper.alloc_reg(MIRType::Int);
                                self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                    0: WidthType::Word,
                                    1: temp_rag_a,
                                    2: RWOOI::Imm(IMM::ImmWordU32(num)),
                                }));
                                self.session.borrow_mut().push(LirStatement::Mov(MOVINS {
                                    0: temp_init_reg,
                                    1: temp_rag_a,
                                }));
                                self.register_helper.drop_reg(&temp_rag_a);
                            }
                            self.register_helper
                                .save_reg_to_indexed_local_array_ele(&array.array_ref, pos, temp_init_reg);
                            self.register_helper.drop_reg(&temp_init_reg);
                        }
                        MIRRight::VarRef(var) => {
                            let reg = self.register_helper.load_var_new(&var);
                            self.register_helper.save_reg_to_indexed_local_array_ele(&array.array_ref, pos, reg);
                            self.register_helper.drop_var_one_time(&var);
                        }
                        MIRRight::ArrEleRef(array_ele) => {
                            let reg = self.register_helper.load_array_ele_new(&array_ele, false, true);
                            self.register_helper.save_reg_to_indexed_local_array_ele(&array.array_ref, pos, reg);
                            self.register_helper.drop_reg(&reg);
                        }
                        MIRRight::Invoke(invoke) => {
                            let ret_reg = self.translate_invoke(invoke);
                            let ret_reg = ret_reg.unwrap();
                            self.register_helper.save_reg_to_indexed_local_array_ele(&array.array_ref, pos, ret_reg);
                            self.register_helper.grant_free(ret_reg);
                        }
                        MIRRight::CmpRef(_) => {
                            panic!("NEVER HERE")
                        }
                    }
                }
            }
            MIRStmt::MathCvt(cvt) => self.translate_cvt(cvt),
            MIRStmt::CondJump(cond_jump) => {
                self.translate_cond_jump(cond_jump);
                self.just_ret = false;
            }
            MIRStmt::Jump(jump) => {
                let label = self.ref_pool.get_sym_ref(&jump.label);
                self.session.borrow_mut().push(LirStatement::Jump(label.clone()))
            }
            MIRStmt::Invoke(invoke) => {
                let reg = self.translate_invoke(invoke);
                if reg.is_some() {
                    self.register_helper.grant_free(reg.unwrap());
                }
                self.just_ret = false;
            }
            MIRStmt::Label(lab) => {
                let label_symbol = self.ref_pool.get_sym_ref(&lab);
                self.session.borrow_mut().push(LirStatement::Label(label_symbol.clone()));
            }
            MIRStmt::Return(ret) => {
                match ret.value {
                    None => {
                        let fun_name = self.ref_pool.get_sym_ref(&self.fun_name.unwrap());
                        let fun_name = self.symbol_table.get_symbol(fun_name).get_string();
                        self.session.borrow_mut().now_return(fun_name, self.is_main, &mut self.symbol_table);
                    },
                    Some(lref) => {
                        match lref {
                            MIRMathLRef::Var(var) => {
                                match var.get_type() {
                                    MIRType::Int => {
                                        self.register_helper.spill_all_active_temp();// 可优化
                                        self.register_helper.deprived_free(HRegister::p0);
                                        let var_reg = self.register_helper.load_var_new(&var);
                                        self.session.borrow_mut().push(LirStatement::Mov(MOVINS {
                                            0: HRegister::p0,
                                            1: var_reg,
                                        }));
                                        self.register_helper.drop_var_one_time(&var);
                                        // 后面还有要翻译的块,
                                        self.register_helper.grant_free(HRegister::p0);
                                    }
                                    MIRType::Float => {
                                        self.register_helper.spill_all_active_temp();// 可优化
                                        self.register_helper.deprived_free(HRegister::fp0);
                                        let var_reg = self.register_helper.load_var_new(&var);
                                        self.session.borrow_mut().push(LirStatement::Mov(MOVINS {
                                            0: HRegister::fp0,
                                            1: var_reg,
                                        }));
                                        self.register_helper.drop_var_one_time(&var);
                                        self.register_helper.grant_free(HRegister::fp0);
                                    }
                                    MIRType::Bool | MIRType::Void => {
                                        panic!("NEVER HERE")
                                    }
                                }
                                let fun_name = self.ref_pool.get_sym_ref(&self.fun_name.unwrap());
                                let fun_name = self.symbol_table.get_symbol(fun_name).get_string();
                                self.session.borrow_mut().now_return(fun_name, self.is_main, &mut self.symbol_table);
                            }
                            MIRMathLRef::ArrayEle(ele) => {
                                self.register_helper.spill_all_active_temp();// 可优化
                                let need_ret_reg = match ele.get_type() {
                                    MIRType::Float => {
                                        HRegister::fp0
                                    }
                                    _ => {
                                        HRegister::p0
                                    }
                                };
                                self.register_helper.deprived_free(need_ret_reg);

                                let ret = self.register_helper.load_array_ele_new(&ele, false, true);
                                self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: need_ret_reg, 1: ret }));
                                self.register_helper.drop_reg(&ret);

                                self.register_helper.grant_free(need_ret_reg);
                                let fun_name = self.ref_pool.get_sym_ref(&self.fun_name.unwrap());
                                let fun_name = self.symbol_table.get_symbol(fun_name).get_string();
                                self.session.borrow_mut().now_return(fun_name, self.is_main, &mut self.symbol_table);
                            }
                        }
                    }
                }
                self.just_ret = true;
            }
            MIRStmt::Sync(syncs) => {}
            MIRStmt::Clear(clears) => {}
            MIRStmt::Drop(_) => {
                panic!("NEVER HERE")
            }
        }
    }

    /// 加载一个布尔右值 , bool指示这个寄存器是不是被绑定了
    pub fn load_cmp_right(&mut self, cmp_right: MIRCmpRight) -> (Option<MIRVarRef>, HRegister) {
        match cmp_right {
            MIRCmpRight::MathL(math) => match math {
                MIRMathLRef::Var(var) => {
                    return (Some(var), self.register_helper.load_var_new(&var));
                }
                MIRMathLRef::ArrayEle(ele) => return (None, self.register_helper.load_array_ele_new(&ele, false, true)),
            },
            MIRCmpRight::Lit(lit) => {
                let lit_type = lit.get_type();
                if lit_type == IRBasicType::FLOAT {
                    let reg = self.register_helper.alloc_reg(MIRType::Float);
                    let temp_reg = self.register_helper.alloc_reg(MIRType::Bool);
                    let lit = lit.float_into_ieee754_u32();
                    self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                        0: WidthType::Word,
                        1: temp_reg,
                        2: RWOOI::Imm(ImmediateBits::ImmWordU32(lit)),
                    }));
                    self.register_helper.drop_reg(&temp_reg);

                    self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: reg, 1: temp_reg }));
                    return (None, reg);
                };
                let reg = self.register_helper.alloc_reg(MIRType::Bool);
                let lit = lit.integer_into_imm_i32();
                self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                    0: WidthType::Word,
                    1: reg,
                    2: RWOOI::Imm(ImmediateBits::ImmWordI32(lit)),
                }));
                return (None, reg);
            }
            MIRCmpRight::CmpL(cmp) => return (Some(cmp.get_var()), self.register_helper.load_var_new(&cmp.get_var())),
        }
    }

    pub fn translate_cond_jump(&mut self, cond_jump: MIRCondJump) {
        let false_exit = cond_jump.get_false_exit();
        let false_exit_label = self.ref_pool.get_sym_ref(&false_exit).clone();
        let true_exit = cond_jump.get_true_exit();
        let true_exit_label = if true_exit.is_some() {
            Some(self.ref_pool.get_sym_ref(&true_exit.unwrap()).clone())
        } else {
            None
        };
        let cond = cond_jump.cond;

        match cond {
            MIRBool::Cmp(cmp) => {
                let oper = cmp.oper;
                let cmp_type = cmp.first.get_source_type();

                let (first_is_bind, first) = self.load_cmp_right(cmp.first);
                let (second_is_bind, second) = self.load_cmp_right(cmp.second);
                if cmp_type != MIRType::Float {
                    match oper {
                        MIRBoolOper::Equal => {
                            self.session.borrow_mut().push(LirStatement::Bne(BNEINS {
                                0: first,
                                1: second,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::NotEqual => {
                            self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                                0: first,
                                1: second,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::Less => {
                            self.session.borrow_mut().push(LirStatement::Bge(BGEINS {
                                0: first,
                                1: second,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::Greater => {
                            self.session.borrow_mut().push(LirStatement::Bge(BGEINS {
                                0: second,
                                1: first,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::LessEqual => {
                            self.session.borrow_mut().push(LirStatement::Blt(BLTINS {
                                0: second,
                                1: first,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::GreaterEqual => {
                            self.session.borrow_mut().push(LirStatement::Blt(BLTINS {
                                0: first,
                                1: second,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::And => {
                            // 因为 Sysy 不支持()&&() 这样的语句 ，所以 x And y 不会出现 <-错误 [7月26日]
                            // And 和 Or 被翻译为多段条判断，所以这里已经没有了
                            todo!("在实际测试中不可达")
                        }
                        MIRBoolOper::Or => {
                            todo!("在实际测试中不可达")
                        }
                    }
                } else {
                    // float 类型的比较特殊处理
                    let cond_reg = self.register_helper.alloc_reg(MIRType::Int);
                    match oper {
                        MIRBoolOper::Equal => {
                            self.session.borrow_mut().push(LirStatement::Fseq(FSEQINS {
                                0: cond_reg,
                                1: first,
                                2: second,
                            }));
                            self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                                0: cond_reg,
                                1: HRegister::zero,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::NotEqual => {
                            // 第一个计算是反的,那么就1跳，也就是bne zero
                            //
                            self.session.borrow_mut().push(LirStatement::Fseq(FSEQINS {
                                0: cond_reg,
                                1: first,
                                2: second,
                            }));
                            self.session.borrow_mut().push(LirStatement::Bne(BNEINS {
                                0: cond_reg,
                                1: HRegister::zero,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::Less => {
                            self.session.borrow_mut().push(LirStatement::Slt(SLTINS {
                                0: cond_reg,
                                1: first,
                                2: RegOrImm::Reg(second),
                            }));
                            self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                                0: cond_reg,
                                1: HRegister::zero,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::Greater => {
                            self.session.borrow_mut().push(LirStatement::Slt(SLTINS {
                                0: cond_reg,
                                1: second,
                                2: RegOrImm::Reg(first),
                            }));
                            self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                                0: cond_reg,
                                1: HRegister::zero,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::LessEqual => {
                            self.session.borrow_mut().push(LirStatement::Fsle(FSLEINS {
                                0: cond_reg,
                                1: first,
                                2: second,
                            }));
                            self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                                0: cond_reg,
                                1: HRegister::zero,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::GreaterEqual => {
                            self.session.borrow_mut().push(LirStatement::Fsle(FSLEINS {
                                0: cond_reg,
                                1: second,
                                2: first,
                            }));
                            self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                                0: cond_reg,
                                1: HRegister::zero,
                                2: false_exit_label,
                            }));
                        }
                        MIRBoolOper::And => {
                            panic!("NEVER HEHR")
                        }
                        MIRBoolOper::Or => {
                            panic!("NEVER HEHR")
                        }
                    }
                    self.register_helper.drop_reg(&cond_reg);
                }

                if true_exit_label.is_some() {
                    self.session.borrow_mut().push(LirStatement::Jump(true_exit_label.unwrap()));
                }
                match first_is_bind {
                    None => {
                        self.register_helper.drop_reg(&first);
                    }
                    Some(var) => {
                        self.register_helper.drop_var_one_time(&var);
                    }
                }
                match second_is_bind {
                    None => {
                        self.register_helper.drop_reg(&second);
                    }
                    Some(var) => {
                        self.register_helper.drop_var_one_time(&var);
                    }
                }
            }
            MIRBool::BoolRef(num) => {
                let bool_value_type = num.get_source_type();
                let (is_bind, reg) = self.load_cmp_right(num);
                if bool_value_type != MIRType::Float {
                    self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                        0: reg,
                        1: HRegister::zero,
                        2: false_exit_label,
                    }));

                    if true_exit_label.is_some() {
                        self.session.borrow_mut().push(LirStatement::Jump(true_exit_label.unwrap()));
                    }
                } else {
                    // 浮点情况比较特殊,转化为normal，然后来判断 <- 错误实现
                    // 浮点的话 直接搬运 到普通寄存器，然后和zero判断
                    let cond_reg = self.register_helper.alloc_reg(MIRType::Int);
                    self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: cond_reg, 1: reg }));
                    self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                        0: cond_reg,
                        1: HRegister::zero,
                        2: false_exit_label,
                    }));

                    if true_exit_label.is_some() {
                        self.session.borrow_mut().push(LirStatement::Jump(true_exit_label.unwrap()));
                    }
                    self.register_helper.drop_reg(&cond_reg);
                }
                match is_bind {
                    None => self.register_helper.drop_reg(&reg),
                    Some(var) => self.register_helper.drop_var_one_time(&var),
                }
            }
        }
    }

    fn new_label(&mut self, prefix: &str) -> AnySymbolRef {
        let label_symbol = AnySymbol::new_rand(prefix);
        let sym_ref = self.symbol_table.add_symbol(label_symbol);
        sym_ref
    }

    pub fn translate_bool_cmp_assign(&mut self, cmp_assign: MIRBoolCmpAssign) {
        let left_var = cmp_assign.left;
        let is_def = cmp_assign.is_def;
        let target_reg = if cmp_assign.is_def {
            self.register_helper.alloc_var_new(&left_var.get_var())
        } else {
            self.register_helper.load_var_for_save(&left_var.get_var())
        };
        let cmp = cmp_assign.right;
        let oper = cmp.oper;
        let cmp_type = cmp.first.get_source_type();

        let (first_is_bind, first) = self.load_cmp_right(cmp.first);
        let (second_is_bind, second) = self.load_cmp_right(cmp.second);

        if cmp_type == MIRType::Float {
            match oper {
                MIRBoolOper::Equal => {
                    self.session.borrow_mut().push(
                        LirStatement::Fseq(FSEQINS{
                            0: target_reg,
                            1: first,
                            2: second,
                        })
                    )
                }
                MIRBoolOper::NotEqual => {
                    self.session.borrow_mut().push(
                        LirStatement::Fseq(FSEQINS{
                            0: target_reg,
                            1: first,
                            2: second,
                        })
                    );
                    self.session.borrow_mut().push(
                        LirStatement::Neg(
                            NEGINS{
                                0: target_reg,
                                1: target_reg,
                            }
                        )
                    )
                }
                MIRBoolOper::Less => {
                    self.session.borrow_mut().push(
                        LirStatement::Slt(SLTINS{
                            0: target_reg,
                            1: first,
                            2: RegOrImm::Reg(second),
                        })
                    )
                }
                MIRBoolOper::Greater => {
                    self.session.borrow_mut().push(
                        LirStatement::Slt(SLTINS{
                            0: target_reg,
                            1: second,
                            2: RegOrImm::Reg(first),
                        })
                    )
                }
                MIRBoolOper::LessEqual => {
                    self.session.borrow_mut().push(
                        LirStatement::Fsle(FSLEINS{
                            0: target_reg,
                            1: first,
                            2: second,
                        })
                    )
                }
                MIRBoolOper::GreaterEqual => {
                    self.session.borrow_mut().push(
                        LirStatement::Fsle(FSLEINS{
                            0: target_reg,
                            1: second,
                            2: first,
                        })
                    )
                }
                MIRBoolOper::And => {
                    panic!("NEVER HERE")
                }
                MIRBoolOper::Or => {
                    panic!("NEVER HERE")
                }
            }
        } else {
            // fixme 理论上可优化
            let false_label = self.new_label(".L_assign_false_label_");
            let true_label = self.new_label(".L_assign_true_label_");
            let exit_label = self.new_label(".L_assign_texit_label_");

            let set_true_stmt = LirStatement::Load(LOADINS {
                0: WidthType::Word,
                1: target_reg,
                2: RWOOI::Imm(IMM::ImmWordI32(1)),
            });
            let set_false_stmt = LirStatement::Mov(MOVINS {
                0: target_reg,
                1: HRegister::zero,
            });
            match oper {
                MIRBoolOper::Equal => {
                    self.session.borrow_mut().push(LirStatement::Beq(BEQINS {
                        0: first,
                        1: second,
                        2: true_label,
                    }));
                }
                MIRBoolOper::NotEqual => {
                    self.session.borrow_mut().push(LirStatement::Bne(BNEINS {
                        0: first,
                        1: second,
                        2: true_label,
                    }));
                }
                MIRBoolOper::Less => {
                    self.session.borrow_mut().push(LirStatement::Blt(BLTINS {
                        0: first,
                        1: second,
                        2: true_label,
                    }));
                }
                MIRBoolOper::Greater => {
                    self.session.borrow_mut().push(LirStatement::Blt(BLTINS {
                        0: second,
                        1: first,
                        2: true_label,
                    }));
                }
                MIRBoolOper::LessEqual => {
                    self.session.borrow_mut().push(LirStatement::Bge(BGEINS {
                        0: second,
                        1: first,
                        2: true_label,
                    }));
                }
                MIRBoolOper::GreaterEqual => {
                    self.session.borrow_mut().push(LirStatement::Bge(BGEINS {
                        0: first,
                        1: second,
                        2: true_label,
                    }));
                }
                MIRBoolOper::And => {
                    todo!("不可达")
                }
                MIRBoolOper::Or => {
                    todo!("不可达")
                }
            }

            self.session.borrow_mut().push(LirStatement::Label(false_label));
            self.session.borrow_mut().push(set_false_stmt);
            self.session.borrow_mut().push(LirStatement::Jump(exit_label));
            self.session.borrow_mut().push(LirStatement::Label(true_label));
            self.session.borrow_mut().push(set_true_stmt);
            self.session.borrow_mut().push(LirStatement::Label(exit_label));
        }
        match first_is_bind {
            None => self.register_helper.drop_reg(&first),
            Some(var) => {
                self.register_helper.drop_var_one_time(&var);
            }
        }

        match second_is_bind {
            None => self.register_helper.drop_reg(&second),
            Some(var) => {
                self.register_helper.drop_var_one_time(&var);
            }
        }
        self.register_helper.save_var_to_addr(&left_var.get_var(), is_def);
    }

    pub fn translate_cvt(&mut self, cvt: MIRMathCvt) {
        match cvt.left {
            MIRMathLRef::Var(var) => {
                let reg = if cvt.is_def {
                    self.register_helper.alloc_var_new(&var)
                } else {
                    self.register_helper.load_var_for_save(&var)
                };
                let from_reg = match &cvt.right {
                    MIRMathLRef::Var(var) => self.register_helper.load_var_new(&var),
                    MIRMathLRef::ArrayEle(array_ele) => self.register_helper.load_array_ele_new(&array_ele, false, true),
                };
                self.session.borrow_mut().push(LirStatement::Cvt(CVTINS { 0: reg, 1: from_reg }));
                self.register_helper.save_var_to_addr(&var, cvt.is_def);
                match cvt.right {
                    MIRMathLRef::Var(var) => self.register_helper.drop_var_one_time(&var),
                    MIRMathLRef::ArrayEle(_) => self.register_helper.drop_reg(&from_reg),
                }
            }
            MIRMathLRef::ArrayEle(_) => {
                todo!("可优化，目前不可达")
                // 目前不可达 , 有必要吗 ？有必要，直接加载元素到指定的寄存器而非临时中间寄存器
                // 另外，如果元素不参与后面的使用，这样也是没有代价的，因为不管怎么说，数组元素的赋值总是需要有一个寄存器的，
            }
        }
    }

    pub fn translate_bool_copy_assign(&mut self, assign: MIRBoolCopyAssign) {
        let is_def = assign.is_def();
        let left = assign.left;
        let right = assign.right;
        let var = left.get_var();
        let tar_reg = if is_def {
            self.register_helper.alloc_var_new(&var)
        } else {
            self.register_helper.load_var_for_save(&var)
        };
        match right {
            MIRCmpRight::MathL(math) => {
                let reg = match &math {
                    MIRMathLRef::Var(var) => self.register_helper.load_var_new(var),
                    MIRMathLRef::ArrayEle(ele) => self.register_helper.load_array_ele_new(ele, false, true),
                };
                self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: tar_reg, 1: reg }));
                match &math {
                    MIRMathLRef::Var(var) => {
                        self.register_helper.drop_var_one_time(var);
                    }
                    MIRMathLRef::ArrayEle(_ele) => self.register_helper.drop_reg(&reg),
                }
            }
            MIRCmpRight::Lit(lit) => {
                // todo 这里的类型值得注意
                // 实际测试不可达
                let immbits = lit.into_immbits();
                self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                    0: WidthType::Word,
                    1: tar_reg,
                    2: RWOOI::Imm(immbits),
                }))
            }
            MIRCmpRight::CmpL(cmp) => {
                let var = cmp.get_var();
                let reg = self.register_helper.load_var_new(&var);
                self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: tar_reg, 1: reg }));
                self.register_helper.drop_var_one_time(&var);
            }
        }
        self.register_helper.save_var_to_addr(&var, is_def);
    }

    // 进入该函数不允许有任何寄存器被占用，这个函数会去剥夺参数寄存器,回值寄存器是剥夺寄存器
    pub fn translate_invoke(&mut self, invoke: MIRInvoke) -> Option<HRegister> {
        self.register_helper.spill_all_active_temp();
        let ret_type = invoke.get_ret_type();
        let fun_name = invoke.get_name();
        let params = invoke.params();
        let mut nreg_index: i32 = 0;
        let mut freg_index: i32 = 0;
        let mut deprived = vec![];
        let mut exceed_params = VecDeque::new();
        for param in params {
            match param {
                MIRNamedParam::Right(right) => {
                    let right_type = right.get_type();
                    // MIR 的类型一定是没有问题的
                    match right_type {
                        MIRType::Int => {
                            if nreg_index >= self.isa_backend.preg_num() as i32 {
                                // int 参数传递超出了寄存器个数
                                exceed_params.push_back(MIRNamedParam::Right(right));
                                nreg_index += 1;
                                continue;
                            } // 这里又是一种中间层
                            let reg = get_reg(self.isa_backend, DetailedRegisterKind::Param, nreg_index as u8);
                            self.register_helper.deprived_free(reg);
                            deprived.push(reg);
                            match right {
                                MIRRight::Lit(lit) => self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                    0: WidthType::Word,
                                    1: reg,
                                    2: RegWithOffOrImm::Imm(lit.into_immbits()),
                                })),
                                MIRRight::VarRef(var_ref) => {
                                    let var_reg = self.register_helper.load_var_new(&var_ref);
                                    self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: reg, 1: var_reg }));
                                    self.register_helper.drop_var_one_time(&var_ref);
                                }
                                MIRRight::ArrEleRef(ele) => {
                                    let tar_reg = self.register_helper.load_array_ele_new(&ele, false, true);
                                    self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: reg, 1: tar_reg }));
                                    self.register_helper.drop_reg(&tar_reg);
                                }
                                MIRRight::CmpRef(_) => {
                                    panic!("NEVER HERE")
                                }
                                MIRRight::Invoke(_) => {
                                    // MIR不会允许这样的情况出现，这样就能避免了二次剥夺
                                    panic!("NEVER HERE")
                                }
                            }
                            nreg_index += 1;
                        }
                        MIRType::Float => {
                            if freg_index >= self.isa_backend.fpreg_num() as i32 {
                                // float 参数传递超出了寄存器个数
                                exceed_params.push_back(MIRNamedParam::Right(right));
                                freg_index += 1;
                                continue; // 后面可能有int类型的传递,所以这里不能break,而通用寄存器未必全部使用完了,所以这里不用break
                            }
                            let reg = get_reg(self.isa_backend, DetailedRegisterKind::FParam, freg_index as u8);
                            self.register_helper.deprived_free(reg);
                            deprived.push(reg);
                            match right {
                                MIRRight::Lit(lit) => {
                                    let temp_int = self.register_helper.alloc_reg(MIRType::Int);
                                    self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                        0: WidthType::Word,
                                        1: temp_int,
                                        2: RWOOI::Imm(ImmediateBits::ImmWordU32(lit.float_into_ieee754_u32())),
                                    }));
                                    self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: reg, 1: temp_int }));
                                    self.register_helper.drop_reg(&temp_int);
                                }
                                MIRRight::VarRef(var_ref) => {
                                    let var_reg = self.register_helper.load_var_new(&var_ref);
                                    self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: reg, 1: var_reg }));
                                    self.register_helper.drop_var_one_time(&var_ref);
                                }
                                MIRRight::ArrEleRef(ele) => {
                                    let tar_reg = self.register_helper.load_array_ele_new(&ele, false, true);
                                    self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: reg, 1: tar_reg }));
                                    self.register_helper.drop_reg(&tar_reg);
                                }
                                MIRRight::CmpRef(_) => {
                                    panic!("NEVER HERE")
                                }
                                MIRRight::Invoke(_) => {
                                    // MIR不会允许这样的情况出现，这样就能避免了二次剥夺
                                    panic!("NEVER HERE")
                                }
                            }
                            freg_index += 1;
                        }
                        MIRType::Bool | MIRType::Void => {
                            panic!("NEVER HERE")
                        }
                    }
                }
                MIRNamedParam::Array(array) => {
                    if nreg_index >= self.isa_backend.preg_num() as i32 {
                        // int 参数传递超出了寄存器个数
                        exceed_params.push_back(MIRNamedParam::Array(array));
                        nreg_index += 1;
                        continue;
                    }
                    let reg = get_reg(self.isa_backend, DetailedRegisterKind::Param, nreg_index as u8);
                    self.register_helper.deprived_free(reg); // pass array
                    self.register_helper.load_array_addr(array, &reg);
                    nreg_index += 1;
                    deprived.push(reg);
                }
            }
        }
        if exceed_params.len() > 0 {
            let need_stack_size = exceed_params.len() * 8; // fixme 单个参数大小为 8 byte
            self.register_helper.add_extra_stack_size(need_stack_size);
            let mut now_param_stack_at = 0;
            // clang 采用的策略是把 sp -> reg
            // 这里我是直接使用 sp
            for param in exceed_params {
                match param {
                    MIRNamedParam::Right(right) => {
                        let right_type = right.get_type();
                        match right {
                            MIRRight::Lit(lit) => {
                                let reg = self.register_helper.alloc_reg(right_type);
                                match lit.get_type() {
                                    IRBasicType::INT => {
                                        self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                            0: WidthType::Word,
                                            1: reg,
                                            2: RWOOI::Imm(ImmediateBits::ImmWordI32(lit.integer_into_imm_i32())),
                                        }));
                                    }
                                    IRBasicType::FLOAT => {
                                        let temp_int = self.register_helper.alloc_reg(MIRType::Int);
                                        self.session.borrow_mut().push(LirStatement::Load(LOADINS {
                                            0: WidthType::Word,
                                            1: temp_int,
                                            2: RWOOI::Imm(ImmediateBits::ImmWordU32(lit.float_into_ieee754_u32())),
                                        }));
                                        self.session.borrow_mut().push(LirStatement::Mov(MOVINS { 0: reg, 1: temp_int }));
                                        self.register_helper.drop_reg(&temp_int);
                                    }
                                    IRBasicType::VOID => {
                                        panic!("NEVER HERE")
                                    }
                                }
                                self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                                    0: WidthType::Word,
                                    1: reg,
                                    2: RegWithOff::suggest_sp_top_based(HRegister::sp, now_param_stack_at),
                                }));
                                self.register_helper.drop_reg(&reg)
                            }
                            MIRRight::VarRef(var) => {
                                let reg = self.register_helper.load_var_new(&var);
                                self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                                    0: WidthType::Word,
                                    1: reg,
                                    2: RegWithOff::suggest_sp_top_based(HRegister::sp, now_param_stack_at),
                                }));
                                self.register_helper.drop_var_one_time(&var)
                            }
                            MIRRight::ArrEleRef(ele) => {
                                let reg = self.register_helper.load_array_ele_new(&ele, false, true);
                                self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                                    0: WidthType::Word,
                                    1: reg,
                                    2: RegWithOff::suggest_sp_top_based(HRegister::sp, now_param_stack_at),
                                }));
                                self.register_helper.drop_reg(&reg)
                            }
                            MIRRight::CmpRef(_) => {
                                panic!("NEVER HERE")
                            }
                            MIRRight::Invoke(_) => {
                                // MIR不会允许这样的情况出现，这样就能避免了二次剥夺
                                panic!("NEVER HERE")
                            }
                        }
                    }
                    MIRNamedParam::Array(array) => {
                        let temp_reg = self.register_helper.alloc_reg(MIRType::Int);
                        self.register_helper.load_array_addr(array, &temp_reg);
                        self.session.borrow_mut().push(LirStatement::Store(STOREINS {
                            0: WidthType::DoubleWord,
                            1: temp_reg,
                            2: RegWithOff::suggest_sp_top_based(HRegister::sp, now_param_stack_at),
                        }));
                        self.register_helper.drop_reg(&temp_reg)
                    }
                }
                now_param_stack_at += 8;
            }
        }

        let fun_label = self.ref_pool.get_sym_ref(&fun_name);
        self.session.borrow_mut().push(LirStatement::Call(fun_label.clone()));
        for derp in deprived {
            self.register_helper.grant_free(derp);
        }
        match ret_type {
            MIRType::Int => {
                self.register_helper.deprived_free(HRegister::p0);
                return Some(HRegister::p0);
            }
            MIRType::Float => {
                self.register_helper.deprived_free(HRegister::fp0);
                return Some(HRegister::fp0);
            }
            MIRType::Bool => {
                panic!("NEVER HERE");
            }
            MIRType::Void => return None,
        }
    }
}

impl MIRVarRef {
    pub(crate) fn get_width_bytes(&self) -> usize {
        match self.get_type() {
            MIRType::Int => 4,
            MIRType::Float => 4,
            MIRType::Bool => 4,
            MIRType::Void => {
                panic!("NEVER HERE")
            }
        }
    }
}

impl MIRType {
    pub fn get_width(&self) -> usize {
        match self {
            MIRType::Int => 4,
            MIRType::Float => 4,
            MIRType::Bool => 4,
            MIRType::Void => {
                panic!("NEVER HERE")
            }
        }
    }
}

impl IRNumber {

    pub fn into_ieee754_or_complement(self) -> u32{
        match self {
            IRNumber::I32(i32) => {
                i32 as u32
            }
            IRNumber::F32(f32) => {
                f32.to_bits()
            }
        }
    }

    pub fn into_immbits(self) -> ImmediateBits{
        match &self {
            IRNumber::I32(i32) => {
                IMM::ImmWordI32(*i32)
            }
            IRNumber::F32(_) => {
                IMM::ImmWordU32(self.float_into_ieee754_u32())
            }
        }
    }

    pub fn prefix_integer_into_imm_i32(self,prefix:MIRMathPrefix)->i32{
        match self {
            IRNumber::I32(i32) => {
                match prefix {
                    MIRMathPrefix::None => {
                        i32
                    }
                    MIRMathPrefix::Neg => {
                        -i32
                    }
                    MIRMathPrefix::Bang => {
                        if i32 == 0 { 1 } else { 0 }
                    }
                    MIRMathPrefix::BangBang => {
                        if i32 == 0 { 0 } else { 1 }
                    }
                    MIRMathPrefix::NegBang => {
                        if i32 == 0 { -1 } else { 0 }
                    }
                    MIRMathPrefix::NegBangBang => {
                        if i32 == 0 { 0 } else { -1 }
                    }
                }
            }
            IRNumber::F32(_) => {
                panic!("NEVER HERE")
            }
        }

    }

    pub fn prefix_float_into_ieee754_u32(self,prefix:MIRMathPrefix) -> u32{
        match self {
            IRNumber::I32(_) => {
                panic!("NEVER HERE")
            }
            IRNumber::F32(f32) => {
                match prefix {
                    MIRMathPrefix::None => {
                        f32.to_bits()
                    }
                    MIRMathPrefix::Neg => {
                        (-f32).to_bits()
                    }
                    MIRMathPrefix::Bang => {
                        if f32 - 0.000 < 0.0000000001f32 {
                            1.0f32.to_bits()
                        } else {
                            0.0f32.to_bits()
                        }
                    }
                    MIRMathPrefix::BangBang => {
                        if f32 - 0.000 < 0.0000000001f32 {
                            0.0f32.to_bits()
                        } else {
                            1.0f32.to_bits()
                        }
                    }
                    MIRMathPrefix::NegBang => {
                        if f32 - 0.000 < 0.0000000001f32 {
                            (-1.0f32).to_bits()
                        } else {
                            0.0f32.to_bits()
                        }
                    }
                    MIRMathPrefix::NegBangBang => {
                        if f32 - 0.000 < 0.0000000001f32 {
                            0.0f32.to_bits()
                        } else {
                            (-1.0f32).to_bits()
                        }
                    }
                }
            }
        }
    }

    pub fn float_into_ieee754_u32(self) -> u32{
        match self {
            IRNumber::I32(_) => {
                panic!("NOT SUPPORT")
            }
            IRNumber::F32(f32) => {
                f32.to_bits()
            }
        }
    }

    pub fn integer_into_imm_i32(self) -> i32 {
        match self {
            IRNumber::I32(i32) => {
                i32
            }
            IRNumber::F32(_) => {
                panic!("NOT SUPPORT")
            }
        }
    }

    pub fn get_correspond_any_reg_kind(&self) -> RegisterKind {
        return match self {
            IRNumber::I32(_) => RegisterKind::AnyNormal,
            IRNumber::F32(_) => RegisterKind::AnyFNormal,
        };
    }
}
