use super::super::passes::{dom::DominatorTreeBuilder, loop_tree::LoopTree};
use crate::frontend::llvm::{
    function::Function,
    instr::{Call, Instruction, Phi, RegUseInstr},
    layout::{BasicBlockNode, InstructionNode},
    llvm_module::LLVMModule,
};
use getset::{Getters, MutGetters};
use std::collections::{HashMap, HashSet};

#[inline]
fn is_pinned_instruction(instr: &Instruction) -> bool {
    // instructions that are control dependent
    // non-pure function calls( side effect functions)
    // load /store instructions
    instr.is_branch()
        || instr.is_ret()
        || instr.is_phi()
        || instr.is_load()
        || instr.is_store()
        || instr.is_call()
        || instr.is_alloca()
}

fn lca_blkid(
    mut blk1: i32,
    mut blk2: i32,
    dom_depth_map: &HashMap<BlockId, i32>,
    dom_ctx: &DomTreeExtractor,
) -> i32 {
    while blk1 != blk2 {
        if dom_depth_map[&blk1] > dom_depth_map[&blk2] {
            blk1 = dom_ctx.idom_map[&blk1];
        } else if dom_depth_map[&blk1] < dom_depth_map[&blk2] {
            blk2 = dom_ctx.idom_map[&blk2];
        } else {
            blk1 = dom_ctx.idom_map[&blk1];
            blk2 = dom_ctx.idom_map[&blk2];
        }
    }
    blk1
}

pub fn gcm_for_module(module: &mut LLVMModule) {
    module.for_each_user_func_mut(|f| {
        gcm_for_function(f);
    });
}

pub fn gcm_for_function(f: &mut Function) {
    let entry = f.entry_bb_id();
    let dom_extractor: DomTreeExtractor;
    let loop_depth: HashMap<i32, i32>;
    {
        let mut dom_ctx = DominatorTreeBuilder::new(f);
        dom_ctx.build_dominator_frontier();
        dom_ctx.build_dominators_map();
        let mut loop_ctx = LoopTree::new(f);
        loop_ctx.loop_tree_builder(&dom_ctx);
        loop_depth = loop_ctx.loop_depth_map().clone();
        dom_extractor = DomTreeExtractor::new(dom_ctx);
    }

    let mut ctx = GCMContext::new();
    {
        // gcm_ctx_construction from dom
        ctx.construct_def_use(f);
        ctx.dom_depth_mut().insert(entry, 0);
        ctx.dom_depth_construct(entry, dom_extractor.dom_children_map());
    }
    let mut visited_instrs: HashSet<InstrId> = HashSet::new();
    ctx.schedule_early(f, &mut visited_instrs);
    ctx.schedule_late(f, &mut visited_instrs, &dom_extractor, &loop_depth);
}

type BlockId = i32;
type InstrId = i32;
type RegId = i32;
#[derive(Getters, MutGetters)]
pub struct GCMContext {
    #[getset(get = "pub", get_mut = "pub")]
    defs: HashMap<RegId, InstrId>,
    #[getset(get = "pub", get_mut = "pub")]
    uses: HashMap<RegId, HashSet<InstrId>>,
    #[getset(get = "pub", get_mut = "pub")]
    dom_depth: HashMap<BlockId, i32>,
    func_arg_register_id: HashSet<RegId>,
}

#[inline]
fn extract_use(reg_use_instr: &dyn RegUseInstr) -> Vec<RegId> {
    // return id()s of each non-immmediate and non-global register
    reg_use_instr
        .uses()
        .into_iter()
        .filter(|reg| !(reg.is_immediate() || reg.is_global()))
        .map(|reg| {
            assert!(!(reg.is_immediate() || reg.is_global()));
            *reg.id()
        })
        .collect::<Vec<RegId>>()
}

impl GCMContext {
    fn new() -> Self {
        Self {
            defs: HashMap::new(),
            uses: HashMap::new(),
            dom_depth: HashMap::new(),
            func_arg_register_id: HashSet::new(),
        }
    }
    #[inline]
    fn is_function_argument_reg(&self, reg_id: &RegId) -> bool {
        return self.func_arg_register_id.contains(reg_id);
    }

