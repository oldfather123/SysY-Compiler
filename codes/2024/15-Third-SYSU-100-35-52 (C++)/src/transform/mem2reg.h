#include <iostream>
#include <vector>
#include <unordered_map>
#include <queue>
#include <stack>
#include "Module.h"
#include "domTreeAnalysis.h"
#include "CFGAnalysis.h"

using namespace std;
void runMem2Reg(Module& mod);
void mem2reg(FunctionPtr func, DominateAnalysisResult& domResult);