pub mod liveness;
use crate::common::r#type::Type;

use std::cmp::max;
use std::fmt::{Debug, Formatter};

use std::collections::{HashMap, HashSet};
use std::fs::remove_dir;

use itertools::Itertools;

use self::liveness::LivenessAnalysis;

use super::arch_info::{RegConvention, RegisterUsage, A0, A7, RA, SP};
use super::function::Function;
use super::instr::*;
use super::register::Reg;

const MAX_USABLE_REG_CNT: usize = 27;
// pub const REGS: [i32; 27] = [
//     A0,
//     A1,
//     A2,
//     A3,
//     A4,
//     A5,
//     A6,
//     A7,

// ];

#[derive(Debug, Clone)]
pub struct Node {
    pub in_graph: bool,
    pub color: i32,
    pub adj_list: HashSet<i32>,
    pub move_adj_list: HashSet<i32>,
    pub adj_list_in_graph: HashSet<i32>,
    pub def_plus_use_cnt: HashMap<i32, i32>,
}

impl Node {
    fn degree(&self) -> usize {
        self.adj_list_in_graph.len()
    }

    fn spill_cost(&self) -> f32 {
        let mut cost = 0;
        self.def_plus_use_cnt.iter().for_each(|(level, cnt)| {
            cost += *cnt * 10i32.pow(*level as u32);
        });

        (cost as f32) / (self.adj_list.len() as f32)
    }
}

#[derive(Default)]
pub(crate) struct InterferenceGraph {
    pub k: usize,
    pub nodes: HashMap<i32, Node>,
    // when u and v are coalesced, combined_mapping[v] = u
    pub combined_mapping: HashMap<i32, i32>,

    pub move_edges: HashSet<(i32, i32)>,
}

impl Debug for InterferenceGraph {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        writeln!(f, "k: {}", self.k)?;
        writeln!(f, "nodes: {:?}", self.nodes.keys().sorted())?;
        writeln!(f, "EDGES: ")?;
        for (n, node) in self.nodes.iter() {
            writeln!(f, "{}: {:?}", n, node.adj_list)?;
        }
        writeln!(f, "combined_mapping: {:?}", self.combined_mapping)?;
        writeln!(f, "move_edges: {:?}", self.move_edges)?;
        writeln!(f, "COLORING: ")?;
        for (n, node) in self.nodes.iter() {
            writeln!(f, "{}: {}", n, node.color)?;
        }
        for u in self.combined_mapping.keys() {
            let mapped = &self.map_combine(*u);
            let u_color = self.nodes.get(mapped).unwrap().color;
            writeln!(f, "{} -> {}: {}", u, mapped, u_color)?;
        }
        Ok(())
    }
}

const UNCOLORED: i32 = i32::MIN;

impl InterferenceGraph {
    pub(crate) fn build(func: &Function, max_reg_cnt: usize, reg_type: Type) -> Self {
        let liveness = LivenessAnalysis::of(func, reg_type);
        let mut ig = InterferenceGraph {
            k: max_reg_cnt,
            nodes: HashMap::new(),
            combined_mapping: HashMap::new(),
            move_edges: HashSet::new(),
        };

        ig.add_node(0);

        for (blockid, block_liveness) in liveness.block_liveness_map.iter() {
            let block_liveness = block_liveness.borrow();
            let block_depth = func.block(*blockid).depth();

            for inst_id in 0..block_liveness.inst_cnt {
                // try to allocate with same reg for reg instr, i.e. y = f(x)
                let inst = &func.block(*blockid).instrs()[inst_id];
                let (x, y, z) = inst.get_operands(reg_type);

                let allocable = |reg_id: i32| reg_id != 0 && reg_id != SP;

                let mut do_node = |reg_id| {
                    if !allocable(reg_id) {
                        return;
                    }
                    ig.add_node(reg_id);
                    let n = ig.nodes.get_mut(&reg_id).unwrap();
                    n.def_plus_use_cnt
                        .entry(*block_depth)
                        .and_modify(|cnt| *cnt += 1)
                        .or_insert(1);
                };
                do_node(x);
                do_node(y);
                do_node(z);

                let mut mov_edge = false;

                if let Some(inst) = inst.as_any().downcast_ref::<RegInstr>() {
                    if matches!(inst.ty(), RegType::Mv) {
                        mov_edge = true;
                    }
                } else if let Some(inst) = inst.as_any().downcast_ref::<FRegInstr>() {
                    if matches!(inst.ty(), FRegType::FmvS) {
                        mov_edge = true;
                    }
                }

                if mov_edge {
                    let u = y;
                    let d = x;

                    if u != d && !ig.has_edge_in_graph(u, d) {
                        ig.add_move_edge(u, d);
                    }

                    for v in block_liveness.get_inst_out(inst_id as i32).iter() {
                        if u != *v && *v != d {
                            ig.add_edge(d, *v);
                            ig.add_edge_in_graph(d, *v);
                        }
                    }
                } else {
                    for u in block_liveness.get_inst_kill(inst_id as i32).iter() {
                        for v in block_liveness.get_inst_out(inst_id as i32).iter() {
                            if u != v {
                                ig.add_edge(*u, *v);
                                ig.add_edge_in_graph(*u, *v);
                            }
                        }
                    }
                }
            }
        }

        ig
    }

    pub fn try_coalesce(&mut self) -> bool {
        let mut has_coalesce = false;
        for (u, v) in self.move_edges.clone() {
            let nu = self.map_combine(u);
            let nv = self.map_combine(v);

            if self.nodes.get(&nu).unwrap().move_adj_list.contains(&nv) && self.can_coalesce(nu, nv)
            {
                // remove move edges if it's also a normal edge
                self.coalesce(nu, nv);
                has_coalesce = true;
            }
        }

        has_coalesce
    }

    // if node has been combined to another node, return the combined node
    pub fn map_combine(&self, node: i32) -> i32 {
        if self.nodes.contains_key(&node) {
            node
        } else {
            return self.map_combine(*self.combined_mapping.get(&node).unwrap());
        }
    }

    pub fn can_coalesce(&self, u: i32, v: i32) -> bool {
        if self.has_edge(u, v) {
            return false;
        }
        let u_node = self.nodes.get(&u).unwrap();
        let v_node = self.nodes.get(&v).unwrap();

        // we just use the remain graph to check if we can coalesce
        let mut big_degree_cnt = 0;
        for n in u_node.adj_list_in_graph.iter() {
            if v_node.adj_list_in_graph.contains(n) {
                if self.nodes.get(n).unwrap().degree() > self.k {
                    big_degree_cnt += 1;
                }
            } else if self.nodes.get(n).unwrap().degree() >= self.k {
                big_degree_cnt += 1;
            }
        }
        for n in v_node.adj_list_in_graph.iter() {
            if !u_node.adj_list_in_graph.contains(n)
                && self.nodes.get(n).unwrap().degree() >= self.k
            {
                big_degree_cnt += 1;
            }
        }

        big_degree_cnt < self.k
    }

