use std::fmt::{Display, Formatter};

use super::{CoreContext, FuncContext, ir_fmt::FunctionFormatter};
use crate::prelude::CEResult;

pub mod datatype;
pub use datatype::Egraph;
pub mod canonicalize;
pub mod elaborate;

const REWRITE_ITERS: usize = 100000;

pub fn egraph(func_ctx: &mut FuncContext, core_ctx: &mut CoreContext) -> CEResult<()> {
    // let mut ctx = EgraphCtx::new(func_ctx, core_ctx);
    // ctx.init_egraph()?;
    // eprintln!("{}", ctx);
    // std::process::exit(0);
    // ctx.saturate()?;
    // ctx.elaborate()?;
    Ok(())
}

use indoc::writedoc;
pub struct EgraphCtx<'a> {
    egraph: Egraph,
    func_ctx: &'a mut FuncContext,
    core_ctx: &'a mut CoreContext,
}

impl<'a> EgraphCtx<'a> {
    pub fn new(func_ctx: &'a mut FuncContext, core_ctx: &'a mut CoreContext) -> Self {
        Self {
            egraph: Egraph::new(),
            func_ctx,
            core_ctx,
        }
    }
}

impl<'a> Display for EgraphCtx<'a> {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let func_fmt = FunctionFormatter::new(&self.func_ctx.func, &self.core_ctx);

        write!(f, "digraph {{\n")?;
        for (eclass, values) in self.egraph.eclasses.iter_all() {
            writedoc!(
                f,
                "
                subgraph cluster_ec{0} {{
                  label=ec{0};
                  style=filled;
                  color=lightgrey;
                  node [style=filled,color=white];
                ",
                eclass
            )?;
            for value in values.0.iter().cloned() {
                let label = func_fmt.format_value(value);
                use crate::ssa::ir::ValueData::*;
                match self.func_ctx.func.dfg.valuedata(value) {
                    Const { .. } => write!(f, "  v{} [label={:?}];\n", value.to_string(), label)?,
                    Inst { inst, .. } => {
                        write!(f, "  v{} [label={:?}];\n", value.to_string(), label)?;
                        let inst = inst.clone();
                        let inst_data = &self.func_ctx.func.dfg.insts[inst];
                        use crate::ssa::ir::InstructionData::*;
                        match inst_data {
                            Binary { op, lhs, rhs } => {
                                write!(f, "  v{} -> v{};\n", value.to_string(), lhs.to_string())?;
                                write!(f, "  v{} -> v{};\n", value.to_string(), rhs.to_string())?;
                            }
                            Unary { op, val } => {
                                write!(f, "  v{} -> v{};\n", value.to_string(), val.to_string())?;
                            }
                            Cast { val, to } => {
                                write!(f, "  v{} -> v{};\n", value.to_string(), val.to_string())?;
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
                            | Unreachable => {
                                unimplemented!()
                            }
                        }
                    }
                    BlockParam { .. } => {
                        write!(f, "  v{} [label={:?}];\n", value, label)?;
                    }
                    GlobalSymbol { .. } => {
                        write!(f, "  v{} [label={:?}];\n", value, label)?;
                    }
                    Alias { original, .. } => {
                        write!(f, "  v{} [label={:?}];\n", value, label)?;
                        write!(f, "  v{} -> v{};\n", value, original)?;
                    }
                    Union { .. } => unimplemented!(),
                }
            }
            write!(f, "}}\n")?;
        }
        write!(f, "}}\n")?;
        Ok(())
    }
}
