#pragma once

#include "Common.h"
#include "IR.h"
#include "LoopDetection.h"
#include "CFA.h"
#include "DFA.h"
#include "UseDefAnalysis.h"

namespace ir {
    struct LoopInvariantAnalysisContext {
        Ptr<ir::Loop> loop;
        Set<ir::Variable> invariantOperands{}; // Loop invariant operands
        Set<ir::InstPtr> invariantInsts{};     // Loop invariant instructions

        ir::CFAContext &cfaCtx;
        ir::DFAContext &dfaCtx;
        ir::UseDefAnalysisContext &useDefCtx;

        bool isInvariant(ir::InstPtr inst) {
            return invariantInsts.count(inst);
        }

        bool isInvariant(const ir::Variable &var) {
            return invariantOperands.count(var);
        }

        bool isInvariant(const ir::Value &value) {
            dbgassert(value.isLiteral() || value.isRegister(), "Invalid value");
            if (value.isLiteral()) {
                return true;
            } else {
                return isInvariant(ir::Variable::reg(value.getRegister()));
            }
        }

        LoopInvariantAnalysisContext(Ptr<ir::Loop> loop, ir::CFAContext &cfaCtx, ir::DFAContext &dfaCtx, ir::UseDefAnalysisContext &useDefCtx);
        LoopInvariantAnalysisContext(const LoopInvariantAnalysisContext &other) = delete;
    };
}