    // briggs conservative policy
    // only retains u
    pub fn coalesce(&mut self, u: i32, v: i32) {
        let u_node = self.nodes.get(&u).unwrap().clone();
        let v_node = self.nodes.get(&v).unwrap().clone();

        // we will use the whole graph for coloring, so we should modify the whole graph
        // remove v's edge
        for n in v_node.adj_list.iter() {
            if v_node.adj_list_in_graph.contains(n) {
                if !u_node.adj_list.contains(n) {
                    if u_node.move_adj_list.contains(n) {
                        self.remove_move_edge(u, *n);
                    }
                    self.add_edge_in_graph(u, *n);
                    self.add_edge(u, *n);
                }

                self.remove_edge(v, *n);
                self.remove_edge_in_graph(v, *n);
            } else {
                if !u_node.adj_list_in_graph.contains(n) {
                    self.add_edge(u, *n);
                }

                self.remove_edge(v, *n);
            }
        }
        // remove v's move edge
        for n in v_node.move_adj_list.iter() {
            if !u_node.move_adj_list.contains(n) && !u_node.adj_list_in_graph.contains(n) {
                self.add_move_edge(u, *n);
            }
            self.remove_move_edge(v, *n);
        }

        // assert_eq!(self.nodes.get(&v).unwrap().adj_list.len(), 0);
        // assert_eq!(self.nodes.get(&v).unwrap().adj_list_in_graph.len(), 0);
        // assert_eq!(self.nodes.get(&v).unwrap().move_adj_list.len(), 0);

        // remove v from the graph
        self.nodes.remove(&v);

        // combine u and v
        self.combined_mapping.insert(v, u);

        self.remove_invalid_move_edges();
    }

    pub fn simplify(&mut self) -> Vec<i32> {
        // remove move edges if it's also a normal edge
        self.remove_invalid_move_edges();

        let mut res = Vec::new();

        // we can only remove small degree node from the graph, and without move edges
        let can_simpilify =
            |node: &Node| node.in_graph && node.degree() < self.k && node.move_adj_list.is_empty();

        let mut stk = self
            .nodes
            .iter()
            .filter(|&(_n, node)| can_simpilify(node))
            .map(|(n, _node)| *n)
            .sorted()
            .rev()
            .collect_vec();

        let mut visited = HashSet::new();
        stk.iter().for_each(|x| {
            visited.insert(*x);
        });

        // bfs
        while !stk.is_empty() {
            let id = stk.pop().unwrap();
            res.push(id);

            // remove the edge related to the node in graph
            // notice that there is no move related edge w.r.t the node
            let node = self.nodes.get_mut(&id).unwrap();

            node.in_graph = false;

            let adj_list: Vec<i32> = node.adj_list.iter().copied().collect();

            node.adj_list_in_graph.clear();

            for adj in adj_list.iter() {
                let adj_node = self.nodes.get_mut(adj).unwrap();
                adj_node.adj_list_in_graph.retain(|x| *x != id);

                if can_simpilify(adj_node) && !visited.contains(adj) {
                    stk.push(*adj);
                    visited.insert(*adj);
                }
            }
        }

        res
    }

    pub fn remove_invalid_move_edges(&mut self) {
        // if the two node has a normal edge, remove the move edge
        for (u, v) in self.move_edges.clone() {
            if self.has_edge(u, v) {
                self.remove_move_edge(u, v);
            }
        }
    }

    // assign colors for special reg
    // e.g. a0-a7, func args
    // it can be seen as a simplification, which is followed by a color assignment
    pub fn assign_special(&mut self) {
        self.remove_invalid_move_edges();

        let mut special_mapping = HashMap::new();
        // special reg alloc for a0-a7 which are used to pass args
        for reg in A0..=A7 {
            special_mapping.insert(reg, reg - 5);
        }
        special_mapping.insert(0, -5);

        for (reg_id, color) in special_mapping {
            if let Some(mut node) = self.nodes.get_mut(&reg_id) {
                node.color = color;

                // if the special node has moved edges, try to coalesce them first
                let move_adj_list = node.move_adj_list.clone();
                move_adj_list.iter().for_each(|n| {
                    if self.can_coalesce(reg_id, *n) {
                        self.coalesce(reg_id, *n);
                    }
                });

                // remove all the edges
                let node = self.nodes.get_mut(&reg_id).unwrap();
                let adj_list = node.adj_list_in_graph.clone();
                adj_list.iter().for_each(|n| {
                    let adj_node = self.nodes.get_mut(n).unwrap();
                    adj_node.adj_list_in_graph.retain(|x| *x != reg_id);
                });

                // because we have removed the node from the graph, the moved edge is no use
                let node = self.nodes.get_mut(&reg_id).unwrap();
                let move_adj_list = node.move_adj_list.clone();
                move_adj_list.iter().for_each(|n| {
                    self.remove_move_edge(reg_id, *n);
                });

                // clear the adj list
                let node = self.nodes.get_mut(&reg_id).unwrap();
                node.adj_list.clear();
                node.move_adj_list.clear();
            }
        }
    }

    pub fn assign_color(&mut self, node_id: i32) -> i32 {
        if self.nodes.get(&node_id).unwrap().color != UNCOLORED {
            return self.nodes.get(&node_id).unwrap().color;
        }

        let mut candidate = (0..self.k).into_iter().collect::<Vec<usize>>();
        for adj in self.nodes.get(&node_id).unwrap().adj_list.iter() {
            let adj = self.nodes.get(adj).unwrap();
            let c = adj.color;
            if c != UNCOLORED {
                let c = c as usize;
                candidate.retain(|&x| x != c);
            }
        }

        let mut node = self.nodes.get_mut(&node_id).unwrap();
        if !candidate.is_empty() {
            node.color = candidate[0] as i32;
        } else {
            node.color = UNCOLORED;
        }

        node.color
    }

    pub fn add_node(&mut self, reg_id: i32) {
        // if reg_id == 0 {
        //     return;
        // }
        self.nodes.entry(reg_id).or_insert(Node {
            in_graph: true,
            color: UNCOLORED,
            adj_list: HashSet::new(),
            move_adj_list: HashSet::new(),
            adj_list_in_graph: HashSet::new(),
            def_plus_use_cnt: HashMap::new(),
        });
    }

    pub fn remove_edge(&mut self, u: i32, v: i32) {
        self.nodes.get_mut(&u).unwrap().adj_list.remove(&v);
        self.nodes.get_mut(&v).unwrap().adj_list.remove(&u);
    }

    pub fn remove_edge_in_graph(&mut self, u: i32, v: i32) {
        self.nodes.get_mut(&u).unwrap().adj_list_in_graph.remove(&v);
        self.nodes.get_mut(&v).unwrap().adj_list_in_graph.remove(&u);
    }

    pub fn has_edge(&self, u: i32, v: i32) -> bool {
        self.nodes.get(&u).unwrap().adj_list.contains(&v)
            || self.nodes.get(&v).unwrap().adj_list.contains(&u)
    }

    #[allow(dead_code)]
    pub fn has_edge_in_graph(&self, u: i32, v: i32) -> bool {
        self.nodes.get(&u).unwrap().adj_list_in_graph.contains(&v)
    }

    pub fn add_edge(&mut self, u: i32, v: i32) {
        assert!(u != v);
        self.add_node(u);
        self.add_node(v);
        self.nodes.get_mut(&u).unwrap().adj_list.insert(v);
        self.nodes.get_mut(&v).unwrap().adj_list.insert(u);
    }

    pub fn add_edge_in_graph(&mut self, u: i32, v: i32) {
        assert!(u != v);
        self.add_node(u);
        self.add_node(v);
        self.nodes.get_mut(&u).unwrap().adj_list_in_graph.insert(v);
        self.nodes.get_mut(&v).unwrap().adj_list_in_graph.insert(u);
    }

