#pragma once

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_module.hpp"

namespace backend::opt::cfg_opt {

class BlockInline {
public:
    static void run(backend::riscv::RVModule *module);

private:
    static constexpr int MAX_LEN = 20;
    static bool pre_simplify(backend::riscv::RVFunction *func);
    static bool block_inline(backend::riscv::RVFunction *function);
};

}  // namespace backend::opt::cfg_opt
