use antlr_rust::{
    error_listener::ErrorListener, errors::ANTLRError, token_factory::TokenFactory, Parser,
};

/// Panic error listener
#[derive(Default)]
pub struct SysYErrorListener;

impl<'a, T: Parser<'a>> ErrorListener<'a, T> for SysYErrorListener {
    fn syntax_error(
        &self,
        _recognizer: &T,
        _offending_symbol: Option<&<T::TF as TokenFactory<'a>>::Inner>,
        line: isize,
        column: isize,
        msg: &str,
        _error: Option<&ANTLRError>,
    ) {
        panic!("line {}:{} error: {}", line, column, msg);
    }
}
