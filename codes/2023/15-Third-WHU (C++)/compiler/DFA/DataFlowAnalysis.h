#pragma once
#include "ExpManager.h"
#include "AvaExp.h"
#include"BlockManager.h"
#include <unordered_set>
#include <unordered_map>
using namespace std;

class DataFlowAnalysis
{
public:

	CFG* cfg;

	int blockNum;

	//为到达定值服务
	map<string, vector<unsigned int>> def_varMap;//变量与定值的关联
	unsigned int def_count = 0;
	vector<string> def_var;

	//为活跃变量服务
	vector<string> live_varsSet;
	map<string, ResultType> live_typeMap;

	unordered_set<string> array_set;
	//寄存器分配
	unordered_set<string> cannot_spill;
	unordered_set<string> max_cost;
	unordered_map<string, int> name_num;

	//可用表达式
	//vector<AvaExp*> ava_expSet;
	map<long, string> reg_exp;
	map<string, int> ava_exp_index;
	int ava_count;
	vector<string> ava_exp_vec;//可用表达式
	set<string> ava_exp_set;//上面vector的副本，便于查找

	map<string, Symbol> name_sympol_map;//名字和符号表的对应
	/*可用表达式操作符合集*/
	unordered_set<string> ava_opSet = { PLUS,SUBTRACT,MULTIPLY,DIVIDE,GREATER_THAN,GREATER_EQUAL,
		LESS_THAN,LESS_EQUAL,EQUAL,NOT_EQUAL,LOGICAL_NOT,LOGICAL_AND,LOGICAL_OR,REMAINDER ,ARRAYSUB_NODE };

	unordered_set<string> single_opSet = { LOGICAL_NOT,INT_TO_FLOAT,FLOAT_TO_INT };
	//判断是不是函数调用
	unordered_set<string> CommonOPNameVec = { PLUS ,SUBTRACT,MULTIPLY,DIVIDE,GREATER_THAN,GREATER_EQUAL ,LESS_THAN ,LESS_EQUAL,EQUAL,NOT_EQUAL ,
										LOGICAL_NOT,LOGICAL_AND,LOGICAL_OR,REMAINDER,ARRAY_INIT_NODE,IMPLICIT_INIT_NODE,
		ARRAYSUB_NODE,INT_TO_FLOAT ,FLOAT_TO_INT,ARRAY_TO_POINT };

	DataFlowAnalysis(CFG* c) :cfg(c) { Init(); }

	//主要是对基本块中的DAG进行拓扑排序
	void Init() {
		blockNum = cfg->basicBlocks.size();
		/*for (int i = 0; i < blockNum; i++) {
			topologicalSort(cfg->basicBlocks[i]);
		}*/
		//在可用表达式的分析算法中，要将entry块的out单独输出为0；
		cfg->entry->dataInfo->AvaInSet.reset();
	}

	//拓扑排序
	void topologicalSort();
	void topologicalSort(BasicBlock* currentBlock);
	void topo_dfs(set<DAGNode*>& temp, vector<DAGNode*>& list, DAGNode* node);


//实现gen和kill集的实现
void rd_traversal();//到达定值

void lv_traversal();//活跃变量
void lv_traversal(bool ig);

void ae_traversal(SymbolTable*& sym);//可用表达式

void ae_traversal(bool mir);

//int isContainExp(AvaExp* exp);

int get_exp_index(string exp);

//void fill_operand(AvaExp* exp, DAGNode* input, int index);

//设置基本块在到达定值分析时的输入
void setDefIn(BasicBlock* b) {
	b->dataInfo->defInSet.reset();
	for (unsigned int i = 0; i < b->preBlock.size(); i++) {
		b->dataInfo->defInSet |= b->preBlock[i]->dataInfo->defOutSet;
	}
}
//设置基本块在活跃变量分析时的输出（逆向分析）
void setLiveOut(BasicBlock* b) {
	b->dataInfo->liveOutSet.reset();
	for (unsigned int i = 0; i < b->postBlock.size(); i++) {
		b->dataInfo->liveOutSet |= b->postBlock[i]->dataInfo->liveInSet;
	}
}

//设置可用表达式的输入
void setAvaIn(BasicBlock* b) {
	if (b->preBlock.size() > 0) {
		b->dataInfo->AvaInSet = b->preBlock[0]->dataInfo->AvaOutSet;
	}
	for (unsigned int i = 1; i < b->preBlock.size(); i++) {
		b->dataInfo->AvaInSet &= b->preBlock[i]->dataInfo->AvaOutSet;
	}
}

//到达定值分析
void reachingDefinationAnalysis();

//活跃变量分析
void liveVariablesAnalysis(bool ig);

//可用表达式分析
void availableExpressionsAnalysis(SymbolTable*& sym, bool mir);

void print(string s) {
	//cout << s << endl;
}

};

