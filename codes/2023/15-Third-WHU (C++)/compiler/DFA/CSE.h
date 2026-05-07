#ifndef CSE_H
#define CSE_H
#include "DataFlowAnalysis.h"

//公共子表达消除
void exp_elimination(DataFlowAnalysis* dfa, SymbolTable* sym, bool mir, map<string, vector<string>> &globals);
#endif