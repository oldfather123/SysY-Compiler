use super::dom::DominatorTreeBuilder;
use crate::{
    common::{immediate::Immediate, r#type::Type},
    frontend::llvm::{function::Function, instr::*, llvm_module::LLVMModule, ssa::SSARightValue},
};
use getset::{Getters, MutGetters, Setters};
use std::collections::{HashMap, HashSet};
use std::option::Option;

pub fn unary_compute(op: UnaryOp, v: Immediate) -> Immediate {
    match op {
        UnaryOp::Neg => Immediate::Int(-v.as_int().unwrap()),
        UnaryOp::Not => Immediate::Int((*v.as_int().unwrap() == 0) as i32),
        UnaryOp::Fptosi => Immediate::Int(*v.as_float().unwrap() as i32),
        UnaryOp::Sitofp => Immediate::Float(*v.as_int().unwrap() as f32),
        _ => panic!("unary_compute: unknown op {:?}", op),
    }
}

pub fn binary_compute(op: BinaryOp, s1: Immediate, s2: Immediate) -> Immediate {
    match op {
        BinaryOp::Add => Immediate::Int(s1.as_int().unwrap() + s2.as_int().unwrap()),
        BinaryOp::Sub => Immediate::Int(s1.as_int().unwrap() - s2.as_int().unwrap()),
        BinaryOp::Mul => Immediate::Int(s1.as_int().unwrap() * s2.as_int().unwrap()),
        BinaryOp::Div => Immediate::Int(s1.as_int().unwrap() / s2.as_int().unwrap()),
        BinaryOp::Mod => {
            if s1.get_type() == Type::Int && s2.get_type() == Type::Int {
                Immediate::Int(s1.as_int().unwrap() % s2.as_int().unwrap())
            } else {
                panic!("binary_compute: mod only support int");
            }
        }
        BinaryOp::Shl => Immediate::Int(s1.as_int().unwrap() << s2.as_int().unwrap()),
        BinaryOp::AShr => Immediate::Int(s1.as_int().unwrap() >> s2.as_int().unwrap()),
        BinaryOp::LShr => Immediate::Int(s1.as_int().unwrap() >> s2.as_int().unwrap()),
        BinaryOp::FAdd => Immediate::Float(s1.as_float().unwrap() + s2.as_float().unwrap()),
        BinaryOp::FSub => Immediate::Float(s1.as_float().unwrap() - s2.as_float().unwrap()),
        BinaryOp::FMul => Immediate::Float(s1.as_float().unwrap() * s2.as_float().unwrap()),
        BinaryOp::FDiv => Immediate::Float(s1.as_float().unwrap() / s2.as_float().unwrap()),
    }
}

pub fn cmp_compute(cmp_type: CmpType, s1: Immediate, s2: Immediate) -> Immediate {
    match cmp_type {
        CmpType::EQ => Immediate::Int((s1 == s2) as i32),
        CmpType::NEQ => Immediate::Int((s1 != s2) as i32),
        CmpType::SGT => {
            if s1.get_type() == Type::Float && s2.get_type() == Type::Float {
                Immediate::Int((s1.as_float().unwrap() > s2.as_float().unwrap()) as i32)
            } else {
                Immediate::Int((s1.as_int().unwrap() > s2.as_int().unwrap()) as i32)
            }
        }
        CmpType::SGE => {
            if s1.get_type() == Type::Float && s2.get_type() == Type::Float {
                Immediate::Int((s1.as_float().unwrap() >= s2.as_float().unwrap()) as i32)
            } else {
                Immediate::Int((s1.as_int().unwrap() >= s2.as_int().unwrap()) as i32)
            }
        }
        CmpType::SLT => {
            if s1.get_type() == Type::Float && s2.get_type() == Type::Float {
                Immediate::Int((s1.as_float().unwrap() < s2.as_float().unwrap()) as i32)
            } else {
                Immediate::Int((s1.as_int().unwrap() < s2.as_int().unwrap()) as i32)
            }
        }
        CmpType::SLE => {
            if s1.get_type() == Type::Float && s2.get_type() == Type::Float {
                Immediate::Int((s1.as_float().unwrap() <= s2.as_float().unwrap()) as i32)
            } else {
                Immediate::Int((s1.as_int().unwrap() <= s2.as_int().unwrap()) as i32)
            }
        }
    }
}

pub fn is_useless_compute(op: BinaryOp, s1: Immediate, s2: Immediate) -> bool {
    match op {
        BinaryOp::Add => *s1.as_int().unwrap() == 0 || *s2.as_int().unwrap() == 0,
        BinaryOp::FAdd => *s1.as_float().unwrap() == 0.0 || *s2.as_float().unwrap() == 0.0,
        BinaryOp::Sub => *s2.as_int().unwrap() == 0,
        BinaryOp::FSub => *s2.as_float().unwrap() == 0.0,
        BinaryOp::Mul => *s1.as_int().unwrap() == 1 || *s2.as_int().unwrap() == 1,
        BinaryOp::FMul => *s1.as_float().unwrap() == 1.0 || *s2.as_float().unwrap() == 1.0,
        BinaryOp::Div => *s2.as_int().unwrap() == 1,
        BinaryOp::FDiv => *s2.as_float().unwrap() == 1.0,
        _ => false,
    }
}

pub fn is_commutable_compute(op: BinaryOp) -> bool {
    match op {
        BinaryOp::Add | BinaryOp::Mul | BinaryOp::FAdd | BinaryOp::FMul => true,
        _ => false,
    }
}

fn find_same_value(phi: &Phi) -> Option<SSARightValue> {
    let mut r0: Option<SSARightValue> = None;
    for (r, _) in phi.uses().iter() {
        if r != phi.d1() {
            if r0.is_some() && r0.unwrap() != *r {
                return None;
            }
            r0 = Some(r.clone());
        }
    }
    r0
}

#[derive(Debug, Getters, Setters, MutGetters, Clone)]
struct GVNNode {
    #[getset(get = "pub", get_mut = "pub")]
    outs: Vec<i32>,
    #[getset(get = "pub", get_mut = "pub")]
    instrs: Vec<i32>,
    #[getset(get = "pub", get_mut = "pub")]
    new_scalars: HashSet<Immediate>,
    #[getset(get = "pub", get_mut = "pub")]
    new_const_regs: HashSet<i32>,
    #[getset(get = "pub", get_mut = "pub")]
    new_unaries: HashSet<(UnaryOp, i32)>,
    #[getset(get = "pub", get_mut = "pub")]
    new_binaries: HashSet<(BinaryOp, i32, i32)>,
    #[getset(get = "pub", get_mut = "pub")]
    new_array_indexs: HashSet<(i32, i32, i32)>,
    #[getset(get = "pub", get_mut = "pub")]
    new_addrs: HashSet<SSARightValue>,
    #[getset(get = "pub", get_mut = "pub")]
    new_cmps: HashSet<(CmpType, i32, i32)>,
}

impl GVNNode {
    fn new() -> Self {
        Self {
            outs: Vec::new(),
            instrs: Vec::new(),
            new_scalars: HashSet::new(),
            new_const_regs: HashSet::new(),
            new_unaries: HashSet::new(),
            new_binaries: HashSet::new(),
            new_array_indexs: HashSet::new(),
            new_addrs: HashSet::new(),
            new_cmps: HashSet::new(),
        }
    }

    fn add_out_node(&mut self, node: i32) {
        self.outs.push(node);
    }
}

#[derive(Debug, Getters, MutGetters, Setters)]
struct GVNContext<'a> {
    nodes: Vec<i32>,
    scalar_reg_by_value: HashMap<Immediate, i32>,
    scalar_value_by_reg: HashMap<i32, Immediate>,
    mov_regs: HashMap<i32, i32>,
    unary_values: HashMap<(UnaryOp, i32), i32>,
    binary_values: HashMap<(BinaryOp, i32, i32), i32>,
    cmp_values: HashMap<(CmpType, i32, i32), i32>,
    array_index_values: HashMap<(i32, i32, i32), i32>,
    addr_values: HashMap<SSARightValue, i32>,
    #[getset(get = "pub", get_mut = "pub")]
    entry: i32,
    uses: Vec<Vec<i32>>,
    #[getset(get = "pub", get_mut = "pub")]
    func: &'a mut Function,
    #[getset(get = "pub", get_mut = "pub")]
    gvn_nodes: HashMap<i32, GVNNode>,
}

