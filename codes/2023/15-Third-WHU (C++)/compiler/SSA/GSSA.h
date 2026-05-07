#include "SSA.h"
#include "../IR/BasicBlock.h"
#include "../IR/CFG.h"
#include "../IR/DAG.h"
#include "../IR/GCFG.h"
#include "../Frontend/SymbolTable.h"



#include<vector>
#include<map>


class GSSA
{
public:
	GSSA(GCFG* g,SymbolTable* s) :gcfg(g),symboltable(s) {
		for (pair<string,CFG*> iter : g->controlFlow) {
			gssa[iter.first] = new SSA(iter.second,iter.first,symboltable);
		}
	}

	~GSSA();

	void GSSA_WORK() {
		for (pair<string, SSA*> iter : gssa) {
			iter.second->SSA_WORK();
		}
	}

	void GSSA_DESTROY() {
		for (pair<string, SSA*> iter : gssa) {
			iter.second->destroy2();
		}
	}


	void GSSA_TOPOSORTING() {
		for (pair<string, SSA*> iter : gssa) {
			iter.second->toposorting();
		}
	}

	void GSSA_PUMP() {
		for (pair<string, CFG*> iter : gcfg->controlFlow) {
			iter.second->pump();
		}
	}

	vector<string> GSSA_PHIINFO(CFG* cfg, BasicBlock* b) {
		return gssa[cfg->name]->phivar_rec(b);
	}

private:
	GCFG* gcfg;
	SymbolTable* symboltable;
	map<string, SSA*> gssa;
};

