#pragma once
#ifndef BB_H
#define BB_H
#include"DAG.h"
#include"../DFA/BlockDataInfo.h"
using namespace std;

class BasicBlock;

// 边（Edge）类
class Edge {
public:
	BasicBlock* source;  // 边的起始基本块
	BasicBlock* target;  // 边的目标基本块

	Edge(BasicBlock* src, BasicBlock* tgt) : source(src), target(tgt) {}
};

class BasicBlock
{	
public:
	enum LabelType { NONE, WHILE_CONDITION, WHILEBODY,NO_USE,IF_BEGIN,IF_END,IF_END_AND_WHILEBODY };
	int id;//唯一标识基本块
	LabelType label;//标识特殊的基本块
	DAG* dag;
	Scope* scope;//当前基本块的作用域
	int LoopLevel = 0;

	BasicBlock(int id,Scope* scope) {
		this->id = id;
		this->label = LabelType::NONE;
		dag = new DAG();
		this->scope = scope;
		dataInfo = new BlockDataInfo();
	}

	BasicBlock(BasicBlock* block, Scope* scope) {
		this->id = 0;
		this->label = block->label;
		this->dag = new DAG();
		this->scope = scope;
		this->True_False_Exit = block->True_False_Exit;
		this->isCondition = block->isCondition;
		dataInfo = new BlockDataInfo();
	}

	vector<BasicBlock*> preBlock;//前驱
	vector<BasicBlock*> postBlock;//后继

	//给WHILE_CONDITION使用的
	pair<BasicBlock*, BasicBlock*> True_False_Exit;
	bool isCondition = false;

	BlockDataInfo* dataInfo;

	vector<DAGNode*> topoList;//拓扑排序的结果

	void pump() {
		cout << "BasicBlock" << id << endl;
		dag->pump();
		cout << endl << endl;
	}

};
#endif // !CHA_H
