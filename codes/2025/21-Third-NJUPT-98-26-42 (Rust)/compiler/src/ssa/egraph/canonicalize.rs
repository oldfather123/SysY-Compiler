use itertools::Itertools;
use log::warn;

use super::{EgraphCtx, REWRITE_ITERS};
use crate::{
    prelude::*,
    ssa::{Cursor, FuncCursor},
};

impl<'a> EgraphCtx<'a> {
    pub(super) fn init_egraph(&mut self) -> CEResult<()> {
        let mut cursor = FuncCursor::new(&mut self.func_ctx.func, self.core_ctx);
        while let Some(block) = cursor.next_block() {
            for param in cursor.func.dfg.block_params(block).iter().cloned() {
                self.egraph.insert(param);
            }
            while let Some(inst) = cursor.next_inst() {
                let inst_data = cursor.func.dfg.insts[inst];
                use crate::ssa::ir::InstructionData::*;
                match inst_data {
                    Binary { lhs, rhs, .. } => {
                        self.egraph.ensure(lhs);
                        self.egraph.ensure(rhs);
                        let value = cursor.func.dfg.first_result(inst);
                        self.egraph.insert(value);
                    }
                    Unary { val, .. } => {
                        self.egraph.ensure(val);
                        let value = cursor.func.dfg.first_result(inst);
                        self.egraph.insert(value);
                    }
                    Cast { val, to } => {
                        self.egraph.ensure(val);
                        let value = cursor.func.dfg.first_result(inst);
                        self.egraph.insert(value);
                    }
                    Call { .. }
                    | ArrayAlloc { .. }
                    | ArrayGet { .. }
                    | ArrayPut { .. }
                    | ArraySlice { .. }
                    | StackAddr { .. }
                    | GlobalAddr { .. }
                    | Load { .. }
                    | Store { .. }
                    | GlobalScalarGet { .. }
                    | GlobalScalarSet { .. }
                    | Jump { .. }
                    | BranchTable { .. }
                    | Brif { .. }
                    | Return { .. }
                    | ReturnCall { .. }
                    | Unreachable => {}
                }
            }
        }
        Ok(())
    }

    pub(super) fn saturate(&mut self) -> CEResult<()> {
        for _ in 0..REWRITE_ITERS {
            if self.rewrite_once()? {
                continue;
            }
            return Ok(());
        }
        warn!("failed to saturate egraph");
        Ok(())
    }

    /// returns whether egraph has been changed
    fn rewrite_once(&mut self) -> CEResult<bool> {
        Ok(false)
    }
}
