//! Tarjan算法找强连通分量SCC
//!
use super::call_graph::CallGraph;
use super::ctx::CoreContext;
use super::ir::FuncRef;
use crate::prelude::*;
use std::ops::Range;

/// 强连通分量集合
#[derive(Debug)]
pub struct StrongConnecComps {
    /// 从SCC到component_nodes中对应节点范围的映射
    components: RefMap<Scc, Range<u32>>,

    /// 存储各个SCC包含的函数节点
    component_nodes: Vec<FuncRef>,
}

impl StrongConnecComps {
    /// 寻找强连通分量
    pub fn find(call_graph: &CallGraph) -> Self {
        let scc_finder = TarjanSccFinder::new();
        scc_finder.find_sccs(call_graph)
    }

    /// 获取强连通分量的数量
    pub fn len(&self) -> usize {
        self.components.len()
    }

    /// 检查是否为空
    pub fn is_empty(&self) -> bool {
        self.components.is_empty()
    }

    /// 获取指定SCC包含的函数节点
    pub fn nodes(&self, scc: Scc) -> &[FuncRef] {
        let range = &self.components[scc];
        let start = range.start as usize;
        let end = range.end as usize;
        &self.component_nodes[start..end]
    }

    /// 迭代所有SCC及其包含的节点
    ///
    /// 迭代顺序为逆拓扑序（后继节点先于前驱节点）
    pub fn iter(&self) -> impl Iterator<Item = (Scc, &[FuncRef])> {
        self.components.iter().map(|(scc, range)| {
            let start = range.start as usize;
            let end = range.end as usize;
            (scc, &self.component_nodes[start..end])
        })
    }

    /// 获取所有SCC的迭代器
    pub fn sccs(&self) -> impl Iterator<Item = Scc> + '_ {
        self.components.keys()
    }

    /// 判断指定的SCC是否只包含一个节点
    pub fn is_singleton(&self, scc: Scc) -> bool {
        let range = &self.components[scc];
        range.end - range.start == 1
    }

    /// 判断指定的SCC是否是递归的（包含自调用）
    pub fn is_recursive(&self, scc: Scc, call_graph: &CallGraph) -> bool {
        let nodes = self.nodes(scc);

        // 单节点SCC：检查是否自调用
        if nodes.len() == 1 {
            call_graph.is_self_recursive(nodes[0])
        } else {
            // 多节点SCC总是递归的（相互递归）
            true
        }
    }

    /// 获取每个函数所属的SCC
    pub fn node_to_scc(&self) -> RefSparseMap<FuncRef, Scc> {
        let mut mapping = RefSparseMap::new();
        for (scc, nodes) in self.iter() {
            for &node in nodes {
                mapping.insert(node, scc);
            }
        }
        mapping
    }

    /// 构建SCC的凝聚图 -- 每个SCC作为一个大节点构成的DAG
    pub fn build_condensation(&self, call_graph: &CallGraph) -> SccGraph {
        let node_to_scc = self.node_to_scc();
        let mut scc_edges: RefSparseMap<Scc, Vec<Scc>> = RefSparseMap::new();

        // 收集SCC之间的边
        for (from_scc, from_nodes) in self.iter() {
            let mut to_sccs = Vec::new();

            for &from_node in from_nodes {
                for &to_node in call_graph.callees(from_node) {
                    if let Some(&to_scc) = node_to_scc.get(to_node) {
                        if from_scc != to_scc && !to_sccs.contains(&to_scc) {
                            to_sccs.push(to_scc);
                        }
                    }
                }
            }

            if !to_sccs.is_empty() {
                scc_edges.insert(from_scc, to_sccs);
            }
        }
        SccGraph::from_edges(self.sccs().collect(), scc_edges)
    }

    /// 打印SCC统计信息
    pub fn print_stats(&self, core_ctx: &CoreContext, call_graph: &CallGraph) {
        eprintln!("=== SCC Statistics ===");
        eprintln!("Total SCCs: {}", self.len());

        let mut singleton_count = 0;
        let mut non_recursive_singleton_count = 0;
        let mut recursive_count = 0;

        for (scc, nodes) in self.iter() {
            if nodes.len() == 1 {
                singleton_count += 1;
                if call_graph.is_self_recursive(nodes[0]) {
                    recursive_count += 1;
                    eprintln!("  Self-recursive function: {}", &core_ctx.names[nodes[0]]);
                } else {
                    non_recursive_singleton_count += 1;
                }
            } else {
                recursive_count += 1;
                eprintln!("  Mutually recursive SCC with {} functions:", nodes.len());
                for &func in nodes {
                    eprintln!("    - {}", &core_ctx.names[func]);
                }
            }
        }

        eprintln!("Total singleton SCCs: {}", singleton_count);
        eprintln!("  - Non-recursive: {}", non_recursive_singleton_count);
        eprintln!(
            "  - Self-recursive: {}",
            singleton_count - non_recursive_singleton_count
        );
        eprintln!(
            "Mutually recursive SCCs: {}",
            recursive_count - (singleton_count - non_recursive_singleton_count)
        );
    }

    /// 克隆所有SCC的节点
    pub fn all_nodes_cloned(&self) -> RefSecMap<Scc, Vec<FuncRef>> {
        let mut result = RefSecMap::new();
        for (scc, nodes) in self.iter() {
            result.insert(scc, nodes.to_vec());
        }
        result
    }
}

