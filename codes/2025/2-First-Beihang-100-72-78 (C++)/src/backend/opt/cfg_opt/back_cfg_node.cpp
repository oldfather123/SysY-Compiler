#include "backend/opt/cfg_opt/back_cfg_node.hpp"

namespace backend::opt::cfg_opt {

void BackCFGNode::connect(std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> &result,
                          backend::riscv::RVBasicBlock *src,
                          backend::riscv::RVBasicBlock *dst,
                          double prob) {
    // 获取源节点的CFG节点
    auto *src_node = result[src];
    // 获取目标节点的CFG节点
    auto *dst_node = result[dst];

    // 检查源节点是否已经有到目标节点的连接，如果有则累加概率
    if (src_node->suc.find(dst) != src_node->suc.end()) {
        src_node->suc[dst] += prob;
    } else {
        src_node->suc[dst] = prob;
    }

    // 检查目标节点是否已经有来自源节点的连接，如果有则累加概率
    if (dst_node->pre.find(src) != dst_node->pre.end()) {
        dst_node->pre[src] += prob;
    } else {
        dst_node->pre[src] = prob;
    }
}

}  // namespace backend::opt::cfg_opt
