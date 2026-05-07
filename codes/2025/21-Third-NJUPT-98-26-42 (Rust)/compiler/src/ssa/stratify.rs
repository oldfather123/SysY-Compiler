//! SCC凝聚图的分层和拓扑排序 -- 用于自底向上的函数内联
//!
//! 该模块将SCC凝聚图转换为分层结构，用于确定函数内联的顺序
//! 分层结构是一个并行执行计划：
//! - 每一层包含可以并行处理的SCC
//! - 第i层必须在第i+1层之前处理
//! - 同一层内的SCC可以按任意顺序甚至并行处理
//!
//! 对于如下的调用图：
//! ```text
//! +---+   +---+   +---+
//! | a |-->| b |-->| c |
//! +---+   +---+   +---+
//!   |       |
//!   |       |     +---+
//!   |       '---->| d |
//!   |             +---+
//!   |
//!   |     +---+   +---+
//!   '---->| e |-->| f |
//!         +---+   +---+
//!           |
//!           |     +---+
//!           '---->| g |
//!                 +---+
//! ```
//!
//! 分层结果为：
//! ```text
//! [
//!     {c, d, f, g},  // 叶子节点，可以并行处理
//!     {b, e},        // 依赖第一层的节点
//!     {a},           // 根节点
//! ]
//! ```

use super::ctx::CoreContext;
use super::scc::{SccGraph, StrongConnecComps};
use crate::prelude::*;
use std::ops::Range;

/// SCC的分层结构
/// 用于自底向上的函数内联
#[derive(Debug)]
pub struct Strata {
    /// 每一层的范围 -- 在layer_elems中的索引
    layers: Vec<Range<u32>>,

    /// 存储所有层的SCC
    layer_elems: Vec<Scc>,
}

impl Strata {
    /// 从SCC和凝聚图创建分层结构
    pub fn from_sccs(_sccs: &StrongConnecComps, scc_graph: &SccGraph) -> Self {
        // 计算每个SCC的入度 -- 未满足的依赖数
        let mut in_degree = RefSecMap::<Scc, u32>::new();

        // 初始化所有SCC的入度
        for scc in scc_graph.nodes.iter() {
            in_degree[*scc] = 0;
        }

        // 计算入度
        for from_scc in scc_graph.nodes.iter() {
            for to_scc in scc_graph.successors(*from_scc) {
                in_degree[*to_scc] += 1;
            }
        }

        // 构建分层
        let mut layers = Vec::new();
        let mut layer_elems = Vec::new();

        // 找到所有入度为0的节点作为第一层
        let mut current_layer: Vec<Scc> = scc_graph
            .nodes
            .iter()
            .filter(|&&scc| in_degree[scc] == 0)
            .copied()
            .collect();

        let mut next_layer = Vec::new();

        while !current_layer.is_empty() {
            // 对于当前层的每个节点，减少其后继的入度
            for &scc in &current_layer {
                for &successor in scc_graph.successors(scc) {
                    debug_assert!(in_degree[successor] > 0);
                    in_degree[successor] -= 1;

                    // 如果入度变为0，加入下一层
                    if in_degree[successor] == 0 {
                        next_layer.push(successor);
                    }
                }
            }

            // 将当前层添加到结果中
            let range = Self::extend_with_range(&mut layer_elems, current_layer.drain(..));
            layers.push(range);

            // 交换当前层和下一层
            std::mem::swap(&mut current_layer, &mut next_layer);
        }

        // 验证所有节点都已处理
        debug_assert!(
            in_degree.values().all(|&deg| deg == 0),
            "所有SCC的入度应该都为0"
        );

        // 注意：反转层次以得到逆拓扑序 -- 叶子节点在前，根节点在后
        // 这是自底向上内联所需要的顺序
        layers.reverse();

        // 重新构建 layer_elems 以匹配反转后的层次
        let mut new_layer_elems = Vec::with_capacity(layer_elems.len());
        let mut new_layers = Vec::with_capacity(layers.len());

        // 注意：这里不要再 rev() 了，layers 已经是反转后的顺序
        for layer_range in layers.iter() {
            let start = layer_range.start as usize;
            let end = layer_range.end as usize;
            let layer_slice = &layer_elems[start..end];

            let new_range =
                Self::extend_with_range(&mut new_layer_elems, layer_slice.iter().copied());
            new_layers.push(new_range);
        }

        Strata {
            layers: new_layers,
            layer_elems: new_layer_elems,
        }
    }

