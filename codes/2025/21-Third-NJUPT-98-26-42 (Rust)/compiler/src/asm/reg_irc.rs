//! Iterated Register Coalescing, IRC 图着色寄存器分配
//!
//! 算法基于 Appel & George 的 Iterated Register Coalescing 实现:
//! <https://www.cs.princeton.edu/~appel/papers/regalloc.pdf>
//!
//! 设计特点:
//! - 通用IRC结构: 使用泛型IRC<R: IRCNode>同时处理XReg和FReg, 实现高效算法复用
//! - 相当于线性分配的图着色： 改进后的IRC现在不处理Spill逻辑，100%单遍扫描即可分配成功
//!
//! 核心流程：
//! 1. Build: 构建干扰图
//! 2. Simplify: 移除低度节点
//! 3. Coalesce: 合并移动相关的节点
//! 4. Freeze: 冻结低度移动相关节点
//! 5. Spill: 溢出高度节点（在我们的两阶段设计中不应该发生）
//! 6. Select: 为节点分配颜色

use super::{inst::*, reg::*, reg_alloc::*};
use crate::asm::stack::{SlotId, SlotKind, StackLayout};
use crate::prelude::*;
use std::collections::{BTreeMap, BTreeSet, HashMap};
use std::fmt::Debug;
use std::hash::Hash;

// ===================================================================
// 泛型约束
// ===================================================================

/// 可以作为IRC节点的类型约束
pub trait IRCNode: Copy + Clone + Eq + Hash + Debug + Ord {
    /// 判断是否为物理寄存器（预着色）
    fn is_physical(&self) -> bool;

    /// 判断是否为虚拟寄存器
    fn is_virtual(&self) -> bool;

    /// 转换为通用Reg类型（用于与外部接口交互）
    fn to_reg(&self) -> Reg;

    /// 从通用Reg类型转换回来
    fn try_from_reg(reg: Reg) -> Option<Self>;
}

// 为XReg实现IRCNode trait
impl IRCNode for XReg {
    fn is_physical(&self) -> bool {
        self.inner().is_physical()
    }

    fn is_virtual(&self) -> bool {
        self.inner().is_virtual()
    }

    fn to_reg(&self) -> Reg {
        *self.inner()
    }

    fn try_from_reg(reg: Reg) -> Option<Self> {
        XReg::try_from(reg).ok()
    }
}

// 为FReg实现IRCNode trait
impl IRCNode for FReg {
    fn is_physical(&self) -> bool {
        self.inner().is_physical()
    }

    fn is_virtual(&self) -> bool {
        self.inner().is_virtual()
    }

    fn to_reg(&self) -> Reg {
        *self.inner()
    }

    fn try_from_reg(reg: Reg) -> Option<Self> {
        FReg::try_from(reg).ok()
    }
}

// ===================================================================
// IRC 主结构
// ===================================================================

/// IRC 图着色寄存器分配器
pub struct IRC<R: IRCNode> {
    /// K值：可用物理寄存器的数量
    k: usize,

    // === 节点工作列表 ===
    /// 预着色节点（物理寄存器）
    precolored: BTreeSet<R>,
    /// 初始节点（所有虚拟寄存器）
    initial: BTreeSet<R>,
    /// 低度非移动相关节点
    simplify_worklist: BTreeSet<R>,
    /// 低度移动相关节点
    freeze_worklist: BTreeSet<R>,
    /// 高度节点
    spill_worklist: BTreeSet<R>,
    /// 本轮被溢出的节点（在我们的设计中不应该发生）
    spilled_nodes: BTreeSet<R>,
    /// 已合并的节点
    coalesced_nodes: BTreeSet<R>,
    /// 已着色的节点
    colored_nodes: BTreeSet<R>,
    /// 简化栈（按顺序存储被移除的节点）
    select_stack: Vec<R>,

    // === 移动指令集合 ===
    /// 已合并的移动
    coalesced_moves: BTreeSet<(R, R)>,
    /// 受约束的移动（源和目标冲突）
    constrained_moves: BTreeSet<(R, R)>,
    /// 冻结的移动（不再考虑合并）
    frozen_moves: BTreeSet<(R, R)>,
    /// 待处理的移动
    worklist_moves: BTreeSet<(R, R)>,
    /// 活跃的移动
    active_moves: BTreeSet<(R, R)>,

    // === 图信息 ===
    /// 邻接集合（用于快速判断边是否存在）
    adj_set: BTreeSet<(R, R)>,
    /// 邻接表（每个节点的邻居列表）
    adj_list: BTreeMap<R, BTreeSet<R>>,
    /// 节点的度
    degree: BTreeMap<R, usize>,
    /// 节点关联的移动指令
    move_list: BTreeMap<R, BTreeSet<(R, R)>>,
    /// 合并后的别名映射
    alias: BTreeMap<R, R>,

    // === 最终结果 ===
    /// 颜色映射：虚拟寄存器 -> 物理寄存器
    pub color_map: BTreeMap<R, R>,
}

impl<R: IRCNode> IRC<R> {
    /// 创建新的IRC实例
    pub fn new(precolored_regs: &[R]) -> Self {
        let precolored_set: BTreeSet<R> = precolored_regs.iter().cloned().collect();
        let k = precolored_set.len();

        // 预着色节点的颜色是它们自身
        let mut color_map = BTreeMap::new();
        for &preg in &precolored_set {
            color_map.insert(preg, preg);
        }

        Self {
            k,
            precolored: precolored_set,
            initial: BTreeSet::new(),
            simplify_worklist: BTreeSet::new(),
            freeze_worklist: BTreeSet::new(),
            spill_worklist: BTreeSet::new(),
            spilled_nodes: BTreeSet::new(),
            coalesced_nodes: BTreeSet::new(),
            colored_nodes: BTreeSet::new(),
            select_stack: Vec::new(),
            coalesced_moves: BTreeSet::new(),
            constrained_moves: BTreeSet::new(),
            frozen_moves: BTreeSet::new(),
            worklist_moves: BTreeSet::new(),
            active_moves: BTreeSet::new(),
            adj_set: BTreeSet::new(),
            adj_list: BTreeMap::new(),
            degree: BTreeMap::new(),
            move_list: BTreeMap::new(),
            alias: BTreeMap::new(),
            color_map,
        }
    }

    /// 设置初始节点集合（所有虚拟寄存器）
    pub fn set_initial(&mut self, nodes: BTreeSet<R>) {
        self.initial = nodes;
    }

