#include "PassBuilder.hpp"
#include "AllUnrolling.hpp"
#include "CSE.hpp"
#include "ConstProp.hpp"
#include "DeadCode.hpp"
#include "FuncInline.hpp"
#include "GVN.hpp"
#include "InstCombine.hpp"
#include "LICM.hpp"
#include "LoopDetection.hpp"
#include "LoopGepCombine.hpp"
#include "LoopRotate.hpp"
#include "LoopSimplify.hpp"
#include "Mem2Reg.hpp"
#include "PartiallyUnrolling.hpp"
#include "PassManager.hpp"
#include "SCEV.hpp"
#include "SimplifyCFG.hpp"
#include "TailRecurEliminate.hpp"

#include <iostream>
#include <sstream>

void PassBuilder::registerPass(const std::string &name, PassCreator creator) {
    passCreators_[name] = std::move(creator);
}

std::unique_ptr<Pass<Module>> PassBuilder::createPass(const std::string &name) {
    auto it = passCreators_.find(name);
    if (it != passCreators_.end()) {
        return it->second();
    }
    return nullptr;
}

void PassBuilder::registerAnalysisPassManager(
    std::shared_ptr<AnalysisPassManager> apm) {
#define ANALYSIS_PASS(PASS) apm->registerPass<PASS>();
#include "PassRegistry.def"

    apm_ = apm;
}

void PassBuilder::registerPassManager(std::shared_ptr<PassManager> pm) {
#define MODULE_TRANSFORM_PASS(NAME, PASS)                                      \
    registerPass(NAME, []() { return std::make_unique<PASS>(); });

#define FUNCTION_TRANSFORM_PASS(NAME, PASS)                                    \
    registerPass(NAME, []() {                                                  \
        return std::make_unique<PassAdapter<Function, Module>>(                \
            std::make_unique<PASS>());                                         \
    });

#include "PassRegistry.def"

    pm_ = pm;
}

template <typename... PassNames>
bool PassBuilder::collectPass(PassNames &&...passNames) {
    auto pm = pm_.lock();
    if (!pm) {
        std::cerr << "PassManager is not available\n";
        return false;
    }

    return (collectSinglePass(std::forward<PassNames>(passNames), pm) && ...);
}

bool PassBuilder::collectSinglePass(const std::string &passName,
                                    std::shared_ptr<PassManager> pm) {
    auto pass = createPass(passName);
    if (!pass) {
        std::cerr << "Pass " << passName << " not found\n";
        return false;
    }
    pm->addPass(std::move(pass));
    return true;
}

bool PassBuilder::parsePipeline(std::string pipeline, OptimizationLevel opt) {
    auto pm = pm_.lock();
    if (!pm) {
        std::cerr << "PassManager is not available\n";
        return false;
    }

    if (opt >= OptimizationLevel::O1) {
        collectPass("DeadCode", "SimplifyCFG");
        collectPass("Mem2Reg", "DeadCode", "ConstProp", "InstCombine",
                    "SimplifyCFG", "CSE");
        collectPass("GVN");
        collectPass("FuncInline");
        collectPass("LoopSimplify", "LICM",
                    "DeadCode"); // LICM creates empty blocks, bad
        collectPass("AllUnrolling", "DeadCode", "SimplifyCFG", "ConstProp",
                    "LoopGepCombine");
        collectPass("LoopSimplify", "LICM", "SimplifyCFG");
        collectPass("CSE", "ConstProp", "InstCombine");
        collectPass("TailRecurEliminate", "DeadCode");
        return true;
    }

    if (pipeline.empty()) {
        collectPass("SimplifyCFG", "Mem2Reg", "DeadCode");
        return true;
    }

    pipeline = pipeline + ",";
    std::istringstream iss(pipeline);
    std::string passName;
    while (std::getline(iss, passName, ',')) {
        collectPass(passName);
    }
    return true;
}
