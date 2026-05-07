use crate::frontend::llvm::function::Function;
use getset::{Getters, MutGetters};
use std::collections::{HashMap, HashSet};

type IdBB = i32;

#[derive(Debug, Getters, MutGetters)]
pub struct DominatorTreeBuilder<'func_lifetime> {
    #[getset(get = "pub", get_mut = "pub")]
    f: &'func_lifetime Function,
    root: IdBB,
    #[getset(get = "pub")]
    children_map: HashMap<IdBB, HashSet<IdBB>>,
    #[getset(get = "pub")]
    dominators_map: HashMap<IdBB, HashSet<IdBB>>,
    #[getset(get = "pub")]
    dominator_frontier_map: HashMap<IdBB, HashSet<IdBB>>,
    #[getset(get = "pub")]
    dfsn_order: Vec<IdBB>,
    #[getset(get = "pub")]
    idom_map: HashMap<IdBB, IdBB>,
    #[getset(get = "pub")]
    dom_children_map: HashMap<IdBB, HashSet<IdBB>>,
}

impl<'func_lifetime> DominatorTreeBuilder<'func_lifetime> {
    pub fn new(f: &'func_lifetime Function) -> Self {
        let root = f.layout().entry_block().expect("entry block not found");
        Self {
            f,
            root: root,
            children_map: HashMap::new(),
            dominators_map: HashMap::new(),
            dominator_frontier_map: HashMap::new(),
            dfsn_order: Vec::new(),
            idom_map: HashMap::new(),
            dom_children_map: HashMap::new(),
        }
    }

    pub fn build_dominators_map(&mut self) {
        let universe: HashSet<IdBB> = self.f.layout().block_iter().collect();
        for bb in self.f.layout().block_iter() {
            self.dominators_map.insert(bb, universe.clone());
        }
        self.dominators_map
            .get_mut(&self.root)
            .unwrap()
            .retain(|x| x == &self.root);
        let mut changed = true;
        while changed {
            changed = false;
            for bb in self.f.layout().block_iter() {
                let mut dom_set: HashSet<IdBB> = HashSet::new();
                for pred in self.f.bb(bb).unwrap().prev_bb() {
                    if dom_set.is_empty() {
                        dom_set = self.dominators_map[pred].clone();
                    } else {
                        dom_set = dom_set
                            .intersection(&self.dominators_map[pred])
                            .cloned()
                            .collect();
                        // dom = dom & pred.dom where dom is nonempty
                    }
                }
                dom_set.insert(bb);
                // dom = dom | bb
                if dom_set != self.dominators_map[&bb] {
                    self.dominators_map.insert(bb, dom_set);
                    changed = true;
                }
            }
        }
    }

    // answer where child is dominated by parent( parent is a member of child's dominators set)
    pub fn is_dom(&self, child: IdBB, parent: IdBB) -> bool {
        self.dominators_map
            .get(&child)
            .expect(format!(" blkid: {} of function: {} not found", child, self.f.name()).as_str())
            .contains(&parent)
    }

    fn dfsn(
        &self,
        bb: IdBB,
        current_level: i32,
        idfn: &mut Vec<IdBB>,
        order: &mut HashMap<IdBB, i32>,
    ) {
        if order[&bb] == current_level {
            return;
        }
        if current_level == 1 {
            idfn.push(bb);
        }
        *(order.get_mut(&bb).unwrap()) = current_level;
        for child in self.children_map[&bb].iter() {
            self.dfsn(*child, current_level, idfn, order);
        }
    }

    /// record each cfg node's children nodes in a hashmap
    fn build_children_map(&mut self) {
        for bb in self.f.layout().block_iter() {
            let children = self.f.bb(bb).unwrap().succ_bb().clone();
            self.children_map.insert(bb, children.into_iter().collect());
        }
    }