    /// 添加干扰边
    pub fn add_edge(&mut self, u: R, v: R) {
        if u == v {
            return; // 自环不添加
        }

        let edge = (u, v);
        let reverse_edge = (v, u);

        // 如果边已存在，不重复添加
        if self.adj_set.contains(&edge) {
            return;
        }

        // 添加边到邻接集合
        self.adj_set.insert(edge);
        self.adj_set.insert(reverse_edge);

        // 更新邻接表和度
        if !u.is_physical() {
            self.adj_list.entry(u).or_default().insert(v);
            *self.degree.entry(u).or_insert(0) += 1;
        }

        if !v.is_physical() {
            self.adj_list.entry(v).or_default().insert(u);
            *self.degree.entry(v).or_insert(0) += 1;
        }

        // 调试：跟踪函数参数的干扰边
        let u_str = format!("{:?}", u);
        let v_str = format!("{:?}", v);
        if u_str.contains("VXId(1)")
            || u_str.contains("VXId(2)")
            || v_str.contains("VXId(1)")
            || v_str.contains("VXId(2)")
        {
            eprintln!("DEBUG_EDGE: {:?} -- {:?}", u, v);
        }
    }

    /// 添加移动指令
    pub fn add_move(&mut self, dst: R, src: R) {
        let mov = (dst, src);

        // 将移动添加到工作列表
        self.worklist_moves.insert(mov);

        // 更新节点的移动列表
        self.move_list.entry(dst).or_default().insert(mov);
        self.move_list.entry(src).or_default().insert(mov);
    }

    /// 获取节点的度（预着色节点度为无穷大）
    fn get_degree(&self, n: R) -> usize {
        if n.is_physical() {
            usize::MAX // 预着色节点度视为无穷大
        } else {
            self.degree.get(&n).cloned().unwrap_or(0)
        }
    }

    /// 获取节点的别名（处理合并后的节点）
    fn get_alias(&self, n: R) -> R {
        if self.coalesced_nodes.contains(&n) {
            if let Some(&alias) = self.alias.get(&n) {
                return self.get_alias(alias); // 递归查找
            }
        }
        n
    }

    /// 获取节点的邻居（排除已选择和已合并的节点）
    fn adjacent(&self, n: R) -> BTreeSet<R> {
        let adj = self.adj_list.get(&n).cloned().unwrap_or_default();

        // 排除已在栈上和已合并的节点
        let mut result = BTreeSet::new();
        for &neighbor in &adj {
            if !self.select_stack.contains(&neighbor) && !self.coalesced_nodes.contains(&neighbor) {
                result.insert(neighbor);
            }
        }
        result
    }

    /// 获取节点相关的移动指令
    fn node_moves(&self, n: R) -> BTreeSet<(R, R)> {
        let moves = self.move_list.get(&n).cloned().unwrap_or_default();

        // 只返回活跃或待处理的移动
        moves
            .intersection(
                &self
                    .active_moves
                    .union(&self.worklist_moves)
                    .cloned()
                    .collect(),
            )
            .cloned()
            .collect()
    }

    /// 判断节点是否与移动相关
    fn move_related(&self, n: R) -> bool {
        !self.node_moves(n).is_empty()
    }

    // ===================================================================
    // 算法主要阶段
    // ===================================================================

    /// 创建初始工作列表
    pub fn make_worklist(&mut self) {
        let initial = self.initial.clone();
        self.initial.clear();

        for n in initial {
            let degree = self.get_degree(n);

            if degree >= self.k {
                self.spill_worklist.insert(n);
            } else if self.move_related(n) {
                self.freeze_worklist.insert(n);
            } else {
                self.simplify_worklist.insert(n);
            }
        }
    }

    /// 主循环
    pub fn main_loop(&mut self) {
        self.main_loop_with_debug(false)
    }

    /// 带调试信息的主循环
    pub fn main_loop_with_debug(&mut self, debug: bool) {
        let mut iteration = 0;
        loop {
            iteration += 1;

            if debug {
                eprintln!("\n--- IRC 主循环迭代 {} ---", iteration);
                eprintln!(
                    "工作列表状态: simplify={}, freeze={}, spill={}, moves={}",
                    self.simplify_worklist.len(),
                    self.freeze_worklist.len(),
                    self.spill_worklist.len(),
                    self.worklist_moves.len()
                );
            }

            if !self.simplify_worklist.is_empty() {
                if debug {
                    let node = *self.simplify_worklist.iter().next().unwrap();
                    eprintln!("操作: 简化节点 {:?}", node);
                }
                self.simplify();
            } else if !self.worklist_moves.is_empty() {
                if debug {
                    let mov = *self.worklist_moves.iter().next().unwrap();
                    eprintln!("操作: 尝试合并移动 {:?} <- {:?}", mov.0, mov.1);
                }
                self.coalesce();
            } else if !self.freeze_worklist.is_empty() {
                if debug {
                    let node = *self.freeze_worklist.iter().next().unwrap();
                    eprintln!("操作: 冻结节点 {:?}", node);
                }
                self.freeze();
            } else if !self.spill_worklist.is_empty() {
                if debug {
                    let node = *self.spill_worklist.iter().next().unwrap();
                    eprintln!("操作: 选择溢出节点 {:?}", node);
                }
                self.select_spill();
            } else {
                if debug {
                    eprintln!("主循环结束，选择栈大小: {}", self.select_stack.len());
                }
                break;
            }
        }
    }

    /// 简化：移除低度节点
    fn simplify(&mut self) {
        // 从简化工作列表中取一个节点
        let n = *self.simplify_worklist.iter().next().unwrap();
        self.simplify_worklist.remove(&n);

        // 压入选择栈
        self.select_stack.push(n);

        // 减少所有邻居的度
        let neighbors = self.adjacent(n);
        for m in neighbors {
            self.decrement_degree(m);
        }
    }

    /// 减少节点的度
    fn decrement_degree(&mut self, m: R) {
        let d = self.get_degree(m);

        if m.is_physical() {
            return; // 物理寄存器的度不变
        }

        // 减少度
        if let Some(degree) = self.degree.get_mut(&m) {
            *degree = degree.saturating_sub(1);
        }

        // 如果度从k降到k-1，节点变为低度
        if d == self.k {
            // 使能相关的移动
            let nodes_to_enable = self
                .adjacent(m)
                .union(&std::iter::once(m).collect())
                .cloned()
                .collect();
            self.enable_moves(nodes_to_enable);

            // 从溢出列表移到冻结或简化列表
            self.spill_worklist.remove(&m);
            if self.move_related(m) {
                self.freeze_worklist.insert(m);
            } else {
                self.simplify_worklist.insert(m);
            }
        }
    }

    /// 使能移动指令
    fn enable_moves(&mut self, nodes: BTreeSet<R>) {
        for n in nodes {
            let moves = self.node_moves(n);
            for mov in moves {
                if self.active_moves.contains(&mov) {
                    self.active_moves.remove(&mov);
                    self.worklist_moves.insert(mov);
                }
            }
        }
    }

