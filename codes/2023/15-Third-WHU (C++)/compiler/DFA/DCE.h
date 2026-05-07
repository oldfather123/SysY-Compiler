#ifndef DCE_H
#define DCE_H
#include "DataFlowAnalysis.h"

void mark(DataFlowAnalysis* dfa, SymbolTable* sym);
bool sweep(DataFlowAnalysis* dfa);

//ËÀ´úÂëÏû³ı
void eliminate_dead_code(DataFlowAnalysis* dfa, SymbolTable* sym);
#endif 