    fn schedule_early(&self, f: &mut Function, instr_visited: &mut HashSet<InstrId>) {
        let instrs = f
            .instructions()
            .iter()
            .filter(|(_, instr_node)| is_pinned_instruction(*instr_node))
            .map(|(instr_id, _)| *instr_id)
            .collect::<Vec<InstrId>>();

        for cur_instr_id in instrs {
            if instr_visited.contains(&cur_instr_id) {
                continue;
            }
            instr_visited.insert(cur_instr_id);
            let cur_instr = f.instructions().get(&cur_instr_id).unwrap();
            assert!(is_pinned_instruction(cur_instr));
            if let Some(reg_use_instr) = cur_instr.instr().try_as_reg_use_instr() {
                let use_vec = extract_use(reg_use_instr);
                for reg_use_id in use_vec {
                    assert!(reg_use_id != -1);
                    if let Some(def_instr_id) = self.defs.get(&reg_use_id).clone() {
                        self.instr_schedule_early(*def_instr_id, instr_visited, f);
                    } else {
                        // argument assertions to gurantee correctness
                        // each argument of function call is passed to local variable thus
                        // assert!(self.uses.get(&reg_use_id).unwrap().len() == 1);
                        assert!(self.is_function_argument_reg(&reg_use_id));
                    }
                }
            }
        }
    }

    fn instr_schedule_early(
        &self,
        arg_instr_id: InstrId,
        visited: &mut HashSet<InstrId>,
        f: &mut Function,
    ) -> BlockId {
        let mut target_bb = f.entry_bb_id();
        {
            let arg_instr = f.instructions().get(&arg_instr_id).unwrap();
            if visited.contains(&arg_instr_id) || is_pinned_instruction(arg_instr) {
                return arg_instr.bb_id();
            }
            visited.insert(arg_instr_id);
            if let Some(reg_use_instr) = arg_instr.instr().try_as_reg_use_instr() {
                let use_vec: Vec<RegId> = extract_use(reg_use_instr);
                for reg_use_id in use_vec {
                    assert!(reg_use_id != -1);
                    let ret;
                    if let Some(def_instr_id) = self.defs.get(&reg_use_id) {
                        ret = self.instr_schedule_early(*def_instr_id, visited, f);
                    } else {
                        // argument assertions to gurantee correctness
                        // each argument of function call is passed to local variable thus
                        // assert!(self.uses.get(&reg_use_id).unwrap().len() == 1);
                        assert!(self.is_function_argument_reg(&reg_use_id));
                        ret = f.entry_bb_id();
                    }
                    if self.dom_depth[&ret] > self.dom_depth[&target_bb] {
                        target_bb = ret;
                    }
                }
            }
        }
        let arg_instr = f.instructions().get(&arg_instr_id).unwrap();
        assert!(!is_pinned_instruction(arg_instr));
        if target_bb != arg_instr.bb_id() {
            instruction_movement(arg_instr_id, f, target_bb, false);
            assert!(f.instructions().get(&arg_instr_id).unwrap().bb_id() == target_bb);
        }
        return f.instructions().get(&arg_instr_id).unwrap().bb_id();
    }

