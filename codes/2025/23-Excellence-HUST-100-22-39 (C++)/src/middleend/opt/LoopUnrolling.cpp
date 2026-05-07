#include "LoopUnrolling.hpp"

bool LoopUnrolling::can_unroll(LoopPtr loop) {
    // TODO: 检测逻辑
    return false;
}

void LoopUnrolling::do_unroll(LoopPtr loop) {
    // TODO: 实际进行展开
}

void LoopUnrolling::execute() {
    for(auto& [func, loops]: loops) {
        for(auto loop: *loops) {
            if(can_unroll(loop)) {
                // TODO
            }
        }
    }
}