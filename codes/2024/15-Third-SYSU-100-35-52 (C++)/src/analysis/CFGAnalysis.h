#pragma once
#include <Block.h>
#include <Function.h>

struct BasicBlockCFGInfo final {
    std::vector<BasicBlockPtr> predecessors;
    std::vector<BasicBlockPtr> successors;
};

class CFGAnalysisResult final {
    std::unordered_map<BasicBlockPtr, BasicBlockCFGInfo> mInfo;

public:
    std::unordered_map<BasicBlockPtr, BasicBlockCFGInfo>& storage() {
        return mInfo;
    }
    const std::vector<BasicBlockPtr>& predecessors(BasicBlockPtr block) const;
    const std::vector<BasicBlockPtr>& successors(BasicBlockPtr block) const;
};

CFGAnalysisResult runCFGAnalysis(FunctionPtr func);