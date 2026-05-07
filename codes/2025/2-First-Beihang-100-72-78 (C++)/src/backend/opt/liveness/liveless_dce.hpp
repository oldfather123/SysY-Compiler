#pragma once

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_instruction.hpp"
#include "backend/riscv/rv_module.hpp"

namespace backend::opt::liveness {

class LivelessDCE {
public:
    static void run(backend::riscv::RVModule *module);
    static void run_on_func(backend::riscv::RVFunction *function);

private:
    static bool can_be_delete(backend::riscv::RVInstruction *inst);
};

}  // namespace backend::opt::liveness
