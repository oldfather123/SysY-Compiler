#pragma once

#include "middleend/ir.hpp"

namespace middleend {

void mark_pure_functions(ir::Module *module);

void remove_useless_function(ir::Module *module);

class Optimizer {
public:
    bool check_assignment(ir::Module *module);
    bool check_use_def(ir::Module *module);
    Optimizer(ir::Module *module, bool o1);
};


} // namespace middleend