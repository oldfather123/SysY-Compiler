use std::collections::{HashSet, HashMap};

use super::{structure::TransUnit, value::InstructionValue, IrPass};

pub struct ComputeCallGraph;

impl IrPass for ComputeCallGraph {
    fn run(&self, unit: &mut TransUnit) {
        unit.compute_callgraph();
    }
}

#[derive(Debug, Clone)]
pub struct CallGraphInfo {
    pub caller: HashSet<String>,
    pub callee: HashSet<String>,
}

impl CallGraphInfo {
    pub fn new() -> Self {
        Self {
            caller: HashSet::new(),
            callee: HashSet::new(),
        }
    }
}

impl Default for CallGraphInfo {
    fn default() -> Self {
        Self::new()
    }
}

fn dfs1(cg: &HashMap<String, CallGraphInfo>, vis: &mut HashSet<String>, s: &mut Vec<String>, u: &str) {
    vis.insert(u.to_owned());
    for v in &cg[u].callee {
        if !vis.contains(v) {
            dfs1(cg, vis, s, v);
        }
    }
    s.push(u.to_owned());
}

fn dfs2(cg: &HashMap<String, CallGraphInfo>, color: &mut HashMap<String, usize>, u: &str, c: usize) {
    color.insert(u.to_owned(), c);
    for v in &cg[u].caller {
        if !color.contains_key(v) {
            dfs2(cg, color, v, c);
        }
    }
}

fn kosaraju(cg: &HashMap<String, CallGraphInfo>) -> Vec<Vec<String>> {
    let mut vis = HashSet::new();
    let mut s = Vec::new();
    for u in cg.keys() {
        if !vis.contains(u) {
            dfs1(cg, &mut vis, &mut s, u);
        }
    }

    let mut color = HashMap::new();
    let mut c = 0;
    for u in s.iter().rev() {
        if !color.contains_key(u) {
            c += 1;
            dfs2(cg, &mut color, u, c);
        }
    }

    let mut res = vec![Vec::new(); c];
    for (u, c) in color {
        res[c - 1].push(u);
    }

    res
}

impl TransUnit {
    pub fn compute_callgraph(&mut self) {
        let cg = self.do_compute_callgraph();

        let scc = kosaraju(&cg);
        let norec = scc
            .iter()
            .filter(|scc| scc.len() == 1 && !cg[&scc[0]].caller.contains(&scc[0]))
            .map(|scc| scc[0].clone())
            .collect::<HashSet<_>>();

        self.callgraph = cg;
        self.norec = norec;
    }

    fn do_compute_callgraph(&self) -> HashMap<String, CallGraphInfo> {
        let mut res = HashMap::new();
        
        for (decl, _ty) in &self.external {
            // external functions
            res.insert(decl.clone(), CallGraphInfo::new());
        }

        for (name, func) in &self.funcs {
            res.entry(name.clone()).or_default();
            for &bb in &func.bbs {
                let block = &self.blocks[bb];
                let mut iter = block.insts_start;
                while let Some(vid) = iter {
                    let inst = &self.values[vid];
                    iter = inst.next;

                    let inst = inst.value.as_inst();
                    match inst {
                        InstructionValue::Call(c) => {
                            res.entry(name.clone()).or_default().callee.insert(c.func.clone());
                            res.entry(c.func.clone()).or_default().caller.insert(name.clone());
                        },
                        InstructionValue::TailCall(c) => {
                            res.entry(name.clone()).or_default().callee.insert(c.func.clone());
                            res.entry(c.func.clone()).or_default().caller.insert(name.clone());
                        },
                        _ => {},
                    }
                }
            }
        }

        // println!("callgraph: {:#?}", res);

        res
    }

}