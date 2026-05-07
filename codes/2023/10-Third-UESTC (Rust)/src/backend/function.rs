use super::{
    arch_info::RegConvention,
    block::{Block, BlockId},
    instr::*,
    misc::{AsmContext, MappingInfo, StackObject},
    register::Reg,
};
use crate::common::{
    constant::{BLOCK_LABEL_PREFIX, ENTRY_BLOCK_LABEL_PREFIX},
    r#type::Type,
};
use crate::frontend::llvm::function::Function as LLVMFunction;
use crate::optimize::passes::dom::DominatorTreeBuilder;
use derive_new::new;
use getset::{Getters, MutGetters, Setters};
use std::{
    cell::RefCell,
    collections::{HashMap, HashSet},
    rc::Rc,
};

#[derive(Default)]
pub struct StackFrame {
    top: u32,
    stk: Vec<Rc<RefCell<StackObject>>>,
}

impl StackFrame {
    pub fn new() -> Self {
        StackFrame {
            top: 0,
            stk: vec![],
        }
    }

    pub fn push_stack_object(&mut self, so: Rc<RefCell<StackObject>>) {
        so.borrow_mut().set_position(self.top as i32);
        self.top += so.borrow().size();
        self.stk.push(so);
    }

    pub fn push_word(&mut self) -> usize {
        self.push(4)
    }
    pub fn push_dword(&mut self) -> usize {
        self.push(8)
    }

    // size is in bytes
    // return the index of stack object in self.stk
    pub fn push(&mut self, size: u32) -> usize {
        let so = Rc::new(RefCell::new(StackObject::new(self.top as i32, size)));
        self.stk.push(so);
        self.top += size;

        self.stk.len() - 1
    }

    pub fn get_stack_object(&self, index: usize) -> Rc<RefCell<StackObject>> {
        self.stk[index].clone()
    }

    // align top to 8 bytes
    pub fn total_frame_size(&self) -> usize {
        let mut res = self.top;
        let align = 16;
        if res % align != 0 {
            res += align - res % align;
        }
        res as usize
    }
}

#[derive(new, Getters, Setters, MutGetters)]
pub struct Function {
    #[getset(get = "pub", set = "pub")]
    name: String,
    #[new(default)]
    #[getset(get = "pub", set = "pub")]
    entry_block_id: BlockId,
    #[new(default)]
    args: Vec<Reg>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    blocks: Vec<Block>,
    #[new(default)]
    stack_objects: Vec<Rc<RefCell<StackObject>>>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    caller_stack_objects: Vec<Rc<RefCell<StackObject>>>,
    // context
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    spilling_reg: HashSet<Reg>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    symbol_reg: HashMap<Reg, String>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    stack_addr_reg: HashMap<Reg, (Rc<RefCell<StackObject>>, i32)>,
    #[new(default)]
    reg_num: i32,
    #[new(default)]
    float_regs: HashSet<Reg>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub", set = "pub")]
    ctx: Rc<RefCell<AsmContext>>,
    #[getset(get = "pub")]
    #[new(default)]
    arg_reg_ids: Vec<i32>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    // reg_id -> stack_object index
    callee_saved_regs: HashMap<i32, usize>,
    #[new(default)]
    #[getset(get = "pub", get_mut = "pub")]
    sf: Rc<RefCell<StackFrame>>,
}

impl Function {
    pub(crate) fn block(&self, block_id: BlockId) -> &Block {
        &self.blocks[(block_id - self.entry_block_id) as usize]
    }
    pub(crate) fn block_mut(&mut self, block_id: BlockId) -> &mut Block {
        &mut self.blocks[(block_id - self.entry_block_id) as usize]
    }
    pub(crate) fn handle_params(
        &mut self,
        entry_block: &mut Block,
        llvm_function: &LLVMFunction,
        mapping_info: &mut MappingInfo,
    ) {
        let mut int_arg_count = 0;
        let mut float_arg_count = 0;
        for arg_data in llvm_function.arg_list().as_normal().unwrap().iter() {
            let cur_reg = mapping_info.from_ssa_rvalue(&arg_data.to_arg_rvalue());
            if arg_data.get_type() == Type::Int || arg_data.is_array_arg() {
                if int_arg_count < RegConvention::<i32>::ARGUMENT_REGISTER_COUNT {
                    entry_block.push_back(Box::new(RegInstr::new_move(
                        cur_reg,
                        Reg::new_int(RegConvention::<i32>::ARGUMENT_REGISTERS[int_arg_count]),
                    )));
                } else {
                    let caller_stack_object_rc_refcell =
                        Rc::new(RefCell::new(StackObject::new(-1, arg_data.size())));
                    entry_block.push_back(Box::new(LoadStackInstr::new(
                        cur_reg,
                        -1,
                        caller_stack_object_rc_refcell.clone(),
                    )));
                    self.caller_stack_objects
                        .push(caller_stack_object_rc_refcell.clone());
                };
                int_arg_count += 1;
            } else if arg_data.get_type() == Type::Float {
                if float_arg_count < RegConvention::<f32>::ARGUMENT_REGISTER_COUNT {
                    entry_block.push_back(Box::new(FRegInstr::new_fmove(
                        cur_reg,
                        Reg::new_float(RegConvention::<f32>::ARGUMENT_REGISTERS[float_arg_count]),
                    )));
                } else {
                    let caller_stack_object_rc_refcell =
                        Rc::new(RefCell::new(StackObject::new(-1, arg_data.size())));
                    entry_block.push_back(Box::new(LoadStackInstr::new(
                        cur_reg,
                        -1,
                        caller_stack_object_rc_refcell.clone(),
                    )));
                    self.caller_stack_objects
                        .push(caller_stack_object_rc_refcell.clone());
                };
                float_arg_count += 1;
            } else {
                panic!("Unsupported type for function argument");
            };
            self.args.push(cur_reg);
        }
    }

