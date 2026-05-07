use crate::backend::{block::Block, function::Function, instr::InstrTrait};
use crate::common::r#type::Type;
use itertools::Itertools;
use std::{
    cell::RefCell,
    collections::{HashMap, HashSet},
    rc::Rc,
    vec,
};

pub(crate) struct LivenessAnalysis {
    pub block_liveness_map: HashMap<i32, Rc<RefCell<BlockLiveness>>>,
}

impl LivenessAnalysis {
    pub fn of(func: &Function, reg_type: Type) -> LivenessAnalysis {
        let mut res = LivenessAnalysis {
            block_liveness_map: HashMap::new(),
        };

        let mut insts_map = HashMap::new();
        let mut changed_set = HashSet::new();

        for block in func.blocks().iter() {
            // 获取block中的inst_ids
            let insts = block.instrs().iter().rev().collect_vec();
            // 构造block_liveness

            insts_map.insert(
                *block.id(),
                (0..(insts.len())).into_iter().rev().collect_vec(),
            );

            res.block_liveness_map.insert(
                *block.id(),
                Rc::new(RefCell::new(BlockLiveness::new(insts, reg_type))),
            );

            changed_set.insert(*block.id());
        }

        let order = res.get_toplogical_order(func);

        // 不动点算法：迭代直到不再变化
        // println!("BEGIN LIVENESS ANALYSIS ITERATON");
        let mut _loop_cnt = 0;
        loop {
            // println!("{}", loop_cnt);
            _loop_cnt += 1;
            let mut changed = false;
            for &block_id in order.iter() {
                let block = func.block(block_id);
                // let bb = function.basic_blocks().get(&block_id).unwrap();
                let block_liveness = res.block_liveness_map.get(block.id()).unwrap();

                let mut need_update = false;
                if changed_set.contains(&block_id) {
                    need_update = true;
                } else {
                    for succ_id in block.out_edges() {
                        if changed_set.contains(succ_id) {
                            need_update = true;
                            break;
                        }
                    }
                }

                if need_update {
                    if block_liveness.clone().borrow_mut().update(
                        block,
                        func,
                        &res.block_liveness_map,
                        &insts_map,
                    ) {
                        changed_set.insert(block_id);
                        changed = true;
                    } else {
                        changed_set.remove(&block_id);
                    }
                }
            }
            if !changed {
                break;
            }
        }
        // println!("END LIVENESS ANALYSIS ITERATION");

        res
    }

    fn get_toplogical_order(&self, function: &Function) -> Vec<i32> {
        let mut visited = HashSet::new();
        let mut res = vec![];
        let mut stack = vec![*function.entry_block_id()];
        while let Some(block_id) = stack.pop() {
            if visited.contains(&block_id) {
                continue;
            }
            visited.insert(block_id);
            res.push(block_id);
            let block = function.block(block_id);
            for succ_id in block.out_edges() {
                stack.push(*succ_id);
            }
        }
        res.reverse();
        res
    }
}

#[derive(Clone, Debug)]
pub struct BlockLiveness {
    inst_gen_map: HashMap<i32, HashSet<i32>>,
    inst_kill_map: HashMap<i32, HashSet<i32>>,
    inst_in_map: HashMap<i32, HashSet<i32>>,
    inst_out_map: HashMap<i32, HashSet<i32>>,
    psuedo_in: HashSet<i32>,
    pub inst_cnt: usize,
}

impl BlockLiveness {
    pub(crate) fn new(insts: Vec<&Box<dyn InstrTrait>>, reg_type: Type) -> BlockLiveness {
        let mut block_liveness = BlockLiveness {
            inst_gen_map: HashMap::new(),
            inst_kill_map: HashMap::new(),
            inst_in_map: HashMap::new(),
            inst_out_map: HashMap::new(),
            psuedo_in: HashSet::new(),
            inst_cnt: insts.len(),
        };

        let mut inst_id = insts.len() as i32;
        for i in insts {
            inst_id -= 1;

            let kill = i.def_id_vec(reg_type).into_iter().collect::<HashSet<_>>();
            let gen = i.use_id_vec(reg_type).into_iter().collect::<HashSet<_>>();

            block_liveness.set_inst_gen(inst_id, gen);
            block_liveness.set_inst_kill(inst_id, kill);
            block_liveness.set_inst_in(inst_id, HashSet::new());
            block_liveness.set_inst_out(inst_id, HashSet::new());
        }

        block_liveness
    }

    pub fn set_inst_gen(&mut self, id: i32, gen: HashSet<i32>) {
        self.inst_gen_map.insert(id, gen);
    }

    pub fn set_inst_kill(&mut self, id: i32, kill: HashSet<i32>) {
        self.inst_kill_map.insert(id, kill);
    }

    pub fn set_inst_in(&mut self, id: i32, in_: HashSet<i32>) {
        self.inst_in_map.insert(id, in_);
    }

    pub fn set_inst_out(&mut self, id: i32, out: HashSet<i32>) {
        self.inst_out_map.insert(id, out);
    }

    pub fn get_inst_gen(&self, id: i32) -> &HashSet<i32> {
        self.inst_gen_map.get(&id).unwrap()
    }

    pub fn get_inst_kill(&self, id: i32) -> &HashSet<i32> {
        self.inst_kill_map.get(&id).unwrap()
    }

    pub fn get_inst_in(&self, id: i32) -> &HashSet<i32> {
        self.inst_in_map.get(&id).unwrap()
    }

    pub fn get_inst_out(&self, id: i32) -> &HashSet<i32> {
        self.inst_out_map.get(&id).unwrap()
    }

    pub(crate) fn update(
        &mut self,
        bb: &Block,
        _function: &Function,
        block_liveness: &HashMap<i32, Rc<RefCell<BlockLiveness>>>,
        insts_map: &HashMap<i32, Vec<usize>>,
    ) -> bool {
        let mut changed = false;

        let mut in_ = HashSet::new();
        let mut out;

        for succ in bb.out_edges().iter() {
            let succ_bb_block_liveness = block_liveness.get(succ).unwrap();
            let succ_bb_block_liveness = succ_bb_block_liveness.try_borrow().unwrap();

            let succ_in = &succ_bb_block_liveness.psuedo_in;
            // 所有后继块合并成一个虚拟块，计算该块的in
            in_ = in_.union(succ_in).cloned().collect();
        }

        // 更新每条指令的in和out
        if insts_map[bb.id()].is_empty() {
            if self.psuedo_in.len() != in_.len() {
                changed = true;
            }
            self.psuedo_in = in_;
        } else {
            for &inst_id in insts_map[bb.id()].iter() {
                let inst_id = inst_id as i32;
                let gen = self.get_inst_gen(inst_id);
                let kill = self.get_inst_kill(inst_id);

                out = in_.clone();
                let s: HashSet<_> = out.difference(kill).cloned().collect();
                in_ = gen.union(&s).cloned().collect::<HashSet<i32>>();

                if in_.len() != self.get_inst_in(inst_id).len() {
                    changed = true;
                    self.set_inst_in(inst_id, in_.clone());
                }

                if out.len() != self.get_inst_out(inst_id).len() {
                    changed = true;
                    self.set_inst_out(inst_id, out.clone());
                }
            }

            self.psuedo_in = in_;
        }

        changed
    }
}
