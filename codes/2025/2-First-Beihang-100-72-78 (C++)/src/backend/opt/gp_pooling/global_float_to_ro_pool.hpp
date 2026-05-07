#pragma once

#include "backend/opt/gp_pooling/gp_pool.hpp"
#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_module.hpp"

namespace backend::opt::gp_pooling {

class GlobalFloatToRoPool {
public:
    static constexpr GPpool *g_ppool = nullptr;
    static void run(backend::riscv::RVModule *module);

private:
    static void run_on_block(backend::riscv::RVBasicBlock *block);
};

}  // namespace backend::opt::gp_pooling
