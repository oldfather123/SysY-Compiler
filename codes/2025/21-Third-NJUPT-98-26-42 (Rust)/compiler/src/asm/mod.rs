pub mod cfg;
pub mod ctx;
pub mod generated;
pub mod index;
pub mod inst;
pub mod liveness;
pub mod loop_analysis;
pub mod lowering;
pub mod prog;
pub mod reg;
pub mod reg_alloc;
pub mod reg_irc;
pub mod reg_spill;
pub mod stack;

use crate::{asm::lowering::lower_module, prelude::*};

pub use prog::RvProg;

#[derive(Debug, derive_new::new)]
pub struct AsmPass;

impl CompilerPass for AsmPass {
    fn name(&self) -> &'static str {
        "asm"
    }

    fn run(&mut self, ctx: &mut CompilerContext) -> CEResult<()> {
        let core_ctx = ctx
            .core_ctx
            .as_ref()
            .ok_or_else(|| CErr::Internal("No SSA core context available".to_string()))?;

        let func_ctxs = ctx
            .func_ctxs
            .as_ref()
            .ok_or_else(|| CErr::Internal("No SSA function contexts available".to_string()))?;

        let (vprog, irc_failed) = lower_module(core_ctx, func_ctxs, &ctx.settings.reg_alloc_debug)?;
        ctx.rvprog = Some(vprog);
        ctx.irc_failed = irc_failed; // 设置IRC失败标志
        Ok(())
    }

    fn stage(&self) -> CompilerStage {
        CompilerStage::ASM
    }

    fn write_output(&self, ctx: &CompilerContext, writer: &mut dyn std::io::Write) -> CEResult<()> {
        let rvprog = ctx
            .rvprog
            .as_ref()
            .ok_or_else(|| CErr::asm_err("No RvProg available"))?;
        write!(writer, "{}", rvprog).map_err(|e| CErr::asm_err(format!("Write error: {}", e)))
    }
}
