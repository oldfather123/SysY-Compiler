extern crate antlr_rust;
extern crate structopt;

use antlr_rust::{common_token_stream::CommonTokenStream, InputStream, Parser as AntlrParser};
use std::fs::File;
use std::io::Write;
use std::str::FromStr;
use structopt::StructOpt;
use sysycc_compiler::frontend::{
    antlr_dep::sysylexer::SysYLexer, antlr_dep::sysyparser::SysYParser,
    antlr_dep::sysyvisitor::SysYVisitor, ast_visitor::SysYAstVisitor,
    error_listener::SysYErrorListener, llvm::llvm_module::LLVMModule,
};
use sysycc_compiler::optimize::passes::dce::remove_useless_bb;
use sysycc_compiler::optimize::passes::{
    check_ir::check_module, dce::remove_unused_def, gcm::gcm_for_module, mem2reg::mem2reg,
    check_ir::check_module, dce::remove_unused_def, gvn::global_value_numbering, mem2reg::mem2reg,
};

/// Command Line Options Parser
#[derive(StructOpt, Debug)]
#[structopt(name = "sysy_optimize")]
struct CompilerOptions {
    input_file: std::path::PathBuf,
    #[structopt(short, help = "output file")]
    output_file: Option<String>,
    #[structopt(long, default_value = "INFO", help = "config log filter level")]
    log_level: String,
}

fn main() {
    let cmdline_options = CompilerOptions::from_args();
    {
        let env = env_logger::Env::new();
        let mut builder = env_logger::Builder::new();
        builder.filter_level(
            log::LevelFilter::from_str(&cmdline_options.log_level).expect("wrong log level"),
        );
        builder.parse_env(env);
        builder.init();
    }
    let contents =
        std::fs::read_to_string(cmdline_options.input_file).expect("cannot open source file");
    let input = InputStream::new(contents.as_bytes());

    let lexer = SysYLexer::new(input);
    let token_stream = CommonTokenStream::new(lexer);
    let mut parser = SysYParser::new(token_stream);

    /* register my error listener */
    parser.remove_error_listeners();
    parser.add_error_listener(Box::new(SysYErrorListener::default()));

    let mut llvm_module: LLVMModule = LLVMModule::new();

    /* syntax analysis */
    let mut ast_visitor = SysYAstVisitor::new(&mut llvm_module);
    let ctx = parser.compUnit().expect("syntax error");

    /* semantic analysis */
    ast_visitor.visit_compUnit(&ctx);
    ast_visitor.return_content();

    /* passes */
    // mem2reg
    // println!("{}", llvm_module);
    remove_useless_bb(&mut llvm_module);
    mem2reg(&mut llvm_module);
    remove_unused_def(&mut llvm_module);
    global_value_numbering(&mut llvm_module);
    remove_unused_def(&mut llvm_module);
    gcm_for_module(&mut llvm_module);
    check_module(&llvm_module);

    if let Some(output_path) = cmdline_options.output_file {
        let mut output_file = File::create(output_path).expect("cannot open output file");
        write!(output_file, "{}", llvm_module).expect("cannot write to output file");
    } else {
        println!("{}", llvm_module);
    }
}
