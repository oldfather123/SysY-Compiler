#include "backend/opt/cfg_opt/block_re_sort.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "backend/opt/cfg_opt/gen_cfg.hpp"

namespace backend::opt::cfg_opt {

void BlockReSort::block_sort(backend::riscv::RVModule *module) {
    // 遍历模块中的所有函数
    for (const auto &func_pair : module->get_functions()) {
        backend::riscv::RVFunction *function = func_pair.second;
        // 跳过外部函数
        if (function->is_external()) continue;
        optimize_block_layout(function);
    }
}

std::vector<int> BlockReSort::solve_pettis_hansen(const std::vector<int> &weights,
                                                  const std::vector<double> &freq,
                                                  const std::vector<BranchEdge> &edges) {
    int block_count = weights.size();

    // Stage1: chain decomposition
    std::vector<int> fa(block_count);
    std::vector<std::pair<int, std::vector<int>>> chains(block_count);

    // 初始化
    for (int idx = 0; idx < block_count; ++idx) {
        chains[idx] = std::make_pair(INT_MAX, std::vector<int>{idx});
        fa[idx] = idx;
    }

    // 构建边信息，按频率和概率排序
    std::vector<std::pair<BranchEdge, double>> edge_info;
    edge_info.reserve(edges.size());
    for (const auto &edge : edges) {
        edge_info.emplace_back(edge, freq[edge.source] * edge.prob);
    }

    // 按权重降序排序
    std::sort(
        edge_info.begin(), edge_info.end(), [](const auto &lhs, const auto &rhs) { return lhs.second > rhs.second; });

    // 构建图
    std::vector<std::vector<int>> graph(block_count);

    // 查找父节点的lambda函数
    std::function<int(int)> find_fa = [&fa, &find_fa](int u) -> int {
        if (fa[u] == u) return u;
        fa[u] = find_fa(fa[u]);
        return fa[u];
    };

    int p = 0;
    for (const auto &entry : edge_info) {
        const BranchEdge &e = entry.first;
        int u = e.source;
        int v = e.target;

        if (u == v) continue;

        graph[u].push_back(v);

        auto &pv_cv = chains[v];
        if (find_fa(v) != v) continue;

        int fu = find_fa(u);
        if (fu == v) continue;

        auto &pu_cu = chains[fu];
        if (pu_cu.second.back() != u) continue;

        pu_cu.first = std::min(std::min(pu_cu.first, pv_cv.first), ++p);
        pu_cu.second.insert(pu_cu.second.end(), pv_cv.second.begin(), pv_cv.second.end());
        fa[v] = fu;
    }

    // 确保入口块在开头
    if (find_fa(0) != 0) {
        assert(false && "Entry block not found");
    }

    // 使用优先队列处理工作列表
    std::priority_queue<int, std::vector<int>, std::function<bool(int, int)>> work_list(
        [&chains](int lhs, int rhs) -> bool { return chains[lhs].first > chains[rhs].first; });

    std::unordered_set<int> inserted;
    std::unordered_set<int> inserted_work_list;

    work_list.push(0);
    inserted_work_list.insert(0);

    std::vector<int> seq;
    seq.reserve(block_count);

    while (!work_list.empty()) {
        int k = work_list.top();
        work_list.pop();

        // 添加链中的所有块
        for (int u : chains[k].second) {
            if (inserted.insert(u).second) {
                seq.push_back(u);
            }
        }

        // 处理后继块
        for (int u : chains[k].second) {
            for (int v : graph[u]) {
                if (inserted.find(v) != inserted.end()) continue;

                int head = find_fa(v);
                if (inserted_work_list.insert(head).second) {
                    work_list.push(head);
                }
            }
        }
    }

    return seq;
}

void BlockReSort::optimize_block_layout(backend::riscv::RVFunction *func) {
    // 如果基本块数量少于等于2，不需要优化
    if (func->get_blocks().size() <= 2) return;

    // 计算控制流图
    auto cfg = GenCFG::calc_cfg(func);

    // 准备数据
    std::vector<int> weights;
    std::vector<BranchEdge> edges;
    std::unordered_map<backend::riscv::RVBasicBlock *, int> idx_map;
    std::vector<double> freq;

    // 计算块频率
    auto block_freq = GenCFG::call_freq(func, cfg);

    // 构建索引映射和权重
    int idx = 0;
    for (auto *block : func->get_blocks()) {
        idx_map[block] = idx++;
        weights.push_back(block->get_insts().size());
    }

    // 构建边和频率
    idx = 0;
    for (auto *block : func->get_blocks()) {
        int block_idx = idx++;
        freq.push_back(block_freq[block]);

        // 添加后继边
        for (const auto &pair : cfg[block]->suc) {
            edges.emplace_back(block_idx, idx_map[pair.first], pair.second);
        }
    }

    // 求解Pettis-Hansen算法
    std::vector<int> seq = solve_pettis_hansen(weights, freq, edges);

    // 确保入口块在开头
    if (seq[0] != 0) {
        assert(false && "Entry block is not at the beginning");
    }

    // 重新排列基本块

    // 保存原始的基本块顺序
    std::vector<backend::riscv::RVBasicBlock *> original_blocks;
    for (auto *block : func->get_blocks()) {
        original_blocks.push_back(block);
    }

    // 直接调用reorder_blocks方法重新排序
    func->reorder_blocks(seq, original_blocks);
}

}  // namespace backend::opt::cfg_opt
