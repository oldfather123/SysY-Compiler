#pragma once
#include "Function.h"
#include <Block.h>
#include <CFGAnalysis.h>
#include <Instruction.h>
#include <Loop.h>
#include <domTreeAnalysis.h>

struct LoopBodyInfo final {
    BasicBlock* loop;
    PhiInstruction* indvar;
    Value* bound;
    PhiInstruction* rec;
    bool recUsedByOuter;
    bool recUsedByInner;
    Value* recInnerStep;
    CallInstruction* recNext;
    BasicBlock* exit;
};


bool extractLoopBody(FunctionPtr func, Loop* loop, const DominateAnalysisResult& dom, const CFGAnalysisResult& cfg,
                     Module& mod, bool independent, bool allowInnermost, bool allowInnerLoop, bool onlyAddRec,
                     bool estimateBlockSizeForUnroll, bool needSubLoop, bool convertReduceToAtomic, bool duplicateCmp,
                     LoopBodyInfo* ret);