    #[allow(dead_code)]
    pub fn get_color(&self, reg_id: i32) -> i32 {
        self.nodes.get(&reg_id).unwrap().color
    }

    fn remove_move_edge(&mut self, u: i32, v: i32) {
        if u == v {
            return;
        }
        if u > v {
            return self.remove_move_edge(v, u);
        }

        // if u == 37 && v == 38 {
        //     panic!("");
        // }

        self.move_edges.remove(&(u, v));

        self.nodes.get_mut(&u).unwrap().move_adj_list.remove(&v);
        self.nodes.get_mut(&v).unwrap().move_adj_list.remove(&u);
    }

    fn add_move_edge(&mut self, u: i32, v: i32) {
        if u == v {
            return;
        }
        if u > v {
            return self.add_move_edge(v, u);
        }

        self.add_node(u);
        self.add_node(v);

        assert!(!self.nodes[&u].adj_list_in_graph.contains(&v));
        assert!(!self.nodes[&v].adj_list_in_graph.contains(&u));

        self.move_edges.insert((u, v));

        self.nodes.get_mut(&u).unwrap().move_adj_list.insert(v);
        self.nodes.get_mut(&v).unwrap().move_adj_list.insert(u);
    }

    fn try_freeze(&mut self) -> bool {
        let mut to_freeze = -1;
        let mut to_freeze_degree = 0;
        for (n, node) in self.nodes.iter() {
            if node.degree() < self.k {
                if node.degree() > to_freeze_degree {
                    to_freeze = *n;
                    to_freeze_degree = node.degree();
                }
                if node.degree() == self.k - 1 {
                    break;
                }
            }
        }

        if to_freeze == -1 {
            return false;
        }

        // remove all the move edges related to `to_freeze`
        for adj in self
            .nodes
            .get(&to_freeze)
            .unwrap()
            .move_adj_list
            .clone()
            .iter()
        {
            self.remove_move_edge(to_freeze, *adj);
        }

        true
    }

    fn simplify_optimistic(&mut self) -> Vec<i32> {
        let mut res = vec![];
        for (n, node) in self.nodes.iter() {
            if node.in_graph {
                res.push(*n);
            }
        }

        res
    }
}

pub(crate) fn spill_rewrite(
    f: &mut Function,
    spill_reg: i32,
    reg_type: Type,
    max_reg_id: &mut i32,
) {
    let sf = f.sf().clone();
    let mut sf = sf.borrow_mut();

    // println!("MAX REG ID: {}", max_reg_id);
    let store_reg_id = *max_reg_id + 1;
    let load_reg_id = *max_reg_id + 2;

    *max_reg_id += 2;

    // 可能可以优化
    let index = sf.push_dword();
    let so = sf.get_stack_object(index);

    // deal with def
    for block in f.blocks_mut().iter_mut() {
        let mut store_after_ids = vec![];
        for (inst_index, inst) in block.instrs_mut().iter_mut().enumerate() {
            let (kill, _, _) = inst.get_operands(reg_type);
            if kill == spill_reg {
                let mut r = inst.regs_mut();
                for reg in r.iter_mut() {
                    if *reg.id() == spill_reg {
                        reg.set_id(store_reg_id);

                        store_after_ids.push(inst_index + 1);
                        break;
                    }
                }
            }
        }
        for inst_index in store_after_ids.iter().rev() {
            let sp = Reg::new_int(SP);
            let reg_to_store = if matches!(reg_type, Type::Int) {
                Reg::new_int(store_reg_id)
            } else {
                Reg::new_float(store_reg_id)
            };
            let offset = *so.borrow().position();
            let ty = StoreType::Sd;
            // let ty = match *so.borrow().size() {
            //     1 => StoreType::Sb,
            //     2 => StoreType::Sh,
            //     4 => StoreType::Sw,
            //     8 => StoreType::Sd,
            //     _ => panic!("invalid size"),
            // };
            let store: Box<dyn InstrTrait> = if matches!(reg_type, Type::Int) {
                Box::new(StoreInstr::new(
                    sp,
                    reg_to_store,
                    ImmeValueType::Direct(offset),
                    None,
                    ty,
                ))
            } else {
                Box::new(FStoreInstr::new(
                    sp,
                    reg_to_store,
                    ImmeValueType::Direct(offset),
                    None,
                ))
            };
            block.instrs_mut().insert(*inst_index, store);
        }
    }

    // deal with use
    for block in f.blocks_mut().iter_mut() {
        let mut load_before_ids = vec![];
        for (inst_index, inst) in block.instrs_mut().iter_mut().enumerate() {
            let (_, gen1, gen2) = inst.get_operands(reg_type);
            if gen1 == spill_reg || gen2 == spill_reg {
                let def_cnt = inst.defs().len();
                let mut r = inst.regs_mut();

                let mut flag = false; // 只插入一次
                for reg in r.iter_mut().skip(def_cnt) {
                    if *reg.id() == spill_reg {
                        reg.set_id(load_reg_id);

                        if !flag {
                            load_before_ids.push(inst_index);
                            flag = true;
                        }
                    }
                }
            }
        }

        let sp = Reg::new_int(SP);

        for inst_index in load_before_ids.iter().rev() {
            let load_to_reg = if matches!(reg_type, Type::Int) {
                Reg::new_int(load_reg_id)
            } else {
                Reg::new_float(load_reg_id)
            };
            let offset = *so.borrow().position();
            let store: Box<dyn InstrTrait> = if matches!(reg_type, Type::Int) {
                Box::new(LoadInstr::new(load_to_reg, sp, offset, LoadType::Ld))
            } else {
                Box::new(FLoadInstr::new(
                    load_to_reg,
                    sp,
                    ImmeValueType::Direct(offset),
                    None,
                ))
            };
            block.instrs_mut().insert(*inst_index, store);
        }
    }
}

// 根据寄存器分配结果，将所有的寄存器替换为分配的寄存器
fn replace_regs(function: &mut Function, reg_map: &HashMap<i32, i32>) {
    for block in function.blocks_mut().iter_mut() {
        for inst in block.instrs_mut().iter_mut() {
            for reg in inst.regs_mut().iter_mut() {
                let reg_id = reg.id();
                if reg_map.contains_key(reg_id) {
                    reg.set_id(*reg_map.get(reg_id).unwrap());
                }
            }
        }
    }
}

