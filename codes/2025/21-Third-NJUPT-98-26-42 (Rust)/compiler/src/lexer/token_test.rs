//! Token 单元测试

#[cfg(test)]
mod tests {
    use super::super::token::*;
    use super::super::token_impl::Lexer;

    #[test]
    fn test_keywords() {
        let source = "int float void const if else while break continue return".to_string();
        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        let expected_types = [
            TokenType::Int,
            TokenType::Float,
            TokenType::Void,
            TokenType::Const,
            TokenType::If,
            TokenType::Else,
            TokenType::While,
            TokenType::Break,
            TokenType::Continue,
            TokenType::Return,
            TokenType::Eof,
        ];

        assert_eq!(tokens.len(), expected_types.len());
        for (token, expected) in tokens.iter().zip(expected_types.iter()) {
            assert_eq!(token.kind, *expected);
        }
    }

    #[test]
    fn test_operators() {
        let source = "+ - * / % < <= > >= == != && || ! =".to_string();
        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        let expected_types = [
            TokenType::Plus,
            TokenType::Minus,
            TokenType::Star,
            TokenType::Slash,
            TokenType::Percent,
            TokenType::Lt,
            TokenType::Le,
            TokenType::Gt,
            TokenType::Ge,
            TokenType::Eq,
            TokenType::Ne,
            TokenType::And,
            TokenType::Or,
            TokenType::Not,
            TokenType::Assign,
            TokenType::Eof,
        ];

        assert_eq!(tokens.len(), expected_types.len());
        for (token, expected) in tokens.iter().zip(expected_types.iter()) {
            assert_eq!(token.kind, *expected);
        }
    }

    #[test]
    fn test_symbols() {
        let source = "( ) [ ] { } ; ,".to_string();
        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        let expected_types = [
            TokenType::LParen,
            TokenType::RParen,
            TokenType::LBrack,
            TokenType::RBrack,
            TokenType::LBrace,
            TokenType::RBrace,
            TokenType::Semi,
            TokenType::Comma,
            TokenType::Eof,
        ];

        assert_eq!(tokens.len(), expected_types.len());
        for (token, expected) in tokens.iter().zip(expected_types.iter()) {
            assert_eq!(token.kind, *expected);
        }
    }

    #[test]
    fn test_numbers() {
        let source = "123 456 3.14 0.5 42.0 0x1a 0XFF 0xb".to_string();
        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        let expected = [
            (TokenType::IntLit, "123"),
            (TokenType::IntLit, "456"),
            (TokenType::FloatLit, "3.14"),
            (TokenType::FloatLit, "0.5"),
            (TokenType::FloatLit, "42.0"),
            (TokenType::IntLit, "0x1a"),
            (TokenType::IntLit, "0XFF"),
            (TokenType::IntLit, "0xb"),
        ];

        assert_eq!(tokens.len(), expected.len() + 1); // +1 for EOF
        for (i, (expected_type, expected_text)) in expected.iter().enumerate() {
            assert_eq!(tokens[i].kind, *expected_type);
            assert_eq!(tokens[i].text, *expected_text);
        }
    }

    #[test]
    fn test_identifiers() {
        let source = "variable_name func123 _private".to_string();
        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        let expected = [
            (TokenType::Ident, "variable_name"),
            (TokenType::Ident, "func123"),
            (TokenType::Ident, "_private"),
        ];

        assert_eq!(tokens.len(), expected.len() + 1); // +1 for EOF
        for (i, (expected_type, expected_text)) in expected.iter().enumerate() {
            assert_eq!(tokens[i].kind, *expected_type);
            assert_eq!(tokens[i].text, *expected_text);
        }
    }

    #[test]
    fn test_comments() {
        let source = "int x; // line comment\n/* block comment */ float y;".to_string();
        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        let expected_types = [
            TokenType::Int,
            TokenType::Ident,
            TokenType::Semi,
            TokenType::Float,
            TokenType::Ident,
            TokenType::Semi,
            TokenType::Eof,
        ];

        assert_eq!(tokens.len(), expected_types.len());
        for (token, expected) in tokens.iter().zip(expected_types.iter()) {
            assert_eq!(token.kind, *expected);
        }
    }

    #[test]
    fn test_simple_program() {
        let source = r#"
int main() {
    int x = 42;
    if (x > 0) {
        return x + 1;
    }
    return 0;
}
"#
        .to_string();

        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        // 验证基本结构
        assert!(tokens.len() > 20); // 至少有这么多token
        assert!(tokens.last().unwrap().is_eof());

        // 验证包含预期的token类型
        let token_types: Vec<_> = tokens.iter().map(|t| &t.kind).collect();
        assert!(token_types.contains(&&TokenType::Int));
        assert!(token_types.contains(&&TokenType::If));
        assert!(token_types.contains(&&TokenType::Return));
        assert!(token_types.contains(&&TokenType::LBrace));
        assert!(token_types.contains(&&TokenType::RBrace));
    }

    #[test]
    fn test_token_position() {
        let source = "int x = 42;".to_string();
        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        // 验证第一个token (int) 的位置
        assert_eq!(tokens[0].pos.line, 1);
        assert_eq!(tokens[0].pos.column, 1);
        assert_eq!(tokens[0].pos.offset, 0);
        assert_eq!(tokens[0].pos.len, 3);

        // 验证标识符 x 的位置
        assert_eq!(tokens[1].pos.line, 1);
        assert_eq!(tokens[1].pos.column, 5);
        assert_eq!(tokens[1].pos.offset, 4);
        assert_eq!(tokens[1].pos.len, 1);
    }

