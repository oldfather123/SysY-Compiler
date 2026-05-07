#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class RemoveUselessMemory {
private:
    CFG *cfg_;
    UseDefAnalysis *uda_;
    ir::Function *func_;
    bool run();

public:
    ~RemoveUselessMemory() {
        delete uda_;
    }

    RemoveUselessMemory(CFG *cfg) : cfg_(cfg) {
        func_ = cfg->get_func();
        uda_ = new UseDefAnalysis(cfg_);

        while(run());
    }
};

} // namespace middleend