pub(crate) fn register_allocate(func: &mut Function) {
    let mut register_allocate = |reg_type| {
        for block in func.blocks_mut() {
            if block.instrs().is_empty() {
                block.instrs_mut().push(Box::new(RegInstr::new_move(
                    Reg::new_int(0),
                    Reg::new_int(0),
                )));
            }
        }

        let mut allocation = HashMap::new();

        // 1. build  interference graph
        // println!("BUILD GRAPH");
        let mut ig = InterferenceGraph::build(func, MAX_USABLE_REG_CNT, reg_type);

        // println!("ASSIGN SPECIAL");
        ig.assign_special();

        // println!("{:?}", ig);

        let mut stk = vec![];

        let mut max_reg_id = 0xFFFFF;
        loop {
            // println!("BEGIN LOOP");
            // 2. simplify the graph
            stk.extend(ig.simplify().into_iter());

            // println!("PASS SIMPLIFY");

            // println!("{:?}", stk);

            // 3. coalesce move edges
            if ig.try_coalesce() {
                // println!("3");
                continue;
            }

            // println!("PASS TRY COALESCE");

            // 4. freeze move edges related to small degree node
            if ig.try_freeze() {
                // println!("4");
                continue;
            }

            // println!("PASS TRY FREEZE");

            let mut spill_set = HashSet::new();

            if stk.len() != ig.nodes.len() {
                stk.extend(ig.simplify_optimistic());
            }

            // 5. assign color
            // while !stk.is_empty() {
            stk.sort_by_key(|x| (ig.nodes.get(x).unwrap().spill_cost() * 10000f32) as u32);
            // stk.sort_by(|x, y| {
            //     ig.nodes
            //         .get(x)
            //         .unwrap()
            //         .spill_cost()
            //         .partial_cmp(&ig.nodes.get(y).unwrap().spill_cost())
            //         .unwrap_or(std::cmp::Ordering::Equal)
            // });
            for &n in stk.iter().rev() {
                // let n = stk.pop().unwrap();
                let c = ig.assign_color(n);
                if c == UNCOLORED {
                    spill_set.insert(n);
                }
            }

            if !spill_set.is_empty() {
                for n in &spill_set {
                    spill_rewrite(func, *n, reg_type, &mut max_reg_id);
                }

                // restart the whole process
                stk.clear();
                ig = InterferenceGraph::build(func, MAX_USABLE_REG_CNT, reg_type);
                ig.assign_special();
                continue;
            } else {
                // println!("FINISHED REG ALLOC!!!");
                for (reg_id, node) in ig.nodes.iter() {
                    let c = node.color;
                    allocation.insert(*reg_id, c + 5);
                }
                for &v in ig.combined_mapping.keys() {
                    let mapped = ig.map_combine(v);
                    allocation.insert(v, *allocation.get(&mapped).unwrap());
                }
                break;
            }
        }

        // println!("{:?}", ig);
        replace_regs(func, &allocation);
        // println!("END LOOP");
    };

    register_allocate(Type::Int);
    register_allocate(Type::Float);
}

pub(crate) fn allocate_load_stack(function: &mut Function) {
    let sf = function.sf().clone();
    let mut sf = sf.borrow_mut();

    for block in function.blocks_mut().iter_mut() {
        for inst in block.instrs_mut().iter_mut() {
            if let Some(x) = inst.as_any_mut().downcast_mut::<LoadStackAddrInstr>() {
                let so = x.stack_object_mut().clone();
                sf.push_stack_object(so.clone());

                // x.set_offset(*so.borrow().position());
            }
        }
    }
}

pub(crate) fn backpatch_arg_stack_offset(func: &mut Function) {
    let sf = func.sf().clone();
    let framesize = sf.borrow().total_frame_size();
    let mut offset = 0;
    for so in func.caller_stack_objects_mut().iter_mut() {
        let mut so = so.borrow_mut();
        so.set_position((framesize + offset) as i32);
        offset += *so.size() as usize;
    }
}

pub(crate) fn insert_prologue(function: &mut Function) {
    let sf = function.sf().clone();
    let sf = sf.borrow();

    if sf.total_frame_size() == 0 {
        return;
    }

    let callee_saved_regs = function
        .callee_saved_regs()
        .clone()
        .iter()
        .map(|(k, v)| (*k, *v))
        .sorted_by_key(|x| -(x.1 as i32))
        .collect_vec();

    if let Some(block) = function.blocks_mut().get_mut(0) {
        let insts = block.instrs_mut();

        let sp = Reg::new_int(SP);
        let offset = ImmeValueType::Direct(-(sf.total_frame_size() as i32));
        let ty = RegImmeType::Addi;

        let mut insert_idx = 0;

        // addi sp, sp, -xx
        let build_sf = RegImmeInstr::new(sp, sp, offset, ty, None);
        insts.insert(insert_idx, Box::new(build_sf));
        insert_idx += 1;

        for (reg_id, so_idx) in callee_saved_regs.iter() {
            let reg = if *reg_id > 0 {
                Reg::new_int(*reg_id)
            } else {
                Reg::new_float(-*reg_id)
            };

            let so = sf.get_stack_object(*so_idx);

            let offset = ImmeValueType::Direct(*so.borrow().position());
            let store: Box<dyn InstrTrait> = if *reg_id > 0 {
                Box::new(StoreInstr::new(sp, reg, offset, None, StoreType::Sd))
            } else {
                Box::new(FStoreInstr::new(sp, reg, offset, None))
            };
            insts.insert(insert_idx, store);
            insert_idx += 1;
        }
    }
}

pub(crate) fn insert_epilogue(function: &mut Function) {
    let sf = function.sf().clone();
    let sf = sf.borrow();

    if sf.total_frame_size() == 0 {
        return;
    }

    let callee_saved_regs = function
        .callee_saved_regs()
        .clone()
        .iter()
        .map(|(k, v)| (*k, *v))
        .sorted_by_key(|x| -(x.1 as i32))
        .collect_vec();

    let mut ret_block_index = 0;
    let mut ret_index = 0;
    for (block_idx, block) in function.blocks().iter().enumerate() {
        for (inst_idx, inst) in block.instrs().iter().enumerate() {
            if inst.as_any().downcast_ref::<ReturnInstr>().is_some() {
                ret_block_index = block_idx;
                ret_index = inst_idx;
            }
        }
    }

    let insts = function
        .blocks_mut()
        .get_mut(ret_block_index)
        .unwrap()
        .instrs_mut();

    let sp = Reg::new_int(SP);
    let _ = Reg::new_int(RA);
    let offset = ImmeValueType::Direct(sf.total_frame_size() as i32);
    let ty = RegImmeType::Addi;

    let mut insert_idx = ret_index;

    // let load_ra = LoadInstr::new(ra, sp, 0, crate::instr::LoadType::Ld);
    // insts.insert(insert_idx, Box::new(load_ra));
    // insert_idx += 1;

    for (reg_id, so_idx) in callee_saved_regs.iter() {
        let reg = if *reg_id > 0 {
            Reg::new_int(*reg_id)
        } else {
            Reg::new_float(-*reg_id)
        };
        let so = sf.get_stack_object(*so_idx);
        let offset = *so.borrow().position();
        let load: Box<dyn InstrTrait> = if *reg_id > 0 {
            Box::new(LoadInstr::new(reg, sp, offset, LoadType::Ld))
        } else {
            Box::new(FLoadInstr::new(
                reg,
                sp,
                ImmeValueType::Direct(offset),
                None,
            ))
        };
        insts.insert(insert_idx, load);
        insert_idx += 1;
    }

    let release_sf = RegImmeInstr::new(sp, sp, offset, ty, None);
    insts.insert(insert_idx, Box::new(release_sf));
}

