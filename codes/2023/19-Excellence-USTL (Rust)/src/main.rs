extern crate core;

mod asm;
mod ast;
mod compiler;
mod driver;
mod echo;
mod hir;
mod lexer;
mod lir;
mod mir;
mod optimizer;
mod parser;
mod session;
mod span;
mod sysy_std;
mod util;

use crate::util::Colorize;
use std::{env, thread};

use crate::compiler::{CompileOptions, CompileResult};
use crate::driver::CompilerBuildResult;

fn main() {
    let runtime = thread::Builder::new()
        .stack_size(1024 * 1024 * 30) // 8 危险 10 能通过
        .name("compiler".to_string())
        .spawn(move || {
            let mut args: Vec<String> = env::args().collect();
            args.remove(0);
            let built_compiler = driver::CommandParamParser::parse_commands(args);
            match built_compiler {
                CompilerBuildResult::NoParam => {
                    println!("{}", "YetJustSysyc is now available ! Enjoy it!".green())
                }
                CompilerBuildResult::BuildError => {
                    println!("{}", "build compiler error".red())
                }
                CompilerBuildResult::Ok(mut compiler) => {
                    // 这里是第二级编译参数的控制
                    // ！！！！ 但是 ！！！！ 对于优化等级，输入，输出等由构造器控制的参数 ，谨慎修改
                    compiler.open_option(CompileOptions::HirLitExprPreComp);
                    compiler.open_option(CompileOptions::HirConstArrDimPreComp);
                    compiler.set_warning_mode(false);

                    let ret = compiler.compile();
                    match ret {
                        CompileResult::Ok => {
                            println!("{}", "编译通过".green())
                        }
                        CompileResult::AstErr => {
                            println!("{}", "Ast错误".red())
                        }
                        CompileResult::HirErr => {
                            println!("{}", "语义错误".red())
                        }
                    }
                }
            }
        });
    let result = runtime.unwrap().join();
    match result {
        Ok(_) => {}
        Err(_) => {
            println!("{}", "工作线程panic".red())
        }
    }
}