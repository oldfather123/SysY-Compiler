//! SSA Builder 核心算法
//!
//! 基于 Braun M. 的 Simple and Efficient Construction of Static Single Assignment Form. 算法
//! <https://link.springer.com/content/pdf/10.1007/978-3-642-37051-9_6.pdf>

use super::function::*;
use super::ir::*;
use crate::prelude::*;
use std::collections::HashSet;
use std::mem;

/// SSA 构造器
#[derive(Debug)]
pub struct SSABuilder {
    /// 记录每个变量在每个块里的最新值
    variables: NestedRefSecMap<NameRef, Block, Option<Value>>,
    /// 块的构造辅助信息
    ssa_blocks: RefSecMap<Block, SSABlockData>,
    /// 防止循环引用的访问集合
    visited: HashSet<Block>,
    /// 状态机任务栈
    tasks: Vec<Task>,
    /// 状态机结果栈
    results: Vec<Value>,
}

/// 状态机任务
#[derive(Debug)]
enum Task {
    /// 使用变量 - 参数为产生跳转的指令
    UseVar(Inst),
    /// 完成前驱查找
    FinishPredLookup(Value, Block),
}

/// 块的构造时辅助信息
#[derive(Debug, Clone, Default)]
struct SSABlockData {
    /// 前驱指令列表
    preds: Vec<Inst>,
    /// 密封状态
    sealed: Sealed,
    /// 单前驱优化
    single_pred: Option<Block>,
}

/// 块的密封状态
#[derive(Debug, Clone)]
enum Sealed {
    /// 未密封 - 包含未定义变量列表
    No { undef_vars: Vec<NameRef> },
    /// 已密封
    Yes,
}

impl Default for Sealed {
    fn default() -> Self {
        Sealed::No {
            undef_vars: Vec::new(),
        }
    }
}

impl Default for SSABuilder {
    fn default() -> Self {
        Self::new()
    }
}

impl SSABuilder {
    pub fn new() -> Self {
        Self {
            variables: NestedRefSecMap::new(),
            ssa_blocks: RefSecMap::new(),
            visited: HashSet::new(),
            tasks: Vec::new(),
            results: Vec::new(),
        }
    }

    pub fn clear(&mut self) {
        self.variables.clear();
        self.ssa_blocks.clear();
        self.visited.clear();
        self.tasks.clear();
        self.results.clear();
    }

    pub fn is_empty(&self) -> bool {
        self.variables.is_empty()
            && self.ssa_blocks.is_empty()
            && self.visited.is_empty()
            && self.tasks.is_empty()
            && self.results.is_empty()
    }

    //==============================================================================
    // 块操作
    //==============================================================================

    /// 获取块的前驱指令列表
    fn predecessors(&self, block: Block) -> &[Inst] {
        self.ssa_blocks[block].preds.as_slice()
    }

    pub fn has_any_predecessors(&self, block: Block) -> bool {
        !self.predecessors(block).is_empty()
    }

    pub fn is_sealed(&self, block: Block) -> bool {
        matches!(self.ssa_blocks[block].sealed, Sealed::Yes)
    }

    /// 声明一个新块
    pub fn declare_block(&mut self, block: Block) {
        let _ = &mut self.ssa_blocks[block];
    }

    /// 声明块的前驱
    pub fn declare_block_predecessor(&mut self, block: Block, inst: Inst) {
        self.ssa_blocks[block].preds.push(inst);
    }

    /// 移除块的前驱
    pub fn remove_block_predecessor(&mut self, block: Block, inst: Inst) {
        let data = &mut self.ssa_blocks[block];
        let pred = data
            .preds
            .as_slice()
            .iter()
            .position(|&branch| branch == inst)
            .expect("要移除的前驱未声明");
        data.preds.swap_remove(pred);
    }

    /// 密封一个块 - 所有前驱均已知
    pub fn seal_block(&mut self, block: Block, func: &mut Function) {
        self.seal_one_block(block, func);
    }

