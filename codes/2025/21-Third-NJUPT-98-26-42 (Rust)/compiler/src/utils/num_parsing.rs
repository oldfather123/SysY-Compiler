//! 数字解析
//!
//! 整数和浮点数的C格式解析，十进制、八进制、十六进制等
//! 用于parser和interpreter的共同需求

/// 解析整数字面量，支持十进制、八进制(0开头)和十六进制(0x开头)
pub fn parse_int_literal(text: &str) -> Result<i32, std::num::ParseIntError> {
    if text.starts_with("0x") || text.starts_with("0X") {
        // 十六进制：移除0x前缀并解析
        let hex_part = &text[2..];
        i32::from_str_radix(hex_part, 16)
    } else if text.starts_with("0") && text.len() > 1 && text.chars().all(|c| c.is_ascii_digit()) {
        // 八进制：以0开头的纯数字
        i32::from_str_radix(text, 8)
    } else {
        // 十进制：默认情况
        text.parse::<i32>()
    }
}

/// 解析浮点数字面量，支持十六进制格式
pub fn parse_float_literal(text: &str) -> Result<f32, std::num::ParseFloatError> {
    if text.starts_with("0x") || text.starts_with("0X") {
        parse_hex_float(text)
    } else if text.starts_with("-0x") || text.starts_with("-0X") {
        // 处理负的十六进制浮点数
        let positive_part = &text[1..]; // 去掉负号
        parse_hex_float(positive_part).map(|val| -val)
    } else {
        text.parse::<f32>()
    }
}

/// 解析十六进制浮点数 (如 0x1.921fb6p+1)
fn parse_hex_float(text: &str) -> Result<f32, std::num::ParseFloatError> {
    // 移除0x前缀
    let hex_part = &text[2..];

    // 分离尾数和指数部分
    let (mantissa_str, exponent) = if let Some(p_pos) = hex_part.find(['p', 'P']) {
        let mantissa = &hex_part[..p_pos];
        let exp_str = &hex_part[p_pos + 1..];
        let exp_val = exp_str.parse::<i32>().unwrap_or(0);
        (mantissa, exp_val)
    } else {
        (hex_part, 0)
    };

    // 分离整数和小数部分
    let (integer_part, fractional_part) = if let Some(dot_pos) = mantissa_str.find('.') {
        let int_part = &mantissa_str[..dot_pos];
        let frac_part = &mantissa_str[dot_pos + 1..];
        (int_part, frac_part)
    } else {
        (mantissa_str, "")
    };

    // 转换为十进制值
    let mut value = 0.0f32;

    // 处理整数部分
    if !integer_part.is_empty() {
        if let Ok(int_val) = u32::from_str_radix(integer_part, 16) {
            value += int_val as f32;
        }
    }

    // 处理小数部分
    if !fractional_part.is_empty() {
        let mut frac_value = 0.0f32;
        let mut weight = 1.0f32 / 16.0f32;
        for c in fractional_part.chars() {
            if let Some(digit) = c.to_digit(16) {
                frac_value += digit as f32 * weight;
                weight /= 16.0f32;
            } else {
                // 遇到非十六进制字符，停止解析
                break;
            }
        }
        value += frac_value;
    }

    // 应用指数 (2的幂次)
    if exponent != 0 {
        value *= 2.0f32.powi(exponent);
    }

    Ok(value)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_int_literal() {
        // 十进制
        assert_eq!(parse_int_literal("123").unwrap(), 123);
        assert_eq!(parse_int_literal("0").unwrap(), 0);

        // 八进制
        assert_eq!(parse_int_literal("0123").unwrap(), 83); // 8^2 + 2*8 + 3 = 64 + 16 + 3 = 83
        assert_eq!(parse_int_literal("077").unwrap(), 63); // 7*8 + 7 = 56 + 7 = 63

        // 十六进制
        assert_eq!(parse_int_literal("0x123").unwrap(), 291); // 256 + 32 + 3 = 291
        assert_eq!(parse_int_literal("0XFF").unwrap(), 255);
    }

    #[test]
    fn test_parse_float_literal() {
        // 十进制
        assert_eq!(parse_float_literal("3.14").unwrap(), 3.14);
        assert_eq!(parse_float_literal("123.0").unwrap(), 123.0);

        // 十六进制
        assert_eq!(parse_float_literal("0x1.0p0").unwrap(), 1.0);
        assert_eq!(parse_float_literal("0x1.0p1").unwrap(), 2.0);

        // 负的十六进制
        assert_eq!(parse_float_literal("-0x1.0p0").unwrap(), -1.0);
        assert_eq!(parse_float_literal("-0x1.0p1").unwrap(), -2.0);
    }

    #[test]
    fn test_parse_hex_float() {
        // 基本测试
        assert_eq!(parse_hex_float("0x1.0p0").unwrap(), 1.0);
        assert_eq!(parse_hex_float("0x1.0p1").unwrap(), 2.0);
        assert_eq!(parse_hex_float("0x1.0p-1").unwrap(), 0.5);

        // 复杂测试
        assert!((parse_hex_float("0x1.999999999999ap-4").unwrap() - 0.1).abs() < 0.01);
    }
}
