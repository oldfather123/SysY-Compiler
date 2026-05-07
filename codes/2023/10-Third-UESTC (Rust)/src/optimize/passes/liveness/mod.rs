use crate::frontend::llvm::{basic_block::BasicBlock, function::Function, instr::Instruction};
use std::{
    cell::RefCell,
    collections::{HashMap, HashSet},
    fmt::{Display, Formatter},
    rc::Rc,
};

pub struct LivenessAnalysis<'a> {
    pub block_liveness_map: HashMap<i32, Rc<RefCell<BlockLiveness>>>,
    pub function: &'a Function,
}

impl<'a> LivenessAnalysis<'a> {
    pub fn new(function: &'a Function) -> LivenessAnalysis<'a> {
        let mut res = LivenessAnalysis {
            block_liveness_map: HashMap::new(),
            function,
        };

        let func_layout = &function.layout();

        for block_id in func_layout.block_iter() {
            // 获取block中的inst_ids
            let inst_ids: Vec<i32> = func_layout.inst_iter(block_id).into_iter().rev().collect();

            // 转换为inst_id和inst的对
            let insts: Vec<(i32, &Instruction)> = inst_ids
                .iter()
                .map(|&id| (id, function.instructions().get(&id).unwrap()))
                .collect();

            println!("SHOW YOU !!!!");
            insts.iter().for_each(|(id, i)| {
                println!("{}: {:?}", id, i);
            });

            // 构造block_liveness
            res.block_liveness_map
                .insert(block_id, Rc::new(RefCell::new(BlockLiveness::new(insts))));
        }

        // 不动点算法：迭代直到不再变化
        loop {
            let mut changed = false;
            for block_id in func_layout.block_iter() {
                let bb = function.basic_blocks().get(&block_id).unwrap();
                let block_liveness = res.block_liveness_map.get(&block_id).unwrap();
                changed = changed
                    || block_liveness.clone().borrow_mut().update(
                        bb,
                        res.function,
                        &res.block_liveness_map,
                    );
            }
            if !changed {
                break;
            }
        }

        res
    }
}

#[derive(Clone, Debug)]
pub struct BlockLiveness {
    inst_gen_map: HashMap<i32, HashSet<i32>>,
    inst_kill_map: HashMap<i32, HashSet<i32>>,
    inst_in_map: HashMap<i32, HashSet<i32>>,
    inst_out_map: HashMap<i32, HashSet<i32>>,
    pub insts: Vec<i32>,
}

impl Display for BlockLiveness {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        for inst_id in &self.insts {
            writeln!(
                f,
                "inst_id: {}, gen: {:?}, kill: {:?}, in: {:?}, out: {:?}",
                inst_id,
                self.get_inst_gen(*inst_id),
                self.get_inst_kill(*inst_id),
                self.get_inst_in(*inst_id),
                self.get_inst_out(*inst_id)
            )?;
        }
        Ok(())
    }
}

impl BlockLiveness {
    pub fn new(insts: Vec<(i32, &Instruction)>) -> BlockLiveness {
        let mut block_liveness = BlockLiveness {
            inst_gen_map: HashMap::new(),
            inst_kill_map: HashMap::new(),
            inst_in_map: HashMap::new(),
            inst_out_map: HashMap::new(),
            insts: insts.iter().map(|(id, _)| *id).collect(),
        };

        for (inst_id, i) in insts {
            let (kill, gen1, gen2) = i.instr().get_operands();

            let mut gen = HashSet::new();
            if gen1 != 0 {
                gen.insert(gen1);
            }
            if gen2 != 0 {
                gen.insert(gen2);
            }

            block_liveness.set_inst_gen(inst_id, gen);

            block_liveness.set_inst_kill(
                inst_id,
                if kill != 0 {
                    vec![kill].into_iter().collect()
                } else {
                    HashSet::new()
                },
            );

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

    pub fn update(
        &mut self,
        bb: &BasicBlock,
        function: &Function,
        block_liveness: &HashMap<i32, Rc<RefCell<BlockLiveness>>>,
    ) -> bool {
        let mut changed = false;

        let mut in_ = HashSet::new();
        let mut out;

        for succ in bb.succ_bb().iter() {
            let succ_bb_first_inst = function.layout().block_node(*succ).first_inst.unwrap();
            let succ_bb_block_liveness = block_liveness.get(&succ).unwrap();
            let succ_bb_block_liveness = succ_bb_block_liveness.try_borrow().unwrap();
            let succ_in = succ_bb_block_liveness.get_inst_in(succ_bb_first_inst);

            // 所有后继块合并成一个虚拟块，计算该块的in
            in_ = in_.union(succ_in).cloned().collect();
        }

        // 更新每条指令的in和out
        for &inst_id in self.insts.clone().iter() {
            let gen = self.get_inst_gen(inst_id);
            let kill = self.get_inst_kill(inst_id);

            out = in_.clone();
            let s: HashSet<_> = out.difference(kill).cloned().collect();
            in_ = gen.union(&s).cloned().collect::<HashSet<i32>>();

            if !in_.eq(self.get_inst_in(inst_id)) {
                changed = true;
                self.set_inst_in(inst_id, in_.clone());
            }

            if !out.eq(self.get_inst_out(inst_id)) {
                changed = true;
                self.set_inst_out(inst_id, out.clone());
            }
        }

        changed
    }
}
