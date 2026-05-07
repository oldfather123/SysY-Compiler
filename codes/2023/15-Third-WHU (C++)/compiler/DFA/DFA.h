#pragma once
#include "../IR/GCFG.h"
#include "InterferenceGraph.h"
class DFA
{
public:
	GCFG* gcfg;
	map<CFG*, DataFlowAnalysis*> cfg_dfa;
	map<CFG*, IGManager*> igs;
	SymbolTable* sym;

	DFA(GCFG* gcfg, SymbolTable* s);

	void dfa_topo();
	//”≈ªØ
	void pass(map<string, vector<string>> cfg_globals, int Opt);

	void mir_pass(map<string, vector<string>> cfg_globals, int Opt);
};