    /// 辅助函数：将元素添加到向量并返回范围
    fn extend_with_range(dest: &mut Vec<Scc>, items: impl IntoIterator<Item = Scc>) -> Range<u32> {
        let start = dest.len() as u32;
        dest.extend(items);
        let end = dest.len() as u32;
        start..end
    }

    /// 获取层数
    pub fn num_layers(&self) -> usize {
        self.layers.len()
    }

    /// 迭代所有层
    ///
    /// 返回的迭代器按照处理顺序返回每一层的SCC列表
    pub fn layers(&self) -> impl ExactSizeIterator<Item = &[Scc]> {
        self.layers.iter().map(|range| {
            let start = range.start as usize;
            let end = range.end as usize;
            &self.layer_elems[start..end]
        })
    }

    /// 获取指定层的SCC
    pub fn layer(&self, index: usize) -> Option<&[Scc]> {
        self.layers.get(index).map(|range| {
            let start = range.start as usize;
            let end = range.end as usize;
            &self.layer_elems[start..end]
        })
    }

    /// 获取逆拓扑序的SCC列表（用于内联顺序）
    ///
    /// 返回的顺序是：叶子节点在前，根节点在后
    pub fn reverse_topological_order(&self) -> Vec<Scc> {
        self.layer_elems.clone()
    }

    /// 获取拓扑序的SCC列表
    ///
    /// 返回的顺序是：根节点在前，叶子节点在后
    pub fn topological_order(&self) -> Vec<Scc> {
        let mut result = self.layer_elems.clone();
        result.reverse();
        result
    }

    /// 打印分层统计信息
    pub fn print_stats(&self, sccs: &StrongConnecComps, core_ctx: &CoreContext) {
        eprintln!("=== Stratification Statistics ===");
        eprintln!("Total layers: {}", self.num_layers());

        for (i, layer) in self.layers().enumerate() {
            eprintln!("Layer {} ({} SCCs):", i, layer.len());

            for &scc in layer {
                let nodes = sccs.nodes(scc);
                if nodes.len() == 1 {
                    eprintln!("  - SCC: {}", &core_ctx.names[nodes[0]]);
                } else {
                    eprintln!("  - Mutually recursive SCC ({} functions):", nodes.len());
                    for &func in nodes {
                        eprintln!("      {}", &core_ctx.names[func]);
                    }
                }
            }
        }

        eprintln!("\nInlining order (reverse topological):");
        for &scc in &self.reverse_topological_order() {
            let nodes = sccs.nodes(scc);
            if nodes.len() == 1 {
                eprintln!("  {}", &core_ctx.names[nodes[0]]);
            } else {
                eprintln!("  Mutually recursive SCC:");
                for &func in nodes {
                    eprintln!("    {}", &core_ctx.names[func]);
                }
            }
        }
    }

    /// 确定哪些SCC可以被内联
    ///
    /// 返回一个映射，表示每个SCC是否可以被内联
    /// 根据激进策略：只有递归的SCC不能被内联
    pub fn compute_inlinable_sccs(
        &self,
        sccs: &StrongConnecComps,
        call_graph: &super::call_graph::CallGraph,
    ) -> RefSparseMap<Scc, bool> {
        let mut inlinable = RefSparseMap::new();

        for &scc in &self.layer_elems {
            // 只有非递归的SCC才能被内联
            let can_inline = !sccs.is_recursive(scc, call_graph);
            inlinable.insert(scc, can_inline);
        }

        inlinable
    }
}
