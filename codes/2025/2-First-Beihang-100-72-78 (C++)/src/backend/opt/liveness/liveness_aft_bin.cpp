#include "backend/opt/liveness/liveness_aft_bin.hpp"

namespace backend::opt::liveness {

std::unordered_map<backend::riscv::RVBasicBlock *, backend::opt::cfg_opt::BackCFGNode *> *LivenessAftBin::cfg = nullptr;

void LivenessAftBin::run_on_func(backend::riscv::RVFunction *function) {
    // TODO: 在函数上运行块内联后活跃性分析
}

std::vector<backend::riscv::RVBasicBlock *> LivenessAftBin::call_topo_sort_aft(backend::riscv::RVFunction *function) {
    // TODO: 调用拓扑排序
    return std::vector<backend::riscv::RVBasicBlock *>();
}

void LivenessAftBin::dfs(backend::riscv::RVBasicBlock *rb,
                         std::vector<backend::riscv::RVBasicBlock *> &res,
                         std::unordered_set<backend::riscv::RVBasicBlock *> &vis) {
    // TODO: 深度优先搜索
}

std::unordered_set<backend::riscv::RVBasicBlock *> LivenessAftBin::call_ret_block(
    backend::riscv::RVFunction *function) {
    // TODO: 调用返回块
    return std::unordered_set<backend::riscv::RVBasicBlock *>();
}

void LivenessAftBin::clear() {
    // TODO: 清理
}

}  // namespace backend::opt::liveness