pub(crate) fn save_callee_saved_regs(func: &mut Function) {
    let mut save_callee_saved_regs = |reg_type| {
        let sf = func.sf().clone();
        let mut sf = sf.borrow_mut();

        // collect_callee_saved_regs
        let mut regs = HashSet::new();
        // if there is a call, then it is not a leaf function
        let mut is_leaf = true;
        for block in func.blocks().iter() {
            for inst in block.instrs().iter() {
                if inst.as_any().downcast_ref::<CallInstr>().is_some() {
                    is_leaf = false;
                }
                let t = inst.get_operands(reg_type);
                let v = vec![t.0, t.1, t.2];
                for reg_id in v.iter() {
                    if matches!(reg_type, Type::Int) {
                        if *reg_id < 32
                            && RegConvention::<i32>::REGISTER_USAGE[*reg_id as usize]
                                == RegisterUsage::CalleeSaved
                        {
                            regs.insert(*reg_id);
                        }
                    } else if *reg_id < 32
                        && RegConvention::<f32>::REGISTER_USAGE[*reg_id as usize]
                            == RegisterUsage::CalleeSaved
                    {
                        regs.insert(*reg_id);
                    }
                }
            }
        }

        // for ele in regs {
        //     func.callee_saved_regs_mut().insert(ele, 0);
        // }

        for reg in regs {
            let idx = sf.push_dword();
            if matches!(reg_type, Type::Int) {
                func.callee_saved_regs_mut().insert(reg, idx);
            } else {
                func.callee_saved_regs_mut().insert(-reg, idx);
            }
        }

        if !is_leaf {
            func.callee_saved_regs_mut().insert(RA, sf.push_dword()); // reserved for ra
        }
    };
    save_callee_saved_regs(Type::Int);
    save_callee_saved_regs(Type::Float)
}

pub(crate) fn save_caller_saved_regs(func: &mut Function) {
    let mut save_caller_saved_regs = |reg_type| {
        let l = LivenessAnalysis::of(func, reg_type);
        let sf = func.sf().clone();
        let mut sf = sf.borrow_mut();

        for block in func.blocks_mut().iter_mut() {
            let mut insert_pos = vec![];
            let mut insert_store_reg_offset_map = HashMap::new();

            for (inst_id, inst) in block.instrs().iter().enumerate() {
                let mut need_push_args_to_stack = false;
                let block_id = *block.id();
                let block_liveness = l.block_liveness_map[&block_id].borrow();
                {
                    if let Some(call) = inst.as_any().downcast_ref::<CallInstr>() {
                        if *call.int_arg_cnt() > 8 || *call.float_arg_cnt() > 8 {
                            need_push_args_to_stack = true;
                        }
                    } else {
                        continue;
                    }
                }

                let mut out = block_liveness.get_inst_out(inst_id as i32).clone();
                // return value can be changed
                out.remove(&A0);

                if matches!(reg_type, Type::Int) {
                    out.retain(|x| {
                        *x < 32
                            && RegConvention::<i32>::REGISTER_USAGE[*x as usize]
                                == RegisterUsage::CallerSaved
                    });
                } else {
                    out.retain(|x| {
                        *x < 32
                            && RegConvention::<f32>::REGISTER_USAGE[*x as usize]
                                == RegisterUsage::CallerSaved
                    });
                }

                // println!("ASM {} OUT {:?}", inst.gen_asm(), out);
                let mut store_reg_offset = vec![];

                // calculate the position to store regs
                for reg_id in out {
                    let so_idx = sf.push_dword();
                    let so = sf.get_stack_object(so_idx);
                    let pos = *so.borrow().position();

                    store_reg_offset.push((reg_id, pos));
                }

                insert_pos.push((inst_id, need_push_args_to_stack));
                insert_store_reg_offset_map.insert(inst_id, store_reg_offset);
            }
            for (pos, need_push_args_to_stack) in insert_pos.iter().rev() {
                let store_reg_offset = &insert_store_reg_offset_map[pos];

                // restore after function call
                for &(reg_id, offset) in store_reg_offset.iter() {
                    let reg = if matches!(reg_type, Type::Int) {
                        Reg::new_int(reg_id)
                    } else {
                        Reg::new_float(reg_id)
                    };
                    let sp = Reg::new_int(SP);

                    let load: Box<dyn InstrTrait> = if matches!(reg_type, Type::Int) {
                        Box::new(LoadInstr::new(reg, sp, offset, LoadType::Ld))
                    } else {
                        Box::new(FLoadInstr::new(
                            reg,
                            sp,
                            ImmeValueType::Direct(offset),
                            None,
                        ))
                    };

                    block
                        .instrs_mut()
                        .insert(*pos + if *need_push_args_to_stack { 2 } else { 1 }, load);
                }

                // store before function call
                for &(reg_id, offset) in store_reg_offset.iter() {
                    let reg = if matches!(reg_type, Type::Int) {
                        Reg::new_int(reg_id)
                    } else {
                        Reg::new_float(reg_id)
                    };
                    let sp = Reg::new_int(SP);

                    let store: Box<dyn InstrTrait> = if matches!(reg_type, Type::Int) {
                        Box::new(StoreInstr::new(
                            sp,
                            reg,
                            ImmeValueType::Direct(offset),
                            None,
                            StoreType::Sd,
                        ))
                    } else {
                        Box::new(FStoreInstr::new(
                            sp,
                            reg,
                            ImmeValueType::Direct(offset),
                            None,
                        ))
                    };

                    block.instrs_mut().insert(
                        if *need_push_args_to_stack {
                            *pos - 1
                        } else {
                            *pos
                        },
                        store,
                    );
                }
            }
        }
    };
    save_caller_saved_regs(Type::Int);
    save_caller_saved_regs(Type::Float);
}

fn peephole_remove_unused_def(func: &mut Function) -> bool {
    let mut changed = false;

    // remove unused def
    let mut remove_unused_def = |reg_type| {
        let analysis = LivenessAnalysis::of(func, reg_type);
        for (block_id, liveness) in analysis.block_liveness_map.iter() {
            let block = func.block_mut(*block_id);
            let mut remove_indices = vec![];
            let liveness = liveness.borrow();
            for (inst_idx, inst) in block.instrs().iter().enumerate() {
                let out = liveness.get_inst_out(inst_idx as i32);
                let kill = inst.def_id_vec(reg_type);

                // no def
                if kill.is_empty() {
                    continue;
                }

                // call instruction is special
                if inst.as_any().downcast_ref::<CallInstr>().is_some() {
                    continue;
                }

                // if inst.as_any().downcast_ref::<ImmeInstr>().is_none() {
                //     continue;
                // }

                if kill.iter().all(|x| !out.contains(x)) {
                    // println!("remove {}", inst.gen_asm());
                    remove_indices.push(inst_idx);
                }
            }

            for idx in remove_indices.iter().rev() {
                block.instrs_mut().remove(*idx);
                changed = true;
            }
        }
    };

    remove_unused_def(Type::Int);
    // remove_unused_def(Type::Float);
    changed
}

fn peephole_ld(func: &mut Function) -> bool {
    let mut changed = false;

    for block in func.blocks_mut() {
        let mut inst_to_remove = vec![];
        for (i, inst) in block.instrs().iter().enumerate().skip(1) {
            let last = &block.instrs()[i - 1];
            if let Some(load) = inst.as_any().downcast_ref::<LoadInstr>() {
                if let Some(store) = last.as_any().downcast_ref::<StoreInstr>() {
                    if matches!(load.ty(), LoadType::Lw) && matches!(store.ty(), StoreType::Sw)
                        || matches!(load.ty(), LoadType::Ld) && matches!(store.ty(), StoreType::Sd)
                    {
                        if let ImmeValueType::Direct(store_offset) = store.offset() {
                            if *load.rs1().id() == *store.rs1().id()
                                && load.offset() == store_offset
                                && *load.rd().id() == *store.rs2().id()
                            {
                                inst_to_remove.push(i);
                            }
                        }
                    }
                }
            }
        }
        for i in inst_to_remove.iter().rev() {
            block.instrs_mut().remove(*i);
            changed = true;
        }
    }

    changed
}

