//! 核心上下文 & 函数上下文
//!
//! - CoreContext: 全局核心上下文，但是会带有一大堆构建完SSA就没用的东西
//! - FuncContext: 函数级上下文，包含了函数的所有信息
//!
//! 理论上这玩意用个Arc就能并行编译了，有谁去试试 :)

use super::cfg::*;
use super::domtree::*;
use super::function::*;
use super::globals::*;
use super::ir::*;
use super::loop_analysis::*;
// use super::mem_analysis::*;
use super::call_graph::*;
use super::scc::*;
use super::ssa_builder::*;
use super::stratify::*;
use crate::prelude::*;
use crate::ssa::egraph::egraph;

//==============================================================================
// 全局核心上下文
//==============================================================================

#[derive(Debug)]
pub struct CoreContext {
    /// 符号名管理 - 统一管理所有NameRef
    pub names: Names,
    /// 符号类型
    pub name_types: RefSecMap<NameRef, Type>,
    /// 外部函数信息
    pub ext_funcs: RefSparseMap<FuncRef, ExtFuncData>,
    /// 全局符号
    pub globals: RefSparseMap<Global, GlobalData>,
    /// 类型池
    pub type_dims: U32OptVecPool,

    // === SSA 构建相关 ===
    /// 核心构建器
    pub ssa_builder: SSABuilder,

    // === 构建时状态 ===
    /// 循环上下文栈 - (continue_block, break_block)
    pub loop_stack: Vec<(Block, Block)>,
    /// 当前块
    pub current_block: Option<Block>,
}

impl Default for CoreContext {
    fn default() -> Self {
        Self::new()
    }
}

impl CoreContext {
    /// 创建新的构建上下文
    pub fn new() -> Self {
        let mut names = RefMap::new();
        // 插入占位符到names，确保第一个值是0
        let placeholder_name: NameRef = names.intern_owned("###PLACEHOLDER###".to_string());
        assert_eq!(placeholder_name.index(), 1, "占位符名称的索引必须是0");

        Self {
            names: Names(names),
            name_types: RefSecMap::new(),
            ext_funcs: RefSparseMap::new(),
            globals: RefSparseMap::new(),
            type_dims: U32OptVecPool::new(),
            ssa_builder: SSABuilder::new(),
            loop_stack: Vec::new(),
            current_block: None,
            // memory_analyzer: MemoryAnalyzer::new(),
        }
    }

    /// 清理状态以便复用
    pub fn clear(&mut self) {
        // 只清理构建时状态，保留全局信息
        self.ssa_builder.clear();
        self.loop_stack.clear();
        self.current_block = None;
        // self.memory_analyzer.clear();
    }

    /// 检查是否为空
    pub fn is_empty(&self) -> bool {
        self.ssa_builder.is_empty() && self.loop_stack.is_empty() && self.current_block.is_none()
    }

    /// 从数组指针ptr的name_ref中获取token的name_ref
    pub fn name_ref_token_ref(&mut self, ptr_ref: NameRef) -> NameRef {
        let ptr_s = self.names.ref_name(ptr_ref).to_string();
        self.make_token_ref(ptr_s)
    }

    /// 从数组指针ptr的数组名构造并获取token的name_ref
    pub fn make_token_ref<S: AsRef<str>>(&mut self, s: S) -> NameRef {
        let token_name = format!("{}#mem", s.as_ref());
        self.name_ref_owned(token_name)
    }

    /// 获取或创建名称引用
    pub fn name_ref<S: AsRef<str>>(&mut self, s: S) -> NameRef {
        self.names.name_ref(s)
    }

    /// 获取或创建名称引用（拥有所有权）
    pub fn name_ref_owned(&mut self, name: String) -> NameRef {
        self.names.name_ref_owned(name)
    }

    /// 获取名称字符串
    pub fn ref_name(&self, name_ref: NameRef) -> &str {
        self.names.ref_name(name_ref)
    }

