#include "IRPasses.h"
#include "Mem2Reg.h"
#include "DCE.h"
#include "CopyProp.h"
#include "ConstFold.h"
#include "InstCombine.h"
#include "CFGSimplify.h"
#include "StrengthReduce.h"
#include "CSE.h"
#include "SSADestruct.h"
#include "FuncInline.h"
#include "LICM.h"
#include "LoopStrengthReduce.h"
#include "PureFunctionAnalysis.h"
#include "LoopIndVarAnalysis.h"
#include "GEP2Add.h"
#include "StoreProp.h"
using namespace ir;

void ir::runIRPasses(Ptr<Module> module, int optLevel) {

    auto spreadValues = [&](FuncPtr func, unsigned maxIter = INT_MAX) {
        FixedPoint{
            [&](bool &changed) {
                changed |= copyProp(func);
                changed |= constFold(func);
                if (optLevel >= 1) {
                    changed |= instCombine(func);
                }
            },
            true,
            "Spread Values",
            func->name(),
            maxIter,
        };
    };

    auto cseCombo = [&](FuncPtr func, unsigned maxIter = INT_MAX) {
        FixedPoint{
            [&](bool &changed) {
                changed |= cse(func);
                changed |= copyProp(func);
                changed |= dce(func);
            },
            true,
            "CSE Combo",
            func->name(),
            maxIter,
        };
    };

    dbgout
        << std::endl
        << "*** IR transforms start ***" << std::endl;

#ifdef DEBUG
    // optLevel = 0;
#endif
    if (optLevel == 0) {
        for (auto func : module->funcSet()) {
            if (func->isPrototype()) {
                continue;
            }
            mem2reg(func);
            gep2add(func);
            spreadValues(func, 3);
            dce(func, 1);
            cfgSimplify(func);
            dce(func, 1);
            strengthReduce(func, true);
            ssaDestruct(func);
        }
    } else {
        for (auto func : module->funcSet()) {
            if (func->isPrototype()) {
                continue;
            }
            mem2reg(func);
            gep2add(func);
            spreadValues(func);
            dce(func);
            cfgSimplify(func);
        }
        PureFunctionAnalysisContext{module};
        for (auto func : module->funcSet()) {
            if (func->isPrototype()) {
                continue;
            }
            cseCombo(func);
            storeProp(func);
            spreadValues(func);
            dce(func);
            licm(func);
            loopStrengthReduce(func);
            // dbgout << "CHECKPOINT:" << std::endl;    // DEBUG
            // dbgout << func->toString() << std::endl; // DEBUG
            cfgSimplify(func);
            strengthReduce(func);
            spreadValues(func);
            dce(func);
            cseCombo(func);
        }
        for (auto func : module->funcSet()) {
            if (func->isPrototype()) {
                continue;
            }
            if (funcInline(func, 10)) {
                spreadValues(func);
                dce(func);
                cfgSimplify(func);
                cseCombo(func);
                storeProp(func);
                spreadValues(func);
                dce(func);
                licm(func);
                loopStrengthReduce(func);
                cfgSimplify(func);
                strengthReduce(func);
                spreadValues(func);
                dce(func);
            }
        }
        for (auto func : module->funcSet()) {
            if (func->isPrototype()) {
                continue;
            }
            ssaDestruct(func);
            cfgSimplify(func);
            strengthReduce(func, true);
        }
    }

    dbgout << std::endl
           << "*** IR transforms end ***" << std::endl;
}