    /// 合并：尝试合并移动相关的节点
    fn coalesce(&mut self) {
        // 从工作列表中取一个移动
        let mov = *self.worklist_moves.iter().next().unwrap();
        let (x, y) = mov;

        self.worklist_moves.remove(&mov);

        // 获取别名
        let x = self.get_alias(x);
        let y = self.get_alias(y);

        // 确保u是预着色的（如果有的话）
        let (u, v) = if y.is_physical() { (y, x) } else { (x, y) };

        if u == v {
            // 同一节点，直接合并
            self.coalesced_moves.insert(mov);
            self.add_worklist(u);
        } else if v.is_physical() || self.adj_set.contains(&(u, v)) {
            // 两个都是预着色的，或者它们冲突
            self.constrained_moves.insert(mov);
            self.add_worklist(u);
            self.add_worklist(v);
        } else if (u.is_physical() && self.ok_to_coalesce_with_precolored(u, v))
            || (!u.is_physical()
                && self.conservative(self.adjacent(u).union(&self.adjacent(v)).cloned().collect()))
        {
            // 可以安全合并
            self.coalesced_moves.insert(mov);
            self.combine(u, v);
            self.add_worklist(u);
        } else {
            // 暂时不能合并
            self.active_moves.insert(mov);
        }
    }

    /// 检查是否可以与预着色节点合并
    fn ok_to_coalesce_with_precolored(&self, u: R, v: R) -> bool {
        for t in self.adjacent(v) {
            if !self.ok(t, u) {
                return false;
            }
        }
        true
    }

    /// George测试
    fn ok(&self, t: R, r: R) -> bool {
        self.get_degree(t) < self.k || t.is_physical() || self.adj_set.contains(&(t, r))
    }

    /// Briggs测试
    fn conservative(&self, nodes: BTreeSet<R>) -> bool {
        let mut k_count = 0;
        for n in nodes {
            if self.get_degree(n) >= self.k {
                k_count += 1;
            }
        }
        k_count < self.k
    }

    /// 合并两个节点
    fn combine(&mut self, u: R, v: R) {
        // 从工作列表中移除v
        if self.freeze_worklist.contains(&v) {
            self.freeze_worklist.remove(&v);
        } else {
            self.spill_worklist.remove(&v);
        }

        // v被合并到u
        self.coalesced_nodes.insert(v);
        self.alias.insert(v, u);

        // 合并移动列表
        let v_moves = self.move_list.get(&v).cloned().unwrap_or_default();
        self.move_list.entry(u).or_default().extend(v_moves);

        // 将v的所有邻居添加为u的邻居
        let v_adj = self.adjacent(v);
        for t in v_adj {
            self.add_edge(t, u);
            self.decrement_degree(t);
        }

        // 如果u的度变高，从冻结列表移到溢出列表
        if self.get_degree(u) >= self.k && self.freeze_worklist.contains(&u) {
            self.freeze_worklist.remove(&u);
            self.spill_worklist.insert(u);
        }
    }

    /// 将节点添加回工作列表
    fn add_worklist(&mut self, u: R) {
        if !u.is_physical() && !self.move_related(u) && self.get_degree(u) < self.k {
            self.freeze_worklist.remove(&u);
            self.simplify_worklist.insert(u);
        }
    }

    /// 冻结：放弃合并某个低度移动相关节点
    fn freeze(&mut self) {
        // 从冻结列表中取一个节点
        let u = *self.freeze_worklist.iter().next().unwrap();
        self.freeze_worklist.remove(&u);
        self.simplify_worklist.insert(u);

        // 冻结相关的移动
        self.freeze_moves(u);
    }

    /// 冻结节点相关的移动
    fn freeze_moves(&mut self, u: R) {
        let moves = self.node_moves(u);

        for mov in moves {
            let (x, y) = mov;

            // 找到另一端
            let v = if self.get_alias(y) == self.get_alias(u) {
                self.get_alias(x)
            } else {
                self.get_alias(y)
            };

            // 从活跃移动转为冻结移动
            self.active_moves.remove(&mov);
            self.frozen_moves.insert(mov);

            // 如果v不再与移动相关且是低度的，可以简化
            if !self.move_related(v) && self.get_degree(v) < self.k {
                self.freeze_worklist.remove(&v);
                self.simplify_worklist.insert(v);
            }
        }
    }

    /// 选择溢出节点
    fn select_spill(&mut self) {
        // 采用标准算法：选择一个节点，移到简化列表，并冻结其移动
        // 这在我们的两阶段设计中，是作为一种"乐观着色"的准备步骤
        let m = *self.spill_worklist.iter().next().unwrap();
        self.spill_worklist.remove(&m);
        self.simplify_worklist.insert(m);
        self.freeze_moves(m);
    }