/// Tarjan迭代
struct TarjanSccFinder {
    /// DFS索引计数器
    index_counter: u32,

    /// 每个节点的DFS索引
    indices: RefSparseMap<FuncRef, u32>,

    /// 每个节点的lowlink值（能到达的最小索引）
    lowlinks: RefSparseMap<FuncRef, u32>,

    /// 正在处理的节点栈
    stack: Vec<FuncRef>,

    /// 标记节点是否在栈中
    on_stack: RefSparseMap<FuncRef, bool>,

    /// 收集到的SCC
    components: RefMap<Scc, Range<u32>>,
    component_nodes: Vec<FuncRef>,
}

impl TarjanSccFinder {
    fn new() -> Self {
        Self {
            index_counter: 0,
            indices: RefSparseMap::new(),
            lowlinks: RefSparseMap::new(),
            stack: Vec::new(),
            on_stack: RefSparseMap::new(),
            components: RefMap::new(),
            component_nodes: Vec::new(),
        }
    }

    fn find_sccs(mut self, call_graph: &CallGraph) -> StrongConnecComps {
        let mut dfs_stack: Vec<DfsFrame> = Vec::new();

        for func in call_graph.nodes() {
            if !self.indices.contains_key(func) {
                dfs_stack.push(DfsFrame::new(func));
                self.iterative_dfs(&mut dfs_stack, call_graph);
            }
        }

        StrongConnecComps {
            components: self.components,
            component_nodes: self.component_nodes,
        }
    }

    fn iterative_dfs(&mut self, dfs_stack: &mut Vec<DfsFrame>, call_graph: &CallGraph) {
        while let Some(mut frame) = dfs_stack.pop() {
            match frame.state {
                DfsState::FirstVisit => {
                    // 第一次访问节点
                    let v = frame.node;

                    self.indices.insert(v, self.index_counter);
                    self.lowlinks.insert(v, self.index_counter);
                    self.index_counter += 1;

                    self.stack.push(v);
                    self.on_stack.insert(v, true);

                    // 准备访问后继节点
                    frame.state = DfsState::VisitingSuccessors;
                    frame.successors = call_graph.callees(v).to_vec();
                    dfs_stack.push(frame);
                }

                DfsState::VisitingSuccessors => {
                    let v = frame.node;

                    if let Some(w) = frame.successors.pop() {
                        // 还有后继节点要访问
                        dfs_stack.push(frame);

                        if !self.indices.contains_key(w) {
                            // 未访问过的节点
                            dfs_stack.push(DfsFrame::new(w));
                        } else if self.on_stack.get(w).copied().unwrap_or(false) {
                            // w在栈中，更新v的lowlink
                            let v_lowlink = self.lowlinks[v];
                            let w_index = self.indices[w];
                            self.lowlinks.insert(v, v_lowlink.min(w_index));
                        }
                    } else {
                        // 所有后继节点访问完毕
                        frame.state = DfsState::PostVisit;
                        dfs_stack.push(frame);
                    }
                }

                DfsState::PostVisit => {
                    let v = frame.node;

                    // 更新前驱的lowlink
                    if let Some(parent_frame) = dfs_stack.last() {
                        if parent_frame.state == DfsState::VisitingSuccessors {
                            let parent = parent_frame.node;
                            let parent_lowlink = self.lowlinks[parent];
                            let v_lowlink = self.lowlinks[v];
                            self.lowlinks.insert(parent, parent_lowlink.min(v_lowlink));
                        }
                    }

                    // 检查是否是SCC的根
                    if self.lowlinks[v] == self.indices[v] {
                        // 找到一个SCC，弹出栈中的节点
                        let start = self.component_nodes.len() as u32;

                        loop {
                            let w = self.stack.pop().unwrap();
                            self.on_stack.insert(w, false);
                            self.component_nodes.push(w);

                            if w == v {
                                break;
                            }
                        }

                        let end = self.component_nodes.len() as u32;
                        self.components.insert(start..end);
                    }
                }
            }
        }
    }
}