    fn schedule_late(
        &mut self,
        f: &mut Function,
        instr_visited: &mut HashSet<InstrId>,
        dom_ctx: &DomTreeExtractor,
        loop_depth_map: &HashMap<i32, i32>,
    ) {
        // clear visited set from schedule_early
        instr_visited.clear();
        let instrs = f
            .instructions()
            .into_iter()
            .filter(|(_, instr_node)| is_pinned_instruction(*instr_node))
            .map(|(instr_id, _)| *instr_id)
            .collect::<Vec<InstrId>>();
        for cur_instr_id in instrs {
            if instr_visited.contains(&cur_instr_id) {
                continue;
            }
            instr_visited.insert(cur_instr_id);
            let cur_instr = f.instructions().get(&cur_instr_id).unwrap();
            assert!(is_pinned_instruction(cur_instr));
            // schedule the instruction use that this definition
            if let Some(reg_wrt_instr) = cur_instr.instr().try_as_reg_write_instr() {
                if let Some(def_reg) = reg_wrt_instr.des_register() {
                    let def_reg_id = def_reg.id();
                    for use_instr_id in self.uses.get(def_reg_id).unwrap().clone() {
                        self.instr_schedule_late(
                            use_instr_id,
                            instr_visited,
                            f,
                            dom_ctx,
                            loop_depth_map,
                        );
                    }
                } else {
                    // void call function
                    assert!(cur_instr.is_call());
                    #[cfg(debug_assertions)]
                    {
                        let call_instr = cur_instr.instr().as_any().downcast_ref::<Call>().unwrap();
                        let call_func_name = call_instr.func_name();
                        println!("void function :{call_func_name}");
                    }
                }
            }
        }
    }