    /// 分配颜色
    pub fn assign_colors(&mut self, func_name: Option<&str>) -> CEResult<()> {
        // 处理选择栈中的所有节点
        while let Some(n) = self.select_stack.pop() {
            // 如果是预着色节点，跳过
            if n.is_physical() {
                self.colored_nodes.insert(n);
                continue;
            }

            // 收集所有可用颜色
            let mut ok_colors: BTreeSet<R> = self.precolored.clone();

            // 移除所有已着色邻居使用的颜色
            // 注意：这里不应该使用adj_list，因为它包含了还在选择栈上的节点
            // 只有已经被着色的邻居才会影响当前节点的颜色选择
            if let Some(adj) = self.adj_list.get(&n) {
                for &w in adj {
                    let alias_w = self.get_alias(w);
                    // 只考虑已经被着色的节点（在colored_nodes中或是预着色节点）
                    if self.colored_nodes.contains(&alias_w) || alias_w.is_physical() {
                        if let Some(&color) = self.color_map.get(&alias_w) {
                            ok_colors.remove(&color);
                        } else if alias_w.is_physical() {
                            // 预着色节点的颜色就是它自己
                            ok_colors.remove(&alias_w);
                        }
                    }
                }
            }

            if ok_colors.is_empty() {
                // 无法着色，这是一个严重错误
                // 在Spill已经降低压力到k以下后，IRC理论上应该总能成功
                eprintln!("ERROR: 节点 {:?} 无法着色！", n);
                if let Some(adj) = self.adj_list.get(&n) {
                    eprintln!("  邻居数量: {}", adj.len());
                    eprintln!("  度数: {}", self.degree.get(&n).unwrap_or(&0));

                    // 详细打印所有邻居节点
                    eprintln!("  邻居节点列表:");
                    let mut phys_neighbors = Vec::new();
                    let mut virt_neighbors = Vec::new();
                    for &neighbor in adj {
                        if neighbor.is_physical() {
                            phys_neighbors.push(neighbor);
                        } else {
                            virt_neighbors.push(neighbor);
                        }
                    }
                    eprintln!(
                        "    物理寄存器邻居 ({}个): {:?}",
                        phys_neighbors.len(),
                        phys_neighbors
                    );
                    eprintln!(
                        "    虚拟寄存器邻居 ({}个): {:?}",
                        virt_neighbors.len(),
                        virt_neighbors
                    );

                    // 打印已使用的颜色
                    let mut used_colors = BTreeSet::new();
                    for &w in adj {
                        let alias_w = self.get_alias(w);
                        if let Some(&color) = self.color_map.get(&alias_w) {
                            used_colors.insert(color);
                        }
                    }
                    eprintln!(
                        "  已被邻居使用的颜色 ({}个): {:?}",
                        used_colors.len(),
                        used_colors
                    );
                }
                eprintln!("  预着色寄存器数量 k = {}", self.precolored.len());

                // 生成干扰图的graphviz可视化
                if let Some(func_name) = func_name {
                    self.generate_graphviz_for_node(n, func_name);
                }

                // 收集详细信息用于错误报告
                let neighbor_count = self.adj_list.get(&n).map(|adj| adj.len()).unwrap_or(0);
                let degree = self.degree.get(&n).unwrap_or(&0);

                return Err(CErr::Internal(format!(
                    "IRC着色失败：节点 {:?} 无法找到可用颜色 (度数={}, 邻居数={}, k={})",
                    n,
                    degree,
                    neighbor_count,
                    self.precolored.len()
                )));
            } else {
                // 选择一个颜色
                let color = *ok_colors.iter().next().unwrap();
                self.colored_nodes.insert(n);
                self.color_map.insert(n, color);
            }
        }

        // 为合并的节点分配颜色
        for &n in &self.coalesced_nodes {
            let alias_n = self.get_alias(n);
            if let Some(&color) = self.color_map.get(&alias_n) {
                self.color_map.insert(n, color);
            }
        }

        // 检查是否有溢出节点（不应该发生）
        if !self.spilled_nodes.is_empty() {
            return Err(CErr::Internal(format!(
                "IRC分配失败：存在 {} 个无法着色的节点",
                self.spilled_nodes.len()
            )));
        }

        Ok(())
    }

    /// 获取调试信息
    pub fn debug_info(&self) -> String {
        format!(
            "IRC状态: simplify={}, freeze={}, spill={}, moves={}, colored={}, spilled={}",
            self.simplify_worklist.len(),
            self.freeze_worklist.len(),
            self.spill_worklist.len(),
            self.worklist_moves.len(),
            self.colored_nodes.len(),
            self.spilled_nodes.len()
        )
    }

    /// 输出详细的干扰图调试信息
    pub fn dump_interference_graph(&self) -> String {
        let mut output = String::new();

        // 1. 输出所有节点及其度数
        output.push_str("=== 节点度数信息 ===\n");
        let mut nodes: Vec<_> = self.degree.iter().collect();
        nodes.sort_by_key(|(node, _)| format!("{:?}", node));
        for (node, degree) in nodes {
            output.push_str(&format!("  {:?}: degree = {}\n", node, degree));
        }

        // 2. 输出干扰边
        output.push_str("\n=== 干扰边 ===\n");
        let mut edges: Vec<_> = self.adj_set.iter().collect();
        edges.sort_by_key(|(u, v)| (format!("{:?}", u), format!("{:?}", v)));
        let mut printed_edges = BTreeSet::new();
        for (u, v) in edges {
            let edge_key = if format!("{:?}", u) < format!("{:?}", v) {
                (u, v)
            } else {
                (v, u)
            };
            if !printed_edges.contains(&edge_key) {
                output.push_str(&format!("  {:?} -- {:?}\n", edge_key.0, edge_key.1));
                printed_edges.insert(edge_key);
            }
        }

        // 3. 输出移动指令
        output.push_str("\n=== 移动指令 ===\n");
        output.push_str(&format!(
            "待处理移动 (worklist_moves): {}\n",
            self.worklist_moves.len()
        ));
        for mov in &self.worklist_moves {
            output.push_str(&format!("  {:?} <- {:?}\n", mov.0, mov.1));
        }

        output.push_str(&format!(
            "\n活跃移动 (active_moves): {}\n",
            self.active_moves.len()
        ));
        for mov in &self.active_moves {
            output.push_str(&format!("  {:?} <- {:?}\n", mov.0, mov.1));
        }

        output.push_str(&format!(
            "\n已合并移动 (coalesced_moves): {}\n",
            self.coalesced_moves.len()
        ));
        for mov in &self.coalesced_moves {
            output.push_str(&format!("  {:?} <- {:?}\n", mov.0, mov.1));
        }

        output.push_str(&format!(
            "\n受约束移动 (constrained_moves): {}\n",
            self.constrained_moves.len()
        ));
        for mov in &self.constrained_moves {
            output.push_str(&format!("  {:?} <- {:?}\n", mov.0, mov.1));
        }

        output.push_str(&format!(
            "\n冻结移动 (frozen_moves): {}\n",
            self.frozen_moves.len()
        ));
        for mov in &self.frozen_moves {
            output.push_str(&format!("  {:?} <- {:?}\n", mov.0, mov.1));
        }

        // 4. 输出节点工作列表状态
        output.push_str("\n=== 节点工作列表 ===\n");
        output.push_str(&format!("初始节点 (initial): {}\n", self.initial.len()));
        for node in &self.initial {
            output.push_str(&format!("  {:?}\n", node));
        }

        output.push_str(&format!(
            "\n简化列表 (simplify_worklist): {}\n",
            self.simplify_worklist.len()
        ));
        for node in &self.simplify_worklist {
            output.push_str(&format!("  {:?}\n", node));
        }

        output.push_str(&format!(
            "\n冻结列表 (freeze_worklist): {}\n",
            self.freeze_worklist.len()
        ));
        for node in &self.freeze_worklist {
            output.push_str(&format!("  {:?}\n", node));
        }

        output.push_str(&format!(
            "\n溢出列表 (spill_worklist): {}\n",
            self.spill_worklist.len()
        ));
        for node in &self.spill_worklist {
            output.push_str(&format!("  {:?}\n", node));
        }

        // 5. 输出合并关系
        output.push_str("\n=== 合并关系 ===\n");
        output.push_str(&format!(
            "已合并节点 (coalesced_nodes): {}\n",
            self.coalesced_nodes.len()
        ));
        for node in &self.coalesced_nodes {
            if let Some(alias) = self.alias.get(&node) {
                output.push_str(&format!("  {:?} -> {:?}\n", node, alias));
            }
        }

        // 6. 输出颜色分配结果
        output.push_str("\n=== 颜色分配 ===\n");
        let mut color_entries: Vec<_> = self.color_map.iter().collect();
        color_entries.sort_by_key(|(node, _)| format!("{:?}", node));
        for (node, color) in color_entries {
            if node.is_virtual() {
                output.push_str(&format!("  {:?} -> {:?}\n", node, color));
            }
        }

        // 7. 输出预着色节点
        output.push_str("\n=== 预着色节点 ===\n");
        let mut precolored: Vec<_> = self.precolored.iter().collect();
        precolored.sort_by_key(|node| format!("{:?}", node));
        for node in precolored {
            output.push_str(&format!("  {:?}\n", node));
        }

        output
    }

