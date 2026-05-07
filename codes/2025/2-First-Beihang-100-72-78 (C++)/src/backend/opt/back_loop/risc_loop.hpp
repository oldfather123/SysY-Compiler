#pragma once

#include <unordered_set>
#include <vector>

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"

namespace backend::opt::back_loop {

class RiscLoop {
public:
    std::unordered_set<backend::riscv::RVBasicBlock *> blocks;
    std::unordered_set<backend::riscv::RVBasicBlock *> enterings;
    std::unordered_set<RiscLoop *> sub_loops;

    static void build_loops(backend::riscv::RVFunction *rf,
                            backend::riscv::RVFunction *mf,
                            std::unordered_map<backend::riscv::RVBasicBlock *, backend::riscv::RVBasicBlock *> &map);

private:
    static RiscLoop *rec_build_loop(
        void *loop, std::unordered_map<backend::riscv::RVBasicBlock *, backend::riscv::RVBasicBlock *> &map);
};

}  // namespace backend::opt::back_loop
