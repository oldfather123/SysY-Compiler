fn legal_char(ch: char) -> bool {
    ch.is_alphabetic() || ch.is_digit(10) || ch == '_'
}

pub fn mangle_global_var_name(s: &str) -> String {
    assert!(s.len() > 0);
    let mut ret = String::new();
    ret.push_str("_m");
    let mut last: isize = -1;
    for (i, c) in s.chars().enumerate() {
        if !legal_char(c) {
            if i as isize - last - 1 > 0 {
                ret.push_str(&(i as isize - last - 1).to_string());
                ret.push_str(&s[(last + 1) as usize..i]);
            }
            let cur = c as u32;
            ret.push('0');
            ret.push_str(&format!("{:x}", cur / 16));
            ret.push_str(&format!("{:x}", cur % 16));
            last = i as isize;
        }
    }
    if last < s.len() as isize - 1 {
        ret.push_str(&((s.len() as isize - 1) - last).to_string());
        ret.push_str(&s[(last + 1) as usize..]);
    }
    ret
}
