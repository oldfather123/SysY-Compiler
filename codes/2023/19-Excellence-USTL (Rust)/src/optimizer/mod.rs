use crate::hir::hir::HirModule;
use crate::mir::{MIRBlock, MIRFun, MIRModule, MIRStmt, MIRUnit};
use crate::util::HashInsertVec;

mod mir_copy_minimal;
mod mir_insert_drop;
mod mir_comm_right_remove;

pub use mir_copy_minimal::MIRRemoveLazyRight;
pub use mir_insert_drop::MIRInsertDrop;
pub use mir_comm_right_remove::MIRCommRightRemove;

#[derive(Copy, Clone, Ord, PartialOrd, Eq, PartialEq)]
pub enum OptLevel {
    // 不优化
    O0 = 0,
    // 简单优化
    O1 = 1,
    // SSA 级别优化
    O2 = 2,
}

pub trait MIRBlockOptPass {
    fn pass(&mut self, input: MIRBlock) -> MIRBlock;
}

pub trait MIRFunOptPass {
    fn pass(&mut self, input: MIRFun) -> MIRFun;
}

pub trait PassInstance {
    fn instance() -> Self;
}

trait HIROptPass {}

pub struct HirOpt {
    hir_module: HirModule,
}

pub struct MirOpt {
    mir_module: MIRModule,
}

impl MirOpt {
    pub fn input(hir: MIRModule) -> Self {
        Self { mir_module: hir }
    }

    pub fn pass_fun(&mut self, mut passer: impl MIRFunOptPass) {
        for units in self.mir_module.take_units() {
            match units {
                MIRUnit::Fun(fun) => self.mir_module.add_unit(MIRUnit::Fun(passer.pass(fun))),
                MIRUnit::Def(def) => self.mir_module.add_unit(MIRUnit::Def(def)),
            }
        }
    }

    pub fn pass_block(&mut self, mut passer: impl MIRBlockOptPass) {
        for units in self.mir_module.take_units() {
            match units {
                MIRUnit::Fun(mut fun) => {
                    let mut new_fun = MIRFun::new(fun.get_name().clone());
                    let blocks = fun.take_blocks();
                    new_fun.params = fun.params;
                    match fun.ret_type {
                        None => {}
                        Some(ret) => new_fun.set_ret_type(ret),
                    }
                    for block in  blocks{
                        let block = passer.pass(block);
                        new_fun.add_block(block);
                    }
                    self.mir_module.add_unit(MIRUnit::Fun(new_fun))
                }
                MIRUnit::Def(def) => self.mir_module.add_unit(MIRUnit::Def(def)),
            }
        }
    }

    pub fn output(self) -> MIRModule {
        self.mir_module
    }
}

impl HirOpt {
    pub fn input(hir: HirModule, _opt_level: OptLevel) -> Self {
        Self { hir_module: hir }
    }
    #[allow(unused)]
    fn pass(&mut self, passer: impl HIROptPass) {
        // opt
    }

    pub fn output(self) -> HirModule {
        self.hir_module
    }
}

impl MIRBlock {
    pub(crate) fn to_link_list(mut self) -> HashInsertVec<MIRStmt> {
        self.take_stmts()
    }
}