fn peephole_mv(func: &mut Function) -> bool {
    let mut changed = false;

    // remove mov
    let mut peephole_mv = |reg_type| {
        // let analysis = LivenessAnalysis::of(func, reg_type);

        // let mut push_map = vec![];

        for block_idx in 0..func.blocks().len() {
            // func.blocks_mut().iter_mut().for_each(|block| {
            // let mut remove_movs = vec![];
            // let liveness = analysis
            //     .block_liveness_map
            //     .get(func.blocks()[block_idx].id())
            //     .unwrap()
            //     .borrow();

            for inst_idx in 0..func.blocks()[block_idx].instrs().len() {
                let mut y = -1;
                let mut x = -1;
                let block = &func.blocks()[block_idx];
                if reg_type == Type::Int {
                    if let Some(reg_inst) =
                        block.instrs()[inst_idx].as_any().downcast_ref::<RegInstr>()
                    {
                        if matches!(reg_inst.ty(), RegType::Mv) {
                            // y = x
                            (y, x, _) = reg_inst.get_operands(reg_type);
                        }
                    }
                } else if let Some(reg_inst) = block.instrs()[inst_idx]
                    .as_any()
                    .downcast_ref::<FRegInstr>()
                {
                    if matches!(reg_inst.ty(), FRegType::FmvS) {
                        (y, x, _) = reg_inst.get_operands(reg_type);
                    }
                }
                if y != -1 && x != -1 && y != 0 && x != 0 {
                    let mut removable = true;
                    // let mut need_push = true;
                    for i in (inst_idx + 1)..block.instrs().len() {
                        let inst = &block.instrs()[i];

                        // mov for return
                        // if inst.as_any().downcast_ref::<ReturnInstr>().is_some() && y == A0 {
                        //     removable = false;
                        //     // need_push = false;
                        //     break;
                        // } else
                        if let Some(_call) = inst.as_any().downcast_ref::<CallInstr>() {
                            if y == A0 {
                                removable = false;
                            }
                            // let uses = inst.use_id_vec(reg_type);
                            // if uses.contains(x) ||
                            // mov for args
                            // if (A0..=A7).contains(&y) && ((y - A0) as usize) < *call.int_arg_cnt() {
                            //     removable = false;
                            //     // need_push = false;
                            //     break;
                            // }
                        }

                        // if let Some(d) = inst.get_operands(reg_type).0;
                        // if d == x {
                        //     if liveness.get_inst_out(i as i32).contains(&y) {
                        //         removable = false;
                        //     }
                        //     // need_push = false;
                        //     break;
                        // } else if d == y {
                        //     // need_push = false;
                        //     break;
                        // }
                    }

                    if removable {
                        let mut stk = vec![(*block.id(), inst_idx + 1)];
                        let mut visited = HashSet::new();

                        while !stk.is_empty() {
                            let (bid, start_inst_idx) = stk.pop().unwrap();
                            if !visited.contains(&bid) {
                                visited.insert(bid);
                            } else {
                                continue;
                            }
                            let block = &mut func.block_mut(bid);
                            let mut breaked = false;
                            for i in start_inst_idx..block.instrs().len() {
                                let inst = &mut block.instrs_mut()[i];
                                let def_cnt = inst.defs().len();
                                if !inst.regs_mut().is_empty() {
                                    for reg in inst.regs_mut().iter_mut().skip(def_cnt) {
                                        if *reg.id() == y && *reg.ty() == reg_type {
                                            reg.set_id(x);
                                            // println!("CHANGED FROM {} TO {}", y, x);
                                            changed = true;
                                        }
                                    }
                                }

                                if let Some(d) = inst.def_id_vec(reg_type).first() {
                                    let d = *d;
                                    if d == y || d == x {
                                        breaked = true;
                                        break;
                                    }
                                }
                            }

                            if !breaked {
                                let block = &func.block(bid);
                                for &succ in block.out_edges() {
                                    if func.block(succ).in_edges().len() == 1
                                        && func.block(succ).in_edges()[0] == bid
                                    {
                                        stk.push((succ, 0));
                                    }
                                }
                            }
                        }
                    }
                    // }
                }
            }

            // remove the mov
            // for inst_idx in remove_movs.iter().rev() {
            //     // println!("REMOVE MOV: {}", block.instrs()[*inst_idx].gen_asm());
            //     block.instrs_mut().remove(*inst_idx);
            //     changed = true;
            // }
        }
    };

    peephole_mv(Type::Int);
    peephole_mv(Type::Float);

    // // push the mov to next block
    // for (block_id, inst) in push_map {
    //     let block = func.block_mut(block_id);
    //     block.instrs_mut().insert(0, inst);
    //     changed = true;
    // }
    changed
}

fn peephole_mv_x_x(func: &mut Function) -> bool {
    let mut changed = false;

    // remove `mv t0, t0`
    // remove `fmv ft0, ft0`
    for block in func.blocks_mut().iter_mut() {
        let mut insts_to_remove = vec![];
        for (i, inst) in block.instrs_mut().iter_mut().enumerate() {
            if let Some(x) = inst.as_any().downcast_ref::<RegInstr>() {
                let (u, v, _) = x.get_operands(Type::Int);
                if matches!(x.ty(), RegType::Mv) && u == v {
                    insts_to_remove.push(i);
                }
            } else if let Some(x) = inst.as_any().downcast_ref::<FRegInstr>() {
                let (u, v, _) = x.get_operands(Type::Float);
                if matches!(x.ty(), FRegType::FmvS) && u == v {
                    insts_to_remove.push(i);
                }
            }
            // else if let Some(x) = inst.as_any().downcast_ref::<FRegInstr>() {
            //     let (u, v, _) = x.get_operands();
            //     if matches!(x.ty(), FRegType::FmvS) && u == v {
            //         insts_to_remove.push(i);
            //     }
            // }
        }

        while !insts_to_remove.is_empty() {
            let i = insts_to_remove.pop().unwrap();
            block.instrs_mut().remove(i);
            changed = true;
        }
    }

    changed
}

