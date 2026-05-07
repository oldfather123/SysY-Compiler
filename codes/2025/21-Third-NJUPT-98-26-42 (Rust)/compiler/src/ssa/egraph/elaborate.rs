use log::warn;

use super::{EgraphCtx, REWRITE_ITERS};
use crate::prelude::CEResult;

impl<'a> EgraphCtx<'a> {
    pub(super) fn elaborate(&mut self) -> CEResult<()> {
        Ok(())
    }

    fn elaborate_bb(&mut self) -> CEResult<()> {
        Ok(())
    }
}
