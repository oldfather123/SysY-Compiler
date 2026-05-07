#pragma once
#ifndef RegisterAllocate_H
#define RegisterAllocate_H

#include "../IR/DAG.h"
#include "../DFA/InterferenceGraph.h"
#include "../DFA/DFA.h"

//Õ»¼Ä´æÆ÷·ÖÅä
//void stackRegisterAllocate(std::vector<DAGNode*> &linearDAG, std::map<Symbol*, int> &m, int &n);

//Í¼×ÅÉ«¼Ä´æÆ÷·ÖÅä
void graphColoringRegisterAllocation(GCFG* gcfg, DFA* dfa, SymbolTable* globalsymboltable);

#endif