fn peephole_jump(func: &mut Function) -> bool {
    let mut changed = false;

    // `j .L3; .L3: j .L4` => `j .L4;`
    loop {
        let mut simplified = false;

        let mut jump_map = HashMap::new();
        let mut name2id = HashMap::new();
        let mut id2name = HashMap::new();

        let mut jump_to_push: Box<dyn InstrTrait> =
            Box::new(JumpInstr::new_jump("impossible".to_string()));

        let mut block_to_remove_id = -1;

        for block in func.blocks_mut().iter_mut() {
            let block_name = block.name().clone();

            name2id.insert(block_name.clone(), *block.id());
            id2name.insert(*block.id(), block_name.clone());

            if !simplified {
                if let Some(inst) = block.instrs_mut().iter_mut().next() {
                    // the first inst is `j label`
                    if let Some(x) = inst.as_any().downcast_ref::<JumpInstr>() {
                        if matches!(x.ty(), JumpType::J) {
                            jump_map.insert(block_name.clone(), x.label().clone());

                            jump_to_push = Box::new(JumpInstr::new_jump(x.label().clone()));

                            block_to_remove_id = *block.id();

                            changed = true;
                            simplified = true;
                        }
                    }
                }
            }
        }

        // jump simplification
        func.blocks_mut().iter_mut().for_each(|block| {
            for inst in block.instrs_mut().iter_mut() {
                if let Some(x) = inst.as_any_mut().downcast_mut::<JumpInstr>() {
                    if let Some(label) = jump_map.get(x.label()) {
                        x.set_label(label.clone());
                    }
                } else if let Some(x) = inst.as_any_mut().downcast_mut::<BranchInstr>() {
                    if let Some(label) = jump_map.get(x.label()) {
                        x.set_label(label.clone());
                    }
                }
            }
        });

        // map id to new id, because we will remove some block
        let mut map_id = HashMap::new();

        {
            // adjust block's id, because block's id is related to its index in an array
            let mut remove_cnt = 0;
            for block in func.blocks_mut().iter_mut() {
                if jump_map.contains_key(block.name()) {
                    remove_cnt += 1;
                } else {
                    map_id.insert(*block.id(), *block.id() - remove_cnt);
                    *block.id_mut() -= remove_cnt;
                }
            }
        }

        // adjust succ block's id
        let mut prev_block_id = -1;
        func.blocks_mut().iter_mut().for_each(|block| {
            // !!! only work if only remove one block
            if *block.id() < block_to_remove_id {
                prev_block_id = max(prev_block_id, *block.id());
            }

            for block_id in block.out_edges_mut().iter_mut() {
                let mut redirect = false;
                if let Some(succ_label) = id2name.get(block_id) {
                    if let Some(label) = jump_map.get(succ_label) {
                        *block_id = map_id[&name2id[label]];
                        redirect = true;
                    }
                }
                if !redirect {
                    *block_id = map_id[block_id];
                }
            }

            for block_id in block.in_edges_mut().iter_mut() {
                let mut redirect = false;
                if let Some(succ_label) = id2name.get(block_id) {
                    if let Some(label) = jump_map.get(succ_label) {
                        *block_id = map_id[&name2id[label]];
                        redirect = true;
                    }
                }
                if !redirect {
                    *block_id = map_id[block_id];
                }
            }
        });

        if prev_block_id != -1 {
            func.block_mut(prev_block_id)
                .instrs_mut()
                .push(jump_to_push);
        }

        func.blocks_mut()
            .retain(|b| !jump_map.contains_key(b.name()));

        if !simplified {
            break;
        }
    }

    // remove redundant jump, e.g. `j .L1; j .L2;` => `j .L1;`
    for block in func.blocks_mut() {
        let mut inst_to_remove = vec![];
        for i in 1..block.instrs_mut().len() {
            let inst0 = &block.instrs()[i - 1];
            let inst1 = &block.instrs()[i];
            if inst0.as_any().downcast_ref::<JumpInstr>().is_some()
                && inst1.as_any().downcast_ref::<JumpInstr>().is_some()
            {
                inst_to_remove.push(i);
            }
        }

        for i in inst_to_remove.iter().rev() {
            block.instrs_mut().remove(*i);
            changed = true;
        }
    }

    changed
}

fn peephole_li(func: &mut Function) -> bool {
    let mut changed = false;

    let mut constant_propagation_for_li = |reg_type| {
        for block in func.blocks_mut().iter_mut() {
            let mut li_map = HashMap::new();
            let mut rewrite_inst_map: HashMap<usize, Box<dyn InstrTrait>> = HashMap::new();

            for (inst_idx, inst) in block.instrs_mut().iter_mut().enumerate() {
                let (d, u1, u2) = inst.get_operands(reg_type);
                if inst.as_any().downcast_ref::<CallInstr>().is_some() {
                    // li stop propagation when meeting a call
                    li_map.clear();
                } else if let Some(imme_inst) = inst.as_any().downcast_ref::<ImmeInstr>() {
                    if matches!(imme_inst.ty(), ImmeType::Li) {
                        if let ImmeValueType::Direct(val) = imme_inst.imme() {
                            li_map.insert(*imme_inst.rd().id(), *val);
                        }
                    }
                } else {
                    if let Some(reg_reg_inst) = inst.as_any().downcast_ref::<RegRegInstr>() {
                        let u1_in_map = li_map.contains_key(&u1);
                        let u2_in_map = li_map.contains_key(&u2);
                        if u1 != 0 && u2 != 0 && u1_in_map && u2_in_map {
                            // we can calculate the result directly
                            let x = li_map[&u1];
                            let y = li_map[&u2];
                            let res = match reg_reg_inst.ty() {
                                RegRegType::Add => x + y,
                                RegRegType::Addw => x + y,
                                RegRegType::Sub => x - y,
                                RegRegType::Subw => x - y,
                                RegRegType::Mul => x * y,
                                RegRegType::Mulw => x * y,
                                RegRegType::Div => x / y,
                                RegRegType::Divw => x / y,
                                RegRegType::Rem => x % y,
                                RegRegType::Remw => x % y,
                                RegRegType::Sll => ((x as u64) << (y as u64)) as i32,
                                RegRegType::Sllw => ((x as u32) << (y as u32)) as i32,
                                RegRegType::Srl => ((x as u64) >> (y as u64)) as i32,
                                RegRegType::Srlw => ((x as u32) >> (y as u32)) as i32,
                                RegRegType::Sra => x >> y,
                                RegRegType::Sraw => x >> y,
                                RegRegType::And => x & y,
                                RegRegType::Or => x | y,
                            };

                            let new_inst = ImmeInstr::new_load_immediate(*reg_reg_inst.rd(), res);
                            rewrite_inst_map.insert(inst_idx, Box::new(new_inst));
                        } else if u1_in_map && u1 != 0 || u2_in_map && u2 != 0 {
                            let li_u = if u1_in_map { u1 } else { u2 };
                            let mut imme = li_map[&li_u];
                            let ty = match reg_reg_inst.ty() {
                                RegRegType::Add => Some(RegImmeType::Addi),
                                RegRegType::Addw => Some(RegImmeType::Addiw),
                                RegRegType::Sub => {
                                    if u2_in_map {
                                        imme = -imme;
                                        Some(RegImmeType::Addi)
                                    } else {
                                        None
                                    }
                                }
                                RegRegType::Subw => {
                                    if u2_in_map {
                                        imme = -imme;
                                        Some(RegImmeType::Addiw)
                                    } else {
                                        None
                                    }
                                }
                                RegRegType::Sll => Some(RegImmeType::Slli),
                                RegRegType::Sllw => Some(RegImmeType::Slliw),
                                RegRegType::Srl => Some(RegImmeType::Srli),
                                RegRegType::Srlw => Some(RegImmeType::Srliw),
                                RegRegType::Sra => Some(RegImmeType::Srai),
                                RegRegType::Sraw => Some(RegImmeType::Sraiw),
                                _ => None,
                            };

                            if imme >= -2048 && imme < 2048 {
                                // we can use addi/slli/srli/srai
                                if let Some(ty) = ty {
                                    let new_inst = RegImmeInstr::new(
                                        reg_reg_inst.rd().clone(),
                                        if u1_in_map {
                                            reg_reg_inst.rs2().clone()
                                        } else {
                                            reg_reg_inst.rs1().clone()
                                        },
                                        ImmeValueType::Direct(imme),
                                        ty,
                                        None,
                                    );
                                    rewrite_inst_map.insert(inst_idx, Box::new(new_inst));
                                }
                            }
                        }
                    } else if let Some(reg_inst) = inst.as_any().downcast_ref::<RegInstr>() {
                        if li_map.contains_key(&u1) && u1 != 0 {
                            let x = li_map[&u1];
                            let res = match reg_inst.ty() {
                                RegType::Mv => x,
                                RegType::Negw => -x,
                                RegType::Seqz => (x == 0) as i32,
                                RegType::Snez => (x != 0) as i32,
                                RegType::Sgtz => (x > 0) as i32,
                                RegType::Sltz => (x < 0) as i32,
                            };
                            let new_inst = ImmeInstr::new_load_immediate(*reg_inst.rd(), res);
                            rewrite_inst_map.insert(inst_idx, Box::new(new_inst));
                        }
                    } else if let Some(reg_imme_inst) = inst.as_any().downcast_ref::<RegImmeInstr>()
                    {
                        if li_map.contains_key(&u1) && u1 != 0 {
                            let x = li_map[&u1];
                            if let ImmeValueType::Direct(y) = reg_imme_inst.offset() {
                                let y = *y;
                                let res = match reg_imme_inst.ty() {
                                    RegImmeType::Addi => x + y,
                                    RegImmeType::Addiw => x + y,
                                    RegImmeType::Slli => ((x as u64) << (y as u32)) as i32,
                                    RegImmeType::Slliw => ((x as u32) << (y as u32)) as i32,
                                    RegImmeType::Srli => ((x as u64) >> (y as u32)) as i32,
                                    RegImmeType::Srliw => ((x as u32) >> (y as u32)) as i32,
                                    RegImmeType::Srai => x >> (y as u32),
                                    RegImmeType::Sraiw => x >> (y as u32),
                                };

                                let new_inst =
                                    ImmeInstr::new_load_immediate(*reg_imme_inst.rd(), res);
                                rewrite_inst_map.insert(inst_idx, Box::new(new_inst));
                            }
                        }
                    }
                    li_map.remove(&d);
                }
            }

            for (i, inst) in rewrite_inst_map {
                block.instrs_mut()[i] = inst;
                changed = true;
            }
        }
    };
    constant_propagation_for_li(Type::Int);
    // constant_propagation_for_li(Type::Float);

    changed
}

