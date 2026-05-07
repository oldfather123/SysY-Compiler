#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

class InductionVarSimplification {
private:
    CFG *cfg_;
    UseDefAnalysis *uda_;
    ir::Function *func_;
    bool run();
    
public:
    ~InductionVarSimplification() {
        delete uda_;
    }

    InductionVarSimplification(CFG *cfg) : cfg_(cfg) {
        func_ = cfg->get_func();
        uda_ = new UseDefAnalysis(cfg_);
        while(run()){
            delete uda_;
            uda_ = new UseDefAnalysis(cfg_);
        };
    }

};

} // namespace middleend