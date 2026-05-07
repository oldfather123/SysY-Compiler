extern crate antlr_rust;
extern crate structopt;

use antlr_rust::{common_token_stream::CommonTokenStream, InputStream, Parser as AntlrParser};
use std::fs::OpenOptions;
use std::io::Write;
use std::str::FromStr;
use structopt::StructOpt;
use sysycc_compiler::frontend::{
    antlr_dep::sysylexer::SysYLexer, antlr_dep::sysyparser::SysYParser,
    antlr_dep::sysyvisitor::SysYVisitor, ast_visitor::SysYAstVisitor,
    error_listener::SysYErrorListener, llvm::llvm_module::LLVMModule,
};
use sysycc_compiler::optimize::passes::dom::DominatorTreeBuilder;

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
    // dom
    llvm_module.for_each_user_func(|func| {
        let mut dom = DominatorTreeBuilder::new(func);
        dom.build_dominator_frontier();
        if let Some(output_path) = &cmdline_options.output_file {
            let mut output_file = OpenOptions::new()
                .create(true)
                .write(true)
                .append(true)
                .open(output_path)
                .expect("cannot open output file");
            writeln!(
                output_file,
                "{}'s children_map: {:#?}",
                func.name(),
                dom.children_map()
            )
            .expect("cannot write to output file");
            writeln!(
                output_file,
                "{}'s idom_map: {:#?}",
                func.name(),
                dom.idom_map()
            )
            .expect("cannot write to output file");
            writeln!(
                output_file,
                "{}'s dominator_frontier_map: {:#?}",
                func.name(),
                dom.dominator_frontier_map()
            )
            .expect("cannot write to output file");
        } else {
            println!("{}'s idom_map: {:#?}", func.name(), dom.idom_map());
            println!(
                "{}'s dominator_frontier_map: {:#?}",
                func.name(),
                dom.dominator_frontier_map()
            );
        };
    });
}
