#pragma once
#include "../IR/BasicBlock.h"
#include "../IR/CFG.h"
#include "../IR/DAG.h"
#include "../IR/GCFG.h"
#include "../Frontend/SymbolTable.h"


#include<algorithm>
#include<iostream>
#include<map>
#include<set>
#include<string>
using namespace std;


struct phi_info
{
	Symbol* name;
	vector<Symbol*> para;//只存名字，最后到符号表里找
	vector<BasicBlock*> branch;//用到name的块
};



class SSA {


public:
	//控制流图
	CFG* cfg;
	string cfgname;
	//符号表
	SymbolTable* symboltable;
	//头部节点
	BasicBlock*& entry;
	//函数参数块
	BasicBlock* parablock;
	Edge* paraedge;
	//所有基本块
	vector< BasicBlock* >& basicBlocks;
	//no_use标志
	vector<int> usetag;
	//所有基本块的pre
	vector<vector<BasicBlock*>> cfg_pre;
	



	//所有基本块的Dom
	//vector<vector<BasicBlock*>> cfg_dom;
	//所有基本块的IDom
	vector<BasicBlock*> cfg_idom;
	//支配边界
	//vector<vector<BasicBlock*>> DF;
	//支配者树后继节点
	map<BasicBlock*, vector<BasicBlock*>> Domtree_sons;


	map<BasicBlock*, set<BasicBlock*>> ccfg_dom;
	map<BasicBlock*, set<BasicBlock*>>  ccfg_DF;

	//块的%PHI函数变量原名字记录，不被改变
	map<BasicBlock*, vector<Symbol*>> phirec;
	//块的%PHI函数参数记录，第一个为被赋值的新变量名。paraiter中存了原变量名(key)，本块中应该取的新名字(name)，被填充的参数名字(para)，参数名来自的块(branch)
	map <BasicBlock*,map<Symbol*,phi_info>>phi_para;
	//块的def变量记录
	map<BasicBlock*, vector<Symbol*>> defvar;
	//SSA重建，把当前块的变量名当作key，value等待汇点回填。将phi_para的%PHI函数信息进行反函数操作，变成记录到每个块的赋值语句
	//重建后的语句结果：string2=string1
	map < BasicBlock*, map<Symbol*, Symbol*>> phi_des;
	//重建回的块，一个变量的副本有一个就行了
	map<BasicBlock*, map<long, DAGNode*>> copysrc_record;



	//全局的变量名字 
	vector<Symbol*> globalvar;
	//变量和用到它的块
	map<Symbol*, vector<BasicBlock*>> var_blocks;
	//每个变量的重命名计数
	map<string, int> var_count;
	//汇点将前面分支命名的变量pop
	map<Symbol*, vector<int>> var_stack;



	//map<string, long>& cse_exp_reg;//保证每个表达式
	//map<long, string>& cse_reg_exp;
	map<string, int> copy_count;




public:
	SSA(CFG* ccfg,string name,SymbolTable* s): 
		/*
		初始化cfg，并引用entry和基本块向量组，初始化pre向量组
		*/
		cfg(ccfg),
		cfgname(name),
		symboltable(s),
		entry(cfg->entry), 
		basicBlocks(cfg->basicBlocks)
	{	



		//处理函数参数，新增基本块
		preprocess();



		cfg->name = cfgname;

		toposorting();


		//cfg_dom = vector<vector<BasicBlock*>>(basicBlocks.size());
		cfg_idom = vector<BasicBlock*>(basicBlocks.size());
		//DF= vector<vector<BasicBlock*>>(basicBlocks.size());


	}


	//对runner节点查找在basicBlocks向量的下标
	int find_basicblocks_index(BasicBlock*& runner);



	//dom求交,返回在第三个参数
	//void and_dom(vector<BasicBlock*>& dom1, vector<BasicBlock*>& dom2, vector<BasicBlock*>& res);


	//计算支配者树的后继节点，idom的逆关系
	//void cal_domtree(BasicBlock* b, vector<BasicBlock*>& domtree_successor);


	//对index节点的df，加入basicblock节点，取并集
	//void addDF(int& index, BasicBlock*& basicblock);



	void preprocess();
	void postprocess();
	void find_NO_USE(BasicBlock* ava);
	void del_NO_USE();
	void cal_Dom();
	void cal_IDom();
	void cal_DF();


	void detect_var(BasicBlock* b,vector<Symbol*>& varkill,DAGNode* node);
	void place_phi();
	void var_rename(BasicBlock* b, DAGNode* node);
	void rename();
	void block_rename(BasicBlock* b);
	Symbol* newname(Symbol* s);
	Symbol* new_copyname(Symbol* s);
	void carryout_phi();
	void destroy();
	void insertcopies();
	void destroy2();



	void show();
	vector<string> phivar_rec(BasicBlock*);
	void toposorting();
	void topotravel(BasicBlock* b_iter, vector<BasicBlock*>& toporesult);

	void SSA_WORK() {

		de_while_preedge();
		cal_Dom();
		cal_IDom();
		add_while_preedge();
		cal_DF();
		//de_while_preedge();
		place_phi();
		rename();
		//cfg->pump();
		carryout_phi();
		postprocess();
		//destroy();
		//cfg->pump();
	};





	void add_while_preedge() {
		for (int b_index = 0; b_index < basicBlocks.size(); b_index++) {
			if (basicBlocks[b_index]->label == BasicBlock::WHILEBODY || basicBlocks[b_index]->label == BasicBlock::IF_END_AND_WHILEBODY) {
				basicBlocks[b_index]->postBlock[0]->preBlock.push_back(basicBlocks[b_index]);
			}
		}
	}
	void de_while_preedge() {
		for (int b_index = 0; b_index < basicBlocks.size(); b_index++) {
			if (basicBlocks[b_index]->label == BasicBlock::WHILEBODY || basicBlocks[b_index]->label == BasicBlock::IF_END_AND_WHILEBODY) {
				BasicBlock* temp_whilecondition = basicBlocks[b_index]->postBlock[0];
				temp_whilecondition->preBlock.erase(find(temp_whilecondition->preBlock.begin(),temp_whilecondition->preBlock.end(),basicBlocks[b_index]));
			}
		}
	}

};


