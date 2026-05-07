extern crate antlr_rust;
extern crate structopt;

use antlr_rust::{common_token_stream::CommonTokenStream, InputStream, Parser as AntlrParser};
use std::fs::File;
use std::io::Write;
use std::str::FromStr;
use structopt::StructOpt;
use sysycc_compiler::backend::program::Program;
use sysycc_compiler::frontend::{
    antlr_dep::sysylexer::SysYLexer, antlr_dep::sysyparser::SysYParser,
    antlr_dep::sysyvisitor::SysYVisitor, ast_visitor::SysYAstVisitor,
    error_listener::SysYErrorListener, llvm::llvm_module::LLVMModule,
};
use sysycc_compiler::optimize::optimize_ir;

/// Command Line Options Parser
#[derive(StructOpt, Debug)]
#[structopt(name = "compiler")]
struct CompilerOptions {
    input_file: std::path::PathBuf,
    #[structopt(short = "S", help = "Compile only, do not assemble or link")]
    _compile_only: bool, // useless, just a occupation for compatibility
    #[structopt(short, help = "output assembly file")]
    output_file: Option<String>,
    #[structopt(long, default_value = "INFO", help = "config log filter level")]
    log_level: String,
    #[structopt(short = "O", default_value = "0", help = "optimization level")]
    _optimization_level: u8,
    #[structopt(short = "--enable", help = "enable optimization passes")]
    enable_passes: Vec<String>,
    #[structopt(short = "--disable", help = "disable optimization passes")]
    disable_passes: Vec<String>,
}

fn parse_passes(cmdline_options: &CompilerOptions) -> Vec<String> {
    let mut passes = vec![
        "opt".to_string(),
        "mem2reg".to_string(),
        "gvn".to_string(),
        "gcm".to_string(),
        "func_inline".to_string(),
        "misc".to_string(),
        "peephole".to_string(),
        "asm".to_string(),
    ];
    for pass in cmdline_options.enable_passes.iter() {
        if !passes.contains(pass) {
            passes.push(pass.clone());
        }
    }
    for pass in cmdline_options.disable_passes.iter() {
        if let Some(pos) = passes.iter().position(|x| x == pass) {
            passes.remove(pos);
        }
    }
    passes
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

    let enable_passes = parse_passes(&cmdline_options);

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
    optimize_ir(&mut llvm_module, &enable_passes);

    if enable_passes.contains(&"ir".to_string()) {
        if let Some(output_path) = &cmdline_options.output_file {
            let mut output_file = File::create(output_path).expect("cannot open output file");
            write!(output_file, "{}", llvm_module).expect("cannot write to output file");
        } else {
            println!("{}", llvm_module);
        }
    }

    /* backend */
    let mut program = Program::from_llvm_module(&llvm_module);
    program.do_backend_passes();

    if enable_passes.contains(&"asm".to_string()) {
        if let Some(output_path) = &cmdline_options.output_file {
            let mut output_file = File::create(output_path).expect("cannot open output file");
            write!(output_file, "{}", program.gen_asm()).expect("cannot write to output file");
        } else {
            println!("{}", program.gen_asm());
        }
    }
}