pub fn peephole(func: &mut Function) -> bool {
    let mut changed = false;

    changed = changed || peephole_mv_x_x(func);
    changed = changed || peephole_ld(func);

    changed = changed || peephole_li(func);

    changed = changed || peephole_jump(func);

    changed = changed || peephole_mv(func);
    changed = changed || peephole_mv_x_x(func);
    changed = changed || peephole_remove_unused_def(func);

    changed
}

#[cfg(test)]
mod tests {
    use antlr_rust::{common_token_stream::CommonTokenStream, InputStream};
    use itertools::Itertools;

    use crate::{
        backend::{
            program::Program,
            register_allocation::{
                allocate_load_stack, backpatch_arg_stack_offset, insert_epilogue, insert_prologue,
                register_allocate, save_callee_saved_regs, save_caller_saved_regs,
                InterferenceGraph,
            },
        },
        frontend::{
            antlr_dep::{sysylexer::SysYLexer, sysyparser::SysYParser, sysyvisitor::SysYVisitor},
            ast_visitor::SysYAstVisitor,
            llvm::llvm_module::LLVMModule,
        },
        optimize::passes::{
            bb_ops::remove_phi,
            dce::{remove_unused_def, remove_useless_bb},
            mem2reg::mem2reg,
        },
    };

    #[test]
    fn test_ig() {
        let mut ig = InterferenceGraph::default();
        ig.k = 4;

        for i in 100..=106 {
            ig.add_node(i);
        }

        let edge = vec![
            (100, 101),
            (100, 102),
            (100, 104),
            (100, 106),
            (101, 102),
            (101, 103),
            (101, 105),
            (102, 103),
            (102, 104),
            (103, 105),
            (103, 106),
            (104, 105),
            (104, 106),
            (105, 106),
        ];
        let mov_edge = vec![(103, 104)];

        for (u, v) in edge {
            ig.add_edge(u, v);
            ig.add_edge_in_graph(u, v);
        }
        for (u, v) in mov_edge {
            ig.add_move_edge(u, v);
        }

        assert!(ig.simplify().is_empty());
        assert!(ig.try_coalesce());
        let mut stk = ig.simplify();
        assert_eq!(
            stk.clone().into_iter().sorted().collect_vec(),
            vec![100, 101, 102, 103, 105, 106]
        );
        while !stk.is_empty() {
            let n = stk.pop().unwrap();
            ig.assign_color(n);
        }

        // println!("{:?}", ig);
    }
    #[test]
    fn test() {
        let contents =
            std::fs::read_to_string("test/homemade/math.sy").expect("cannot open source file");
        let input = InputStream::new(contents.as_bytes());

        let lexer = SysYLexer::new(input);
        let token_stream = CommonTokenStream::new(lexer);
        let mut parser = SysYParser::new(token_stream);

        let mut llvm_module: LLVMModule = LLVMModule::new();

        /* syntax analysis */
        let mut ast_visitor = SysYAstVisitor::new(&mut llvm_module);
        let ctx = parser.compUnit().expect("syntax error");

        /* semantic analysis */
        ast_visitor.visit_compUnit(&ctx);
        ast_visitor.return_content();

        remove_useless_bb(&mut llvm_module);
        mem2reg(&mut llvm_module);
        remove_unused_def(&mut llvm_module);

        remove_phi(&mut llvm_module);

        println!("{}", llvm_module);

        // for b in main.layout().block_iter() {
        //     main.layout().inst_iter(b).for_each(|i| {
        //         println!("{:?}", main.instructions().get(&i).unwrap());
        //     });
        // }

        let mut p = Program::from_llvm_module(&llvm_module);

        // let main = llvm_module.functions.get_mut("main").unwrap();
        for func in p.functions_mut() {
            println!("{}: ", func.name());

            println!("BEFORE REG ALLOC: ");
            // println!("{}", func);
            for b in func.blocks().iter() {
                println!("{}:", b.name());
                for i in b.instrs().iter() {
                    print!("\t{}", i.gen_asm());
                }
            }

            register_allocate(func);

            allocate_load_stack(func);

            save_caller_saved_regs(func);

            save_callee_saved_regs(func);

            backpatch_arg_stack_offset(func);

            // println!("CALLEE  SAVED: {:?}", func.callee_saved_regs());

            // println!("BEFORE PEEP HOLE: ");
            // for b in func.blocks().iter() {
            //     println!("{}:", b.name());
            //     for i in b.instrs().iter() {
            //         print!("\t{}", i.gen_asm());
            //     }
            // }

            {
                let mut _peephole_cnt = 0;
                // while peephole(func) {
                // println!("PEEPHOLE {}: ", peephole_cnt);
                // peephole_cnt += 1;
                // for b in func.blocks().iter() {
                //     println!("{}:", b.name());
                //     for i in b.instrs().iter() {
                //         print!("\t{}", i.gen_asm());
                //     }
                // }
                // if peephole_cnt == 3 {
                //     break;
                // }
                // }
            }

            insert_prologue(func);
            insert_epilogue(func);
        }
        println!("==================");
        println!("{}", p.gen_asm());
    }
}
