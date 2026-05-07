#[cfg(debug_assertions)]
use crate::compiler::DebugAction;
use crate::echo::{print_echo, print_located_echo, Error, Warning};
use crate::optimizer::OptLevel;
use std::cmp::Ordering;
use std::collections::HashSet;
use crate::compiler::CompileOptions;

pub struct CompileSession {
    is_error: bool,
    errors: Vec<Error>,
    warning: Vec<Warning>,
    #[cfg(debug_assertions)]
    debug_action: Vec<DebugAction>,
    opt_level: OptLevel,
    options:HashSet<CompileOptions>
}

impl CompileSession {
    pub fn new() -> CompileSession {
        CompileSession {
            is_error: false,
            errors: vec![],
            warning: vec![],
            #[cfg(debug_assertions)]
            debug_action: vec![],
            opt_level: OptLevel::O0,
            options: Default::default(),
        }
    }
    /// 向编译会话汇报一个error
    pub fn report_error(&mut self, error: Error) {
        if !self.is_error {
            self.is_error = true
        }
        self.errors.push(error)
    }

    pub fn set_opt(&mut self, opt: OptLevel) {
        self.opt_level = opt
    }

    pub fn get_opt_level(&self) -> OptLevel {
        self.opt_level
    }

    pub fn is_opt_enable(&self,option:CompileOptions) -> bool{
        self.options.contains(&option)
    }

    pub fn open_option(&mut self,option:CompileOptions) -> bool{
        self.options.insert(option)
    }

    #[allow(unused)]
    pub fn close_option(&mut self,option:CompileOptions) -> bool{
        self.options.remove(&option)
    }

    #[cfg(debug_assertions)]
    pub fn add_debug_mode(&mut self, mode: DebugAction) {
        self.debug_action.push(mode);
    }

    /// 向编译会话汇报一个warning
    pub fn report_warning(&mut self, warning: Warning) {
        self.warning.push(warning);
    }

    pub fn print_error(&mut self, source: &str) {
        let mut chars = source.chars();
        let mut base_at = 0;
        let mut line_at = 1;
        let mut column_at = 1;
        self.errors.sort_by(|a, b| {
            if a.get_pos().0 > b.get_pos().0 {
                Ordering::Greater
            } else if a.get_pos().0 == b.get_pos().0 {
                Ordering::Equal
            } else {
                Ordering::Less
            }
        });

        for single in &self.errors {
            if !single.is_located {
                print_echo(single);
                continue;
            }
            let (need_base, need_len) = single.get_pos();
            let mut error_tok = String::new();
            loop {
                if base_at < need_base + need_len && base_at >= need_base {
                    let ch = chars.next();
                    base_at += 1;
                    match ch {
                        Some(ch) => {
                            error_tok.push(ch);
                            if ch == '\n' {
                                line_at += 1;
                                column_at = 0;
                            } else {
                                column_at += 1;
                            }
                        }
                        None => {
                            panic!("never here")
                        }
                    }
                } else if base_at < need_base {
                    let ch = chars.next();
                    base_at += 1;

                    match ch {
                        Some(ch) => {
                            if ch == '\n' {
                                line_at += 1;
                                column_at = 0;
                            } else {
                                column_at += 1;
                            }
                        }
                        None => {
                            panic!("never here")
                        }
                    }
                } else {
                    break;
                }
            }
            print_located_echo(single, line_at, column_at, &error_tok);
        }
    }

    pub fn print_warning(&mut self, source: &str) {
        let mut chars = source.chars();
        let mut base_at = 0;
        let mut line_at = 1;
        let mut column_at = 1;

        self.warning.sort_by(|a, b| {
            if a.get_pos().0 > b.get_pos().0 {
                Ordering::Greater
            } else if a.get_pos().0 == b.get_pos().0 {
                Ordering::Equal
            } else {
                Ordering::Less
            }
        });

        for single in &self.warning {
            if !single.is_located {
                print_echo(single);
                continue;
            }
            let (need_base, need_len) = single.get_pos();
            let mut error_tok = String::new();
            loop {
                if base_at < need_base + need_len && base_at >= need_base {
                    let ch = chars.next();
                    base_at += 1;
                    match ch {
                        Some(ch) => {
                            error_tok.push(ch);
                            if ch == '\n' {
                                line_at += 1;
                                column_at = 0;
                            } else {
                                column_at += 1;
                            }
                        }
                        None => {
                            panic!("never here")
                        }
                    }
                } else if base_at < need_base {
                    let ch = chars.next();
                    base_at += 1;

                    match ch {
                        Some(ch) => {
                            if ch == '\n' {
                                line_at += 1;
                                column_at = 0;
                            } else {
                                column_at += 1;
                            }
                        }
                        None => {
                            panic!("never here")
                        }
                    }
                } else {
                    break;
                }
            }
            print_located_echo(single, line_at, column_at, &error_tok);
        }
    }

    /// 查询上次会话是否出现了Error
    pub fn is_error(&self) -> bool {
        self.is_error
    }
}