    fn build_dfsn_order_from_idom(&mut self) {
        self.build_dom_childrent_map();
        let mut visited: HashSet<IdBB> = HashSet::new();
        self.build_dfsn_order(&mut visited, self.f.entry_bb_id());
    }

    fn build_dom_childrent_map(&mut self) {
        for bb in self.f.layout().block_iter() {
            self.dom_children_map.insert(bb, HashSet::new());
        }
        for (bb, fa) in self.idom_map.iter() {
            if *bb != self.f.entry_bb_id() {
                self.dom_children_map.get_mut(fa).unwrap().insert(*bb);
            }
        }
    }

    fn build_dfsn_order(&mut self, visited: &mut HashSet<IdBB>, root: IdBB) {
        if !visited.contains(&root) {
            visited.insert(root);
            self.dfsn_order.push(root);
            for child in self.dom_children_map.get(&root).unwrap().clone() {
                self.build_dfsn_order(visited, child);
            }
        }
    }

    pub fn build_dominator_frontier(&mut self) {
        // initialize
        self.build_children_map();
        // find idom naive algorithm: multi dfs
        let mut order /* dfn order */ = self.f
            .layout()
            .block_iter()
            .map(|bb| (bb, 0))
            .collect::<HashMap<_, _>>();
        let mut level = 1;
        let mut idfn: Vec<IdBB> = Vec::with_capacity(order.len());
        self.dfsn(self.root, level, &mut idfn, &mut order);
        for node in idfn {
            level += 1;
            *(order.get_mut(&node).unwrap()) = level;
            self.dfsn(self.root, level, &mut vec![], &mut order);
            for dom in self.f.layout().block_iter() {
                if order[&dom] != level {
                    self.idom_map.insert(dom, node);
                }
            }
        }
        // build dfsn_order_from_idom
        self.build_dfsn_order_from_idom();
        // calculate dominent frontier from idom algorithm: engineering a compiler 2nd edition p. 499
        let mut init_df = || {
            for bb in self.f.layout().block_iter() {
                self.dominator_frontier_map.insert(bb, HashSet::new());
            }
        };
        init_df();
        for bb in self.f.layout().block_iter() {
            let preds = self.f.bb(bb).unwrap().prev_bb();
            if preds.len() >= 2 {
                for pred in preds {
                    let mut runner = *pred;
                    while runner != self.idom_map[&bb] {
                        let runner_df = self.dominator_frontier_map.get_mut(&runner).expect(
                            format!("dom_froniter of {:#?} dose not exist", runner).as_str(),
                        );
                        runner_df.insert(bb);
                        runner = *self.idom_map.get(&runner).expect(
                            format!(
                                "idom of {:#?} dose not exist
                            \n predecessors: {:#?}, current_predecessor: {:#?}
                            \n module name: {}",
                                runner,
                                preds,
                                pred,
                                self.f.name()
                            )
                            .as_str(),
                        );
                    }
                }
            }
        }
    }
}

// #[cfg(test)]
// mod tests {
//     use super::super::vicis_core::parser::assembly::module::parse;
//     use super::*;
//     use std::fs;
//     #[test]
//     fn children_map_test() {
//         let ir =
//             fs::read_to_string("../../test/vicis_ir/loop.ll").expect("failed to load *.ll file");
//         let module = parse(ir.as_str()).expect("failed to parse LLVM Assembly");
//         for iterf in module.functions().iter() {
//             let mut test_dom = DominatorTree::new(iterf.1);
//             test_dom.dom_set_builder(iterf.1);
//             println!("dominators_map: {:?}", test_dom.dominators_map);
//             test_dom.children_map_builder(iterf.1);
//             println!("children_map: {:?}", test_dom.children_map);
//             test_dom.dominator_frontier_builder(iterf.1);
//             println!("idom_map: {:?}", test_dom.idom_map);
//             println!(
//                 "dominator_frontier_map: {:?}",
//                 test_dom.dominator_frontier_map
//             );
//         }
//     }
// }
