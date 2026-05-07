#pragma once

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_module.hpp"

namespace backend::opt::back_loop {

class LoopConstLift {
public:
    static void run(backend::riscv::RVModule *module);

private:
    static void run_on_func(backend::riscv::RVFunction *function);
    static bool check_on_loop(backend::riscv::RVFunction *function, void *loop);
    static void run_on_loop(void *loop);
    static void run_on_block_bef_ra(backend::riscv::RVBasicBlock *block);

    static int temp_cnt;
    static constexpr bool lift_flw_pool = true;
};

}  // namespace backend::opt::back_loop
