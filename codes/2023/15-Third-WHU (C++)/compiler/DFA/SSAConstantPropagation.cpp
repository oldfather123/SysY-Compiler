#include "DFA.h"
#include"CostantPropagation.h"

static map<string, pair<int,int>> intValue;//第一个Int表示是否NAC。非1表示不可用
static map<string, pair<int,float>>floatValue;
//static map<Symbol*, map<int,int>>intArrayValue;//map -1 的位置表示是否整个数组NAC
//static map<Symbol*, map<int,float>>floatArrayValue;

static ExpManager* expManager = new ExpManager();

static SymbolTable* stable;

static bool isChanged_constant = false;

//main函数初始化全局量，但是因为ssa没有处理全局，所以做不了
static void initGlobalValue(vector<Symbol*>& globalSymbols,const string& funcname)
{
	for (auto sym : globalSymbols) {
		if (sym->type == i32) {
			intValue[sym->name] = make_pair(1, sym->globalValue.iVal);
		}
		else if (sym->type == f32) {
			floatValue[sym->name] = make_pair(1, sym->globalValue.fVal);
		}
	}
}

static void travelDAG_Constant(DAGNode* node,DAG* dag,BasicBlock* block)
{
	for (auto rely : node->relyNodes) {
		travelDAG_Constant(rely, dag, block);
	}

	for (auto input : node->inputs) {
		travelDAG_Constant(input,dag,block);
	}
	
	if(node->nodeType ==DAGNodeType::STORE){
		if(node->inputs.size()<2)return;
		//暂时不考虑数组的情况
		if (node->inputs[0]->nodeType == DAGNodeType::VALUE_NODE) {
			//不考虑全局变量
			if (stable->isGlobal(node->inputs[0]->symbol))return;
			if (node->inputs[1]->nodeType == DAGNodeType::CONSTANT) {
				if (node->inputs[0]->resultType == ResultType::i32) {
					if (intValue[node->inputs[0]->symbol->name].first != 1) isChanged_constant = true;
					intValue[node->inputs[0]->symbol->name] = make_pair(1, node->inputs[1]->value.iValue);
				}
				else {
					if (floatValue[node->inputs[0]->symbol->name].first != 1) isChanged_constant = true;
					floatValue[node->inputs[0]->symbol->name] = make_pair(1, node->inputs[1]->value.fValue);
				}
			}
			////否则置为不确定
			//else {
			//	if (node->inputs[0]->resultType == ResultType::i32) {
			//		intValue[node->inputs[0]->symbol->name] = make_pair(0, node->inputs[1]->value.iValue);
			//	}
			//	else {
			//		floatValue[node->inputs[0]->symbol->name] = make_pair(0, node->inputs[1]->value.fValue);
			//	}
			//}
		}		
	}
	else if (node->nodeType == DAGNodeType::LOAD) {
		if (node->inputs[0]->nodeType == DAGNodeType::VALUE_NODE) {
			if (node->resultType == ResultType::i32) {
				pair<int, int>result = intValue[node->inputs[0]->symbol->name];
				if (result.first == 1) {
					isChanged_constant = true;
					Value value;
					value.iValue = result.second;
					expManager->transform_to_constantnode(dag, node, value);
				}
			}
			else {
				pair<int, float>result = floatValue[node->inputs[0]->symbol->name];
				if (result.first == 1) {
					isChanged_constant = true;
					Value value;
					value.fValue = result.second;
					expManager->transform_to_constantnode(dag, node, value);
				}
			}
		}
	}
	else if (node->nodeType == DAGNodeType::OP_NODE) {
		if (node->name == BRANCH_NODE) {
			if (node->inputs.size() == 1) {
				if (node->inputs[0]->nodeType == DAGNodeType::CONSTANT) {
					if (node->inputs[0]->resultType == i32) {
						if (node->inputs[0]->value.iValue != 0) {
							remove_postBlock(block, block->postBlock[1]);
							node->inputs.clear();
							node->targets.clear();
							node->targets.push_back(block->postBlock[0]->id);
						}
						else {
							remove_postBlock(block, block->postBlock[0]);
							node->inputs.clear();
							node->targets.clear();
							node->targets.push_back(block->postBlock[0]->id);
						}
					}
					else {
						if (node->inputs[0]->value.fValue != 0) {
							remove_postBlock(block, block->postBlock[1]);
							node->inputs.clear();
							node->targets.clear();
							node->targets.push_back(block->postBlock[0]->id);
						}
						else {
							remove_postBlock(block, block->postBlock[0]);
							node->inputs.clear();
							node->targets.clear();
							node->targets.push_back(block->postBlock[0]->id);
						}
					}
				}
			}
			return;
		}
		//不做处理
		else if(node->name == RETURN_NODE ||node->name == ARRAY_INIT_NODE || node->name == ARRAYSUB_NODE){}
		else {
			std::set<string>::iterator it =  expManager->ava_opSet.find(node->name);
			Value newValue;//节点的新值
			if (it == expManager->ava_opSet.end())//不在运算符表中
			{
				return;
			}

			if (node->inputs.size() == 2) {
				if (node->inputs[0]->nodeType == DAGNodeType::CONSTANT && node->inputs[1]->nodeType == DAGNodeType::CONSTANT) {
					isChanged_constant = true;
					newValue = expManager->folding_calculate_collection(node, node->inputs[0]->value, node->inputs[1]->value);
					expManager->transform_to_constantnode(dag, node, newValue);				
				}
				return;
			}
			else if (node->inputs.size() == 1) {
				if (node->inputs[0]->nodeType == DAGNodeType::CONSTANT) {
					isChanged_constant = true;
					newValue = expManager->folding_calculate_collection(node, node->inputs[0]->value);
					expManager->transform_to_constantnode(dag, node, newValue);
				}
			}
		}
	}
}

