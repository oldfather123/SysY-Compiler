#pragma once
#ifndef Linearize_H
#define Linearize_H

#include "../IR/DAG.h"
#include "../IR/BasicBlock.h"
#include "../IR/CFG.h"
#include "../IR/GCFG.h"
#include "../DFA/InterferenceGraph.h"
#include "../DFA/DFA.h"
#include<unordered_set>



class Linearize
{
public:

	CFG* cfg;
	IGManager* ig;
	//end块id=-1
	//BasicBlock* endblock;
	//节点名/寄存器编号到物理寄存器编号
	map<string, int> color_rec;
	//最终的线性化nodes
	vector<DAGNode*> linear_nodes;
	//记录每个块被跳转的次数
	//live为0表示没有br到块，可能是endbr删除，也可能是连续跳转以至于不可达（第一个为无条件跳转）
	map<int, int>  block_label_live;


	//记录块的id与label关系
	map<int, string> block_label;
	//记录label在最终nodes的位置
	//map<int, string> label_location;
	//在生成代码时出发打印label
	//unordered_set<int> label_trigger;


	int nodescount = 0;
	//先命名，再自增
	int blockcount = 0;
public:
	Linearize(CFG* ccfg, IGManager* igs) :cfg(ccfg), ig(igs) {

	}


	~Linearize();


	//总开关
	vector<DAGNode*> linearize_work();

	//把load变成的move单input变成双inputs
	//void loadmove_apart();


	//#################################
	//第一次遍历
	//#################################
	/*
	1.拆分有条件双分支的branch——>有条件单分支branch+无条件
	2.单输入的load——>双输入的OP_NODE的move节点，并进行合并
	3.return返回节点后面加branch到-1块的，并删除ret
	*/
	void first_travel();


	//#################################
	//需要递归/迭代/回溯/传递
	//#################################
	BasicBlock* find_idblock(int id);
	//跳转到/无条件跳转指令/的传递
	void pass_continue_branch();

	//删除不可达的块，块的第一条是跳转
	void del_unreachblock();

	//窥孔删除块尾无条件跳转
	void del_enduncon_branch();


	//#################################
	//第二次遍历
	//#################################
	/*
	1.删除move操作
	2.删除ret节点
	*/
	void second_travel();
	

	void third_travel();


	//块尾部的有条件+无条件可以被替换成一个有条件以及可以被消除的尾部无条件
	void change_doubleendbr();



	//合到一个nodes的vector
	void mergenodes();



	//删除无用ld/st保存指令
	//短生命周期
	void del_ldst();
	void del_ldst2();


	//addi指令的窥孔优化
	void mergeImmInstruction();






private:

};


class Linear {
public:
	map<CFG*, Linearize*> linear;

	Linear(GCFG* gcfg, DFA* dfa) {
		for (auto it = gcfg->controlFlow.begin(); it != gcfg->controlFlow.end(); it++) {
			linear[it->second] = new Linearize(it->second, dfa->igs[it->second]);
			linear[it->second]->linearize_work();
		}
	}
};





#endif