    /// 为无法着色的节点生成graphviz可视化
    fn generate_graphviz_for_node(&self, node: R, func_name: &str) {
        use std::fs::{self, File};
        use std::io::Write;

        // 确保TEMP目录存在
        let _ = fs::create_dir_all("TEMP");

        // 生成文件名：函数名_节点ID.dot
        let node_str = format!("{:?}", node);
        let node_id = node_str
            .split("(")
            .last()
            .unwrap_or("")
            .split(")")
            .next()
            .unwrap_or("");
        let filename = format!("TEMP/{}_irc_{}.dot", func_name, node_id);
        eprintln!("生成干扰图可视化: {}", filename);

        let mut dot = String::new();
        dot.push_str("digraph InterferenceGraph {\n");
        dot.push_str("  rankdir=LR;\n");
        dot.push_str("  node [shape=circle];\n");
        dot.push_str("\n");

        // 获取节点ID的简化格式
        let get_node_label = |n: R| -> String {
            let node_str = format!("{:?}", n);
            // 从XReg(VX(VXId(278)))提取出278
            if let Some(id) = node_str.split("(").last() {
                if let Some(num) = id.split(")").next() {
                    if n.is_physical() {
                        // 物理寄存器，显示寄存器名
                        return node_str
                            .split("X(")
                            .last()
                            .unwrap_or(&node_str)
                            .split(")")
                            .next()
                            .unwrap_or(&node_str)
                            .to_string();
                    } else {
                        // 虚拟寄存器，显示VX编号
                        return format!("VX{}", num);
                    }
                }
            }
            node_str
        };

        // 中心节点（无法着色的节点）
        let center_label = get_node_label(node);
        dot.push_str(&format!(
            "  \"{}\" [style=filled, fillcolor=red, label=\"{}\"];\n",
            center_label, center_label
        ));

        // 收集所有相关节点
        let mut all_nodes = BTreeSet::new();
        all_nodes.insert(node);

        if let Some(adj) = self.adj_list.get(&node) {
            for &neighbor in adj {
                all_nodes.insert(neighbor);

                // 邻居节点样式
                let neighbor_label = get_node_label(neighbor);
                if neighbor.is_physical() {
                    // 物理寄存器用蓝色
                    dot.push_str(&format!(
                        "  \"{}\" [style=filled, fillcolor=lightblue, label=\"{}\"];\n",
                        neighbor_label, neighbor_label
                    ));
                } else if let Some(&color) = self.color_map.get(&neighbor) {
                    // 已着色的虚拟寄存器用绿色
                    let color_label = get_node_label(color);
                    dot.push_str(&format!(
                        "  \"{}\" [style=filled, fillcolor=lightgreen, label=\"{}\\n→{}\"];\n",
                        neighbor_label, neighbor_label, color_label
                    ));
                } else {
                    // 未着色的虚拟寄存器用白色
                    dot.push_str(&format!(
                        "  \"{}\" [label=\"{}\"];\n",
                        neighbor_label, neighbor_label
                    ));
                }
            }
        }

        dot.push_str("\n  // 干扰边（只显示与中心节点直接相关的边）\n");

        // 只添加与中心节点直接相关的边
        if let Some(adj) = self.adj_list.get(&node) {
            let center_label = get_node_label(node);
            for &neighbor in adj {
                let neighbor_label = get_node_label(neighbor);
                dot.push_str(&format!(
                    "  \"{}\" -> \"{}\" [dir=none];\n",
                    center_label, neighbor_label
                ));
            }
        }

        // 可选：添加邻居之间的边（会让图更复杂）
        // 注释掉以简化图形
        /*
        for &(u, v) in &self.adj_set {
            if all_nodes.contains(&u) && all_nodes.contains(&v) && u != node && v != node {
                if u < v {
                    let u_label = get_node_label(u);
                    let v_label = get_node_label(v);
                    dot.push_str(&format!("  \"{}\" -> \"{}\" [dir=none, style=dotted, color=gray];\n",
                        u_label, v_label));
                }
            }
        }
        */

        // 添加图例
        dot.push_str("\n  // 图例\n");
        dot.push_str("  subgraph cluster_legend {\n");
        dot.push_str("    label=\"图例\";\n");
        dot.push_str("    style=dotted;\n");
        dot.push_str(
            "    legend1 [shape=box, style=filled, fillcolor=red, label=\"无法着色节点\"];\n",
        );
        dot.push_str(
            "    legend2 [shape=box, style=filled, fillcolor=lightblue, label=\"物理寄存器\"];\n",
        );
        dot.push_str("    legend3 [shape=box, style=filled, fillcolor=lightgreen, label=\"已着色虚拟寄存器\"];\n");
        dot.push_str("    legend4 [shape=box, label=\"未着色虚拟寄存器\"];\n");
        dot.push_str("  }\n");

        dot.push_str("}\n");

        // 写入文件
        if let Ok(mut file) = File::create(&filename) {
            if let Err(e) = file.write_all(dot.as_bytes()) {
                eprintln!("警告: 无法写入graphviz文件 {}: {}", filename, e);
            } else {
                eprintln!("成功生成干扰图: {}", filename);
            }
        } else {
            eprintln!("警告: 无法创建graphviz文件 {}", filename);
        }
    }
}

