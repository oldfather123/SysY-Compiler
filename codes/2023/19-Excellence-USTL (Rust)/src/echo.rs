use crate::util::Colorize;

pub enum Type {
    Error,
    Warning,
}

pub trait Echo {
    fn get_type(&self) -> Type;
    fn get_info(&self) -> &str;
    fn get_advise(&self) -> Option<&String>;
}

// 尝试加入多语言支持
pub struct Error {
    info: String,
    from: u32,
    len: u32,
    help: Option<String>,
    pub is_located: bool,
}

impl Error {
    pub fn new(info: &str, help: Option<&str>) -> Error {
        if help.is_some() {
            Error {
                info: info.to_string(),
                from: 0,
                len: 0,
                help: Some(help.unwrap().to_string()),
                is_located: false,
            }
        } else {
            Error {
                info: info.to_string(),
                from: 0,
                len: 0,
                help: None,
                is_located: false,
            }
        }
    }
    pub fn set_pos(&mut self, base: u32, len: u32) {
        self.from = base;
        self.len = len;
    }
    pub fn get_pos(&self) -> (u32, u32) {
        (self.from, self.len)
    }
}

pub struct Warning {
    from: u32,
    len: u32,
    info: String,
    help: Option<String>,
    pub is_located: bool,
}

impl Warning {
    pub fn new(info: &str, help: Option<&str>) -> Warning {
        if help.is_some() {
            Warning {
                from: 0,
                len: 0,
                info: info.to_string(),
                help: Some(help.unwrap().to_string()),
                is_located: false,
            }
        } else {
            Warning {
                from: 0,
                len: 0,
                info: info.to_string(),
                help: None,
                is_located: false,
            }
        }
    }

    pub fn get_pos(&self) -> (u32, u32) {
        (self.from, self.len)
    }

    pub(crate) fn set_pos(&mut self, base: u32, len: u32) {
        self.from = base;
        self.len = len;
    }
}

pub fn make_advise(advise: &str) -> Option<&str> {
    Some(advise)
}

pub fn print_located_echo(echo: &dyn Echo, line: u32, col: u32, token: &str) {
    let echo_type = echo.get_type();
    let echo_info = echo.get_info();
    let echo_advise = echo.get_advise();
    match echo_type {
        Type::Error => {
            let err = "error";
            let err = format!("{} at {}:{} -> \"{}\"", err, line, col, token).red();
            println!("{}", err);
            println!("    info: {}", echo_info);
            match echo_advise {
                None => {}
                Some(help) => {
                    let help = format!("    help: {}", help).green();
                    println!("{}", help)
                }
            }
            println!()
        }
        Type::Warning => {
            let err = "warning";
            let err = format!("{} at {}:{} -> \"{}\"", err, line, col, token).yellow();
            println!("{}", err);
            println!("    info: {}", echo_info);
            match echo_advise {
                None => {}
                Some(help) => {
                    let help = format!("    help: {}", help).green();
                    println!("{}", help)
                }
            }
            println!()
        }
    }
}

pub fn print_echo(echo: &dyn Echo) {
    let echo_type = echo.get_type();
    let echo_info = echo.get_info();
    let echo_advise = echo.get_advise();
    match echo_type {
        Type::Error => {
            let warn = "error".red();
            print!("{}:", warn);
            println!(" {}", echo_info);
            match echo_advise {
                None => {}
                Some(help) => {
                    let help = format!("    help: {}", help).green();
                    println!("{}", help)
                }
            }
            println!()
        }
        Type::Warning => {
            let warn = "warning".yellow();
            print!("{}:", warn);
            println!(" {}", echo_info);
            match echo_advise {
                None => {}
                Some(help) => {
                    let help = format!("    help: {}", help).green();
                    println!("{}", help)
                }
            }
            println!()
        }
    }
}

impl Echo for Error {
    fn get_type(&self) -> Type {
        Type::Error
    }

    fn get_info(&self) -> &str {
        return &self.info;
    }

    fn get_advise(&self) -> Option<&String> {
        self.help.as_ref()
    }
}

impl Echo for Warning {
    fn get_type(&self) -> Type {
        Type::Warning
    }

    fn get_info(&self) -> &str {
        return &self.info;
    }

    fn get_advise(&self) -> Option<&String> {
        self.help.as_ref()
    }
}