    pub(crate) fn construct_from_llvm_function(
        &mut self,
        llvm_function: &LLVMFunction,
        block_num: &mut i32,
    ) {
        self.arg_reg_ids = llvm_function
            .arg_list()
            .as_normal()
            .unwrap()
            .iter()
            .map(|value| *value.id())
            .collect();

        let name = llvm_function.name().to_string();
        self.set_name(name.clone());

        let mut mapping_info = MappingInfo::new();
        for (_, mem_object) in llvm_function.mem_scope().objects() {
            if mem_object.size() == 0 {
                continue;
            }
            let stack_object_rc_refcell =
                Rc::new(RefCell::new(StackObject::from_ssa_lvalue(mem_object)));
            mapping_info
                .obj_mapping_mut()
                .insert(*mem_object.id(), stack_object_rc_refcell.clone());
            if !*mem_object.is_arg() {
                self.stack_objects.push(stack_object_rc_refcell);
            }
        }

        let entry_block_id = *block_num;
        self.set_entry_block_id(entry_block_id);
        *block_num += 1;
        let entry_block = Block::new(entry_block_id, ENTRY_BLOCK_LABEL_PREFIX.to_string() + &name);

        let mut blocks_vec = Vec::new();
        blocks_vec.push(entry_block);

        let mut dom_tree_builder = DominatorTreeBuilder::new(llvm_function);
        dom_tree_builder.build_dominator_frontier();
        // TODO: Loop Context

        let mut block_id = *block_num;
        let start_block_id = block_id;
        let mut bb_ids = llvm_function
            .basic_blocks()
            .keys()
            .map(|x| *x)
            .collect::<Vec<_>>();
        bb_ids.sort(); // 从小到大遍历
        for bb_id in bb_ids {
            let block_name = BLOCK_LABEL_PREFIX.to_string() + block_id.to_string().as_str();
            let block = Block::new(block_id, block_name);
            mapping_info.insert_block(block_id, bb_id);
            block_id += 1;
            blocks_vec.push(block);
        }
        let end_block_id = block_id;
        *block_num = block_id;
        let entry_block = blocks_vec.get_mut(0).unwrap(); // first block is entry block
        self.handle_params(entry_block, llvm_function, &mut mapping_info);
        let real_entry_block_id = *mapping_info
            .block_mapping()
            .get(&llvm_function.entry_bb_id())
            .unwrap();
        // fsrm
        // if name == "main" {
        //     let rtz_reg = mapping_info.new_reg(Type::Int);
        //     entry_block.push_back(Box::new(ImmeInstr::new_load_immediate(rtz_reg, 0b001)));
        //     entry_block.push_back(Box::new(FsrmInstr::new(rtz_reg)));
        // }

        if real_entry_block_id != 1 + entry_block_id {
            entry_block.push_back(Box::new(JumpInstr::new_jump(
                BLOCK_LABEL_PREFIX.to_string() + real_entry_block_id.to_string().as_str(),
            )));
        }
        entry_block.add_out_edge(real_entry_block_id);
        blocks_vec[0].add_in_edge(entry_block_id);
        for self_block_id in start_block_id..end_block_id {
            let next_block_id = if self_block_id == end_block_id - 1 {
                None
            } else {
                Some(self_block_id + 1)
            };
            Block::construct_from_llvm_basic_block(
                &mut blocks_vec,
                self_block_id,
                next_block_id,
                entry_block_id,
                self,
                llvm_function,
                &mut mapping_info,
            )
        }
        self.blocks = blocks_vec;
        self.reg_num = *mapping_info.reg_num();
        self.float_regs = mapping_info.float_regs().clone();
    }
    pub(crate) fn gen_asm(&self) -> String {
        let mut asm = String::new();
        asm.push_str(&format!("\n{}:\n", self.name));
        // asm.push_str(self.ctx.borrow().prologue()().as_str());
        for block in &self.blocks {
            asm.push_str(&block.gen_asm());
        }
        asm
    }
}