    fn instr_schedule_late(
        &mut self,
        arg_instr_id: InstrId,
        visited: &mut HashSet<InstrId>,
        f: &mut Function,
        dom_ctx: &DomTreeExtractor,
        loop_depth: &HashMap<i32, i32>,
    ) -> BlockId {
        #[inline]
        fn phi_casting(instr: &Instruction) -> Option<&Phi> {
            instr.instr().as_any().downcast_ref::<Phi>()
        }

        // return the definition derived from this reg_wrt_instr
        #[inline]
        fn reg_wrt_id(instr: &Instruction) -> Option<i32> {
            if let Some(reg_wrt_instr) = instr.instr().try_as_reg_write_instr() {
                Some(*reg_wrt_instr.des_register().unwrap().id())
            } else {
                None
            }
        }

        #[inline]
        fn ins_node<'function_lifetime>(
            cur_instr_id: &InstrId,
            f: &'function_lifetime Function,
        ) -> &'function_lifetime Instruction {
            f.instructions().get(cur_instr_id).unwrap()
        }

        let arg_instr_as_reg_wrt;
        let arg_instr_blkid: BlockId;
        {
            let arg_instr: &Instruction = ins_node(&arg_instr_id, f);
            arg_instr_blkid = arg_instr.bb_id();
            if visited.contains(&arg_instr_id) || is_pinned_instruction(arg_instr) {
                return arg_instr_blkid;
            }
            arg_instr_as_reg_wrt = reg_wrt_id(arg_instr);
        }
        visited.insert(arg_instr_id);
        let mut lca: Option<BlockId> = None;
        // only destination regsiter is used for cur_instr
        if let Some(des_id) = arg_instr_as_reg_wrt {
            // instruction with value of argument(arg_instr_id) is a reg_wrt_instruction
            for use_instr_id in self.uses().get(&des_id).unwrap().clone() {
                let mut res = Vec::new();
                res.push(self.instr_schedule_late(use_instr_id, visited, f, dom_ctx, loop_depth));
                let use_instr = ins_node(&use_instr_id, f);
                if let Some(phi_instr) = phi_casting(use_instr) {
                    res.clear();
                    let mut pre_bbs: HashSet<BlockId> = HashSet::new();
                    for (src_reg, src_blk_id) in phi_instr.uses() {
                        if *src_reg.id() == des_id {
                            pre_bbs.insert(*src_blk_id);
                        }
                    }
                    assert!(!pre_bbs.is_empty());
                    let predecessors = f
                        .basic_blocks()
                        .get(&use_instr.bb_id())
                        .unwrap()
                        .prev_bb()
                        .into_iter()
                        .map(|id| (*id))
                        .collect::<HashSet<BlockId>>();
                    let intersect_set = pre_bbs
                        .intersection(&predecessors)
                        .into_iter()
                        .map(|id| (*id))
                        .collect::<Vec<BlockId>>();
                    let mut find_set = Vec::<BlockId>::new();
                    for predecessor in predecessors {
                        if pre_bbs.contains(&predecessor) {
                            find_set.push(predecessor);
                        }
                    }
                    assert!(find_set == intersect_set);
                    res = find_set;
                }

                for res_blk in res {
                    if let Some(lca_blk) = lca {
                        lca = Some(lca_blkid(lca_blk, res_blk, &self.dom_depth, dom_ctx));
                    } else {
                        lca = Some(res_blk);
                    }
                }
            }

            {
                let arg_instr: &Instruction = ins_node(&arg_instr_id, f);
                if is_pinned_instruction(arg_instr) {
                    assert!(false);
                    // unreachable one
                    return arg_instr_blkid;
                }
            }
            assert!(lca.is_some());
            let mut best = lca.unwrap();
            let mut cur = lca.unwrap();

            while cur != arg_instr_blkid {
                if loop_depth.get(&cur).unwrap() < loop_depth.get(&best).unwrap() {
                    best = cur;
                }
                cur = dom_ctx.idom_map.get(&cur).unwrap().clone();
            }

            if best != arg_instr_blkid {
                instruction_movement(arg_instr_id, f, best, true);
                assert!(f.instructions().get(&arg_instr_id).unwrap().bb_id() == best);
            }
            return f.instructions().get(&arg_instr_id).unwrap().bb_id();
        } else {
            // instruction with value of argument(arg_instr_id) is not a reg_write_instruction
            return arg_instr_blkid;
        }
    }

    fn construct_def_use(&mut self, f: &Function) {
        // construct normal def/use
        for bb_id in f.layout().block_iter() {
            for inst_id in f.layout().inst_iter(bb_id) {
                // fetch out the instructions
                let cur_instr: &Instruction = f.instructions().get(&inst_id).unwrap();
                // construct the defs
                self.construct_def_use_for_instr(cur_instr, inst_id);
            }
        }
        // reg_id from function declaration arg
        self.func_arg_register_id = f
            .arg_list()
            .as_normal()
            .unwrap()
            .iter()
            .map(|lval| lval.id().clone())
            .collect::<HashSet<RegId>>();
    }

    fn construct_def_use_for_instr(&mut self, cur_instr: &Instruction, inst_id: i32) {
        if let Some(reg_write_instr) = cur_instr.instr().try_as_reg_write_instr() {
            if let Some(des_reg) = reg_write_instr.des_register() {
                // ssa requirement : each register can only be defined once
                assert!(self.defs.get(des_reg.id()).is_none());
                if self.uses.get(des_reg.id()).is_none() {
                    self.uses.insert(*des_reg.id(), HashSet::new());
                }
                self.defs.insert(des_reg.id().clone(), inst_id);
            }
        }
        if let Some(reg_use_instr) = cur_instr.instr().try_as_reg_use_instr() {
            let use_vec = extract_use(reg_use_instr);
            for reg_use_id in use_vec {
                assert!(reg_use_id != -1);
                if let Some(use_instr_set) = self.uses.get_mut(&reg_use_id) {
                    use_instr_set.insert(inst_id);
                } else {
                    self.uses.insert(reg_use_id, HashSet::new());
                    self.uses.get_mut(&reg_use_id).unwrap().insert(inst_id);
                }
            }
        }
    }

    fn dom_depth_construct(
        &mut self,
        cur_bb: BlockId,
        dom_child_map: &HashMap<BlockId, HashSet<BlockId>>,
    ) {
        for child in dom_child_map.get(&cur_bb).unwrap() {
            self.dom_depth
                .insert(*child, self.dom_depth.get(&cur_bb).unwrap() + 1);
            self.dom_depth_construct(*child, dom_child_map);
        }
    }
}

