//! Call Graph
//!
//! 使用密集存储的表示方式，所有边存储在一个连续的向量中，
//! 每个函数通过Range索引其调用的函数列表。

use super::ctx::*;
use super::function::*;
use super::ir::*;
use crate::prelude::*;
use std::ops::Range;

/// Call Graph
#[derive(Debug)]
pub struct CallGraph {
    /// 从每个函数到其调用边的映射
    /// Range指向edge_elems中的子切片
    pub(super) edges: RefSecMap<FuncRef, Range<u32>>,

    /// 密集存储的边元素
    pub(super) edge_elems: Vec<FuncRef>,
}

impl Default for CallGraph {
    fn default() -> Self {
        Self::new()
    }
}

impl CallGraph {
    /// 创建空的调用图
    pub fn new() -> Self {
        Self {
            edges: RefSecMap::new(),
            edge_elems: Vec::new(),
        }
    }

    /// 构建调用图
    ///
    /// 遍历所有函数，收集其调用关系
    pub fn build_from(func_ctxs: &FuncContexts) -> CEResult<Self> {
        let mut graph = CallGraph::new();
        let mut calls = Vec::new();

        // 遍历所有函数
        for (caller_ref, func_ctx) in func_ctxs.ctxs.iter() {
            calls.clear();

            // 收集该函数的所有调用（只包含内部函数）
            Self::collect_calls(&func_ctx.func, &mut calls, func_ctxs)?;

            // 存储调用边
            let range = graph.extend_with_range(calls.drain(..));
            graph.edges[caller_ref] = range;
        }

        Ok(graph)
    }

    /// 收集函数中的所有调用（只包含内部函数）
    fn collect_calls(
        func: &Function,
        calls: &mut Vec<FuncRef>,
        func_ctxs: &FuncContexts,
    ) -> CEResult<()> {
        // 遍历所有指令
        for (_inst, inst_data) in func.dfg.insts.0.iter() {
            match inst_data {
                // 普通函数调用
                InstructionData::Call { func: callee, .. } => {
                    // 只收集内部函数的调用，过滤掉外部函数
                    if func_ctxs.ctxs.contains_key(*callee) {
                        calls.push(*callee);
                    }
                }
                // 尾调用
                InstructionData::ReturnCall { func: callee, .. } => {
                    // 只收集内部函数的调用，过滤掉外部函数
                    if func_ctxs.ctxs.contains_key(*callee) {
                        calls.push(*callee);
                    }
                }
                _ => {}
            }
        }

        Ok(())
    }

    /// 将元素添加到edge_elems并返回范围
    fn extend_with_range(&mut self, items: impl IntoIterator<Item = FuncRef>) -> Range<u32> {
        let start = self.edge_elems.len();
        let start = u32::try_from(start).unwrap();

        self.edge_elems.extend(items);

        let end = self.edge_elems.len();
        let end = u32::try_from(end).unwrap();

        start..end
    }

