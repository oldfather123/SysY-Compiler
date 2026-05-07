#include "GCFG.h"

static void travelDAG_Stack(DAGNode* node)
{
	if (node->nodeType == DAGNodeType::VALUE_NODE) {
		if (node->symbol->type == ResultType::iarray || node->symbol->type == ResultType::farray) {
			node->symbol->isUsed = true;
		}
	}

	for (DAGNode* dagnode : node->relyNodes) {
		travelDAG_Stack(dagnode);
	}

	for (DAGNode* dagnode : node->inputs) {
		travelDAG_Stack(dagnode);
	}
}

static void travelBlock_Stack(CFG* cfg) 
{
	for (BasicBlock* block : cfg->basicBlocks) {
		travelDAG_Stack(block->dag->rootNode);
	}
}

static int caculateStack(int currentOffset,map<string, pair<int, int>>& arrayStack, Scope* scope) {
	int current = currentOffset;
	for (auto it : scope->Symbols) {
		if (it.second->isUsed) {
			int sum = 1;
			for (int i : it.second->dimensions) {
				sum *= i;
			}
			arrayStack[it.first] = make_pair(current, sum);
			current += sum;
		}
	}
	int max = current;
	for (Scope* child : scope->childs) {
		int t_max = caculateStack(current, arrayStack, child);
		if (t_max > max)max = t_max;
	}
	return max;
}

//获取数组栈
void GCFG::getArrayStack(SymbolTable* symbolTable) 
{
	for (auto it : controlFlow) {
		travelBlock_Stack(it.second);
	}

	for (auto it : controlFlow) {
		Scope* funcScope = symbolTable->getFuncScope(it.first);
		int max = caculateStack(0, it.second->arrayStack, funcScope);
		it.second->maxStackSize = max;
	}
}