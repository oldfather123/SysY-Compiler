use crate::echo::{Error, Warning};

#[derive(Clone, Debug, Copy, Eq, PartialEq, Hash)]
pub struct Span {
    base: u32,
    len: u16,
}

impl Span {
    pub fn new(base: u32, len: u16) -> Self {
        Self { base, len }
    }
    pub fn make_error(&self, info: &str, advise: Option<&str>) -> Error {
        let mut erro = Error::new(info, advise);
        erro.set_pos(self.base, self.len as u32);
        erro.is_located = true;
        erro
    }
    pub fn make_warning(&self, info: &str, advise: Option<&str>) -> Warning {
        let mut warning = Warning::new(info, advise);
        warning.set_pos(self.base, self.len as u32);
        warning.is_located = true;
        warning
    }
}