    /// 创建类型维度
    /// TODO: 激进使用 intern 进行去重
    pub fn make_type_dims(&mut self, vec: U32OptVecData) -> U32OptVec {
        self.type_dims.intern_owned(vec)
    }

    /// 获取类型维度数据
    pub fn get_type_dims(&self, vec: U32OptVec) -> &U32OptVecData {
        &self.type_dims[vec]
    }
}

//==============================================================================
// 函数上下文
//==============================================================================

#[derive(Debug)]
pub struct FuncContext {
    pub func: Function,
    pub cfg: ControlFlowGraph,
    pub domtree: DominatorTree,
    pub domtree_preorder: DominatorTreePreorder,
    pub loop_analysis: LoopAnalysis,
}

impl Default for FuncContext {
    fn default() -> Self {
        Self::new()
    }
}

impl FuncContext {
    pub fn new() -> Self {
        Self::from_function(Function::new())
    }

    pub fn from_function(function: Function) -> Self {
        Self {
            func: function,
            cfg: ControlFlowGraph::new(),
            domtree: DominatorTree::new(),
            domtree_preorder: DominatorTreePreorder::new(),
            loop_analysis: LoopAnalysis::new(),
        }
    }

    pub fn clear(&mut self) {
        self.cfg.clear();
        self.domtree.clear();
        self.domtree_preorder = DominatorTreePreorder::new();
        self.loop_analysis.clear();
    }

    pub fn compute_cfg(&mut self, _core_ctx: &mut CoreContext) -> CEResult<()> {
        self.cfg.compute(&self.func);
        Ok(())
    }

    pub fn compute_domtree(&mut self, _core_ctx: &mut CoreContext) -> CEResult<()> {
        self.domtree.compute(&self.func, &self.cfg);
        self.domtree_preorder.compute(&self.domtree, &self.func);
        Ok(())
    }

    pub fn compute_loop_analysis(&mut self, _core_ctx: &mut CoreContext) -> CEResult<()> {
        self.loop_analysis
            .compute(&self.func, &self.cfg, &self.domtree);
        Ok(())
    }

    /// 阶段1优化
    pub fn optimize_phase1(&mut self, core_ctx: &mut CoreContext) -> CEResult<()> {
        // 不可达代码消除
        self.intraproc_analyses(core_ctx)?;
        super::opts1::remove_unreach(&mut self.func, core_ctx, &mut self.cfg, &self.domtree)?;

        // 常量块参数移除 -- 可能真正的位置应该放在Opts2
        // self.intraproc_analyses(core_ctx)?;
        // super::opts1::remove_const_phi(&mut self.func, core_ctx, &self.cfg, &self.domtree)?;

        self.intraproc_analyses(core_ctx)?;
        egraph(self, core_ctx)?;
        Ok(())
    }

    /// 阶段2优化
    pub fn optimize_phase2(&mut self, core_ctx: &mut CoreContext) -> CEResult<()> {
        // 地址计算展开为低级计算
        self.intraproc_analyses(core_ctx)?;
        let expander = super::opts2::AddressExpander::new(&mut self.func, core_ctx, &self.domtree);
        expander.run();

        // 循环变量提取
        // self.intraproc_analyses(core_ctx)?;
        // super::opts2::licm(self, core_ctx)?;

        self.intraproc_analyses(core_ctx)?;
        egraph(self, core_ctx)?;
        Ok(())
    }

    /// 过程内分析
    pub fn intraproc_analyses(&mut self, core_ctx: &mut CoreContext) -> CEResult<()> {
        self.compute_cfg(core_ctx)?;
        self.compute_domtree(core_ctx)?;
        self.compute_loop_analysis(core_ctx)?;
        Ok(())
    }
}

//==============================================================================
// 用于过程间分析的函数上下文
//==============================================================================