    /// 密封一个块 - 所有前驱均已知
    fn seal_one_block(&mut self, block: Block, func: &mut Function) {
        // 获取未定义变量列表
        let undef_vars = match mem::replace(&mut self.ssa_blocks[block].sealed, Sealed::Yes) {
            Sealed::No { undef_vars } => undef_vars,
            Sealed::Yes => return, // 已密封
        };
        let ssa_params = undef_vars.len();

        let preds = self.predecessors(block);

        // 单前驱优化
        if preds.len() == 1 {
            let pred = func.inst_block(preds[0]);
            self.ssa_blocks[block].single_pred = Some(pred);
        }

        for idx in 0..ssa_params {
            let var = undef_vars[idx];
            let block_params = func.dfg.block_params(block);
            let val = block_params[block_params.len() - (ssa_params - idx)];
            // 清空状态机栈
            debug_assert!(self.tasks.is_empty());
            debug_assert!(self.results.is_empty());

            // 开始前驱查找
            self.begin_pred_lookup(val, block);
            // 运行状态机
            self.run_state_machine(func, var, func.dfg.value_type(val));
        }
    }

    //==============================================================================
    // 变量定义与使用
    //==============================================================================

    /// 定义变量
    pub fn def_var(&mut self, var: NameRef, val: Value, block: Block) {
        // 现在 NestedRefSecMap 支持在index_mut的时候自动创建
        // 所以不需要手动检查是否存在
        self.variables[var][block] = Some(val);
    }

    /// 检查变量在指定块是否已定义
    pub fn var_defined_in_block(&self, var: NameRef, block: Block) -> Option<Value> {
        self.variables.get_or_default(var, block)
    }

    /// 使用变量
    pub fn use_var(&mut self, func: &mut Function, block: Block, var: NameRef, ty: Type) -> Value {
        debug_assert!(self.tasks.is_empty());
        debug_assert!(self.results.is_empty());

        // 准备状态机 - 设置初始的查找任务
        self.use_var_nonlocal(func, block, var, ty);

        // 运行状态机

        self.run_state_machine(func, var, ty)
    }

    /// 非本地变量使用 - 填入查找任务
    fn use_var_nonlocal(&mut self, func: &mut Function, mut block: Block, var: NameRef, ty: Type) {
        // 1. Local Value Numbering - 快速路径检查本地缓存
        if let Some(val) = self.variables.get_or_default(var, block) {
            self.results.push(val);
            return;
        }

        // 2. Global Value Numbering - 慢速路径深度查找
        let (val, from) = self.find_var(func, block, var, ty);

        // 3. 缓存填充：将找到的值传播并缓存到沿途所有单前驱路径的块中
        // 但要注意：只有当值不是块参数，或者块参数属于当前块时，才能安全缓存

        // 无需确保变量映射存在 - 因为 NestedRefSecMap 会自动创建
        let var_defs = &mut self.variables[var];

        // 对于单前驱链上的其他块，缓存块参数值
        // 它让一个值看起来好像“穿透”了多个块，但实际上是编译器在背后完成了值的传递
        // 值传播（核心步骤）： find_var 找到了定义 var 的值 val 来自于块 from。然后，while block != from 这个循环开始工作
        // 它从当前块 block 开始，沿着单一前驱的路径一路回溯到 from 块
        // 在这个路径上的每一个中间块，它都会执行 var_defs[block] = PackedOption::from(val)
        // 这行代码的含义是：“我知道 var 的值是 val，现在我把这个信息记录在所有从 from 到 block 的路径上的中间块里
        // 避免作用域违规
        while block != from {
            var_defs[block] = Some(val);
            block = self.ssa_blocks[block].single_pred.unwrap();
        }
    }

