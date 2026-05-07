/// 区别于 AnySymbol , 这只是AST阶段的临时Symbol
#[derive(Clone, Debug, PartialEq, Eq, Hash)]
pub struct Symbol {
    pub string: String,
}

impl Symbol {
    pub fn new(sym: &str) -> Self {
        Self {
            string: sym.to_string(),
        }
    }
}

impl ToString for Symbol {
    fn to_string(&self) -> String {
        self.string.clone()
    }
}
