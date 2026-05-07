#[test]
fn test_riscv_load() {
    use crate::asm::riscv::RISCVBackend;
    use crate::asm::ASMTranslate;
    use crate::ast::table::SymbolTable;
    use crate::lir::lir::{
        HRegister::*, LirStatement, RegWithOff, RegWithOffOrImm, WidthType, LOADINS,
    };

    let mut translate = ASMTranslate {
        translate_backend: &RISCVBackend,
        name_table: &SymbolTable::new(),
    };
    let source = translate.translate_lir(
        LirStatement::Load(LOADINS(
            WidthType::Word,
            p0,
            RegWithOffOrImm::RegWO(RegWithOff::suggest_fp_bottom_based(fp, 0)),
        )),
        None,
    );
    assert_eq!("    lw a0,0(fp)", source)
}
