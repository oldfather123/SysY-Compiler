#![allow(unused)]
use crate::lir::lir::LirPartCode;
use crate::mir::{MIRModule, MIRRefPool};

mod allocator;
mod session;
mod translate3;
mod stacker;

// the driver of translate3


pub struct MIRTranslate3 {

}

impl MIRTranslate3 {

    // t3 的入口

    pub fn translate(mir: MIRModule, ref_pool: MIRRefPool) -> LirPartCode {
        // 先取得寄存器分配方案
        // 然后计算生成LIR

        todo!()
    }
}
