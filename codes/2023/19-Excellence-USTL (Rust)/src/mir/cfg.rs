use crate::mir::{MIRArrayRef, MIRBlock, MIRFun, MIRGlDef, MIRLabel, MIRModule, MIRName, MIRStmt, MIRType, MIRTypedParam, MIRUnit, MIRVarRef};
use crate::util::{ElementRef, HashInsertVec};
use std::collections::{HashMap, VecDeque};
use std::hash::{Hash, Hasher};

pub struct CFG {
    pub(crate) global_defs: VecDeque<MIRGlDef>,
    pub(crate) runnable: VecDeque<CFGFun>,
}

impl CFG {
    pub fn new() -> CFG {
        CFG {
            global_defs: Default::default(),
            runnable: VecDeque::new(),
        }
    }

    pub(crate) fn add_fun(&mut self, unit: CFGFun) {
        self.runnable.push_back(unit)
    }

    pub(crate) fn add_gldef(&mut self, mir_define: MIRGlDef) {
        self.global_defs.push_back(mir_define)
    }

    pub fn to_liner(self) -> MIRModule {
        let mut module = MIRModule::new();
        for def in self.global_defs {
            module.units.push(MIRUnit::Def(def));
        }
        for fun in self.runnable {
            let mir_fun = fun.to_mir_fun();
            module.units.push(MIRUnit::Fun(mir_fun))
        }
        module
    }
}

pub enum CFGLayerObject {
    Layer(Box<CFGBlockLayer>),
    Block(CFGBasicBlockRef),
}

pub struct CFGBlockLayer {
    objects: VecDeque<CFGLayerObject>,
}

impl CFGBlockLayer {
    pub fn new() -> CFGBlockLayer {
        Self { objects: VecDeque::new() }
    }

    pub fn push_object(&mut self, layer_object: CFGLayerObject) {
        self.objects.push_back(layer_object)
    }

    pub fn pop_object(&mut self) -> CFGLayerObject {
        self.objects.pop_back().unwrap()
    }

    pub fn last_layer_mut(&mut self) -> Option<&mut CFGLayerObject> {
        self.objects.back_mut()
    }
}

pub(crate) struct CFGFun {
    name: MIRName,
    pub params: Vec<MIRTypedParam>,
    pub ret_type: MIRType,
    pub(crate)  linear_blocks: HashInsertVec<CFGBasicBlock>,
    pub(crate)  pre_require_blocks: HashMap<CFGBasicBlockRef, CFGBasicBlock>,
    pub(crate)  blocks_layer: CFGBlockLayer,
    /// 记录控制块的嵌套层
    pub(crate)  nested_layer: CFGBlockLayer,
}

impl CFGFun {
    pub fn new(name: MIRName) -> CFGFun {
        CFGFun {
            name,
            params: vec![],
            ret_type: MIRType::Void,
            linear_blocks: HashInsertVec::new(),
            pre_require_blocks: Default::default(),
            blocks_layer: CFGBlockLayer::new(),
            nested_layer: CFGBlockLayer::new(),
        }
    }

    pub fn push_block(&mut self, mut basic_block: CFGBasicBlock) -> CFGBasicBlockRef {
        let block_ref = self.linear_blocks.require_ele_id();
        basic_block.self_ref = Some(block_ref.clone());
        self.linear_blocks.push_with_ele_id(block_ref.clone(), basic_block);

        match self.nested_layer.last_layer_mut() {
            None => {
                self.blocks_layer.push_object(CFGLayerObject::Block(block_ref.clone()));
            }
            Some(_) => {
                self.nested_layer.push_object(CFGLayerObject::Block(block_ref.clone()));
            }
        }
        block_ref
    }

    pub fn get_block_label(&self, block_ref: &CFGBasicBlockRef) -> MIRLabel {
        if self.pre_require_blocks.contains_key(block_ref) {
            return self.pre_require_blocks[block_ref].block_label;
        }
        if self.linear_blocks.is_node_borrow(block_ref) {
            panic!("Node Borrowed")
        }
        let block = &(*self.linear_blocks.borrow_mut_uncheck(block_ref)).value;
        block.copy_name()
    }

    /// 把基本块和基本块索引添加到CFG里面，会替换基本块的label为ref的label
    pub fn push_block_with_ref(&mut self, block_ref: CFGBasicBlockRef, mut basic_block: CFGBasicBlock) {
        let mut pre_block = self.pre_require_blocks.remove(&block_ref).unwrap();
        basic_block.block_label = pre_block.block_label;
        basic_block.from_blocks.append(&mut pre_block.from_blocks);
        basic_block.to_blocks.append(&mut pre_block.to_blocks);
        basic_block.self_ref = Some(block_ref.clone());

        self.linear_blocks.push_with_ele_id(block_ref.clone(), basic_block);
        match self.nested_layer.last_layer_mut() {
            None => {
                self.blocks_layer.push_object(CFGLayerObject::Block(block_ref.clone()));
            }
            Some(_) => {
                self.nested_layer.push_object(CFGLayerObject::Block(block_ref.clone()));
            }
        }
    }
    pub fn new_layer(&mut self) {
        self.nested_layer.push_object(CFGLayerObject::Layer(Box::new(CFGBlockLayer::new())))
    }

