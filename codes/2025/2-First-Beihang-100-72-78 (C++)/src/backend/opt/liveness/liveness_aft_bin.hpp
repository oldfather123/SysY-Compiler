#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "backend/opt/cfg_opt/back_cfg_node.hpp"
#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_instruction.hpp"
#include "backend/riscv/rv_module.hpp"
#include "backend/riscv/rv_operand.hpp"

namespace backend::opt::liveness {

class LivenessAftBin {
public:
    static std::unordered_map<backend::riscv::RVBasicBlock *, backend::opt::cfg_opt::BackCFGNode *> *cfg;
    static std::unordered_map<backend::riscv::RVBasicBlock *, std::unordered_set<backend::riscv::RVReg *>> block_in;
    static std::unordered_map<backend::riscv::RVBasicBlock *, std::unordered_set<backend::riscv::RVReg *>> block_out;
    static std::unordered_map<backend::riscv::RVBasicBlock *, std::unordered_set<backend::riscv::RVReg *>> block_use;
    static std::unordered_map<backend::riscv::RVBasicBlock *, std::unordered_set<backend::riscv::RVReg *>> block_def;
    static std::unordered_map<backend::riscv::RVInstruction *, std::unordered_set<backend::riscv::RVReg *>> out;

    static void run_on_func(backend::riscv::RVFunction *function);

private:
    static std::vector<backend::riscv::RVBasicBlock *> call_topo_sort_aft(backend::riscv::RVFunction *function);
    static void dfs(backend::riscv::RVBasicBlock *rb,
                    std::vector<backend::riscv::RVBasicBlock *> &res,
                    std::unordered_set<backend::riscv::RVBasicBlock *> &vis);
    static std::unordered_set<backend::riscv::RVBasicBlock *> call_ret_block(backend::riscv::RVFunction *function);
    static void clear();
};

}  // namespace backend::opt::liveness
