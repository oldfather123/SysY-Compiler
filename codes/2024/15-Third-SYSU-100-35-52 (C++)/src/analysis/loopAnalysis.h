#pragma once
#include "Loop.h"
#include "domTreeAnalysis.h"
#include "CFGAnalysis.h"
#include "patternMatch.h"

struct LoopAnalysisResult final {
    std::vector<Loop> loops;
};



LoopAnalysisResult runLoopAnalysis(FunctionPtr func, DominateAnalysisResult& dom);