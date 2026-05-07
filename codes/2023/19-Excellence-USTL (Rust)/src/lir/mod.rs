// the different between IR and AST
// AST NEARLY EQUAL TO SOURCE CODE but IR represents some kinds of abstraction

use crate::asm::{get_reg, DetailedRegisterKind, ISABackend};
use crate::ast::table::SymbolTable;
use crate::lir::lir::{HRegister, LirPartCode};
use crate::lir::session::Session;
use crate::lir::translate2::MIRFunTranslate2;
use crate::lir::translate3::MIRTranslate3;
use crate::lir::register_helper2::RegisterHelper2;
use crate::mir::{MIRModule, MIRRefPool, MIRUnit};
use crate::util::MutableRef;
use std::cell::RefCell;
use std::rc::Rc;

mod session;
mod register_helper2;

pub mod lir;
pub mod translate2;
pub mod translate3;

pub struct LIRTranslate<'a> {
    symbol_table: &'a mut SymbolTable,
    register_helper2: RegisterHelper2,
    session: Rc<RefCell<Session>>,
    backend: &'static dyn ISABackend,
    available_regs: Vec<HRegister>,
}

impl<'a> LIRTranslate<'a> {
    pub fn new(table: &'a mut SymbolTable, isa_backend: &'static dyn ISABackend) -> Self {
        let session = Rc::new(RefCell::new(Session::new(isa_backend)));
        let mut free_regs = vec![];
        let param_reg_num = isa_backend.preg_num();
        for i in 0..param_reg_num {
            free_regs.push(get_reg(isa_backend, DetailedRegisterKind::Param, i))
        }
        let saved_reg_num = isa_backend.sreg_num();
        for i in 0..saved_reg_num {
            free_regs.push(get_reg(isa_backend, DetailedRegisterKind::Saved, i))
        }
        let temp_reg_num = isa_backend.treg_num();
        // t0 寄存器用于 远距离链接寄存器
        for i in 1..temp_reg_num {
            free_regs.push(get_reg(isa_backend, DetailedRegisterKind::Temp, i))
        }
        let fparam_reg_num = isa_backend.fpreg_num();
        for i in 0..fparam_reg_num {
            free_regs.push(get_reg(isa_backend, DetailedRegisterKind::FParam, i))
        }
        let fsaved_reg_num = isa_backend.fsreg_num();
        for i in 0..fsaved_reg_num {
            free_regs.push(get_reg(isa_backend, DetailedRegisterKind::FSaved, i))
        }
        let ftemp_reg_num = isa_backend.ftreg_num();
        for i in 0..ftemp_reg_num {
            free_regs.push(get_reg(isa_backend, DetailedRegisterKind::FTemp, i))
        }

        let register_helper2 = RegisterHelper2::stack_16_init(free_regs.clone(), MutableRef::from_ref(session.clone()));
        Self {
            symbol_table: table,
            register_helper2,
            session,
            backend: isa_backend,
            available_regs: free_regs,
        }
    }

    pub fn translate3(&mut self, mir: MIRModule, ref_pool: MIRRefPool) -> LirPartCode {
        // 与其他的翻译方式不同，Translate3 从MIR模块的级别翻译
        // 而不是函数或者其他
        MIRTranslate3::translate(mir, ref_pool)
    }

    pub fn translate2(&mut self, mut mir: MIRModule, mut ref_pool: MIRRefPool) -> LirPartCode {
        let mut translate = MIRFunTranslate2::create(
            &mut ref_pool,
            self.symbol_table,
            &mut self.register_helper2,
            MutableRef::from_ref(self.session.clone()),
            self.backend
        );
        let free_regs = self.available_regs.clone();
        let units = mir.take_units();
        for unit in units {
            match unit {
                MIRUnit::Fun(fun) => {
                    translate.translate_fun(fun, free_regs.clone());
                }
                MIRUnit::Def(def) => translate.translate_gldef(def),
            }
        }
        self.session.borrow_mut().finish()
    }
}