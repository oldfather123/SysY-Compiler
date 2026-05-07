use crate::common::r#type::Type;
use crate::frontend::llvm::instr::FMov;
use crate::frontend::llvm::{
    function::Function,
    instr::{Branch, Instruction, Mov, Phi},
    {llvm_module::LLVMModule, ssa::SSARightValue},
};
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct Movs {
    pub a: [Vec<(SSARightValue, SSARightValue)>; 2],
}

pub fn remove_phi(module: &mut LLVMModule) {
    module.functions_mut().iter_mut().for_each(|(_, f)| {
        remove_phi_in_function(f);
    });
}

pub fn remove_phi_in_function(f: &mut Function) {
    let mut mov_map = HashMap::new();
    let layout = f.layout().clone();

    let mut remove_phis = vec![];
    for bb_id in layout.block_iter() {
        for inst_id in layout.inst_iter(bb_id) {
            let inst = f.instructions().get(&inst_id).unwrap();
            if let Some(Phi { d1, uses }) = inst.instr().as_any().downcast_ref::<Phi>() {
                for (use_, src_bb_id) in uses.iter() {
                    if *use_ != *d1 {
                        let t1 = match use_.get_type() {
                            Type::Float => 1,
                            _ => 0,
                        };
                        let t2 = match d1.get_type() {
                            Type::Float => 1,
                            _ => 0,
                        };
                        assert!(t1 == t2);
                        mov_map
                            .entry((*src_bb_id, bb_id))
                            .or_insert_with(|| Movs {
                                a: [vec![], vec![]],
                            })
                            .a[t1]
                            .push((use_.clone(), d1.clone()));
                    }
                }

                remove_phis.push(inst_id);
            }
        }
    }

    for inst_id in remove_phis.iter() {
        f.remove_inst(*inst_id);
    }
    for ((src_bb_id, bb_id), movs) in mov_map.iter() {
        let new_bb_id = f.alloc_bb();

        // 更新succ和prev
        let blocks = f.basic_blocks_mut();
        blocks.get_mut(&new_bb_id).unwrap().add_succ_bb(*bb_id);
        blocks.get_mut(&new_bb_id).unwrap().add_prev_bb(*src_bb_id);
        blocks
            .get_mut(bb_id)
            .unwrap()
            .replace_prev_bb(*src_bb_id, new_bb_id);
        blocks
            .get_mut(src_bb_id)
            .unwrap()
            .replace_succ_bb(*bb_id, new_bb_id);

        // src 中原来跳转到 bb 的跳转指令，现在跳转到 new_bb

        for inst_id in layout.inst_iter(*src_bb_id) {
            if let Some(inst) = f.instructions_mut().get_mut(&inst_id) {
                if let Some(Branch { label1, label2, .. }) =
                    inst.instr().as_any().downcast_ref::<Branch>()
                {
                    let label1 = *label1;
                    let label2 = *label2;
                    let br = inst
                        .instr_mut()
                        .as_any_mut()
                        .downcast_mut::<Branch>()
                        .unwrap();
                    if label1 == *bb_id {
                        br.label1 = new_bb_id;
                    }
                    if let Some(x) = label2 {
                        if x == *bb_id {
                            br.label2 = Some(new_bb_id);
                        }
                    }
                }
            }
        }

        for mov_vec in movs.a.iter() {
            // println!("{:?}", mov_vec);
            let emit_copy = |a: &SSARightValue, b: &SSARightValue, f: &mut Function| {
                // println!("!!!!!!!!!!!!");
                assert!(a.get_type() == b.get_type());
                if a.get_type() == Type::Float {
                    f.add_inst2bb(Instruction::new(
                        Box::new(FMov::new(b.clone(), a.clone())),
                        new_bb_id,
                    ));
                } else {
                    f.add_inst2bb(Instruction::new(
                        Box::new(Mov::new(b.clone(), a.clone())),
                        new_bb_id,
                    ));
                }
            };

            let mut pred = HashMap::new();
            let mut loc = HashMap::new();
            let mut deg = HashMap::new();

            for (a, b) in mov_vec.iter() {
                pred.insert(b.clone(), a.clone());
                *deg.entry(a.clone()).or_insert(0) += 1;
            }

            // println!("{:?}", deg);
            let mut ready = vec![];
            for (_a, b) in mov_vec.iter() {
                // println!("{:?}", b);
                if !deg.get(b).filter(|x| **x != 0).is_some() {
                    ready.push(b.clone());
                }
            }

            // println!("{:?}", ready);

            while !ready.is_empty() {
                let b = ready.pop().unwrap();
                let a = pred.get(&b).unwrap();
                emit_copy(a, &b, f);
                loc.insert(a, b);

                *deg.get_mut(a).unwrap() -= 1;
                if 0 == *deg.get(a).unwrap() && pred.contains_key(a) {
                    ready.push(a.clone());
                }
            }

            for (_, b) in mov_vec.iter() {
                if matches!(deg.get(b), Some(x) if *x != 0) && loc.contains_key(b) {
                    let mut x = b;
                    loop {
                        deg.insert(x.clone(), 0);
                        ready.push(x.clone());
                        x = pred.get(x).unwrap();
                        if x == b {
                            break;
                        }
                    }

                    ready.push(loc.get(b).unwrap().clone());
                    ready.reverse();

                    while ready.len() > 1 {
                        let x = ready.pop().unwrap();
                        emit_copy(ready.last().unwrap(), &x, f);
                    }
                }
            }

            for (_, b) in mov_vec.iter() {
                if matches!(deg.get(b), Some(x) if *x != 0) {
                    let tmp_reg = f.new_reg(b.get_type());
                    emit_copy(b, &tmp_reg, f);

                    let mut x = b;
                    loop {
                        deg.insert(x.clone(), 0);
                        ready.push(x.clone());
                        x = pred.get(x).unwrap();
                        if x == b {
                            break;
                        }
                    }

                    ready.push(tmp_reg);
                    ready.reverse();

                    while ready.len() > 1 {
                        let x = ready.pop().unwrap();
                        emit_copy(ready.last().unwrap(), &x, f);
                    }
                }
            }
        }

        f.add_inst2bb(Instruction::new(
            Box::new(Branch::new(*bb_id, None, None)),
            new_bb_id,
        ));
    }
}

