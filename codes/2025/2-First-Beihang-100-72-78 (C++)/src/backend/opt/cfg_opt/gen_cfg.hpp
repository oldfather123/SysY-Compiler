#pragma once

#include <unordered_map>
#include <vector>

#include "backend/opt/cfg_opt/back_cfg_node.hpp"
#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_module.hpp"

namespace backend::opt::cfg_opt {

class GenCFG {
public:
    static constexpr double EPS = 1e-8;

    // 该方法是生成控制流图，图中包含转移的概率
    static std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> calc_cfg(backend::riscv::RVFunction *func);
    static void debug(const std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> &cfg);
    // 该方法会根据控制流图生成每一个块的执行概率
    static std::unordered_map<backend::riscv::RVBasicBlock *, double> call_freq(
        backend::riscv::RVFunction *func, std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> &cfg);

private:
    static std::vector<double> solve(int n, double *mat);
};

}  // namespace backend::opt::cfg_opt
