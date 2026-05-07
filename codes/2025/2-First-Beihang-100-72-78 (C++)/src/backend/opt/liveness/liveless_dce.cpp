#include "backend/opt/liveness/liveless_dce.hpp"

namespace backend::opt::liveness {

void LivelessDCE::run(backend::riscv::RVModule *module) {
    // TODO: 运行无活跃性死代码消除
}

void LivelessDCE::run_on_func(backend::riscv::RVFunction *function) {
    // TODO: 在函数上运行
}

bool LivelessDCE::can_be_delete(backend::riscv::RVInstruction *inst) {
    // TODO: 检查是否可以删除
    return false;
}

}  // namespace backend::opt::liveness
