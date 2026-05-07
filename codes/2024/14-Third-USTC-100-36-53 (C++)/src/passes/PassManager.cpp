#include "PassManager.hpp"

template<>
void PassAdapter<Function, Module>::run(Module *item, AnalysisPassManager& APM) {
    for (auto &func : item->get_functions()) {
        if (func.is_declaration())
            continue;
        pass_->run(&func, APM);
    }
}