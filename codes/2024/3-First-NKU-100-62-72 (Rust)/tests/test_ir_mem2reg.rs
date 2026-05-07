use orzcc::{
    collections::diagnostic::RenderOptions,
    frontend::ir::{into_ir, Parser},
    ir::{passes::mem2reg::Mem2reg, passman::GlobalPassMut},
};

#[test]
fn test_ir_mem2reg_basic() {
    let src = include_str!("ir/mem2reg_basic.orzir");
    let parser = Parser::new(src);
    let (ast, mut ctx, mut diag) = parser.parse();

    if !diag.is_empty() {
        println!("{:#?}", ast);
        println!("{}", diag.render(src, &RenderOptions::unicode_round()));
        panic!("ast failed");
    }

    if into_ir(ast, &mut ctx, &mut diag).is_some() {
        println!("{}", ctx.display(true));
    } else {
        println!("{}", diag.render(src, &RenderOptions::unicode_round()));
        panic!("conversion failed");
    }

    let mut mem2reg = Mem2reg::default();

    match GlobalPassMut::run(&mut mem2reg, &mut ctx) {
        Ok((_, changed)) => {
            assert!(changed);
            ctx.alloc_all_names();
            println!("{}", ctx.display(true));

            assert_eq!(ctx.lookup_block("ret").unwrap().params(&ctx).len(), 1);
            assert!(ctx.lookup_value("0").is_none()); // should be removed
        }
        Err(e) => {
            panic!("test failed: {:?}", e);
        }
    }
}
