#pragma once

#include "Common.h"
#include "IR.h"

namespace ir {
    struct PureFunctionAnalysisContext {
        Map<ir::FuncPtr, Set<ir::FuncPtr>> callerToCallees{};
        Map<ir::FuncPtr, Set<ir::FuncPtr>> calleeToCallers{};
        Map<ir::FuncPtr, bool> isPureFunction{};

        void analyzePureness(ir::FuncPtr caller, Map<ir::FuncPtr, bool> &analyzed, Map<ir::FuncPtr, bool> &inStack);

        PureFunctionAnalysisContext(Ptr<ir::Module> module);
        PureFunctionAnalysisContext(const PureFunctionAnalysisContext &other) = delete;

        void printDebug() {
#ifdef DEBUG
            dbgout << "├── Functions pureness:" << std::endl;
            for (auto [func, isPure] : isPureFunction) {
                dbgout << "│   ├── " << func->name() << ": " << isPure << std::endl;
            }
#endif
        }
    };
}
