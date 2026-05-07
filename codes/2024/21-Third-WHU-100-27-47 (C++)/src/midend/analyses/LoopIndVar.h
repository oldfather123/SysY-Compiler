#pragma once

#include "Common.h"
#include "IR.h"
#include "DFA.h"
#include "CFA.h"
#include "UseDefAnalysis.h"
#include "LoopDetection.h"
#include "LoopInvariantAnalysis.h"

namespace ir {

    struct LoopInductionVariableAnalysis {

        Ptr<LoopInvariantAnalysisContext> invariantCtx;
        Ptr<Loop> loop;
        Set<RegPtr> basicIndVar;
        Set<RegPtr> allIndVar;
        Map<RegPtr, Value> BIVStep;
        Map<RegPtr, Value> BIVIniVal;
        Map<RegPtr, Value> IVStep;
        Map<RegPtr, Value> IVIniVal;

        LoopInductionVariableAnalysis(Ptr<Loop> loop, CFAContext &cfaCtx, DFAContext &dfaCtx, UseDefAnalysisContext &useDefCtx) {
            invariantCtx = std::make_shared<LoopInvariantAnalysisContext>(loop, cfaCtx, dfaCtx, useDefCtx);
            findBasicIndVar(loop);
            findIndVar(loop);
        }

        bool fitPattern(Ptr<PhiInst> phiNode, Value val, Value destReg, Ptr<Loop> loop);
        void findBasicIndVar(Ptr<Loop> loop);
        void findIndVar(Ptr<Loop> loop);
        bool isIndVar(RegPtr reg) {
            return allIndVar.count(reg) > 0;
        }
        bool isBasicIndVar(RegPtr reg) {
            return basicIndVar.count(reg) > 0;
        }
        Value getStepOfBIV(RegPtr reg) {
            return BIVStep[reg];
        }
        Value getIniValOfBIV(RegPtr reg) {
            return BIVIniVal[reg];
        }
        Value getStepOfIV(RegPtr reg) {
            return IVStep[reg];
        }
        Value getIniValOfIV(RegPtr reg) {
            return IVIniVal[reg];
        }
        Set<RegPtr> getBasicIndVar() {
            return basicIndVar;
        }
        Set<RegPtr> getAllIndVar() {
            return allIndVar;
        }
    };

    inline void testIndVar(ir::FuncPtr func) {
        dbgout << "test of induction variable start" << std::endl;
        CFAContext cfaCtx{func};
        LoopDetectionContext loopDetCtx{cfaCtx};
        DFAContext dfaCtx{func, DFAContext::LV | DFAContext::RD};
        UseDefAnalysisContext useDefCtx{dfaCtx};

        for (auto loop : loopDetCtx.loops()) {
            auto IV = LoopInductionVariableAnalysis(loop, cfaCtx, dfaCtx, useDefCtx);
            auto set = IV.getAllIndVar();
            dbgout << "induction variable :" << std::endl;
            for (auto a : set) {
                dbgout << a->toString() << std::endl;
            }

            auto basicIV = IV.getBasicIndVar();
            dbgout << "basic induction variable :" << std::endl;
            for (auto a : basicIV) {
                dbgout << a->toString() << std::endl;
                dbgout << "step : " << IV.getStepOfBIV(a).toString() << std::endl;
                dbgout << "init value : " << IV.getIniValOfBIV(a).toString() << std::endl;
            }

            auto allIV = IV.getAllIndVar();
            dbgout << "all induction variable :" << std::endl;
            for (auto a : allIV) {
                dbgout << a->toString() << std::endl;
                dbgout << "step : " << IV.getStepOfIV(a).toString() << std::endl;
                dbgout << "init value : " << IV.getIniValOfIV(a).toString() << std::endl;
            }
        }
    }
}