#[derive(Debug)]
pub struct FuncContexts {
    pub ctxs: RefSparseMap<FuncRef, FuncContext>,
    // === 过程间分析 ===
    pub call_graph: CallGraph,
    pub sccs: Option<StrongConnecComps>,
    pub strata: Option<Strata>,
    // === 内存分析 ===
    // pub memory_analyzer: MemoryAnalyzer,
}

impl Default for FuncContexts {
    fn default() -> Self {
        Self::new()
    }
}

impl FuncContexts {
    pub fn new() -> Self {
        Self {
            ctxs: RefSparseMap::new(),
            call_graph: CallGraph::new(),
            sccs: None,
            strata: None,
        }
    }

    pub fn with_func_ctx(&mut self, func_ctx: FuncContext, core_ctx: &mut CoreContext) {
        let func_ref = core_ctx.name_ref(&func_ctx.func.name);
        self.ctxs.insert(func_ref, func_ctx);
    }

    pub fn optimize(&mut self, core_ctx: &mut CoreContext) -> CEResult<()> {
        // ==================== OPTIMIZATIONS 1 ========================
        // 过程内优化
        for (_func_ref, func_ctx) in self.ctxs.iter_mut() {
            func_ctx.optimize_phase1(core_ctx)?;
        }

        // 过程间：执行函数内联
        // self.interproc_analyses(core_ctx)?;
        // let mut inliner = super::opts1::Inliner::new(core_ctx, self);
        // inliner.run()?;
        self.recompute_all_analyses(core_ctx)?;

        // ==================== OPTIMIZATIONS 2 ========================
        // 过程内优化
        for (_func_ref, func_ctx) in self.ctxs.iter_mut() {
            func_ctx.optimize_phase2(core_ctx)?;
        }
        self.recompute_all_analyses(core_ctx)?;
        // for (_func_ref, func_ctx) in self.ctxs.iter_mut() {
        //     func_ctx.compute_loop_analysis(core_ctx)?;
        //     func_ctx.loop_analysis.dump();
        // }

        Ok(())
    }

    /// 重新计算所有分析信息
    pub fn recompute_all_analyses(&mut self, core_ctx: &mut CoreContext) -> CEResult<()> {
        // 过程中分析
        for (_func_ref, func_ctx) in self.ctxs.iter_mut() {
            func_ctx.intraproc_analyses(core_ctx)?;
        }
        // 过程间分析
        self.interproc_analyses(core_ctx)?;
        Ok(())
    }

    /// 过程间分析
    pub fn interproc_analyses(&mut self, core_ctx: &mut CoreContext) -> CEResult<()> {
        // 构建调用图
        self.call_graph = CallGraph::build_from(self)?;

        // 查找强连通分量
        let sccs = StrongConnecComps::find(&self.call_graph);

        // 构建凝聚图并进行分层
        let scc_graph = sccs.build_condensation(&self.call_graph);
        let strata = Strata::from_sccs(&sccs, &scc_graph);

        // 打印统计信息
        // if std::env::var("RUST_LOG")
        //     .unwrap_or_default()
        //     .contains("debug")
        // {
        //     self.call_graph.print_stats(core_ctx);
        //     sccs.print_stats(core_ctx, &self.call_graph);
        //     strata.print_stats(&sccs, core_ctx);
        // }

        self.sccs = Some(sccs);
        self.strata = Some(strata);
        Ok(())
    }

    pub fn iter(&self) -> impl Iterator<Item = &FuncContext> {
        self.ctxs.values()
    }

    pub fn iter_mut(&mut self) -> impl Iterator<Item = &mut FuncContext> {
        self.ctxs.values_mut()
    }
}

impl<'a> IntoIterator for &'a FuncContexts {
    type Item = &'a FuncContext;
    type IntoIter = Box<dyn Iterator<Item = &'a FuncContext> + 'a>;

    fn into_iter(self) -> Self::IntoIter {
        Box::new(self.ctxs.values())
    }
}
