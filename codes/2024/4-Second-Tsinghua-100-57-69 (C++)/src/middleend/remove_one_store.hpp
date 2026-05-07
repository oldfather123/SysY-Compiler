#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class RemoveOneStore {
private:
    ir::Module *module_;
    void run();
public:
    RemoveOneStore(ir::Module *module) : module_(module) { run(); }
};

} // namespace middleend