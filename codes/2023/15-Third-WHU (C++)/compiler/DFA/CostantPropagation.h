#ifndef ConstantPropagation_H
#define ConstantPropagation_H
#include "DataFlowAnalysis.h"
#include <queue>

//³£Á¿´«²¥ÕÛµþ
void constant_propagation_folding(DataFlowAnalysis* dfa, map<string, vector<string>> cfg_global);

void remove_postBlock(BasicBlock* pre, BasicBlock* post);

void SSA_ConstantPropagation(SymbolTable* table, CFG* cfg,DataFlowAnalysis* dfa);

void cp_in_block(DataFlowAnalysis* dfa);

//
#endif