    #[test]
    fn test_error_handling() {
        let source = "int x = @;".to_string(); // @ 是无效字符
        let mut lexer = Lexer::new(source);
        let result = lexer.tokenize();

        assert!(result.is_err());
        let error = result.unwrap_err();
        assert!(error.to_string().contains("Unexpected character"));
    }

    #[test]
    fn test_unterminated_block_comment() {
        let source = "int x; /* unterminated comment".to_string();
        let mut lexer = Lexer::new(source);
        let result = lexer.tokenize();

        assert!(result.is_err());
        let error = result.unwrap_err();
        assert!(error.to_string().contains("Unterminated block comment"));
    }

    #[test]
    fn test_float_literals() {
        let source = r#"
            const float RADIUS = 5.5, PI = 03.141592653589793, EPS = 1e-6;
            const float PI_HEX = 0x1.921fb6p+1, HEX2 = 0x.AP-3;
            const float FACT = -.33E+5, EVAL1 = PI * RADIUS * RADIUS, EVAL2 = 2 * PI_HEX * RADIUS, EVAL3 = PI * 2 * RADIUS;
            const float CONV1 = 233, CONV2 = 0xfff;
            const int MAX = 1e9, TWO = 2.9, THREE = 3.2, FIVE = TWO + THREE;
        "#
        .to_string();

        let mut lexer = Lexer::new(source);
        let tokens = lexer.tokenize().unwrap();

        let expected = vec![
            // const float RADIUS = 5.5, PI = 03.141592653589793, EPS = 1e-6;
            (TokenType::Const, "const"),
            (TokenType::Float, "float"),
            (TokenType::Ident, "RADIUS"),
            (TokenType::Assign, "="),
            (TokenType::FloatLit, "5.5"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "PI"),
            (TokenType::Assign, "="),
            (TokenType::FloatLit, "03.141592653589793"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "EPS"),
            (TokenType::Assign, "="),
            (TokenType::FloatLit, "1e-6"),
            (TokenType::Semi, ";"),
            // const float PI_HEX = 0x1.921fb6p+1, HEX2 = 0x.AP-3;
            (TokenType::Const, "const"),
            (TokenType::Float, "float"),
            (TokenType::Ident, "PI_HEX"),
            (TokenType::Assign, "="),
            (TokenType::FloatLit, "0x1.921fb6p+1"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "HEX2"),
            (TokenType::Assign, "="),
            (TokenType::FloatLit, "0x.AP-3"),
            (TokenType::Semi, ";"),
            // const float FACT = -.33E+5, EVAL1 = PI * RADIUS * RADIUS, EVAL2 = 2 * PI_HEX *
            (TokenType::Const, "const"),
            (TokenType::Float, "float"),
            (TokenType::Ident, "FACT"),
            (TokenType::Assign, "="),
            (TokenType::Minus, "-"),
            (TokenType::FloatLit, ".33E+5"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "EVAL1"),
            (TokenType::Assign, "="),
            (TokenType::Ident, "PI"),
            (TokenType::Star, "*"),
            (TokenType::Ident, "RADIUS"),
            (TokenType::Star, "*"),
            (TokenType::Ident, "RADIUS"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "EVAL2"),
            (TokenType::Assign, "="),
            (TokenType::IntLit, "2"),
            (TokenType::Star, "*"),
            (TokenType::Ident, "PI_HEX"),
            (TokenType::Star, "*"),
            (TokenType::Ident, "RADIUS"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "EVAL3"),
            (TokenType::Assign, "="),
            (TokenType::Ident, "PI"),
            (TokenType::Star, "*"),
            (TokenType::IntLit, "2"),
            (TokenType::Star, "*"),
            (TokenType::Ident, "RADIUS"),
            (TokenType::Semi, ";"),
            // const float CONV1 = 233, CONV2 = 0xfff;
            (TokenType::Const, "const"),
            (TokenType::Float, "float"),
            (TokenType::Ident, "CONV1"),
            (TokenType::Assign, "="),
            (TokenType::IntLit, "233"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "CONV2"),
            (TokenType::Assign, "="),
            (TokenType::IntLit, "0xfff"),
            (TokenType::Semi, ";"),
            // const int MAX = 1e9, TWO = 2.9, THREE = 3.2, FIVE = TWO + THREE;
            (TokenType::Const, "const"),
            (TokenType::Int, "int"),
            (TokenType::Ident, "MAX"),
            (TokenType::Assign, "="),
            (TokenType::FloatLit, "1e9"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "TWO"),
            (TokenType::Assign, "="),
            (TokenType::FloatLit, "2.9"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "THREE"),
            (TokenType::Assign, "="),
            (TokenType::FloatLit, "3.2"),
            (TokenType::Comma, ","),
            (TokenType::Ident, "FIVE"),
            (TokenType::Assign, "="),
            (TokenType::Ident, "TWO"),
            (TokenType::Plus, "+"),
            (TokenType::Ident, "THREE"),
            (TokenType::Semi, ";"),
        ];

        assert_eq!(tokens.len(), expected.len() + 1, "Mismatch in token count"); // +1 for EOF
        for (i, (expected_type, expected_text)) in expected.iter().enumerate() {
            let token = &tokens[i];
            assert_eq!(
                token.kind, *expected_type,
                "Token kind mismatch at index {}",
                i
            );
            assert_eq!(
                token.text, *expected_text,
                "Token text mismatch for {:?} at index {}",
                token.kind, i
            );
        }
        assert!(tokens.last().unwrap().is_eof());
    }
}
