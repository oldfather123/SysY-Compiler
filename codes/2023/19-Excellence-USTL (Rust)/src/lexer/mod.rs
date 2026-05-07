/// 思路来自 [RUSTC](https://github.com/rust-lang/rust.git)
/// 部分函数来自RUSTC
mod cursor;
mod reader;
pub mod symbol;
pub mod token;

mod tests;

use cursor::Cursor;
use Base::{Decimal, Hexadecimal, Octal};
use LiteralKind::*;
use TokenKind::*;

pub use reader::TokenReader;

#[derive(Debug)]
struct SimpleToken {
    pub kind: TokenKind,
    pub len: u32,
}

impl SimpleToken {
    fn new(kind: TokenKind, len: u32) -> SimpleToken {
        SimpleToken { kind, len }
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum TokenKind {
    /// Multi-char tokens:
    LineComment,
    BlockComment,
    Whitespace,
    Ident,

    /// Examples: `12u8`, `1.0e-40`, `b"123"`. Note that `_` is an invalid
    /// suffix, but may be present here on string and float literals. Users of
    /// this type will need to check for and reject that case.
    ///
    /// See [LiteralKind] for more details.
    Literal {
        kind: LiteralKind,
    },

    // One-char tokens:
    /// ";"
    Semi,
    /// ","
    Comma,
    /// "."
    Dot,
    /// "("
    OpenParen,
    /// ")"
    CloseParen,
    /// "{"
    OpenBrace,
    /// "}"
    CloseBrace,
    /// "["
    OpenBracket,
    /// "]"
    CloseBracket,
    /// "="
    Eq,
    /// "!"
    Bang,
    /// "<"
    Lt,
    /// ">"
    Gt,
    /// "-"
    Minus,
    /// "&"
    And,
    /// "|"
    Or,
    /// "+"
    Plus,
    /// "*"
    Star,
    /// "/"
    Slash,
    /// "%"
    Percent,
    /// Unknown token, not expected by the lexer, e.g. "№"
    Unknown,
    /// End of input.
    UnknownPrefix,
    Eof,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum LiteralKind {
    /// "12_u8", "0o100", "0b120i99", "1f32".
    Int { base: Base, empty_int: bool },
    /// "12.34f32", "1e3", but not "1f32".
    Float {
        base: Base,
        empty_exponent: bool,
        missing_exponent: bool,
        missing_dot_after: bool,
    },
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub enum Base {
    /// Literal starts with "0" and it's not a float number .
    /// there is no float in Octal .
    Octal = 8,
    Decimal = 10,
    /// Literal starts with "0x".
    Hexadecimal = 16,
}

impl Cursor<'_> {
    fn first_kind(&self) -> TokenKind {
        let first = self.first();
        match first {
            ';' => Semi,
            ',' => Comma,
            '.' => Dot,
            '(' => OpenParen,
            ')' => CloseParen,
            '{' => OpenBrace,
            '}' => CloseBrace,
            '[' => OpenBracket,
            ']' => CloseBracket,
            '=' => Eq,
            '!' => Bang,
            // Lt = Less than
            '<' => Lt,
            // Gt = Great than
            '>' => Gt,
            '-' => Minus,
            '&' => And,
            '|' => Or,
            '+' => Plus,
            '*' => Star,
            '%' => Percent,
            _ => Unknown,
        }
    }

    /// Parses a token from the input string.
    fn advance_token(&mut self) -> SimpleToken {
        let first_char = match self.bump() {
            Some(c) => c,
            None => return SimpleToken::new(Eof, 0),
        };
        let token_kind = match first_char {
            // Slash, comment or block comment.
            '/' => match self.first() {
                '/' => self.line_comment(),
                '*' => self.block_comment(),
                _ => Slash,
            },

            // Whitespace ***sequence***.
            c if is_whitespace(c) => self.whitespace(),

            // Identifier (this should be checked after other variant that can
            // start as identifier).
            c if is_id_start(c) => self.ident_or_unknown_prefix(),

            // Numeric literal.
            c @ '0'..='9' => {
                let literal_kind = self.number(c);
                Literal { kind: literal_kind }
            }

            // One-symbol tokens.
            ';' => Semi,
            ',' => Comma,
            '.' => {
                if self.first() >= '0' && self.first() <= '9' {
                    let literal_kind = self.dot_start_number(); // 历史遗留问题
                    Literal { kind: literal_kind }
                } else {
                    Dot
                }
            }
            '(' => OpenParen,
            ')' => CloseParen,
            '{' => OpenBrace,
            '}' => CloseBrace,
            '[' => OpenBracket,
            ']' => CloseBracket,
            '=' => Eq,
            '!' => Bang,
            // Lt = Less than
            '<' => Lt,
            // Gt = Great than
            '>' => Gt,
            '-' => Minus,
            '&' => And,
            '|' => Or,
            '+' => Plus,
            '*' => Star,
            '%' => Percent,

            _ => Unknown,
        };
        let res = SimpleToken::new(token_kind, self.pos_within_token());
        self.reset_pos_within_token();
        res
    }

    // COPY stable
    fn line_comment(&mut self) -> TokenKind {
        self.bump();
        self.eat_while(|c| c != '\n');
        LineComment
    }

    // COPY stable
    fn block_comment(&mut self) -> TokenKind {
        self.bump();

        let mut depth = 1usize;
        while let Some(c) = self.bump() {
            match c {
                #[cfg(feature = "comment_depth")]
                '/' if self.first() == '*' => {
                    self.bump();
                    depth += 1;
                }

                '*' if self.first() == '/' => {
                    self.bump();
                    depth -= 1;
                    if depth == 0 {
                        break;
                    }
                }
                _ => (),
            }
        }

        BlockComment
    }

    // COPY stable
    fn whitespace(&mut self) -> TokenKind {
        self.eat_while(is_whitespace);
        Whitespace
    }

    // COPY stable
    fn ident_or_unknown_prefix(&mut self) -> TokenKind {
        // Start is already eaten, eat the rest of identifier.
        self.eat_while(is_id_continue);
        // Known prefixes must have been handled earlier. So if
        // we see a prefix here, it is definitely an unknown prefix.
        let kind = match self.first() {
            /*
               int name = 00000#;
               int name = 00000";
               int name = 00000\;
                注意返回UnknownPrefix 而不是 UnknownAppendix
                因为是识别任务是识别当前 Token
            */
            '#' | '"' | '\'' | '?' => UnknownPrefix, // 绝不可能出现在标识符或者关键字后面的符号,这里的穷举不全
            _ => Ident,
        };
        if kind == UnknownPrefix {
            self.bump();
        }
        kind
    }

    /*
     * octal-start and hexadecimal start char -> 0(zero)
     *    type           example
     *  HEXADECIMAL     0x12456aF
     *  OCTAL           01433223
     *  DECIMAL         1235
     */
    ///  COPY TESTING
    /// 八进制不支持小数
    /// 3月26日 因为进制和小数方面的问题，该函数暂时不保证完全正确
    /// 3月29日 浮点数测试中,测试不通过， 0xf. 非法浮点数，
    /// 3月30日 浮点识别通过，非法浮点，不予理会，由下一阶段来完成语法检查
    fn number(&mut self, first_digit: char) -> LiteralKind {
        let mut base = Decimal;
        // 第一阶段，进制解析阶段
        let have_before_digit;
        if first_digit == '0' {
            // Attempt to parse encoding base.
            have_before_digit = match self.first() {
                'x' | 'X' => {
                    self.bump();
                    base = Hexadecimal;
                    self.eat_hex_digits()
                }
                // Not a base prefix.
                // 注意 0p10 不合法，(p 只能出现在 0x 后)这里只检查0e10 或者 0.0 或者 012 或者 0E10
                '0'..='9' | '.' | 'e' | 'E' => {
                    base = self.eat_decimal_digits_and_check();
                    true
                }
                // Just a 0.
                _ => {
                    return Int {
                        base: Decimal,
                        empty_int: false,
                    }
                }
            };
        } else {
            // No base prefix, parse number in the usual way.
            have_before_digit = self.eat_decimal_digits();
        };

        // 第二阶段，类型识别阶段

        // 注意，到目前为止，任然无法确定类型为8或者10进制
        // 但是如果是16进制的话，这是可以肯定的，则只需要判断类型即可，
        if base == Hexadecimal {
            return match self.first() {
                '.' => {
                    self.bump();
                    let mut empty_exponent = false;
                    if self.first().is_digit(16) {
                        let dot_after = self.eat_hex_digits(); // hex 确认支持16进制小数部分
                        match self.first() {
                            'p' | 'P' => {
                                self.bump();
                                empty_exponent = !self.eat_hex_float_exponent();
                                Float {
                                    base,
                                    empty_exponent,
                                    missing_exponent: false,
                                    missing_dot_after: dot_after,
                                }
                            }
                            _ => {
                                //  0x10.01f 属于异常字面量
                                Float {
                                    base,
                                    empty_exponent,
                                    missing_exponent: true,
                                    missing_dot_after: dot_after,
                                }
                            }
                        }
                    } else if self.first() == 'p' || self.first() == 'P' {
                        // 0x10.p10 (合法)
                        self.bump();
                        empty_exponent = !self.eat_hex_float_exponent();
                        Float {
                            base,
                            empty_exponent,
                            missing_exponent: false,
                            missing_dot_after: true,
                        }
                    } else {
                        // e.g. 0x10.(合法)
                        // 见 公开样例与运行时库/functional/95_float.sy
                        Float {
                            base,
                            empty_exponent: true,
                            missing_exponent: true,
                            missing_dot_after: true,
                        }
                    }
                }
                // 0x10p10 (合法) 注意这是浮点类型
                'p' | 'P' => {
                    self.bump();
                    let empty_exponent = !self.eat_hex_float_exponent();
                    Float {
                        base,
                        empty_exponent,
                        missing_exponent: false,
                        missing_dot_after: true,
                    }
                }
                // Base prefix was provided, but there were no digits
                _ => Int {
                    base,
                    empty_int: !have_before_digit,
                },
            };
        }

        // 到目前这一步，十六进制的任何相关内容都已经去除
        // 如果之前判断为8进制，那么这里任然不能确定当前为8进制
        // 但是如果之前判断已经是10进制这里则不需要再关注进制，关注类型即可
        // 目前需要处理 是不是8进制
        match self.first() {
            '.' => {
                self.bump();
                // 因为八进制不支持浮点数，所以这里肯定是10进制了
                if base == Octal {
                    base = Decimal;
                }
                let mut empty_exponent = false;
                if self.first().is_digit(10) {
                    let dot_after = self.eat_decimal_digits();
                    match self.first() {
                        'e' | 'E' => {
                            self.bump();
                            empty_exponent = !self.eat_float_exponent();
                            Float {
                                base,
                                empty_exponent,
                                missing_exponent: false,
                                missing_dot_after: dot_after,
                            }
                        }
                        _ => {
                            // 十进制允许不出现 e
                            Float {
                                base,
                                empty_exponent,
                                missing_exponent: true,
                                missing_dot_after: dot_after,
                            }
                        }
                    }
                } else if self.first() == 'e' || self.first() == 'E' {
                    // 10.e10  (合法)
                    self.bump();
                    empty_exponent = !self.eat_float_exponent();
                    Float {
                        base,
                        empty_exponent,
                        missing_exponent: false,
                        missing_dot_after: true,
                    }
                } else {
                    // 10. (合法)
                    Float {
                        base,
                        empty_exponent: true,
                        missing_exponent: true,
                        missing_dot_after: true,
                    }
                }
            }
            'e' | 'E' => {
                // 01234567e10
                // 12345689e10
                self.bump();
                let empty_exponent = !self.eat_float_exponent();
                if base == Octal {
                    base = Decimal;
                }
                Float {
                    base,
                    empty_exponent,
                    missing_exponent: false,
                    missing_dot_after: true,
                }
            }
            // todo 注意这里如果是八进制,需注意检查是否合法，预计将放在语法检查期间
            _ => Int {
                base,
                empty_int: false,
            },
        }
    }

    fn eat_decimal_digits(&mut self) -> bool {
        let mut has_digits = false;
        loop {
            match self.first() {
                '0'..='9' => {
                    has_digits = true;
                    self.bump();
                }
                _ => break,
            }
        }
        has_digits
    }
    /// 注意不要考虑超出范围的字符出现，该函数只负责吃掉合适数量的字面量并检查是否超出8进制，
    /// 如果超出8进制则后面必须出现小数点否则将表现为异常8进制
    /// 字面量的不合适将在下一步表现为语法异常，
    /// 对于语法异常的情况，保证无法编译通过即可
    fn eat_decimal_digits_and_check(&mut self) -> Base {
        let mut base = Octal;
        loop {
            match self.first() {
                '0'..='7' => {
                    self.bump();
                }
                '8'..='9' => {
                    self.bump();
                    base = Decimal;
                }
                _ => break,
            }
        }
        base
    }

    fn eat_hex_digits(&mut self) -> bool {
        let mut has_digits = false;
        loop {
            match self.first() {
                '0'..='9' | 'a'..='f' | 'A'..='F' => {
                    has_digits = true;
                    self.bump();
                }
                _ => break,
            }
        }
        has_digits
    }

    /// Eats the float exponent. Returns true if at least one digit was met,
    /// and returns false otherwise.
    fn eat_float_exponent(&mut self) -> bool {
        if self.first() == '-' || self.first() == '+' {
            self.bump();
        }
        // now matter which type of exponent , they only allow `digit` after the signal char.
        self.eat_decimal_digits()
    }

    fn eat_hex_float_exponent(&mut self) -> bool {
        self.eat_float_exponent()
    }
    /// 进入条件是 . digit
    fn dot_start_number(&mut self) -> LiteralKind {
        let mut empty_exponent = false;
        self.eat_decimal_digits(); // 这里肯定能吃到
        match self.first() {
            'e' | 'E' => {
                self.bump();
                empty_exponent = !self.eat_float_exponent();
                Float {
                    base: Decimal,
                    empty_exponent,
                    missing_exponent: false,
                    missing_dot_after: false,
                }
            }
            _ => {
                // 十进制允许不出现 e
                Float {
                    base: Decimal,
                    empty_exponent,
                    missing_exponent: true,
                    missing_dot_after: false,
                }
            }
        }
    }
}

// stable
pub fn is_id_start(c: char) -> bool {
    (c >= 'a' && c <= 'z') || c == '_' || (c >= 'A' && c <= 'Z')
}

// stable
// BUG！ 5月10日 没有加入数字的支持
pub fn is_id_continue(c: char) -> bool {
    (c >= 'a' && c <= 'z') || c == '_' || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
}

// COPY stable
pub fn is_whitespace(c: char) -> bool {
    // This is Pattern_White_Space.
    //
    // Note that this set is stable (ie, it doesn't change with different
    // Unicode versions), so it's ok to just hard-code the values.

    matches!(
        c,
        // Usual ASCII suspects
        '\u{0009}'   // \t
        | '\u{000A}' // \n
        | '\u{000B}' // vertical tab
        | '\u{000C}' // form feed
        | '\u{000D}' // \r
        | '\u{0020}' // space

        // NEXT LINE from latin1
        | '\u{0085}'

        // Bidi markers
        | '\u{200E}' // LEFT-TO-RIGHT MARK
        | '\u{200F}' // RIGHT-TO-LEFT MARK

        // Dedicated whitespace characters from Unicode
        | '\u{2028}' // LINE SEPARATOR
        | '\u{2029}' // PARAGRAPH SEPARATOR
    )
}
