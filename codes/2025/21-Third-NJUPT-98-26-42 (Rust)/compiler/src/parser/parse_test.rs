//! Parser 单元测试

#[cfg(test)]
mod tests {
    use super::super::ast::*;
    use super::super::parse::Parser;
    use crate::lexer::{Lexer, Token};

    /// util -- 临时Lexer
    fn tokenize_source(source: &str) -> Vec<Token> {
        let mut lexer = Lexer::new(source.to_string());
        lexer.tokenize().expect("Lexing should succeed")
    }

    /// util -- 临时AST上下文
    fn parse_source(source: &str) -> (AstContext, CompUnitRef) {
        let tokens = tokenize_source(source);
        let parser = Parser::new(tokens);
        parser.parse().expect("Parsing should succeed")
    }

    #[test]
    fn test_parse_complex_program() {
        let source = r#"
            const int N = 100;
            int factorial(int n) {
                if (n <= 1) {
                    return 1;
                } else {
                    return n * factorial(n - 1);
                }
            }
            
            int main() {
                int result = factorial(5);
                return result;
            }
        "#;

        let (ctx, root) = parse_source(source);
        let comp_unit = ctx.comp_units.get(root).unwrap();

        assert_eq!(comp_unit.items.len(), 3);

        // const N
        let first_decl = ctx.decls.get(comp_unit.items[0]).unwrap();
        assert!(matches!(first_decl.kind, DeclKind::Const(_)));

        // factorial
        let second_decl = ctx.decls.get(comp_unit.items[1]).unwrap();
        if let DeclKind::Function(func_decl) = &second_decl.kind {
            assert_eq!(func_decl.name, "factorial");
            assert_eq!(func_decl.params.len(), 1);
        } else {
            panic!("Expected function declaration");
        }

        // main
        let third_decl = ctx.decls.get(comp_unit.items[2]).unwrap();
        if let DeclKind::Function(func_decl) = &third_decl.kind {
            assert_eq!(func_decl.name, "main");
            assert!(func_decl.params.is_empty());
        } else {
            panic!("Expected function declaration");
        }
    }

    #[test]
    fn test_parse_error_invalid_expression() {
        let source = "int main() { return +; }";
        let tokens = tokenize_source(source);
        let parser = Parser::new(tokens);
        let result = parser.parse();

        assert!(result.is_err());
    }

    #[test]
    fn test_parse_complex_array_declaration() {
        let source = "int matrix[5 * 2][3 + 1];"; // 常量表达式计算
        let (ctx, root) = parse_source(source);

        let comp_unit = ctx.comp_units.get(root).unwrap();
        let decl = ctx.decls.get(comp_unit.items[0]).unwrap();

        if let DeclKind::Var(var_decl) = &decl.kind {
            assert_eq!(var_decl.items[0].name, "matrix");
            assert_eq!(var_decl.items[0].ty.dimensions.len(), 2);
            assert_eq!(var_decl.items[0].ty.dimensions[0], Some(10)); // 5 * 2 = 10
            assert_eq!(var_decl.items[0].ty.dimensions[1], Some(4)); // 3 + 1 = 4
        } else {
            panic!("Expected variable declaration");
        }
    }
}