#[test]
fn main() {
    use super::mem2reg::mem2reg;
    use crate::common::test_file_iter::TestFileIter;
    use crate::frontend::antlr_dep::sysylexer::SysYLexer;
    use crate::frontend::antlr_dep::sysyparser::SysYParser;
    use crate::frontend::antlr_dep::sysyvisitor::SysYVisitor;
    use crate::frontend::ast_visitor::SysYAstVisitor;
    use crate::optimize::passes::dce::remove_useless_bb;
    use antlr_rust::{common_token_stream::CommonTokenStream, InputStream};
    // use structopt::StructOpt;

    fn has_phi(f: &Function) -> bool {
        for bb in f.layout().block_iter() {
            for i in f.layout().inst_iter(bb) {
                if let Some(_) = f
                    .instructions()
                    .get(&i)
                    .unwrap()
                    .instr()
                    .as_any()
                    .downcast_ref::<Phi>()
                {
                    return true;
                }
            }
        }

        false
    }

    // /// Command Line Options Parser
    // #[derive(StructOpt, Debug)]
    // #[structopt(name = "sysy_optimize")]
    // struct CompilerOptions {
    //     input_file: std::path::PathBuf,
    //     #[structopt(short, help = "output file")]
    //     output_file: Option<String>,
    //     #[structopt(long, default_value = "INFO", help = "config log filter level")]
    //     log_level: String,
    // }

    fn test_file(test_path: String) {
        let contents = std::fs::read_to_string(test_path).expect("cannot open source file");
        let input = InputStream::new(contents.as_bytes());

        let lexer = SysYLexer::new(input);
        let token_stream = CommonTokenStream::new(lexer);
        let mut parser = SysYParser::new(token_stream);

        /* register my error listener */
        parser.remove_parse_listeners();
        // parser.add_parse_listener(Box::new(SysYErrorListener::default()));

        let mut llvm_module: LLVMModule = LLVMModule::new();

        /* syntax analysis */
        let mut ast_visitor = SysYAstVisitor::new(&mut llvm_module);
        let ctx = parser.compUnit().expect("syntax error");

        /* semantic analysis */
        ast_visitor.visit_compUnit(&ctx);
        ast_visitor.return_content();

        /* passes */
        // mem2reg
        remove_useless_bb(&mut llvm_module);
        mem2reg(&mut llvm_module);

        let main = llvm_module.functions_mut().get_mut("main").unwrap();
        // assert!(has_phi(main));
        // println!("{}", main);
        remove_phi_in_function(main);
        assert!(!has_phi(main));
        // println!("{}", main);
    }

    for path in TestFileIter::functional() {
        println!("test file: {}", path);
        test_file(path);
    }
}
