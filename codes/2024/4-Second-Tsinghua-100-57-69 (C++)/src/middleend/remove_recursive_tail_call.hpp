#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"

namespace middleend {

class RemoveRecursiveTailCall {
private:
    ir::Function *func_;
    void run();
public:
    RemoveRecursiveTailCall(ir::Function *func) : func_(func) { run(); }
};

}