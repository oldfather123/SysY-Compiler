//! 性能优化后的词法分析器实现

use super::token::{Token, TokenPos, TokenType};
use crate::prelude::*;

pub struct Lexer {
    /// 源代码
    source: String,
    /// 源代码的字节视图，用于高效访问
    bytes: Vec<u8>,
    /// 当前位置 (字节偏移量)
    pos: usize,
    /// 当前行号 (1-based)
    line: usize,
    /// 当前列号 (1-based)
    column: usize,
}

impl Lexer {
    pub fn new(source: String) -> Self {
        let bytes = source.as_bytes().to_vec(); // 预先复制为 Vec<u8>
        Self {
            source,
            bytes,
            pos: 0,
            line: 1,
            column: 1,
        }
    }

    pub fn tokenize(&mut self) -> Result<Vec<Token>, CErr> {
        let mut tokens = Vec::new();
        loop {
            let token = self.next_token()?;
            let is_eof = token.is_eof();
            tokens.push(token);
            if is_eof {
                break;
            }
        }
        Ok(tokens)
    }

    pub fn next_token(&mut self) -> Result<Token, CErr> {
        self.skip_whitespace();

        if self.is_at_end() {
            let pos = TokenPos::new(self.line, self.column, self.pos, 0);
            return Ok(Token::simple(TokenType::Eof, pos));
        }

        let _start_pos = self.pos;
        let _start_line = self.line;
        let _start_column = self.column;

        let byte = self.cur_byte().unwrap();

        match byte {
            // 注释
            b'/' if self.peek_byte() == Some(b'/') => {
                self.skip_line_comment();
                self.next_token()
            }
            b'/' if self.peek_byte() == Some(b'*') => {
                self.skip_block_comment()?;
                self.next_token()
            }

            // 单字符符号
            b'(' => Ok(self.eat_single_char(TokenType::LParen)),
            b')' => Ok(self.eat_single_char(TokenType::RParen)),
            b'[' => Ok(self.eat_single_char(TokenType::LBrack)),
            b']' => Ok(self.eat_single_char(TokenType::RBrack)),
            b'{' => Ok(self.eat_single_char(TokenType::LBrace)),
            b'}' => Ok(self.eat_single_char(TokenType::RBrace)),
            b';' => Ok(self.eat_single_char(TokenType::Semi)),
            b',' => Ok(self.eat_single_char(TokenType::Comma)),
            b'+' => Ok(self.eat_single_char(TokenType::Plus)),
            b'-' => Ok(self.eat_single_char(TokenType::Minus)),
            b'*' => Ok(self.eat_single_char(TokenType::Star)),
            b'%' => Ok(self.eat_single_char(TokenType::Percent)),

            // 多字符运算符
            b'=' => {
                if self.peek_byte() == Some(b'=') {
                    Ok(self.eat_double_char(TokenType::Eq))
                } else {
                    Ok(self.eat_single_char(TokenType::Assign))
                }
            }
            b'!' => {
                if self.peek_byte() == Some(b'=') {
                    Ok(self.eat_double_char(TokenType::Ne))
                } else {
                    Ok(self.eat_single_char(TokenType::Not))
                }
            }
            b'<' => {
                if self.peek_byte() == Some(b'=') {
                    Ok(self.eat_double_char(TokenType::Le))
                } else {
                    Ok(self.eat_single_char(TokenType::Lt))
                }
            }
            b'>' => {
                if self.peek_byte() == Some(b'=') {
                    Ok(self.eat_double_char(TokenType::Ge))
                } else {
                    Ok(self.eat_single_char(TokenType::Gt))
                }
            }
            b'&' => {
                if self.peek_byte() == Some(b'&') {
                    Ok(self.eat_double_char(TokenType::And))
                } else {
                    Err(self.make_error("Unexpected character '&'"))
                }
            }
            b'|' => {
                if self.peek_byte() == Some(b'|') {
                    Ok(self.eat_double_char(TokenType::Or))
                } else {
                    Err(self.make_error("Unexpected character '|'"))
                }
            }
            b'/' => Ok(self.eat_single_char(TokenType::Slash)),

            // 数字
            b'0'..=b'9' => self.scan_number(),

            // 以小数点开头的浮点数
            b'.' => {
                if let Some(next_byte) = self.peek_byte() {
                    if next_byte.is_ascii_digit() {
                        self.scan_float_starting_with_dot()
                    } else {
                        Err(self.make_error("Unexpected character '.'"))
                    }
                } else {
                    Err(self.make_error("Unexpected character '.'"))
                }
            }

            // 标识符或关键字
            b'a'..=b'z' | b'A'..=b'Z' | b'_' => self.scan_ident(),

            // 其他字符
            _ => Err(self.make_error(&format!("Unexpected character '{}'", byte as char))),
        }
    }

