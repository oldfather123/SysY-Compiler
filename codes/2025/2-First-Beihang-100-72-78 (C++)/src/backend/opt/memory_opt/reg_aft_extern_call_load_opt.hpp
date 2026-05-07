#pragma once

#include <unordered_map>

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_instruction.hpp"
#include "backend/riscv/rv_module.hpp"
#include "backend/riscv/rv_operand.hpp"

namespace backend::opt::memory_opt {

class RegAftExternCallLoadOpt {
public:
    static void run(backend::riscv::RVModule *module);

private:
    static void run_on_block(backend::riscv::RVBasicBlock *block);
    static std::unordered_map<backend::riscv::RVReg *, backend::riscv::RVInstruction *> last_load;
};

}  // namespace backend::opt::memory_opt