fn instruction_movement(
    arg_instr_id: InstrId,
    f: &mut Function,
    target_blkid: BlockId,
    is_front: bool,
) {
    #[inline]
    fn ins_node(f: &mut Function, current_instruction_id: i32) -> &mut InstructionNode {
        f.layout_mut()
            .instructions_mut()
            .get_mut(&current_instruction_id)
            .unwrap()
    }
    #[inline]
    fn blk_node(f: &mut Function, current_bb_id: i32) -> &mut BasicBlockNode {
        f.layout_mut()
            .basic_blocks_mut()
            .get_mut(&current_bb_id)
            .unwrap()
    }

    #[inline]
    fn instruction_registration(f: &mut Function, target_blkid: i32, arg_instr_id: i32) {
        let current_blk_node = blk_node(f, target_blkid).clone();
        let arg_instr_node = ins_node(f, arg_instr_id);
        arg_instr_node
            .next
            .replace(current_blk_node.first_inst.unwrap());
        arg_instr_node.prev.take();
    }

    let arg_instr_node = ins_node(f, arg_instr_id).clone();
    let mut before_movement_instr_set = HashSet::<InstrId>::new();
    let mut after_movement_instr_set = HashSet::<InstrId>::new();
    #[cfg(debug_assertions)]
    {
        // println the movemnent
        let arg_instruction_object = f.instructions().get(&arg_instr_id).unwrap();
        let direction = if is_front { "front" } else { "back" };
        println!(
            "{} | {:?}: {} {} {}",
            arg_instr_id,
            arg_instruction_object,
            arg_instr_node.block.unwrap(),
            direction,
            target_blkid
        );
        for bb_id in f.layout().block_iter() {
            for layout_instr_id in f.layout().inst_iter(bb_id) {
                before_movement_instr_set.insert(layout_instr_id);
                assert!(f.instructions().get(&layout_instr_id).unwrap().bb_id() == bb_id);
            }
        }
    }
    let previous_blk_node = blk_node(f, arg_instr_node.block.unwrap()).clone();
    // currrent instruction must not be the last one
    assert!(arg_instr_id != previous_blk_node.last_inst.unwrap());

    // unregister the instruction
    {
        let (prev_node, next_id) = (arg_instr_node.prev, arg_instr_node.next.unwrap());
        if let Some(prev_id) = prev_node {
            let next_node = ins_node(f, next_id);
            next_node.prev.replace(prev_id);
            let prev_node = ins_node(f, prev_id);
            prev_node.next.replace(next_id);
        } else {
            assert!(arg_instr_id == previous_blk_node.first_inst.unwrap());
            let next_node = ins_node(f, next_id);
            next_node.prev.take();
            let previous_blk_node = blk_node(f, arg_instr_node.block.unwrap());
            previous_blk_node.first_inst.replace(next_id);
        }
    }

    // register the instruction in the target block
    if is_front {
        //front append
        instruction_registration(f, target_blkid, arg_instr_id);
        f.layout_mut()
            .insert_inst_at_start(arg_instr_id, target_blkid);
    } else {
        let current_blk_node = blk_node(f, target_blkid).clone();
        if let Some(last) = current_blk_node.last_inst {
            let last_instr = ins_node(f, last);
            if let Some(prev_before_last) = last_instr.prev {
                // insert the node between prev and last
                last_instr.prev.replace(arg_instr_id);
                let prev_node = ins_node(f, prev_before_last);
                prev_node.next.replace(arg_instr_id);

                let arg_instr_node = ins_node(f, arg_instr_id);
                arg_instr_node.prev.replace(prev_before_last);
                arg_instr_node.next.replace(last);
            } else {
                assert!(last == current_blk_node.first_inst.unwrap());
                instruction_registration(f, target_blkid, arg_instr_id);
                f.layout_mut()
                    .insert_inst_at_start(arg_instr_id, target_blkid);
            }
        } else {
            // empty block
            assert!(current_blk_node.first_inst.is_none() && current_blk_node.last_inst.is_none());
            let arg_instr_node = ins_node(f, arg_instr_id);
            arg_instr_node.next.take();
            arg_instr_node.prev.take();
            f.layout_mut()
                .insert_inst_at_start(arg_instr_id, target_blkid);
        }
    }
    let arg_instr = f.instructions_mut().get_mut(&arg_instr_id).unwrap();
    *arg_instr.bb_id_mut() = target_blkid;
    ins_node(f, arg_instr_id).block.replace(target_blkid);
    #[cfg(debug_assertions)]
    {
        for bb_id in f.layout().block_iter() {
            for layout_instr_id in f.layout().inst_iter(bb_id) {
                after_movement_instr_set.insert(layout_instr_id);
                assert!(f.instructions().get(&layout_instr_id).unwrap().bb_id() == bb_id);
            }
        }
    }
    assert!(before_movement_instr_set == after_movement_instr_set);
}