impl<'a> GVNContext<'a> {
    fn new(dom_children_map: HashMap<i32, HashSet<i32>>, func_: &'a mut Function) -> Self {
        let mut nodes = Vec::new();
        let mut gvn_nodes = HashMap::new();
        nodes.reserve(func_.layout().basic_blocks().len() as usize);
        for bb in func_.layout().block_iter() {
            let mut gvn_node = GVNNode::new();
            let outs = dom_children_map.get(&bb).unwrap();
            for out in outs {
                gvn_node.add_out_node(*out);
            }
            nodes.push(bb);
            gvn_nodes.insert(bb, gvn_node);
        }
        for node in nodes.clone() {
            let gvn_node = gvn_nodes.get_mut(&node).unwrap();
            for instr_id in func_.layout().inst_iter(node) {
                gvn_node.instrs.push(instr_id);
            }
        }
        Self {
            nodes,
            scalar_reg_by_value: HashMap::new(),
            scalar_value_by_reg: HashMap::new(),
            mov_regs: HashMap::new(),
            unary_values: HashMap::new(),
            binary_values: HashMap::new(),
            cmp_values: HashMap::new(),
            array_index_values: HashMap::new(),
            addr_values: HashMap::new(),
            entry: func_.layout().entry_block().unwrap(),
            uses: Vec::new(),
            func: func_,
            // instructions,
            gvn_nodes,
        }
    }

