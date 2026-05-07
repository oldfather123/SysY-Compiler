#[test]
fn test_parse_type() {
    use crate::lexer::cursor::Cursor;
    use crate::lexer::TokenKind::*;
    let mut cursor = Cursor::new("int float void ");
    loop {
        let token = cursor.advance_token();
        match token.kind {
            Ident => {}
            Whitespace => {}
            Eof => {
                return;
            }
            _ => {
                panic!("Error parse type {:?}", token.kind)
            }
        }
    }
}

#[test]
fn test_parse_one_char() {
    use crate::lexer::cursor::Cursor;
    use crate::lexer::TokenKind::*;
    let mut cursor = Cursor::new("()[],.{};=!<> +-*/% | & ,");
    loop {
        let token = cursor.advance_token();
        match token.kind {
            Semi | OpenBrace | CloseBrace | Comma | Dot | OpenBracket | CloseBracket
            | OpenParen | CloseParen | Plus | Minus | Star | And | Or | Eq | Bang | Lt | Gt
            | Slash | Percent => {
                let _ = cursor.now();
                //println!("{}", now)
            }
            Whitespace => {}
            Eof => {
                return;
            }
            _ => {
                panic!("Error parse type {:?}", token.kind)
            }
        }
    }
}

#[test]
fn test_unknown() {
    use crate::lexer::cursor::Cursor;
    use crate::lexer::TokenKind::*;
    let mut cursor = Cursor::new("√œ∑π˚∂åßµµΩ≥≤µç˚¬ƒ˚∆˚®˜˜˜˜¬˚\u{00a1}");
    loop {
        let token = cursor.advance_token();
        match token.kind {
            Unknown => {
                let _ = cursor.now();
                // println!("{}", now)
            }
            Whitespace => {}
            Eof => {
                return;
            }
            _ => {
                panic!("Error parse type {:?}", token.kind)
            }
        }
    }
}

#[test]
fn test_comment() {
    use crate::lexer::cursor::Cursor;
    use crate::lexer::TokenKind::*;
    let mut cursor = Cursor::new("// this is test \n /* */ \n /*  */");
    loop {
        let token = cursor.advance_token();
        match token.kind {
            LineComment => {
                let _ = cursor.now();
                // println!("{}", now)
            }
            BlockComment => {
                let _ = cursor.now();
                // println!("{}", now)
            }
            Whitespace => {}
            Eof => {
                return;
            }
            _ => {
                panic!("Error parse type {:?}", token.kind)
            }
        }
    }
}

#[test]
fn test_literal() {
    use crate::lexer::cursor::Cursor;
    use crate::lexer::Base::Hexadecimal;
    use crate::lexer::LiteralKind;
    use crate::lexer::TokenKind::*;

    let mut cursor = Cursor::new(".33e5 0x.AP-3 100101 0xffff 01234567 012345678  10.0 0.e10 0.1e10 0xfff 0xaffep10 0x 0x0p 0e 0e1 1. 0x1p+10 0x1p-10  ");
    loop {
        let token = cursor.advance_token();
        match token.kind {
            Literal { kind } => {
                match kind {
                    LiteralKind::Int { .. } => {
                        // println!("INT {:?} , {}", base, empty_int);
                    }
                    LiteralKind::Float { .. } => {
                        // println!("FLOAT {:?} {} {}", base, empty_exponent,missing_exponent);
                    }
                }
            }
            Whitespace => {}
            Eof => {
                break;
            }
            _ => {
                panic!("Error parse type {:?}", token.kind)
            }
        }
    }
    cursor = Cursor::new("0x1e+10");
    let mut token = cursor.advance_token();
    match token.kind {
        Literal { .. } => {}
        _ => {
            panic!("error {:?}", token.kind);
        }
    };

    token = cursor.advance_token();
    match token.kind {
        Plus { .. } => {}
        _ => {
            panic!("error {:?}", token.kind);
        }
    };

    token = cursor.advance_token();
    match token.kind {
        Literal { .. } => {}
        _ => {
            panic!("error {:?}", token.kind);
        }
    };

    cursor = Cursor::new("0.f 0xf.");
    token = cursor.advance_token();
    match token.kind {
        Literal { .. } => {}
        _ => {
            panic!("error {:?}", token.kind);
        }
    };
    token = cursor.advance_token();
    match token.kind {
        Ident { .. } => {}
        _ => {
            panic!("error {:?}", token.kind);
        }
    }
    cursor.advance_token(); // Whitespace
    token = cursor.advance_token();
    match token.kind {
        Literal { kind } => match kind {
            LiteralKind::Int { .. } => {}
            LiteralKind::Float {
                base,
                empty_exponent,
                missing_exponent,
                missing_dot_after,
            } => {
                assert_eq!(base, Hexadecimal);
                assert!(empty_exponent);
                assert!(missing_exponent)
            }
        },
        _ => {}
    }
}
