#pragma  once
#include "loopUtils.h"
#include "loopAnalysis.h"

bool runLoopEliminate(FunctionPtr func, DominateAnalysisResult& dom);