// 语法检查和HIR转换
use crate::ast::table::{SymbolTable, VariableLayer};
use crate::ast::AST;
use crate::hir::hir::HirModule;
use crate::session::CompileSession;

pub struct AstChecker<'a> {
    variable_pool: VariableLayer<'a>,
}

impl<'a> AstChecker<'a> {
    pub fn new() -> AstChecker<'a> {
        AstChecker {
            variable_pool: VariableLayer::new(),
        }
    }
    pub fn analyse(
        &mut self,
        ast: AST,
        session: &mut CompileSession,
        name_table: &mut SymbolTable,
    ) -> Result<HirModule, ()> {
        let mut hir = HirModule::new();
        for ast_unit in ast.units {
            let hir_unit = ast_unit.to_hir(&mut self.variable_pool, name_table, session);
            if hir_unit.is_err() {
                return Err(());
            }
            hir.add_unit(hir_unit.unwrap());
        }
        Ok(hir)
    }
}
