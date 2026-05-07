use crate::asm::ISABackend;
use crate::ast::table::SymbolTable;
use crate::lir::lir::LirStatement;

#[allow(unused)]
pub struct LoongArchBackend;

impl ISABackend for LoongArchBackend {
    fn translate_stmt(&self, _lir: LirStatement, _name_table: &SymbolTable) -> String {
        todo!()
    }
}
