#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class RemoveUselessCall {
private:
    ir::Module *module_;
    void run();
    
public:
    ~RemoveUselessCall() {

    }

    RemoveUselessCall(ir::Module *module) : module_(module) { run(); }
};

} // namespace middleend