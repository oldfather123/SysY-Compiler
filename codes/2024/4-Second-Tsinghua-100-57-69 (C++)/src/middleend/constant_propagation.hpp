#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"

namespace middleend {

class ConstantPropagation {
private:
    ir::Function *func_;
    std::unordered_map<int, ConstValue> constant_map; // temp -> constant value
    void run();
public:
    ConstantPropagation(ir::Function *func) : func_(func) { run(); }
};

}