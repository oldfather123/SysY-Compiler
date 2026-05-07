#pragma once

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_module.hpp"
#include "log.hpp"

namespace backend::opt::cfg_opt {

class SimplifyCFG {
public:
    static void run(backend::riscv::RVModule *module);

    // 此方法屏蔽所有一个仅有一个非条件跳转指令的块或者空块
    static bool redirect_goto(backend::riscv::RVFunction *func);
    static bool remove_unused_labels(backend::riscv::RVFunction *func);
    static bool conditional_to_unconditional(backend::riscv::RVFunction *func);
    static bool sfb_opt(backend::riscv::RVFunction *func);
    static bool reorder_branch(backend::riscv::RVFunction *func);
    static bool block_link_opt(backend::riscv::RVFunction *func);
    static void simplify_cfg(backend::riscv::RVFunction *func);
};

}  // namespace backend::opt::cfg_opt
