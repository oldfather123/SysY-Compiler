#include "backend/opt/back_loop/risc_loop.hpp"

namespace backend::opt::back_loop {

void RiscLoop::build_loops(backend::riscv::RVFunction *rf,
                           backend::riscv::RVFunction *mf,
                           std::unordered_map<backend::riscv::RVBasicBlock *, backend::riscv::RVBasicBlock *> &map) {
    // TODO: 构建循环
}

RiscLoop *RiscLoop::rec_build_loop(
    void *loop, std::unordered_map<backend::riscv::RVBasicBlock *, backend::riscv::RVBasicBlock *> &map) {
    // TODO: 递归构建循环
    return nullptr;
}

}  // namespace backend::opt::back_loop
