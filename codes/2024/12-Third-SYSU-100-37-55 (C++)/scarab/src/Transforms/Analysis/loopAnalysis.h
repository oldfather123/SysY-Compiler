#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <stack>
#include <sstream>

#include "Module.h"
#include "Loop.h"
#include "SCEV.h"


void mapSubloopsToParentLoop(LoopPtr loop, FunctionPtr func, stack<BasicBlockPtr>& BackedgeTo);
void computeExitBlocks(FunctionPtr func);
void computePreHeader(FunctionPtr func);
void computeLatch(FunctionPtr func);
void computeLoopIndVar(FunctionPtr func);
void computeLoopBB(FunctionPtr func);
void getLoopDepth(FunctionPtr func);
void loopAnalysis(FunctionPtr func);