    /// 跳过空白字符
    fn skip_whitespace(&mut self) {
        while let Some(byte) = self.cur_byte() {
            if byte.is_ascii_whitespace() {
                self.advance();
            } else {
                break;
            }
        }
    }

    /// 跳过行注释
    fn skip_line_comment(&mut self) {
        while let Some(byte) = self.cur_byte() {
            self.advance();
            if byte == b'\n' {
                break;
            }
        }
    }

    /// 跳过块注释
    fn skip_block_comment(&mut self) -> CEResult<()> {
        self.advance(); // 跳过 '/'
        self.advance(); // 跳过 '*'

        while let Some(byte) = self.cur_byte() {
            if byte == b'*' && self.peek_byte() == Some(b'/') {
                self.advance(); // 跳过 '*'
                self.advance(); // 跳过 '/'
                return Ok(());
            }
            self.advance();
        }

        Err(self.make_error("Unterminated block comment"))
    }

    /// 扫描数字 (包括整数和浮点数)
    fn scan_number(&mut self) -> Result<Token, CErr> {
        let start_pos = self.pos;
        let start_line = self.line;
        let start_column = self.column;
        let mut is_float = false;

        // 十六进制
        if self.cur_byte() == Some(b'0')
            && (self.peek_byte() == Some(b'x') || self.peek_byte() == Some(b'X'))
        {
            return self.scan_hex_number();
        }

        // 十进制整数部分
        while let Some(byte) = self.cur_byte() {
            if byte.is_ascii_digit() {
                self.advance();
            } else {
                break;
            }
        }

        // 小数部分
        if self.cur_byte() == Some(b'.') {
            if let Some(next_byte) = self.peek_byte() {
                if next_byte.is_ascii_digit() || next_byte == b'e' || next_byte == b'E' {
                    is_float = true;
                    self.advance(); // consume '.'
                    while let Some(byte) = self.cur_byte() {
                        if byte.is_ascii_digit() {
                            self.advance();
                        } else {
                            break;
                        }
                    }
                } else {
                    is_float = true;
                    self.advance(); // consume '.' for cases like "1."
                }
            } else {
                is_float = true;
                self.advance(); // consume '.' for cases like "1." at EOF
            }
        }

        // 科学计数法
        if self.cur_byte() == Some(b'e') || self.cur_byte() == Some(b'E') {
            is_float = true;
            self.advance(); // consume 'e' or 'E'
            if self.cur_byte() == Some(b'+') || self.cur_byte() == Some(b'-') {
                self.advance();
            }
            while let Some(byte) = self.cur_byte() {
                if byte.is_ascii_digit() {
                    self.advance();
                } else {
                    break;
                }
            }
        }

        let end_pos = self.pos;
        let text = self.source[start_pos..end_pos].to_string();
        let len = end_pos - start_pos;
        let pos = TokenPos::new(start_line, start_column, start_pos, len);
        let kind = if is_float {
            TokenType::FloatLit
        } else {
            TokenType::IntLit
        };
        Ok(Token::new(kind, text, pos))
    }

    /// 扫描十六进制数
    fn scan_hex_number(&mut self) -> Result<Token, CErr> {
        let start_pos = self.pos;
        let start_line = self.line;
        let start_column = self.column;

        self.advance(); // 0
        self.advance(); // x or X

        let mut has_mantissa_digits = false;

        // 扫描整数部分 (可选)
        while let Some(byte) = self.cur_byte() {
            if byte.is_ascii_hexdigit() {
                self.advance();
                has_mantissa_digits = true;
            } else {
                break;
            }
        }

        let mut is_float = false;

        // 扫描小数部分 (可选)
        if self.cur_byte() == Some(b'.') {
            is_float = true;
            self.advance(); // consume '.'
            while let Some(byte) = self.cur_byte() {
                if byte.is_ascii_hexdigit() {
                    self.advance();
                    has_mantissa_digits = true;
                } else {
                    break;
                }
            }
        }

        // 在 '0x' 或 '0x.' 之后必须有数字
        if !has_mantissa_digits {
            return Err(self.make_error("Malformed hexadecimal number: no digits found"));
        }

        // 扫描二进制指数部分 (对于十六进制浮点数是必需的)
        if self.cur_byte() == Some(b'p') || self.cur_byte() == Some(b'P') {
            is_float = true;
            self.advance(); // consume 'p' or 'P'

            // 扫描可选的符号
            if self.cur_byte() == Some(b'+') || self.cur_byte() == Some(b'-') {
                self.advance();
            }

            let mut has_exponent_digits = false;
            while let Some(byte) = self.cur_byte() {
                if byte.is_ascii_digit() {
                    self.advance();
                    has_exponent_digits = true;
                } else {
                    break;
                }
            }
            if !has_exponent_digits {
                return Err(self.make_error(
                    "Malformed hexadecimal float: no exponent digits found after 'p'",
                ));
            }
        } else if is_float {
            // 按照C标准，如果一个十六进制数有小数点，它必须有 'p' 指数部分
            return Err(self.make_error("Malformed hexadecimal float: missing 'p' exponent"));
        }

        let end_pos = self.pos;
        let text = self.source[start_pos..end_pos].to_string();
        let len = end_pos - start_pos;
        let pos = TokenPos::new(start_line, start_column, start_pos, len);

        let kind = if is_float {
            TokenType::FloatLit
        } else {
            TokenType::IntLit
        };

        Ok(Token::new(kind, text, pos))
    }