    pub fn build_uses(&mut self) {
        self.uses = vec![Vec::new(); self.func.id() as usize];
        for node in self.nodes.iter_mut() {
            for i in self.gvn_nodes.get(&node).unwrap().instrs() {
                if let Some(reg_use_instr) = self
                    .func
                    .instructions()
                    .get(i)
                    .unwrap()
                    .instr()
                    .try_as_reg_use_instr()
                {
                    for reg in reg_use_instr.uses() {
                        if reg.is_immediate() || reg.is_global() {
                            continue;
                        }
                        self.uses[*reg.id() as usize].push(*i);
                    }
                }
            }
        }
    }

    fn replace_same_value(&mut self, target_reg_id: i32, reference_reg_id: i32) {
        assert!(reference_reg_id > 0);
        let uses = self.uses[target_reg_id as usize].clone();
        for _use in uses {
            if let Some(reg_use_instr) = self
                .func_mut()
                .instructions_mut()
                .get_mut(&_use)
                .unwrap()
                .instr_mut()
                .try_as_reg_use_instr_mut()
            {
                for reg in reg_use_instr.uses_mut() {
                    if reg.is_immediate() || reg.is_global() {
                        continue;
                    }
                    if reg.id() == &target_reg_id {
                        reg.set_id(reference_reg_id);
                    }
                }
            }
        }
    }

    fn process_scalar(&mut self, d1_s1: (SSARightValue, SSARightValue), node_id: &i32) {
        if self
            .scalar_reg_by_value
            .contains_key(&d1_s1.1.get_value().unwrap())
        {
            assert!(self.scalar_reg_by_value[&d1_s1.1.get_value().unwrap()] != 0);
            let reg = self.scalar_reg_by_value[&d1_s1.1.get_value().unwrap()];
            self.replace_same_value(*d1_s1.0.id(), reg);
        } else {
            assert!(*d1_s1.0.id() != 0);
            self.scalar_reg_by_value
                .insert(d1_s1.1.get_value().unwrap(), *d1_s1.0.id());
            self.scalar_value_by_reg
                .insert(*d1_s1.0.id(), d1_s1.1.get_value().unwrap());
            self.gvn_nodes
                .get_mut(node_id)
                .unwrap()
                .new_scalars
                .insert(d1_s1.1.get_value().unwrap());
            self.gvn_nodes
                .get_mut(node_id)
                .unwrap()
                .new_const_regs
                .insert(*d1_s1.0.id());
        }
    }

    fn set_load_const(&mut self, inst_id: i32, d1: i32, v: Immediate) {
        if v.get_type() == Type::Int {
            let new_instr = Instruction::new(
                Box::new(Mov::new(
                    SSARightValue::new_reg(d1, Type::Int),
                    SSARightValue::new_imme(v),
                )),
                self.func
                    .instructions()
                    .get(&inst_id)
                    .unwrap()
                    .bb_id()
                    .clone(),
            );
            self.func.instructions_mut().insert(inst_id, new_instr);
        } else {
            let new_instr = Instruction::new(
                Box::new(FMov::new(
                    SSARightValue::new_reg(d1, Type::Float),
                    SSARightValue::new_imme(v),
                )),
                self.func
                    .instructions()
                    .get(&inst_id)
                    .unwrap()
                    .bb_id()
                    .clone(),
            );
            self.func.instructions_mut().insert(inst_id, new_instr);
        }
    }

