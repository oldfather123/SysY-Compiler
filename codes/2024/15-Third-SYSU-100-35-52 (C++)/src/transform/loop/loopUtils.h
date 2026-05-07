
#pragma once
#include "CFGAnalysis.h"
#include "domTreeAnalysis.h"

bool collectLoopBody(BasicBlock* header, BasicBlock* latch, const DominateAnalysisResult& dom, const CFGAnalysisResult& cfg,
                     std::unordered_set<BasicBlock*>& body, bool allowInnerLoop, bool needSubLoop);