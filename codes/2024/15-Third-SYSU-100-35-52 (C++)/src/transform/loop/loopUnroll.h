#pragma  once
#include "loopUtils.h"
#include "loopAnalysis.h"
#include "CFGAnalysis.h"
#include "domTreeAnalysis.h"

bool runLoopUnroll(FunctionPtr func, LoopAnalysisResult& loopInfo, CFGAnalysisResult& cfg);