    fn set_ret_const(&mut self, inst_id: i32, v: Immediate) {
        let new_instr = Instruction::new(
            Box::new(Ret::new(Some(SSARightValue::new_imme(v)))),
            self.func
                .instructions()
                .get(&inst_id)
                .unwrap()
                .bb_id()
                .clone(),
        );
        self.func.instructions_mut().insert(inst_id, new_instr);
    }

    fn process_computed(&mut self, i: i32, d1: i32, computed: Immediate, node_id: &i32) {
        if self.scalar_reg_by_value.contains_key(&computed) {
            self.replace_same_value(d1, self.scalar_reg_by_value.get(&computed).unwrap().clone());
        } else {
            self.scalar_reg_by_value.insert(computed, d1);
            self.scalar_value_by_reg.insert(d1, computed);
            self.gvn_nodes
                .get_mut(node_id)
                .unwrap()
                .new_const_regs
                .insert(d1);
            self.gvn_nodes
                .get_mut(node_id)
                .unwrap()
                .new_scalars
                .insert(computed);
            self.set_load_const(i, d1, computed);
        }
    }

    pub fn dfs(&mut self, node_id: &i32) {
        let outs = self.gvn_nodes.get_mut(node_id).unwrap().outs().clone();
        for i in self.gvn_nodes.get_mut(node_id).unwrap().instrs().clone() {
            if let Some(mov_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .as_any()
                .downcast_ref::<Mov>()
            {
                if mov_instr.s1().is_immediate() {
                    self.process_scalar((mov_instr.d1().clone(), mov_instr.s1().clone()), node_id)
                } else {
                    // if self.mov_regs.contains_key(mov_instr.s1().id()) {
                    //     self.replace_same_value(
                    //         *mov_instr.d1().id(),
                    //         self.mov_regs[mov_instr.s1().id()],
                    //     );
                    // } else if self.scalar_value_by_reg.contains_key(mov_instr.s1().id()) {
                    //     let value = self.scalar_value_by_reg[mov_instr.s1().id()];
                    //     self.process_computed(i, *mov_instr.d1().id(), value, node_id)
                    // } else {
                    //     self.mov_regs
                    //         .insert(*mov_instr.s1().id(), *mov_instr.d1().id());
                    // }
                    self.replace_same_value(*mov_instr.d1().id(), *mov_instr.s1().id());
                }
            } else if let Some(fmov_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .as_any()
                .downcast_ref::<FMov>()
            {
                if fmov_instr.s1().is_immediate() {
                    self.process_scalar((fmov_instr.d1().clone(), fmov_instr.s1().clone()), node_id)
                } else {
                    // if self.mov_regs.contains_key(fmov_instr.s1().id()) {
                    //     self.replace_same_value(
                    //         *fmov_instr.d1().id(),
                    //         self.mov_regs[fmov_instr.s1().id()],
                    //     );
                    // } else if self.scalar_value_by_reg.contains_key(fmov_instr.s1().id()) {
                    //     let value = self.scalar_value_by_reg[fmov_instr.s1().id()];
                    //     self.process_computed(i, *fmov_instr.d1().id(), value, node_id)
                    // } else {
                    //     self.mov_regs
                    //         .insert(*fmov_instr.s1().id(), *fmov_instr.d1().id());
                    // }
                    self.replace_same_value(*fmov_instr.d1().id(), *fmov_instr.s1().id());
                }
            } else if let Some(unary_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .try_as_unary_instr()
            {
                if unary_instr.s1().is_immediate() {
                    let value = unary_instr.s1().get_value().unwrap();
                    let computed = unary_compute(unary_instr.unary_op(), value);
                    self.process_computed(i, *unary_instr.d1().id(), computed, node_id);
                } else {
                    let key = (unary_instr.unary_op(), *unary_instr.s1().id());
                    if self.unary_values.contains_key(&key) {
                        assert!(self.unary_values[&key] != 0);
                        self.replace_same_value(*unary_instr.d1().id(), self.unary_values[&key]);
                    } else if self.scalar_value_by_reg.contains_key(unary_instr.s1().id()) {
                        let value = self.scalar_value_by_reg[unary_instr.s1().id()];
                        let computed = unary_compute(unary_instr.unary_op(), value);
                        self.process_computed(i, *unary_instr.d1().id(), computed, node_id);
                    } else {
                        self.unary_values.insert(key, *unary_instr.d1().id());
                        self.gvn_nodes
                            .get_mut(node_id)
                            .unwrap()
                            .new_unaries
                            .insert(key);
                    }
                }
            } else if let Some(binary_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .try_as_binary_instr()
            {
                if binary_instr.s1().is_immediate() && binary_instr.s2().is_immediate() {
                    let value1 = binary_instr.s1().get_value().unwrap();
                    let value2 = binary_instr.s2().get_value().unwrap();
                    let computed = binary_compute(binary_instr.binary_op(), value1, value2);
                    self.process_computed(i, *binary_instr.d1().id(), computed, node_id);
                } else if !binary_instr.s1().is_immediate() && binary_instr.s2().is_immediate() {
                    if self
                        .scalar_value_by_reg
                        .contains_key(binary_instr.s1().id())
                    {
                        let value1 = self.scalar_value_by_reg[binary_instr.s1().id()];
                        let value2 = binary_instr.s2().get_value().unwrap();
                        let computed = binary_compute(binary_instr.binary_op(), value1, value2);
                        self.process_computed(i, *binary_instr.d1().id(), computed, node_id);
                    } else {
                        // value maybe from other block
                    }
                } else if binary_instr.s1().is_immediate() && !binary_instr.s2().is_immediate() {
                    if self
                        .scalar_value_by_reg
                        .contains_key(binary_instr.s2().id())
                    {
                        let value1 = binary_instr.s1().get_value().unwrap();
                        let value2 = self.scalar_value_by_reg[binary_instr.s2().id()];
                        let computed = binary_compute(binary_instr.binary_op(), value1, value2);
                        self.process_computed(i, *binary_instr.d1().id(), computed, node_id);
                    } else {
                        // value maybe from other block
                    }
                } else {
                    let key = (
                        binary_instr.binary_op().clone(),
                        binary_instr.s1().id().clone(),
                        binary_instr.s2().id().clone(),
                    );
                    if self.binary_values.contains_key(&key) {
                        assert!(self.binary_values[&key] != 0);
                        self.replace_same_value(*binary_instr.d1().id(), self.binary_values[&key]);
                    } else if self
                        .scalar_value_by_reg
                        .contains_key(binary_instr.s1().id())
                        && self
                            .scalar_value_by_reg
                            .contains_key(binary_instr.s2().id())
                    {
                        let value1 = self.scalar_value_by_reg[binary_instr.s1().id()];
                        let value2 = self.scalar_value_by_reg[binary_instr.s2().id()];
                        let computed = binary_compute(binary_instr.binary_op(), value1, value2);
                        self.process_computed(i, *binary_instr.d1().id(), computed, node_id);
                    } else {
                        let mut value1: Option<Immediate> = None;
                        let mut value2: Option<Immediate> = None;
                        if self
                            .scalar_value_by_reg
                            .contains_key(binary_instr.s1().id())
                        {
                            value1 = Some(self.scalar_value_by_reg[binary_instr.s1().id()]);
                        }
                        if self
                            .scalar_value_by_reg
                            .contains_key(binary_instr.s2().id())
                        {
                            value2 = Some(self.scalar_value_by_reg[binary_instr.s2().id()]);
                        }
                        if value1.is_some()
                            && value2.is_some()
                            && is_useless_compute(
                                binary_instr.binary_op(),
                                value1.unwrap(),
                                value2.unwrap(),
                            )
                        {
                            if value1.is_some() {
                                assert!(binary_instr.s1().id() != &0);
                                self.replace_same_value(
                                    *binary_instr.d1().id(),
                                    *binary_instr.s2().id(),
                                );
                            } else if value2.is_some() {
                                assert!(binary_instr.s2().id() != &0);
                                self.replace_same_value(
                                    *binary_instr.d1().id(),
                                    *binary_instr.s1().id(),
                                );
                            } else {
                                // value maybe from other block
                            }
                        } else {
                            let key_s1_s2 = (
                                binary_instr.binary_op(),
                                *binary_instr.s1().id(),
                                *binary_instr.s2().id(),
                            );
                            let key_s2_s1 = (
                                binary_instr.binary_op(),
                                *binary_instr.s2().id(),
                                *binary_instr.s1().id(),
                            );
                            if self.binary_values.contains_key(&key_s1_s2) {
                                assert!(self.binary_values[&key_s1_s2] != 0);
                                self.replace_same_value(
                                    *binary_instr.d1().id(),
                                    self.binary_values[&key_s1_s2],
                                );
                            } else {
                                self.gvn_nodes
                                    .get_mut(node_id)
                                    .unwrap()
                                    .new_binaries_mut()
                                    .insert(key_s1_s2);
                                self.binary_values
                                    .insert(key_s1_s2, *binary_instr.d1().id());
                                if key_s1_s2 != key_s2_s1
                                    && is_commutable_compute(binary_instr.binary_op())
                                {
                                    self.gvn_nodes
                                        .get_mut(node_id)
                                        .unwrap()
                                        .new_binaries
                                        .insert(key_s2_s1);
                                    self.binary_values
                                        .insert(key_s2_s1, *binary_instr.d1().id());
                                }
                            }
                        }
                    }
                }
            } else if let Some(gep_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .as_any()
                .downcast_ref::<Gep>()
            {
                let mut value2: Option<Immediate> = None;
                if gep_instr.index().is_immediate() || gep_instr.index().is_global() {
                    value2 = Some(gep_instr.index().get_value().unwrap());
                } else if self
                    .scalar_value_by_reg
                    .contains_key(gep_instr.index().id())
                {
                    value2 = Some(self.scalar_value_by_reg[gep_instr.index().id()]);
                }
                if value2.is_some() && value2.unwrap() == Immediate::Int(0) {
                    // assert!(gep_instr.s1().id() != &0);
                    // self.replace_same_value(*gep_instr.d1().id(), *gep_instr.s1().id());
                } else if !gep_instr.index().is_immediate() && !gep_instr.index().is_global() {
                    let key = (
                        *gep_instr.s1().id(),
                        *gep_instr.index().id(),
                        *gep_instr.s1().id(),
                    );
                    if self.array_index_values.contains_key(&key) {
                        assert!(self.array_index_values[&key] != 0);
                        self.replace_same_value(
                            *gep_instr.d1().id(),
                            self.array_index_values[&key],
                        );
                    } else {
                        self.array_index_values.insert(key, *gep_instr.d1().id());
                        self.gvn_nodes
                            .get_mut(node_id)
                            .unwrap()
                            .new_array_indexs
                            .insert(key);
                    }
                }
            } else if let Some(icmp_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .as_any()
                .downcast_ref::<Icmp>()
            {
                if (icmp_instr.s1().is_immediate() || icmp_instr.s1().is_global())
                    && (icmp_instr.s2().is_immediate() || icmp_instr.s2().is_global())
                {
                    let value1 = icmp_instr.s1().get_value().unwrap();
                    let value2 = icmp_instr.s2().get_value().unwrap();
                    let computed = cmp_compute(icmp_instr.cmp_type().clone(), value1, value2);
                    self.process_computed(i, *icmp_instr.d1().id(), computed, node_id);
                } else if (!icmp_instr.s1().is_immediate() && !icmp_instr.s1().is_global())
                    && (icmp_instr.s2().is_immediate() || icmp_instr.s2().is_global())
                {
                    if self.scalar_value_by_reg.contains_key(icmp_instr.s1().id()) {
                        let value1 = self.scalar_value_by_reg[icmp_instr.s1().id()];
                        let value2 = icmp_instr.s2().get_value().unwrap();
                        let computed = cmp_compute(icmp_instr.cmp_type().clone(), value1, value2);
                        self.process_computed(i, *icmp_instr.d1().id(), computed, node_id);
                    } else {
                    }
                } else if (icmp_instr.s1().is_immediate() || icmp_instr.s1().is_global())
                    && (!icmp_instr.s2().is_immediate() && !icmp_instr.s2().is_global())
                {
                    if self.scalar_value_by_reg.contains_key(icmp_instr.s2().id()) {
                        let value1 = icmp_instr.s1().get_value().unwrap();
                        let value2 = self.scalar_value_by_reg[icmp_instr.s2().id()];
                        let computed = cmp_compute(icmp_instr.cmp_type().clone(), value1, value2);
                        self.process_computed(i, *icmp_instr.d1().id(), computed, node_id);
                    } else {
                    }
                } else {
                    let key = (
                        icmp_instr.cmp_type.clone(),
                        icmp_instr.s1().id().clone(),
                        icmp_instr.s2().id().clone(),
                    );
                    if self.cmp_values.contains_key(&key) {
                        assert!(self.cmp_values[&key] != 0);
                        self.replace_same_value(*icmp_instr.d1().id(), self.cmp_values[&key]);
                    } else if self.scalar_value_by_reg.contains_key(icmp_instr.s1().id())
                        && self.scalar_value_by_reg.contains_key(icmp_instr.s2().id())
                    {
                        let value1 = self.scalar_value_by_reg[icmp_instr.s1().id()];
                        let value2 = self.scalar_value_by_reg[icmp_instr.s2().id()];
                        let computed = cmp_compute(icmp_instr.cmp_type().clone(), value1, value2);
                        self.process_computed(i, *icmp_instr.d1().id(), computed, node_id);
                    } else {
                        self.cmp_values.insert(key, *icmp_instr.d1().id());
                        self.gvn_nodes
                            .get_mut(node_id)
                            .unwrap()
                            .new_cmps
                            .insert(key);
                    }
                }
            } else if let Some(fcmp_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .as_any()
                .downcast_ref::<Fcmp>()
            {
                if (fcmp_instr.s1().is_immediate() || fcmp_instr.s1().is_global())
                    && (fcmp_instr.s2().is_immediate() || fcmp_instr.s2().is_global())
                {
                    let value1 = fcmp_instr.s1().get_value().unwrap();
                    let value2 = fcmp_instr.s2().get_value().unwrap();
                    let computed = cmp_compute(fcmp_instr.cmp_type().clone(), value1, value2);
                    self.process_computed(i, *fcmp_instr.d1().id(), computed, node_id);
                } else if (!fcmp_instr.s1().is_immediate() && !fcmp_instr.s1().is_global())
                    && (fcmp_instr.s2().is_immediate() || fcmp_instr.s2().is_global())
                {
                    if self.scalar_value_by_reg.contains_key(fcmp_instr.s1().id()) {
                        let value1 = self.scalar_value_by_reg[fcmp_instr.s1().id()];
                        let value2 = fcmp_instr.s2().get_value().unwrap();
                        let computed = cmp_compute(fcmp_instr.cmp_type().clone(), value1, value2);
                        self.process_computed(i, *fcmp_instr.d1().id(), computed, node_id);
                    } else {
                    }
                } else if (fcmp_instr.s1().is_immediate() || fcmp_instr.s1().is_global())
                    && (!fcmp_instr.s2().is_immediate() && !fcmp_instr.s2().is_global())
                {
                    if self.scalar_value_by_reg.contains_key(fcmp_instr.s2().id()) {
                        let value1 = fcmp_instr.s1().get_value().unwrap();
                        let value2 = self.scalar_value_by_reg[fcmp_instr.s2().id()];
                        let computed = cmp_compute(fcmp_instr.cmp_type().clone(), value1, value2);
                        self.process_computed(i, *fcmp_instr.d1().id(), computed, node_id);
                    } else {
                    }
                } else {
                    let key = (
                        fcmp_instr.cmp_type.clone(),
                        fcmp_instr.s1().id().clone(),
                        fcmp_instr.s2().id().clone(),
                    );
                    if self.cmp_values.contains_key(&key) {
                        assert!(self.cmp_values[&key] != 0);
                        self.replace_same_value(*fcmp_instr.d1().id(), self.cmp_values[&key]);
                    } else if self.scalar_value_by_reg.contains_key(fcmp_instr.s1().id())
                        && self.scalar_value_by_reg.contains_key(fcmp_instr.s2().id())
                    {
                        let value1 = self.scalar_value_by_reg[fcmp_instr.s1().id()];
                        let value2 = self.scalar_value_by_reg[fcmp_instr.s2().id()];
                        let computed = cmp_compute(fcmp_instr.cmp_type().clone(), value1, value2);
                        self.process_computed(i, *fcmp_instr.d1().id(), computed, node_id);
                    } else {
                        self.cmp_values.insert(key, *fcmp_instr.d1().id());
                        self.gvn_nodes
                            .get_mut(node_id)
                            .unwrap()
                            .new_cmps
                            .insert(key);
                    }
                }
            // } else if let Some(load_instr) = self
            //     .func
            //     .instructions()
            //     .get(&i)
            //     .unwrap()
            //     .instr()
            //     .as_any()
            //     .downcast_ref::<Load>()
            // {
            //     let key = load_instr.addr().clone();
            //     if self.addr_values.contains_key(&key) {
            //         self.replace_same_value(*load_instr.d1().id(), self.addr_values[&key]);
            //     } else {
            //         self.addr_values.insert(key.clone(), *load_instr.d1().id());
            //         self.gvn_nodes
            //             .get_mut(node_id)
            //             .unwrap()
            //             .new_addrs
            //             .insert(key);
            //     }
            // } else if let Some(store_instr) = self. func.instructions().get(&i).unwrap().instr().as_any().downcast_ref::<Store>() {
            //     let addr = store_instr.addr();
            //     let s1 = store_instr.s1();
            //     if !s1.is_immediate() && !s1.is_global() {
            //         if self.scalar_value_by_reg.contains_key(s1.id()) {
            //             self.scalar_value_by_reg.insert(*addr.id(), self.scalar_value_by_reg[s1.id()]);
            //         }
            //     }
            } else if let Some(phi_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .as_any()
                .downcast_ref::<Phi>()
            {
                if let Some(r0) = find_same_value(phi_instr) {
                    self.replace_same_value(*phi_instr.d1().id(), *r0.id());
                }
            } else if let Some(ret_instr) = self
                .func
                .instructions()
                .get(&i)
                .unwrap()
                .instr()
                .as_any()
                .downcast_ref::<Ret>()
            {
                if let Some(ret_value) = ret_instr.value() {
                    if ret_value.is_immediate() || ret_value.is_global() {
                        continue;
                    }
                    if self.scalar_value_by_reg.contains_key(ret_value.id()) {
                        let value = self.scalar_value_by_reg[ret_value.id()];
                        self.set_ret_const(i, value);
                    }
                }
            }
            // log::info!("instr_end: {:?}", self.func().instructions().get(&i).unwrap().instr());
        }

        for out in outs.iter() {
            self.dfs(out);
        }
        for new_scalar in self.gvn_nodes.get_mut(node_id).unwrap().new_scalars() {
            assert!(self.scalar_reg_by_value.remove(&new_scalar).is_some());
        }

        for new_const_reg in self.gvn_nodes.get_mut(node_id).unwrap().new_const_regs() {
            assert!(self.scalar_value_by_reg.remove(&new_const_reg).is_some());
        }

        for new_unary in self.gvn_nodes.get_mut(node_id).unwrap().new_unaries() {
            assert!(self.unary_values.remove(&new_unary).is_some());
        }

        for new_binary in self.gvn_nodes.get_mut(node_id).unwrap().new_binaries() {
            assert!(self.binary_values.remove(&new_binary).is_some());
        }

        for new_array_index in self.gvn_nodes.get_mut(node_id).unwrap().new_array_indexs() {
            assert!(self.array_index_values.remove(&new_array_index).is_some());
        }

        for new_addr in self.gvn_nodes.get_mut(node_id).unwrap().new_addrs() {
            assert!(self.addr_values.remove(&new_addr).is_some());
        }
    }

    // pub fn reconstruct(&mut self) {
    //     for bb in self.func.layout().block_iter() {
    //         let new_instrs = self.gvn_nodes.get(&bb).unwrap().instrs().clone();
    //         for inst_id in new_instrs {
    //             let instr = self.instructions.get(&inst_id).unwrap();
    //             if instr.removed {
    //                 continue;
    //             }
    //             log::info!("reconstruct: {:?}", self.func.instructions().get(&inst_id).unwrap().instr());
    //         }
    //     }
    // }
}

pub fn global_value_numbering_func(f: &mut Function) {
    let dom_children_map: HashMap<i32, HashSet<i32>>;
    {
        let mut dom_ctx = DominatorTreeBuilder::new(&f);
        dom_ctx.build_dominators_map();
        dom_ctx.build_dominator_frontier();
        dom_children_map = dom_ctx.dom_children_map().clone();
    }
    let mut ctx = GVNContext::new(dom_children_map, f);
    ctx.build_uses();
    let entry_node = ctx.entry;
    ctx.dfs(&entry_node);
}

pub fn global_value_numbering(m: &mut LLVMModule) {
    for (_, f) in m.functions_mut() {
        if *f.is_lib_func() {
            continue;
        }
        global_value_numbering_func(f);
    }
}