    /// 获取调用图中的所有函数节点
    pub fn nodes(&self) -> impl Iterator<Item = FuncRef> + '_ {
        self.edges.keys()
    }

    /// 获取指定函数调用的所有函数
    pub fn callees(&self, caller: FuncRef) -> &[FuncRef] {
        let Range { start, end } = self.edges[caller].clone();
        let start = usize::try_from(start).unwrap();
        let end = usize::try_from(end).unwrap();
        &self.edge_elems[start..end]
    }

    /// 获取指定函数的出度（调用的函数数量）
    pub fn out_degree(&self, func: FuncRef) -> usize {
        self.callees(func).len()
    }

    /// 判断函数是否是叶子节点（不调用其他函数）
    pub fn is_leaf(&self, func: FuncRef) -> bool {
        self.out_degree(func) == 0
    }

    /// 判断函数f是否调用函数g
    pub fn has_edge(&self, f: FuncRef, g: FuncRef) -> bool {
        self.callees(f).contains(&g)
    }

    /// 获取调用图的总边数
    pub fn edge_count(&self) -> usize {
        self.edge_elems.len()
    }

    /// 获取调用图的节点数
    pub fn node_count(&self) -> usize {
        self.edges.len()
    }

    /// 打印调用图的统计信息
    pub fn print_stats(&self, core_ctx: &CoreContext) {
        eprintln!("=== Call Graph Statistics ===");
        eprintln!("Functions: {}", self.node_count());
        eprintln!("Call edges: {}", self.edge_count());

        // 打印每个函数的调用信息
        for func_ref in self.nodes() {
            let func_name = &core_ctx.names[func_ref];
            let callees = self.callees(func_ref);
            if !callees.is_empty() {
                eprintln!("  {} calls:", func_name);
                for callee in callees {
                    let callee_name = &core_ctx.names[*callee];
                    eprintln!("    -> {}", callee_name);
                }
            }
        }
    }

    /// 获取所有函数的列表
    pub fn all_functions(&self) -> Vec<FuncRef> {
        self.nodes().collect()
    }

    /// 遍历调用图的所有边
    pub fn edges(&self) -> impl Iterator<Item = (FuncRef, FuncRef)> + '_ {
        self.edges.iter().flat_map(move |(caller, range)| {
            let start = usize::try_from(range.start).unwrap();
            let end = usize::try_from(range.end).unwrap();
            self.edge_elems[start..end]
                .iter()
                .map(move |&callee| (caller, callee))
        })
    }

    /// 构建反向调用图
    pub fn build_reverse_graph(&self) -> CallGraph {
        let mut reverse = CallGraph::new();
        let mut callers: RefSparseMap<FuncRef, Vec<FuncRef>> = RefSparseMap::new();

        // 收集所有反向边
        for (caller, callee) in self.edges() {
            callers.get_or_insert_with(callee, Vec::new).push(caller);
        }

        // 构建反向图
        for (callee, caller_list) in callers.iter() {
            let mut sorted_callers = caller_list.clone();
            sorted_callers.sort();
            sorted_callers.dedup();
            let range = reverse.extend_with_range(sorted_callers.into_iter());
            reverse.edges[callee] = range;
        }

        // 确保所有节点都在反向图中（即使没有调用者）
        for func in self.nodes() {
            if !reverse.edges.contains_key(func) {
                reverse.edges[func] = 0..0;
            }
        }

        reverse
    }

    /// 获取调用某个函数的所有函数（需要先构建反向图）
    pub fn callers(&self, callee: FuncRef) -> &[FuncRef] {
        // 注意：这个方法假设self是反向图
        self.callees(callee)
    }

    /// 清空调用图
    pub fn clear(&mut self) {
        self.edges.clear();
        self.edge_elems.clear();
    }

    /// 检测函数是否显式自我递归
    pub fn is_self_recursive(&self, func: FuncRef) -> bool {
        self.has_edge(func, func)
    }

    /// 获取所有显式自我递归函数
    pub fn recursive_functions(&self) -> Vec<FuncRef> {
        self.nodes()
            .filter(|&func| self.is_self_recursive(func))
            .collect()
    }

    /// 获取图的稀疏度 -- 边数 / 可能的最大边数
    pub fn sparsity(&self) -> f64 {
        let n = self.node_count() as f64;
        if n <= 1.0 {
            return 0.0;
        }
        let edges = self.edge_count() as f64;
        let max_edges = n * n; // 包括自环
        edges / max_edges
    }

    /// 获取入度为0的函数（没有被调用的函数，通常是入口函数）
    pub fn entry_functions(&self) -> Vec<FuncRef> {
        let reverse = self.build_reverse_graph();
        reverse
            .nodes()
            .filter(|&func| reverse.out_degree(func) == 0)
            .collect()
    }

    /// 获取调用图的最大出度
    pub fn max_out_degree(&self) -> usize {
        self.nodes()
            .map(|func| self.out_degree(func))
            .max()
            .unwrap_or(0)
    }

    /// 当一个函数被内联后，更新调用图
    ///
    /// caller 内联了 callee，那么 caller 的调用会增加 callee 的所有调用，
    /// 同时 caller 到 callee 的调用边需要被移除。
    pub fn update_for_inlining(&mut self, caller: FuncRef, callee: FuncRef) {
        // 获取 callee 调用的所有函数
        let callee_callees = self.callees(callee).to_vec();

        // 获取 caller 当前调用的所有函数
        let mut caller_callees: std::collections::HashSet<FuncRef> =
            self.callees(caller).iter().cloned().collect();

        // 移除 caller -> callee 的边
        caller_callees.remove(&callee);

        // 添加 callee 的所有调用到 caller
        for new_callee in callee_callees {
            caller_callees.insert(new_callee);
        }

        // 更新 caller 的边
        let new_edges: Vec<FuncRef> = caller_callees.into_iter().collect();
        let range = self.extend_with_range(new_edges);
        self.edges[caller] = range;
    }
}
