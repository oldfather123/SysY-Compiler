#include "backend/opt/memory_opt/reg_aft_extern_call_load_opt.hpp"

namespace backend::opt::memory_opt {

std::unordered_map<backend::riscv::RVReg *, backend::riscv::RVInstruction *> RegAftExternCallLoadOpt::last_load;

void RegAftExternCallLoadOpt::run(backend::riscv::RVModule *module) {
    // TODO: 运行外部调用后寄存器load优化
}

void RegAftExternCallLoadOpt::run_on_block(backend::riscv::RVBasicBlock *block) {
    // TODO: 在基本块上运行
}

}  // namespace backend::opt::memory_opt
