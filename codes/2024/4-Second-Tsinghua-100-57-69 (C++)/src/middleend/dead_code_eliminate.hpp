#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"

namespace middleend {

class DeadCodeEliminate {
private:
    ir::Module *module_;
    void run();
    bool eliminate_useless_cf_one_pass(ir::Function* func);
public:
    DeadCodeEliminate(ir::Module *module) : module_(module) { run(); }
};

}