// ===================================================================
// IRC 图着色集成 - RegAllocEnv 的扩展方法
// ===================================================================
impl RegAllocEnv<'_> {
    /// 执行IRC图着色寄存器分配
    pub fn run_irc(&mut self) -> CEResult<()> {
        // 1. 初始化两个独立的IRC环境
        let mut irc_x = IRC::<XReg>::new(&PRECOLORED_XREGS);
        let mut irc_f = IRC::<FReg>::new(&PRECOLORED_FREGS);

        // 2. 构建干扰图和收集初始节点
        self.build_interference_graphs(&mut irc_x, &mut irc_f)?;

        // 3. 创建工作列表
        irc_x.make_worklist();
        irc_f.make_worklist();

        // 4. 运行主循环
        if self.debug_comments && self.debug_irc {
            eprintln!("\n========== IRC 图着色开始 ==========");
            eprintln!("函数: {:?}", self.vfunc.func_name);

            // 输出干扰图构建后的初始状态
            eprintln!("\n【整数寄存器干扰图】构建完成:");
            eprintln!("{}", irc_x.dump_interference_graph());

            eprintln!("\n【浮点寄存器干扰图】构建完成:");
            eprintln!("{}", irc_f.dump_interference_graph());
        }

        // 使用debug_comments控制详细的IRC调试信息
        if self.debug_comments && self.debug_irc {
            eprintln!("\n【开始整数寄存器IRC主循环】");
        }
        irc_x.main_loop_with_debug(self.debug_comments && self.debug_irc);

        if self.debug_comments && self.debug_irc {
            eprintln!("\n【开始浮点寄存器IRC主循环】");
        }
        irc_f.main_loop_with_debug(self.debug_comments && self.debug_irc);

        // 5. 分配颜色
        let func_name = Some(self.vfunc.func_name.as_str());
        irc_x.assign_colors(func_name)?;
        irc_f.assign_colors(func_name)?;

        if self.debug_comments && self.debug_irc {
            eprintln!("\n【图着色后状态】");
            eprintln!("\nIRC X: {}", irc_x.debug_info());
            eprintln!("IRC F: {}", irc_f.debug_info());

            // 输出最终的颜色分配结果
            eprintln!("\n【最终颜色分配】");
            eprintln!("整数寄存器:");
            eprintln!("{}", irc_x.dump_interference_graph());
            eprintln!("\n浮点寄存器:");
            eprintln!("{}", irc_f.dump_interference_graph());

            // 输出颜色映射信息
            eprintln!("\n颜色分配结果:");
            eprintln!("  整数寄存器: {} 个", irc_x.color_map.len());
            eprintln!("  浮点寄存器: {} 个", irc_f.color_map.len());
        }

        // 6. 合并颜色映射并重写程序
        let final_color_map = self.combine_color_maps(&irc_x, &irc_f);

        // 插入调试注释
        if self.debug_comments && self.debug_irc {
            self.insert_color_comments(&final_color_map);
        }

        // 重写指令
        self.rewrite_program(&final_color_map);

        if self.debug_comments && self.debug_irc {
            eprintln!("========== IRC 图着色完成 ==========\n");
        }

        Ok(())
    }

    /// 构建干扰图
    fn build_interference_graphs(
        &mut self,
        irc_x: &mut IRC<XReg>,
        irc_f: &mut IRC<FReg>,
    ) -> CEResult<()> {
        let liveness = self
            .liveness
            .as_ref()
            .ok_or_else(|| CErr::Internal("活跃变量分析结果不存在".to_string()))?;

        // 收集所有虚拟寄存器（用于initial集合）
        let mut initial_x = BTreeSet::<XReg>::new();
        let mut initial_f = BTreeSet::<FReg>::new();

        // 首先，处理函数参数的ABI约束
        // 函数参数需要从特定的物理寄存器传入，这应该被作为Move处理
        // 同时收集所有虚拟参数寄存器，以便后续添加干扰边
        let mut args_x = Vec::<XReg>::new();
        let mut args_f = Vec::<FReg>::new();

        for arg_pair in &self.vfunc.args {
            match arg_pair {
                ArgPair::RegArg { vreg, preg } => {
                    if vreg.is_virtual() {
                        // 将函数参数的ABI约束作为Move处理
                        // 这表示从物理寄存器preg到虚拟寄存器vreg的传递
                        match (
                            <XReg as IRCNode>::try_from_reg(*vreg),
                            <XReg as IRCNode>::try_from_reg(*preg),
                        ) {
                            (Some(v), Some(p)) => {
                                initial_x.insert(v);
                                irc_x.add_move(v, p);
                                // 收集虚拟参数寄存器
                                args_x.push(v);
                            }
                            _ => {
                                if let (Some(v), Some(p)) = (
                                    <FReg as IRCNode>::try_from_reg(*vreg),
                                    <FReg as IRCNode>::try_from_reg(*preg),
                                ) {
                                    initial_f.insert(v);
                                    irc_f.add_move(v, p);
                                    // 收集虚拟参数寄存器
                                    args_f.push(v);
                                }
                            }
                        }
                    }
                }
                ArgPair::StackArg { .. } => {
                    // 栈参数完全不被考虑
                }
                ArgPair::None => {}
            }
        }

        // [关键修复] 为所有函数参数对添加干扰边，因为它们在函数入口处同时活跃
        // 这修复了之前遗漏的问题：函数参数之间必须互相干扰，否则会被错误分配到同一个寄存器
        for i in 0..args_x.len() {
            for j in (i + 1)..args_x.len() {
                irc_x.add_edge(args_x[i], args_x[j]);
            }
        }
        for i in 0..args_f.len() {
            for j in (i + 1)..args_f.len() {
                irc_f.add_edge(args_f[i], args_f[j]);
            }
        }

        // [关键的新增修复] 为每个基本块入口处的活跃变量集构建clique
        // 这确保所有在控制流边界同时活跃的变量都互相干扰
        for &block_id in self.vfunc.cfg.block_order() {
            if let Some(block_liveness) = liveness.liveness.get(&block_id) {
                let live_in_vec: Vec<_> = block_liveness
                    .live_in
                    .iter()
                    .filter(|r| r.is_virtual())
                    .cloned()
                    .collect();

                // 为live_in集合中的所有虚拟寄存器对添加干扰边
                for i in 0..live_in_vec.len() {
                    for j in (i + 1)..live_in_vec.len() {
                        let r1 = live_in_vec[i];
                        let r2 = live_in_vec[j];

                        // 根据寄存器类型添加干扰边
                        match (r1, r2) {
                            (Reg::VX(_), Reg::VX(_)) => {
                                if let (Some(x1), Some(x2)) = (
                                    <XReg as IRCNode>::try_from_reg(r1),
                                    <XReg as IRCNode>::try_from_reg(r2),
                                ) {
                                    irc_x.add_edge(x1, x2);
                                }
                            }
                            (Reg::VF(_), Reg::VF(_)) => {
                                if let (Some(f1), Some(f2)) = (
                                    <FReg as IRCNode>::try_from_reg(r1),
                                    <FReg as IRCNode>::try_from_reg(r2),
                                ) {
                                    irc_f.add_edge(f1, f2);
                                }
                            }
                            _ => {} // 不同类型的寄存器不会相互干扰
                        }
                    }
                }
            }
        }

        // 遍历所有基本块（按逆后序）
        for &block_id in self.vfunc.cfg.block_order().iter().rev() {
            let block_liveness = liveness
                .liveness
                .get(&block_id)
                .ok_or_else(|| CErr::Internal(format!("块 {:?} 缺少活跃变量信息", block_id)))?;
            let block_data = self
                .vfunc
                .block_datas
                .get(&block_id)
                .ok_or_else(|| CErr::Internal(format!("块 {:?} 不存在", block_id)))?;

            // 从块出口的活跃集合开始
            let mut live = block_liveness.live_out.clone();

            // 逆序遍历指令
            for inst in block_data.insts.iter().rev() {
                // 跳过注释
                if matches!(inst, MInst::Comment { .. }) {
                    continue;
                }

                // 获取指令的use和def
                let uses = inst.get_uses();
                let defs = inst.get_defs();

                // 收集虚拟寄存器到initial集合
                for &reg in uses.iter().chain(defs.iter()) {
                    if reg.is_virtual() {
                        if let Some(xreg) = <XReg as IRCNode>::try_from_reg(reg) {
                            initial_x.insert(xreg);
                        } else if let Some(freg) = <FReg as IRCNode>::try_from_reg(reg) {
                            initial_f.insert(freg);
                        }
                    }
                }

                // 处理Move指令
                if let Some((dst, src)) = inst.as_move() {
                    // 从活跃集合中移除源寄存器（Move的特殊处理）
                    live.remove(&src);

                    // 添加到相应的IRC移动列表
                    match (
                        <XReg as IRCNode>::try_from_reg(dst),
                        <XReg as IRCNode>::try_from_reg(src),
                    ) {
                        (Some(dst_x), Some(src_x)) => {
                            irc_x.add_move(dst_x, src_x);
                        }
                        _ => {
                            // 尝试浮点寄存器
                            if let (Some(dst_f), Some(src_f)) = (
                                <FReg as IRCNode>::try_from_reg(dst),
                                <FReg as IRCNode>::try_from_reg(src),
                            ) {
                                irc_f.add_move(dst_f, src_f);
                            }
                        }
                    }
                }

                // 处理CallPseudo的ABI约束（作为Move）
                if let MInst::CallPseudo { ret, args, .. } = inst {
                    // 处理参数约束
                    for arg in args {
                        if let ArgPair::RegArg { vreg, preg } = arg {
                            if vreg.is_virtual() {
                                // 将ABI约束作为Move处理
                                match (
                                    <XReg as IRCNode>::try_from_reg(*vreg),
                                    <XReg as IRCNode>::try_from_reg(*preg),
                                ) {
                                    (Some(v), Some(p)) => irc_x.add_move(p, v),
                                    _ => {
                                        if let (Some(v), Some(p)) = (
                                            <FReg as IRCNode>::try_from_reg(*vreg),
                                            <FReg as IRCNode>::try_from_reg(*preg),
                                        ) {
                                            irc_f.add_move(p, v);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 处理返回值约束
                    if let ArgPair::RegArg { vreg, preg } = ret {
                        if vreg.is_virtual() {
                            match (
                                <XReg as IRCNode>::try_from_reg(*vreg),
                                <XReg as IRCNode>::try_from_reg(*preg),
                            ) {
                                (Some(v), Some(p)) => irc_x.add_move(v, p),
                                _ => {
                                    if let (Some(v), Some(p)) = (
                                        <FReg as IRCNode>::try_from_reg(*vreg),
                                        <FReg as IRCNode>::try_from_reg(*preg),
                                    ) {
                                        irc_f.add_move(v, p);
                                    }
                                }
                            }
                        }
                    }

                    // [关键修复] 为跨调用活跃的虚拟寄存器添加与caller-saved寄存器的干扰边
                    // Call指令会破坏所有caller-saved寄存器。
                    // 一个变量是"跨调用活跃"的，当且仅当它在call的live_out集合中，且不是call的返回值。
                    // 这是ABI契约的要求：caller-saved寄存器会被调用破坏
                    let defs = inst.get_defs();
                    for &live_reg in live.iter().filter(|r| !defs.contains(r)) {
                        if live_reg.is_virtual() {
                            // 处理整数寄存器
                            if let Some(live_vreg) = <XReg as IRCNode>::try_from_reg(live_reg) {
                                for &caller_saved in &CALLER_SAVED_XREGS {
                                    irc_x.add_edge(live_vreg, caller_saved);
                                }
                            }
                            // 处理浮点寄存器
                            else if let Some(live_vreg) =
                                <FReg as IRCNode>::try_from_reg(live_reg)
                            {
                                for &caller_saved in &CALLER_SAVED_FREGS {
                                    irc_f.add_edge(live_vreg, caller_saved);
                                }
                            }
                        }
                    }
                }

                // 处理RetPseudo的ABI约束（作为Move到返回寄存器）
                if let MInst::RetPseudo { ret } = inst {
                    if let ArgPair::RegArg { vreg, preg } = ret {
                        if vreg.is_virtual() {
                            match (
                                <XReg as IRCNode>::try_from_reg(*vreg),
                                <XReg as IRCNode>::try_from_reg(*preg),
                            ) {
                                (Some(v), Some(p)) => irc_x.add_move(p, v),
                                _ => {
                                    if let (Some(v), Some(p)) = (
                                        <FReg as IRCNode>::try_from_reg(*vreg),
                                        <FReg as IRCNode>::try_from_reg(*preg),
                                    ) {
                                        irc_f.add_move(p, v);
                                    }
                                }
                            }
                        }
                    }
                }

                // 处理RetCallPseudo的ABI约束（作为Move）
                if let MInst::RetCallPseudo { args, .. } = inst {
                    // 尾调用的参数处理与CallPseudo类似
                    for arg in args {
                        if let ArgPair::RegArg { vreg, preg } = arg {
                            if vreg.is_virtual() {
                                // 将ABI约束作为Move处理
                                match (
                                    <XReg as IRCNode>::try_from_reg(*vreg),
                                    <XReg as IRCNode>::try_from_reg(*preg),
                                ) {
                                    (Some(v), Some(p)) => irc_x.add_move(p, v),
                                    _ => {
                                        if let (Some(v), Some(p)) = (
                                            <FReg as IRCNode>::try_from_reg(*vreg),
                                            <FReg as IRCNode>::try_from_reg(*preg),
                                        ) {
                                            irc_f.add_move(p, v);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // [关键修复] 尾调用同样会破坏caller-saved寄存器
                    // 尾调用也需要遵守ABI契约
                    let defs = inst.get_defs();
                    for &live_reg in live.iter().filter(|r| !defs.contains(r)) {
                        if live_reg.is_virtual() {
                            // 处理整数寄存器
                            if let Some(live_vreg) = <XReg as IRCNode>::try_from_reg(live_reg) {
                                for &caller_saved in &CALLER_SAVED_XREGS {
                                    irc_x.add_edge(live_vreg, caller_saved);
                                }
                            }
                            // 处理浮点寄存器
                            else if let Some(live_vreg) =
                                <FReg as IRCNode>::try_from_reg(live_reg)
                            {
                                for &caller_saved in &CALLER_SAVED_FREGS {
                                    irc_f.add_edge(live_vreg, caller_saved);
                                }
                            }
                        }
                    }
                }

                // 判断是否是move指令
                let is_move = inst.as_move().is_some();

                // 为所有定义的寄存器添加干扰边
                for &def in &defs {
                    // 1. Def 干扰所有 live-out 寄存器
                    for &live_reg in &live {
                        // 只添加同类型寄存器之间的干扰边
                        match (def, live_reg) {
                            (Reg::VX(_) | Reg::X(_), Reg::VX(_) | Reg::X(_)) => {
                                if let (Some(def_x), Some(live_x)) = (
                                    <XReg as IRCNode>::try_from_reg(def),
                                    <XReg as IRCNode>::try_from_reg(live_reg),
                                ) {
                                    irc_x.add_edge(def_x, live_x);
                                }
                            }
                            (Reg::VF(_) | Reg::F(_), Reg::VF(_) | Reg::F(_)) => {
                                if let (Some(def_f), Some(live_f)) = (
                                    <FReg as IRCNode>::try_from_reg(def),
                                    <FReg as IRCNode>::try_from_reg(live_reg),
                                ) {
                                    irc_f.add_edge(def_f, live_f);
                                }
                            }
                            _ => {} // 不同类型的寄存器不冲突
                        }
                    }

                    // 2. [修复] 对于非move指令，Def还干扰所有Use寄存器
                    if !is_move {
                        for &use_reg in &uses {
                            // 只添加同类型寄存器之间的干扰边
                            match (def, use_reg) {
                                (Reg::VX(_) | Reg::X(_), Reg::VX(_) | Reg::X(_)) => {
                                    if let (Some(def_x), Some(use_x)) = (
                                        <XReg as IRCNode>::try_from_reg(def),
                                        <XReg as IRCNode>::try_from_reg(use_reg),
                                    ) {
                                        irc_x.add_edge(def_x, use_x);
                                    }
                                }
                                (Reg::VF(_) | Reg::F(_), Reg::VF(_) | Reg::F(_)) => {
                                    if let (Some(def_f), Some(use_f)) = (
                                        <FReg as IRCNode>::try_from_reg(def),
                                        <FReg as IRCNode>::try_from_reg(use_reg),
                                    ) {
                                        irc_f.add_edge(def_f, use_f);
                                    }
                                }
                                _ => {} // 不同类型的寄存器不冲突
                            }
                        }
                    }
                }

                // 更新活跃集合
                for &def in &defs {
                    live.remove(&def);
                }
                for &use_reg in &uses {
                    live.insert(use_reg);
                }
            }
        }

        // 设置初始节点集合
        irc_x.set_initial(initial_x);
        irc_f.set_initial(initial_f);

        Ok(())
    }

    /// 合并两个IRC的颜色映射
    fn combine_color_maps(&self, irc_x: &IRC<XReg>, irc_f: &IRC<FReg>) -> BTreeMap<Reg, Reg> {
        let mut final_map = BTreeMap::new();

        // 添加整数寄存器映射
        for (vreg, preg) in &irc_x.color_map {
            if vreg.is_virtual() {
                final_map.insert(vreg.to_reg(), preg.to_reg());
            }
        }

        // 添加浮点寄存器映射
        for (vreg, preg) in &irc_f.color_map {
            if vreg.is_virtual() {
                final_map.insert(vreg.to_reg(), preg.to_reg());
            }
        }

        final_map
    }

    /// 插入颜色分配的调试注释
    fn insert_color_comments(&mut self, color_map: &BTreeMap<Reg, Reg>) {
        // 在第一个块的开始插入颜色映射信息
        let entry_block = self.vfunc.cfg.entry();
        if let Some(block) = self.vfunc.block_datas.get_mut(&entry_block) {
            let mut comments = Vec::new();

            comments.push(MInst::Comment {
                label: "====== IRC Color Result ======".to_string(),
            });

            // 按虚拟寄存器ID排序输出
            let mut sorted_entries: Vec<_> = color_map.iter().collect();
            sorted_entries.sort_by_key(|(vreg, _)| format!("{}", vreg));

            for (vreg, preg) in sorted_entries {
                comments.push(MInst::Comment {
                    label: format!("  {} -> {}", vreg, preg),
                });
            }

            // comments.push(MInst::Comment {
            //     label: "======================".to_string(),
            // });

            // 在原有指令前插入注释
            let mut new_insts = comments;
            new_insts.extend(block.insts.clone());
            block.insts = new_insts;
        }
    }

    /// Color重映射
    fn rewrite_program(&mut self, color_map: &BTreeMap<Reg, Reg>) {
        struct ColorMapper<'a> {
            map: &'a BTreeMap<Reg, Reg>,
            pst_slot_map: &'a HashMap<Reg, SlotId>,
            stack: &'a StackLayout,
        }

        impl<'a> RegMapper for ColorMapper<'a> {
            fn map_reg(&mut self, reg: Reg) -> Reg {
                if reg.is_virtual() {
                    // vreg查找preg
                    self.map.get(&reg).cloned().unwrap_or_else(|| {
                        // 检查是否是传入栈参数的虚拟寄存器
                        // 这些寄存器在新设计下不会被使用（已被 Reload 的新寄存器替代）
                        if let Some(slot_id) = self.pst_slot_map.get(&reg) {
                            if let Some(slot_data) = self.stack.get_slotdata(*slot_id) {
                                if matches!(slot_data.kind, SlotKind::IncomingArg { .. }) {
                                    eprintln!("INFO: VREG {} 未着色, 但不必在意, 这是由于传入参数的栈上IncomingArg的vreg没有被使用, RetCall直接进行了 mem=>mem 内存移动", reg);
                                    return reg; // 返回原始寄存器，不会被实际使用
                                }
                            }
                        }
                        eprintln!("严重警告: VREG {} 未着色， 这已经是panic级别事件了", reg);
                        reg
                    })
                } else {
                    reg // 预着色preg不变 
                }
            }
        }

        let mut mapper = ColorMapper {
            map: color_map,
            pst_slot_map: &self.vfunc.pst_slot_map,
            stack: &self.vfunc.stack,
        };

        for block_data in self.vfunc.block_datas.values_mut() {
            for inst in &mut block_data.insts {
                inst.map_regs(&mut mapper);
            }

            // 可以废弃: 不过还是把块参数也一起更新了
            for param in &mut block_data.params {
                *param = mapper.map_reg(*param);
            }
        }

        // 更新Call ABI约束
        for arg in &mut self.vfunc.args {
            match arg {
                ArgPair::RegArg { vreg, .. } => {
                    *vreg = mapper.map_reg(*vreg);
                }
                ArgPair::StackArg { vreg, .. } => {
                    *vreg = mapper.map_reg(*vreg);
                }
                ArgPair::None => {}
            }
        }
    }
}
