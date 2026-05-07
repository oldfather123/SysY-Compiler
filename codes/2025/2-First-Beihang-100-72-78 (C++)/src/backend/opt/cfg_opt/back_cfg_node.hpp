#pragma once

#include <unordered_map>

#include "backend/riscv/rv_basic_block.hpp"

namespace backend::opt::cfg_opt {

class BackCFGNode {
public:
    // pair后面跟的是概率
    std::unordered_map<backend::riscv::RVBasicBlock *, double> suc;
    std::unordered_map<backend::riscv::RVBasicBlock *, double> pre;

    static void connect(std::unordered_map<backend::riscv::RVBasicBlock *, BackCFGNode *> &result,
                        backend::riscv::RVBasicBlock *src,
                        backend::riscv::RVBasicBlock *dst,
                        double prob);
};

}  // namespace backend::opt::cfg_opt