#[allow(dead_code)]
#[derive(Getters)]
struct DomTreeExtractor {
    #[getset(get = "pub")]
    children_map: HashMap<BlockId, HashSet<BlockId>>,
    #[getset(get = "pub")]
    dominators_map: HashMap<BlockId, HashSet<BlockId>>,
    #[getset(get = "pub")]
    dominator_frontier_map: HashMap<BlockId, HashSet<BlockId>>,
    #[getset(get = "pub")]
    dfsn_order: Vec<BlockId>,
    #[getset(get = "pub")]
    idom_map: HashMap<BlockId, BlockId>,
    #[getset(get = "pub")]
    dom_children_map: HashMap<BlockId, HashSet<BlockId>>,
}

impl DomTreeExtractor {
    fn new(ctx: DominatorTreeBuilder) -> Self {
        Self {
            children_map: ctx.children_map().clone(),
            dominators_map: ctx.dominators_map().clone(),
            dominator_frontier_map: ctx.dominator_frontier_map().clone(),
            dfsn_order: ctx.dfsn_order().clone(),
            idom_map: ctx.idom_map().clone(),
            dom_children_map: ctx.dom_children_map().clone(),
        }
    }
}

#[cfg(test)]
mod test {
    extern crate antlr_rust;
    extern crate structopt;

    use super::gcm_for_module;
    use crate::frontend::{
        antlr_dep::sysylexer::SysYLexer, antlr_dep::sysyparser::SysYParser,
        antlr_dep::sysyvisitor::SysYVisitor, ast_visitor::SysYAstVisitor,
        error_listener::SysYErrorListener, llvm::llvm_module::LLVMModule,
    };
    use crate::optimize::passes::bb_ops::remove_phi;
    use crate::optimize::passes::{
        check_ir::check_module,
        dce::{remove_unused_def, remove_useless_bb},
        mem2reg::mem2reg,
    };
    use antlr_rust::{common_token_stream::CommonTokenStream, InputStream, Parser as AntlrParser};
    use std::fs::File;
    use std::io::Write;
    use std::str::FromStr;

    // #[test]
    #[allow(dead_code)]
    fn main() {
        let file_name_noext = "56_sort_test2";
        let path_buf = "test/functional/";
        let test_infile_name = format!("{file_name_noext}.sy");
        let test_outfile_name = format!("{file_name_noext}.ll");
        let output_path = format!("./ci_output/{}", test_outfile_name);
        let mut output_file = File::create(output_path).expect("cannot open output file");

        let cur_log_level = "Debug";
        {
            let env = env_logger::Env::new();
            let mut builder = env_logger::Builder::new();
            builder
                .filter_level(log::LevelFilter::from_str(cur_log_level).expect("wrong log level"));
            builder.parse_env(env);
            builder.init();
        }
        let contents = std::fs::read_to_string(format!("{}{}", path_buf, test_infile_name))
            .expect("cannot open source file");
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
        check_module(&llvm_module);
        remove_unused_def(&mut llvm_module);
        check_module(&llvm_module);
        writeln!(output_file, "Before:{}", llvm_module).expect("cannot write to output file");
        gcm_for_module(&mut llvm_module);
        writeln!(output_file, "after gcm: {}", llvm_module).expect("cannot write to output file");
        check_module(&llvm_module);
        remove_phi(&mut llvm_module);
        // check_module(&llvm_module);
        //check_module(&llvm_module);
        llvm_module.before_backend();

        writeln!(output_file, "final: {}", llvm_module).expect("cannot write to output file");

        //println!("{}", llvm_module);
    }
}
