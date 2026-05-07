//! AST简单单元测试
//! 验证AST基础功能正常工作

use super::ast::*;

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_binary_expression() {
        let mut ctx = AstContext::new();

        // 1 + 2
        let left = ctx.add_expr(Expr::new(ExprKind::Int(1)));
        let right = ctx.add_expr(Expr::new(ExprKind::Int(2)));
        let binary = Expr::new(ExprKind::Binary {
            lhs: left,
            op: BinOp::Add,
            rhs: right,
        });
        let binary_ref = ctx.add_expr(binary);

        let retrieved = ctx.get_expr(binary_ref).unwrap();
        match &retrieved.kind {
            ExprKind::Binary { lhs, op, rhs } => {
                assert_eq!(*op, BinOp::Add);

                let left_expr = ctx.get_expr(*lhs).unwrap();
                let right_expr = ctx.get_expr(*rhs).unwrap();

                match (&left_expr.kind, &right_expr.kind) {
                    (ExprKind::Int(1), ExprKind::Int(2)) => {}
                    _ => panic!("Unexpected expression values"),
                }
            }
            _ => panic!(""),
        }
    }

    #[test]
    fn test_variable_declaration() {
        let mut ctx = AstContext::new();

        // int x = 5;
        let init_expr = ctx.add_expr(Expr::new(ExprKind::Int(5)));
        let var_item = VarDefItem::new(
            "x".to_string(),
            TypeNode::new(BaseType::Int, vec![]),
            Some(init_expr),
        );
        let var_decl = VarDecl::new(BaseType::Int, vec![var_item]);
        let decl = Decl::new(DeclKind::Var(var_decl));
        let decl_ref = ctx.add_decl(decl);

        let retrieved = ctx.get_decl(decl_ref).unwrap();
        match &retrieved.kind {
            DeclKind::Var(var_decl) => {
                assert_eq!(var_decl.base_type, BaseType::Int);
                assert_eq!(var_decl.items.len(), 1);
                assert_eq!(var_decl.items[0].name, "x");
                assert!(var_decl.items[0].init.is_some());
            }
            _ => panic!(""),
        }
    }
}
