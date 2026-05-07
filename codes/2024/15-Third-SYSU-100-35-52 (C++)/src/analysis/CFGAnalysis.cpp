#include "CFGAnalysis.h"
#include "Function.h"
#include "Instruction.h"

// NOLINTBEGIN

const std::vector<BasicBlockPtr>& CFGAnalysisResult::predecessors(BasicBlockPtr block) const {
    return mInfo.at(block).predecessors;
}
const std::vector<BasicBlockPtr>& CFGAnalysisResult::successors(BasicBlockPtr block) const {
    return mInfo.at(block).successors;
}

CFGAnalysisResult runCFGAnalysis(FunctionPtr func) {  // NOLINT
    CFGAnalysisResult result;
    auto& storage = result.storage();
    for(const auto& bb : func->getBasicBlocks()) {
        auto& self = storage[bb.get()];
        const auto terminator = bb->getTerminator();
        if(terminator->isBranch()) {
            const auto branch = dynamic_cast<BrInstruction*>(terminator);
            const auto trueTarget = branch->getTrueTarget();
            const auto falseTarget = branch->getFalseTarget();
            self.successors.emplace_back(trueTarget);
            storage[trueTarget].predecessors.emplace_back(bb.get());
            if(falseTarget && falseTarget != trueTarget) {
                self.successors.emplace_back(falseTarget);
                storage[falseTarget].predecessors.emplace_back(bb.get());
            }
        }
    }
    for(auto& bb : func->getBasicBlocks()) {
        bb->predBasicBlocks.clear();
        bb->succBasicBlocks.clear();
        bb->predBasicBlocks = storage[bb.get()].predecessors;
        bb->succBasicBlocks = storage[bb.get()].successors;
    }
    assert(storage[func->getEntryBlock()].predecessors.empty());
    return result;
}

// NOLINTEND