/// DFS栈帧
struct DfsFrame {
    node: FuncRef,
    state: DfsState,
    successors: Vec<FuncRef>,
}

impl DfsFrame {
    fn new(node: FuncRef) -> Self {
        Self {
            node,
            state: DfsState::FirstVisit,
            successors: Vec::new(),
        }
    }
}

/// DFS状态
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum DfsState {
    FirstVisit,         // 第一次访问
    VisitingSuccessors, // 正在访问后继节点
    PostVisit,          // 后访问（所有后继都已访问）
}

/// SCC凝聚图
#[derive(Debug)]
pub struct SccGraph {
    /// SCC节点列表
    pub nodes: Vec<Scc>,

    /// 边映射：从SCC到其后继SCC列表
    pub edges: RefSecMap<Scc, Range<u32>>,

    /// 边存储
    pub edge_elems: Vec<Scc>,
}

impl SccGraph {
    /// 从边集合创建SCC图
    fn from_edges(nodes: Vec<Scc>, edges_map: RefSparseMap<Scc, Vec<Scc>>) -> Self {
        let mut edges = RefSecMap::new();
        let mut edge_elems = Vec::new();

        for node in &nodes {
            if let Some(successors) = edges_map.get(*node) {
                let start = edge_elems.len() as u32;
                edge_elems.extend(successors);
                let end = edge_elems.len() as u32;
                edges[*node] = start..end;
            } else {
                edges[*node] = 0..0;
            }
        }

        Self {
            nodes,
            edges,
            edge_elems,
        }
    }

    /// 获取指定SCC的后继
    pub fn successors(&self, scc: Scc) -> &[Scc] {
        let range = &self.edges[scc];
        let start = range.start as usize;
        let end = range.end as usize;
        &self.edge_elems[start..end]
    }

    /// 拓扑排序（用于确定内联顺序）
    pub fn topological_sort(&self) -> Result<Vec<Scc>, &'static str> {
        let mut in_degree = RefSecMap::new();
        let mut queue = Vec::new();
        let mut result = Vec::new();

        // 计算入度
        for &node in &self.nodes {
            in_degree[node] = 0;
        }

        for &node in &self.nodes {
            for &succ in self.successors(node) {
                in_degree[succ] += 1;
            }
        }

        // 找到所有入度为0的节点
        for &node in &self.nodes {
            if in_degree[node] == 0 {
                queue.push(node);
            }
        }

        // 执行拓扑排序
        while let Some(node) = queue.pop() {
            result.push(node);

            for &succ in self.successors(node) {
                in_degree[succ] -= 1;
                if in_degree[succ] == 0 {
                    queue.push(succ);
                }
            }
        }

        if result.len() != self.nodes.len() {
            return Err("SCC凝聚图存在环（这不应该发生）");
        }

        // 反转结果以获得正确的依赖顺序
        result.reverse();
        Ok(result)
    }
}