static void travelBasicBlock_Constant(BasicBlock* block)
{
	//visited[block->id] = 1;
	travelDAG_Constant(block->dag->rootNode,block->dag,block);

	/*for (auto tblock : block->postBlock) {
		if(visited[tblock->id]!=1)
			travelBasicBlock_Constant(tblock, visited);
	}*/
}

void travelArraySubDag(DAG* dag,DAGNode* node)
{
	if (node->name == ARRAYSUB_NODE) {
		if (node->inputs.size() == 2) {
			if (node->inputs[1]->nodeType == CONSTANT) {
				if (node->outputs[0]->nodeType == DAGNodeType::LOAD || node->outputs[0]->nodeType == DAGNodeType::STORE) {
					node->outputs[0]->value.iValue = node->inputs[1]->value.iValue;
					dag->remove_edge(node, node->inputs[1]);
				}	
			}
			else if(node->inputs[1]->inputs.size()==2 && node->inputs[1]->nodeType == DAGNodeType::OP_NODE&& node->inputs[1]->name == PLUS) {
				if (node->inputs[1]->inputs[1]->nodeType == DAGNodeType::CONSTANT) {
					if (node->outputs[0]->nodeType == DAGNodeType::LOAD || node->outputs[0]->nodeType == DAGNodeType::STORE) {
						node->outputs[0]->value.iValue = node->inputs[1]->inputs[1]->value.iValue;
						auto temp = node->inputs[1]->inputs[0];
						dag->remove_edge(node->inputs[1], node->inputs[1]->inputs[0]);
						dag->remove_edge(node, node->inputs[1]);
						temp->outputs.clear();
						node->inputs.push_back(temp);
						temp->outputs.push_back(node);
					}	
				}
			}
		}
	}

	for (auto rely : node->relyNodes) {
		travelArraySubDag(dag,rely);
	}

	for (auto input : node->inputs) {
		travelArraySubDag(dag,input);
	}	
}

void travelArraySubBlock(map<BasicBlock*,int>& visited,BasicBlock* block)
{
	visited[block] = 1;
	travelArraySubDag(block->dag,block->dag->rootNode);

	for (auto tblock : block->postBlock) {
		if (visited[tblock] != 1) {
			travelArraySubBlock(visited, tblock);
		}
	}
}

void SSA_ConstantPropagation(SymbolTable* table,CFG* cfg,DataFlowAnalysis* dfa)
{
	intValue.clear();
	floatValue.clear();

	//处理不了全局的问题，有一个办法就是可以简单的处理一下entry块的全局赋值*******
	//vector<Symbol*> globalSymbols = table->getGlobalSymbol();
	//if(cfg->name == "main")
		//initGlobalValue(globalSymbols,cfg->name);
	stable = table;

	map<int, int>visited;
	//扫多遍
	do {
		isChanged_constant = false;
		for (auto block : cfg->basicBlocks) {
			travelBasicBlock_Constant(block);
		}
	} while (isChanged_constant);
	
	cp_in_block(dfa);

	map<BasicBlock*, int>visitedblock;
	travelArraySubBlock(visitedblock,cfg->entry);
	/*for (int i = 0;i < 2;i++) {
		visited.clear();
		travelBasicBlock_Constant(cfg->entry, visited);
	}*/
}
