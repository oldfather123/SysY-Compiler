#include "CostantPropagation.h"
#include "DCE.h"
#include "CSE.h"
#include "../RISC_V/RegisterSpill.h"
#include "DFA.h"
#include "StackFrameExtension.h"
#include "LoopOpimization.h"
#include "LoopInduction.h"

DFA::DFA(GCFG* gcfg, SymbolTable* s) {
	this->gcfg = gcfg;
	sym = s;
	cfg_dfa.clear();
	for (pair<string, CFG*> iter : gcfg->controlFlow) {
		iter.second->cfg_traversal();
		DataFlowAnalysis* newDFA = new DataFlowAnalysis(iter.second);
		cfg_dfa[iter.second] = newDFA;
		igs[iter.second] = new IGManager(newDFA);
	}
}

void DFA::dfa_topo() {
	for (auto it = cfg_dfa.begin(); it != cfg_dfa.end(); it++) {
		it->second->topologicalSort();
		stack_frame_extension(it->second, sym);
	}
}

//Ż
void DFA::pass(map<string, vector<string>> cfg_globals, int Opt) {
	if (Opt) {
		for (auto it = cfg_dfa.begin(); it != cfg_dfa.end(); it++) {
			SSA_ConstantPropagation(sym, it->second->cfg,it->second);
			LoopInduction* loopinduction = new LoopInduction(it->second->cfg, it->second, sym);
			loopinduction->forward_replace();
			//constant_propagation_folding(it->second, cfg_globals);
			eliminate_dead_code(it->second, sym);
		}
		LoopOptManager* loopManager = new LoopOptManager();
		loopManager->LICM_OPT(this->gcfg, this, this->sym);

		for (auto it = cfg_dfa.begin(); it != cfg_dfa.end(); it++) {
			exp_elimination(it->second, sym, 0, cfg_globals);
		}
	}
	else {
		for (auto it = cfg_dfa.begin(); it != cfg_dfa.end(); it++) {
			SSA_ConstantPropagation(sym, it->second->cfg,it->second);
			//constant_propagation_folding(it->second, cfg_globals);
			eliminate_dead_code(it->second, sym);
		}
	}

}


void DFA::mir_pass(map<string, vector<string>> cfg_globals, int Opt) {
	if (!Opt) return;
	for (auto it = cfg_dfa.begin(); it != cfg_dfa.end(); it++) {
		exp_elimination(it->second, sym, 1,cfg_globals);
	}
}
