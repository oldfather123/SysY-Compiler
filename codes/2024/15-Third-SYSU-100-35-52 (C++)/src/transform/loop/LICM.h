#pragma  once

#include "loopAnalysis.h"
#include "loopUtils.h"
#include "domTreeAnalysis.h"
#include "CFGAnalysis.h"
#include "Function.h"

bool runLICM(FunctionPtr func, DominateAnalysisResult& dom, CFGAnalysisResult& cfg);