    /// 查找变量定义
    /// 1. 找不到就添加块参数并作为定义
    /// 2. 如果pred唯一且sealed，那么该块中的定义与前驱相同，无需块参数
    /// 3. 如果有循环，选择在循环入口处添加块参数
    fn find_var(
        &mut self,
        func: &mut Function,
        mut block: Block,
        var: NameRef,
        ty: Type,
    ) -> (Value, Block) {
        // 1. 尝试沿单前驱链向上查找（分两步避免借用冲突）
        self.visited.clear();
        let var_defs = &mut self.variables[var];
        while let Some(pred) = self.ssa_blocks[block].single_pred {
            if !self.visited.insert(block) {
                break;
            }
            block = pred;
            if let Some(val) = var_defs.get(block).copied().flatten() {
                self.results.push(val);
                return (val, block);
            }
        }

        // 2. 如果找不到，说明到达了一个合并点或未定义点，需要创建新的 SSA 值
        let val = func.dfg.append_block_param(block, ty);
        var_defs[block] = Some(val);

        // 3. 为新创建的块参数，安排对所有前驱的递归查找任务
        match &mut self.ssa_blocks[block].sealed {
            Sealed::Yes => self.begin_pred_lookup(val, block),
            Sealed::No { undef_vars } => {
                undef_vars.push(var);
                self.results.push(val);
            }
        }

        (val, block)
    }

    /// 开始前驱查找
    fn begin_pred_lookup(&mut self, sentinel: Value, dest_block: Block) {
        // 添加完成查找任务
        self.tasks
            .push(Task::FinishPredLookup(sentinel, dest_block));

        // 为每个前驱添加使用变量任务
        self.tasks.extend(
            self.ssa_blocks[dest_block]
                .preds
                .as_slice()
                .iter()
                .rev()
                .copied()
                .map(Task::UseVar),
        );
    }

    /// 完成前驱查找
    fn finish_pred_lookup(
        &mut self,
        func: &mut Function,
        sentinel: Value,
        dest_block: Block,
    ) -> Value {
        let num_preds = self.predecessors(dest_block).len();

        // 从结果栈中取出所有前驱的值
        let results = self.results.drain(self.results.len() - num_preds..);

        let pred_val = {
            let mut iter = results
                .as_slice()
                .iter()
                .map(|&val| func.dfg.resolve_aliases(val))
                .filter(|&val| val != sentinel);
            if let Some(val) = iter.next() {
                if iter.all(|other| other == val) {
                    Some(val)
                } else {
                    None
                }
            } else {
                // 变量从未定义，直接panic
                panic!(
                    "Error in block {:?}: A variable was used without being defined in any predecessor path. This often indicates an issue in unreachable code.",
                    dest_block
                );
            }
        };

        if let Some(pred_val) = pred_val {
            func.dfg.remove_block_param(sentinel);
            func.dfg.change_to_alias(sentinel, pred_val);
            pred_val
        } else {
            let preds = &self.ssa_blocks[dest_block].preds;
            let dfg = &mut func.dfg;
            for (idx, &val) in results.as_slice().iter().enumerate() {
                let pred = preds[idx];
                let branch = pred;

                let dests = dfg.insts[branch].branch_destination_mut(&mut dfg.jump_tables);
                for block in dests {
                    if block.block() == dest_block {
                        block.append_argument(val, &mut dfg.value_vecs);
                    }
                }
            }
            sentinel
        }
    }
    /// 运行状态机
    fn run_state_machine(&mut self, func: &mut Function, var: NameRef, ty: Type) -> Value {
        while let Some(task) = self.tasks.pop() {
            match task {
                Task::UseVar(branch) => {
                    let block = func.inst_block(branch);
                    // 直接调用 use_var_nonlocal 将值推入结果栈
                    self.use_var_nonlocal(func, block, var, ty);
                }
                Task::FinishPredLookup(sentinel, dest_block) => {
                    let val = self.finish_pred_lookup(func, sentinel, dest_block);
                    self.results.push(val);
                }
            }
        }
        self.results.pop().unwrap()
    }
}
