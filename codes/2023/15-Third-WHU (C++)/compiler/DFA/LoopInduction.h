#pragma once
#include "DFA.h"

class LoopInduction
{
public:

	map<string, DAGNode*> var_assign;
	CFG* cfg;
	DataFlowAnalysis* dfa;
	SymbolTable* symboltable;
	LoopInduction(CFG* ccfg, DataFlowAnalysis* ddfa,SymbolTable* s):cfg(ccfg),dfa(ddfa),symboltable(s){
		dfa->topologicalSort();
	}


	void varassign_detect(DAGNode* n,DAGNode* last);
	DAGNode* struct_copy(DAGNode* n);

	void forward_replace();

	

};