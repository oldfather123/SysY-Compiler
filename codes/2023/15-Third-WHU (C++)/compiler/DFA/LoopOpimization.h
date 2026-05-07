#pragma once
#include"DFA.h"
#include<algorithm>

enum ConditionType { C_NONE, GREATER, GREATER_E, LESS, LESS_E, E_EQUAL, N_EQUAL };
enum LoopUnrollType{NONE_UNROLL,ALL_UNROLL,PART_UNROLL};
typedef struct Boarder {
	ResultType type;
	int ival;
	float fval;
}Boarder, InitialVal;

typedef struct LoopStep {
	ResultType type = i32;
	int istep = 0;
	float fstep = 0;
}LoopStep;

//循环树数据结构
typedef struct LoopTreeNode {
	BasicBlock* block;//while_condition条件基本块
	LoopTreeNode* parent;
	vector<LoopTreeNode*>childs;
	bool canUnroll;
	bool isUnRolled;

	int times = 0;//循环次数
	LoopStep step;//步长
	ConditionType conditionType = ConditionType::C_NONE;//条件
	InitialVal initialVal;//初始值
	Boarder boarderVal;//边界

	int childTimes = 1;

	Symbol* changableVar = nullptr;
	LoopTreeNode(BasicBlock* block, LoopTreeNode* parent) : block(block), parent(parent),
		canUnroll(false), isUnRolled(false) {
		if (parent)parent->childs.push_back(this);
	};
}LoopTreeNode;

class LoopOptManager {
public:

	//cfg对应循环树
	map<string, LoopTreeNode*> LoopTrees;

	//一个函数对应的DFS线性化循环节点
	vector<LoopTreeNode*>DFSLoopTrees;

	//求循环树
	void getLoopTree(GCFG* gcfg);
	//一个函数的循环树线性化(不加入根结点)
	void getDFSLoopTrees(const string& name);
	//获取所有基本块的循环层级
	void getLoopLevel(GCFG* gcfg);

	void LICM_OPT(GCFG* gcfg,DFA* dfa,SymbolTable* table);
	void LoopUnrolling(GCFG* gcfg, SymbolTable* symbolTable);

};