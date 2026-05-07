#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class ExpandInstruction {
private:
    CFG *cfg_;
    UseDefAnalysis *uda_;
    ir::Function *func_;
    void run();
    void max_min();

public:
    ~ExpandInstruction() {
        delete uda_;
    }

    ExpandInstruction(CFG *cfg) : cfg_(cfg) {
        func_ = cfg->get_func();
        uda_ = new UseDefAnalysis(cfg_);
        run();
    }
};

} // namespace middleend