    /// 扫描以小数点开头的浮点数
    fn scan_float_starting_with_dot(&mut self) -> Result<Token, CErr> {
        let start_pos = self.pos;
        let start_line = self.line;
        let start_column = self.column;

        self.advance(); // consume '.'

        while let Some(byte) = self.cur_byte() {
            if byte.is_ascii_digit() {
                self.advance();
            } else {
                break;
            }
        }

        if self.cur_byte() == Some(b'e') || self.cur_byte() == Some(b'E') {
            self.advance();
            if self.cur_byte() == Some(b'+') || self.cur_byte() == Some(b'-') {
                self.advance();
            }
            while let Some(byte) = self.cur_byte() {
                if byte.is_ascii_digit() {
                    self.advance();
                } else {
                    break;
                }
            }
        }

        let end_pos = self.pos;
        let text = self.source[start_pos..end_pos].to_string();
        let len = end_pos - start_pos;
        let pos = TokenPos::new(start_line, start_column, start_pos, len);
        Ok(Token::new(TokenType::FloatLit, text, pos))
    }

    /// 扫描标识符或关键字
    fn scan_ident(&mut self) -> Result<Token, CErr> {
        let start_pos = self.pos;
        let start_line = self.line;
        let start_column = self.column;

        while let Some(byte) = self.cur_byte() {
            if byte.is_ascii_alphanumeric() || byte == b'_' {
                self.advance();
            } else {
                break;
            }
        }

        let end_pos = self.pos;
        let text_slice = &self.source[start_pos..end_pos];
        let len = end_pos - start_pos;
        let pos = TokenPos::new(start_line, start_column, start_pos, len);

        let token_type = match text_slice {
            "int" => TokenType::Int,
            "float" => TokenType::Float,
            "void" => TokenType::Void,
            "const" => TokenType::Const,
            "if" => TokenType::If,
            "else" => TokenType::Else,
            "while" => TokenType::While,
            "break" => TokenType::Break,
            "continue" => TokenType::Continue,
            "return" => TokenType::Return,
            _ => TokenType::Ident,
        };

        Ok(Token::new(token_type, text_slice.to_string(), pos))
    }

    /// 消费单个字符 Token
    fn eat_single_char(&mut self, token_type: TokenType) -> Token {
        let start_pos = self.pos;
        let start_line = self.line;
        let start_column = self.column;
        self.advance();
        let text = self.source[start_pos..self.pos].to_string();
        let pos = TokenPos::new(start_line, start_column, start_pos, 1);
        Token::new(token_type, text, pos)
    }

    /// 消费双字符 Token
    fn eat_double_char(&mut self, token_type: TokenType) -> Token {
        let start_pos = self.pos;
        let start_line = self.line;
        let start_column = self.column;
        self.advance();
        self.advance();
        let text = self.source[start_pos..self.pos].to_string();
        let pos = TokenPos::new(start_line, start_column, start_pos, 2);
        Token::new(token_type, text, pos)
    }

    /// 前进一个字节
    fn advance(&mut self) {
        if let Some(byte) = self.cur_byte() {
            if byte == b'\n' {
                self.line += 1;
                self.column = 1;
            } else {
                self.column += 1;
            }
            self.pos += 1;
        }
    }

    /// 获取当前字节
    #[inline(always)]
    fn cur_byte(&self) -> Option<u8> {
        self.bytes.get(self.pos).copied()
    }

    /// 查看下一个字节
    #[inline(always)]
    fn peek_byte(&self) -> Option<u8> {
        self.bytes.get(self.pos + 1).copied()
    }

    /// 是否到达文件末尾
    #[inline(always)]
    fn is_at_end(&self) -> bool {
        self.pos >= self.bytes.len()
    }

    /// 创建错误
    fn make_error(&self, msg: &str) -> CErr {
        CErr::lexer_err(self.line, self.column, msg)
    }
}