    pub fn exit_layer(&mut self) {
        let last_layer = self.nested_layer.pop_object();
        let last = &mut self.nested_layer.last_layer_mut();
        match last {
            None => {
                // 已经退出了所有层，进入到了函数体
                self.blocks_layer.push_object(last_layer);
            }
            Some(_) => {
                self.nested_layer.push_object(last_layer);
            }
        }
    }
    pub fn connect_pre_to_block(&mut self, target_ref: &CFGBasicBlockRef, pre_block: CFGBasicBlockRef) {
        if self.pre_require_blocks.contains_key(target_ref) {
            let mut block = self.pre_require_blocks.remove(target_ref).unwrap();
            block.add_pre(pre_block);
            self.pre_require_blocks.insert(target_ref.clone(),block);
            return;
        }
        let mut target_block = self.linear_blocks.borrow_mut_uncheck(target_ref);
        target_block.value.add_pre(pre_block);
        drop(target_block);
    }

    pub fn connect_succ_to_block(&mut self, target_ref: &CFGBasicBlockRef, succ_block: CFGBasicBlockRef) {
        if self.pre_require_blocks.contains_key(target_ref) {
            let mut block = self.pre_require_blocks.remove(target_ref).unwrap();
            block.add_succ(succ_block);
            self.pre_require_blocks.insert(target_ref.clone(),block);
            return;
        }
        let mut target_block = self.linear_blocks.borrow_mut_uncheck(target_ref);
        target_block.value.add_succ(succ_block);
        drop(target_block);
    }

    pub fn link(&mut self, pre: CFGBasicBlockRef, succ: CFGBasicBlockRef) {
        self.connect_pre_to_block(&succ, pre.clone());
        self.connect_succ_to_block(&pre, succ);
    }

    pub fn pre_acquire_block_ref(&mut self, block_label: MIRLabel) -> CFGBasicBlockRef {
        let id = self.linear_blocks.require_ele_id();
        let cfg_bs = CFGBasicBlock::new(block_label);
        self.pre_require_blocks.insert(id.clone(), cfg_bs);
        id
    }

    pub fn add_var_param(&mut self, var: MIRVarRef) {
        self.params.push(MIRTypedParam::Var(var))
    }

    pub fn add_array_param(&mut self, array: MIRArrayRef) {
        self.params.push(MIRTypedParam::Array(array))
    }

    pub fn get_name(&self) -> &MIRName {
        &self.name
    }

    pub fn to_mir_fun(self) -> MIRFun {
        let mut mir_fun = MIRFun::new(self.name);
        mir_fun.params = self.params;
        match self.ret_type {
            MIRType::Int => {
                mir_fun.ret_type = Some(MIRType::Int);
            }
            MIRType::Float => {
                mir_fun.ret_type = Some(MIRType::Float);
            }
            MIRType::Bool => {
                mir_fun.ret_type = None;
            }
            MIRType::Void => {
                mir_fun.ret_type = None;
            }
        };
        for block in self.linear_blocks {
            let mir_block = block.to_mir_block();
            mir_fun.add_block(mir_block)
        }
        mir_fun
    }
}

pub type CFGBasicBlockRef = ElementRef<CFGBasicBlock>;

/// the Hash implementation is hash(mir_name) , for mir_name is global unique
pub struct CFGBasicBlock {
    /// 来源块
    pub(crate) from_blocks: VecDeque<CFGBasicBlockRef>,
    /// 前往块
    pub(crate) to_blocks: VecDeque<CFGBasicBlockRef>,
    pub(crate) block_label: MIRName,
    pub(crate) stmts: HashInsertVec<MIRStmt>,
    pub(crate) self_ref: Option<CFGBasicBlockRef>,
}

impl Hash for CFGBasicBlock {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.block_label.hash(state)
    }
}
impl PartialEq for CFGBasicBlock {
    fn eq(&self, other: &Self) -> bool {
        self.block_label == other.block_label
    }
}


impl CFGBasicBlock {
    pub fn new(block_label: MIRName) -> CFGBasicBlock {
        CFGBasicBlock {
            from_blocks: Default::default(),
            to_blocks: Default::default(),
            block_label,
            stmts: HashInsertVec::new(),
            self_ref: None,
        }
    }

    pub fn add_stmt(&mut self, stmt: MIRStmt) {
        self.stmts.push(stmt);
    }

    /// 从cfg unit 获得引用
    pub fn copy_name(&self) -> MIRName {
        self.block_label
    }

    /// 给该cfg unit 添加前节点
    pub fn add_pre(&mut self, cfg_unit: CFGBasicBlockRef) {
        self.from_blocks.push_back(cfg_unit);
    }

    /// 给该cfg unit 添加后节点
    pub fn add_succ(&mut self, cfg_unit: CFGBasicBlockRef) {
        self.to_blocks.push_back(cfg_unit);
    }

    pub fn to_mir_block(self) -> MIRBlock {
        let mut mir_block = MIRBlock::new(self.block_label);
        mir_block.stmts = self.stmts;
        mir_block
    }
}
