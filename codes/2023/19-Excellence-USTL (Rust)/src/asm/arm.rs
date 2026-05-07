#![allow(unused)]

use crate::asm::ISABackend;
use crate::ast::table::SymbolTable;
use crate::lir::lir::{HRegister, LirStatement};

pub struct AArch64Backend;

impl ISABackend for AArch64Backend {
    fn translate_stmt(&self, lir: LirStatement, name_table: &SymbolTable) -> String {
        todo!()
    }

    fn support_zero(&self) -> bool {
        todo!()
    }

    fn support_fp(&self) -> bool {
        todo!()
    } // x28

    fn preg_num(&self) -> u8 {
        todo!()
    }

    fn sreg_num(&self) -> u8 {
        todo!()
    }

    fn treg_num(&self) -> u8 {
        todo!()
    }

    fn fsreg_num(&self) -> u8 {
        todo!()
    }

    fn ftreg_num(&self) -> u8 {
        todo!()
    }

    fn fpreg_num(&self) -> u8 {
        todo!()
    }

    fn get_reg_name(&self, which: HRegister) -> String {
        todo!()
    }
}

pub struct Arm9Backend;

impl ISABackend for Arm9Backend {
    fn translate_stmt(&self, lir: LirStatement, name_table: &SymbolTable) -> String {
        todo!()
    }

    fn support_zero(&self) -> bool {
        todo!()
    }

    fn support_fp(&self) -> bool {
        // x28
        todo!()
    }

    fn preg_num(&self) -> u8 {
        todo!()
    }

    fn sreg_num(&self) -> u8 {
        todo!()
    }

    fn treg_num(&self) -> u8 {
        todo!()
    }

    fn fsreg_num(&self) -> u8 {
        todo!()
    }

    fn ftreg_num(&self) -> u8 {
        todo!()
    }

    fn fpreg_num(&self) -> u8 {
        todo!()
    }

    fn get_reg_name(&self, which: HRegister) -> String {
        todo!()
    }
}
