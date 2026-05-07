#include "backend/opt/back_loop/loop_const_lift.hpp"

namespace backend::opt::back_loop {

int LoopConstLift::temp_cnt = 0;

void LoopConstLift::run(backend::riscv::RVModule *module) {
    // TODO: 实现循环不变量外提
}

void LoopConstLift::run_on_func(backend::riscv::RVFunction *function) {
    // TODO: 在函数尺度新建中转块
}

bool LoopConstLift::check_on_loop(backend::riscv::RVFunction *function, void *loop) {
    // TODO: 检查循环
    return false;
}

void LoopConstLift::run_on_loop(void *loop) {
    // TODO: 在循环上运行
}

void LoopConstLift::run_on_block_bef_ra(backend::riscv::RVBasicBlock *block) {
    // TODO: 在寄存器分配前在基本块上运行
}

}  // namespace backend::opt::back_loop
