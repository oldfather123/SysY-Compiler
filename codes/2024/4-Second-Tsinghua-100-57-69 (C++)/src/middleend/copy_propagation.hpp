#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/use_def_analysis.hpp"

namespace middleend {

void copy_propagation(UseDefAnalysis *ud, Temp *dst, Temp *src);
class CopyPropagation {
private:
    CFG *cfg_;
    UseDefAnalysis *ud_;
    ir::Function *func_;
    std::unordered_map<int, Temp*> copy_map; // temp index -> temp
    std::unordered_map<std::string, Temp *> name_map_;
    void run();
public:
    void address_resolve();
    ~CopyPropagation() {
        delete cfg_;
    }
    CopyPropagation(ir::Function *func) : func_(func) { run(); }
};

}