use crate::ast::{AstUnit, ExternalFun, AST};
use crate::lexer::TokenReader;
use crate::parser::TokenParser;
use crate::session::CompileSession;

pub fn get_sysy_std() -> AST {
    let codes = "
int getint(){}
int getch(){}
float getfloat(){}
int getarray(int a[]){}
int getfarray(float a[]) {}
void putint(int a){ }
void putch(int a){  }
void putarray(int n,int a[]){}
void putfloat(float a) {}
void putfarray(int n, float a[]) {}
void starttime(){}
void stoptime(){}
void _sysy_starttime(int lineno){}
void _sysy_stoptime(int lineno){}
";
    let lexical = TokenReader::new(codes);
    let mut temp_session = CompileSession::new();
    let mut parser = TokenParser::new(lexical, &mut temp_session);
    let ret = parser.parse_ast();
    if ret.is_err() {
        temp_session.print_error(codes);
        panic!("std library code error");
    }
    let mut ret_ast = AST::new();
    for single_unit in ret.unwrap().get_units() {
        match single_unit {
            AstUnit::FnDefine(define) => {
                let external = ExternalFun {
                    name: define.name,
                    ret: define.ret_type,
                    params: define.params,
                };
                ret_ast.push(AstUnit::ExternalFn(external));
            }
            _ => {
                panic!("std library code error");
            }
        }
    }
    return ret_ast;
}
