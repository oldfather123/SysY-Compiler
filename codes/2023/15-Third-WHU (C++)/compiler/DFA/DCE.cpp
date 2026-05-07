#include "DCE.h"
#include <unordered_map>

//死代码消除

unordered_set<string> used_var_set;//使用过的变量

//unordered_set<DAGNode*> useless_nodes;
unordered_map<DAGNode*, BasicBlock*> useless_nodes;

unordered_set<DAGNodeType> useless_nodeType = { REG_NODE,CONSTANT,VALUE_NODE };

//判断是不是函数调用和跳转
unordered_set<string> cse_op = { PLUS ,SUBTRACT,MULTIPLY,DIVIDE,GREATER_THAN,GREATER_EQUAL ,LESS_THAN ,LESS_EQUAL,EQUAL,NOT_EQUAL ,
									LOGICAL_NOT,LOGICAL_AND,LOGICAL_OR,REMAINDER,ARRAY_INIT_NODE,IMPLICIT_INIT_NODE,
	ARRAYSUB_NODE,INT_TO_FLOAT ,FLOAT_TO_INT,ARRAY_TO_POINT };

//拷贝函数
void copy_node(DAGNode* source, DAGNode* target) {
	source->name = target->name;
	source->resultType = target->resultType;
	source->inst = target->inst;
	source->isConst = target->isConst;
	source->nodeType = target->nodeType;
	source->offset = target->offset;
	source->outputs = target->outputs;
	source->inputs = target->inputs;

	source->parameter = target->parameter;
	source->resultId = target->resultId;
	source->resultType = target->resultType;

	source->spill = target->spill;

	source->symbol = target->symbol;
	source->targets = target->targets;
	source->value = target->value;
	source->relyNodes = target->relyNodes;
	source->precolor = target->precolor;
}

//找到当前语句的上一条语句
DAGNode* last_rely(vector<DAGNode*>& topolist, DAGNode* node) {
	bool findeded = false;
	DAGNode* result = NULL;
	for (int i = 0; i < topolist.size(); i++) {
		if (topolist[i] == node) {
			findeded = true;
		}
		else if (findeded && topolist[i]->relyNodes.size() != 0 && topolist[i]->relyNodes[0] == node) {
			result = topolist[i];
		}
	}
	return result;
}


void mark(DataFlowAnalysis* dfa, SymbolTable* sym) {
	useless_nodes.clear();
	dfa->liveVariablesAnalysis(0);
	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
		vector<string> block_out = (*blockIt)->dataInfo->bitSet_to_vector((*blockIt)->dataInfo->liveOutSet, dfa->live_varsSet);
		set<string> live_set(block_out.begin(), block_out.end());
		for (auto dagnode = (*blockIt)->topoList.rbegin(); dagnode != (*blockIt)->topoList.rend(); dagnode++) {
			string left_name;
			string right_name;
			Symbol* symbol;
			switch ((*dagnode)->nodeType) {
			case STORE: 
				if ((*dagnode)->inputs.size() < 2) break;
				left_name = "";
				//获得store的左值
				if ((*dagnode)->inputs[sd_variableName]->name == ARRAYSUB_NODE) {
					left_name = (*dagnode)->inputs[sd_variableName]->inputs[0]->name;
					symbol = (*dagnode)->inputs[sd_variableName]->inputs[0]->symbol;
					live_set.insert("%" + to_string((*dagnode)->inputs[sd_variableName]->resultId));
					if ((*dagnode)->inputs[sd_variableName]->inputs[0]->resultType == ipoint || (*dagnode)->inputs[sd_variableName]->inputs[0]->resultType == fpoint) {
						live_set.insert(left_name);
					}
				}
				else {
					left_name = (*dagnode)->inputs[sd_variableName]->name;
					symbol = (*dagnode)->inputs[sd_variableName]->symbol;
				}
				//标记为无用结点
				if (live_set.find(left_name) == live_set.end() && !sym->isGlobal(symbol) && (*dagnode)->inputs[sd_assignment]->name != "%PHI") {
					useless_nodes[(*dagnode)] = (*blockIt);
				}

				//修改活跃集
				live_set.erase(left_name);
				if ((*dagnode)->inputs[sd_assignment]->nodeType != CONSTANT) {
					right_name = (*dagnode)->inputs[sd_assignment]->nodeType == VALUE_NODE ? (*dagnode)->inputs[sd_assignment]->name : ("%" + to_string((*dagnode)->inputs[sd_assignment]->resultId));
					live_set.insert(right_name);
				}
				break;
			case LOAD:
				left_name = "%" + to_string((*dagnode)->resultId);
				//标记为无用结点
				if (live_set.find(left_name) == live_set.end()) {
					useless_nodes[(*dagnode)] = (*blockIt);
				}
				//修改活跃集
				live_set.erase(left_name);
				right_name = (*dagnode)->inputs[0]->nodeType == VALUE_NODE ? (*dagnode)->inputs[0]->name : ("%" + to_string((*dagnode)->inputs[0]->resultId));
				live_set.insert(right_name);
				break;
			case OP_NODE:
				left_name = "%" + to_string((*dagnode)->resultId);
				//标记为无用结点
				if (live_set.find(left_name) == live_set.end() && cse_op.find((*dagnode)->name) != cse_op.end()) {
					useless_nodes[(*dagnode)] = (*blockIt);
				}
				//修改活跃集
				live_set.erase(left_name);
				for (int i = 0; i < (*dagnode)->inputs.size(); i++) {
					if ((*dagnode)->inputs[i]->nodeType != CONSTANT) {
						right_name = (*dagnode)->inputs[i]->nodeType == VALUE_NODE ? (*dagnode)->inputs[i]->name : ("%" + to_string((*dagnode)->inputs[i]->resultId));
						live_set.insert(right_name);
					}
				}
				break;
			default:
				break;
			}

		}
	}
}

//删除所有不在使用集合里面变量的赋值语句，记得改依赖关系,bool记录是否发生了消除
bool sweep(DataFlowAnalysis* dfa) {
	bool ischanged = false;
	BlockManager* bM = new BlockManager();
	for (auto dagnode = useless_nodes.begin(); dagnode != useless_nodes.end(); dagnode++) {
		string assign_name;
		DAGNode* temp = NULL;
		switch ((*dagnode).first->nodeType) {
		case STORE: {
			//删除当前store
			if (useless_nodeType.find((*dagnode).first->inputs[sd_assignment]->nodeType) == useless_nodeType.end()) {
				DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);
				last_state->relyNodes.clear();
				last_state->relyNodes.push_back((*dagnode).first->inputs[sd_assignment]);
				(*dagnode).first->inputs[sd_assignment]->relyNodes = (*dagnode).first->relyNodes;
				(*dagnode).second->dag->remove_edge((*dagnode).first, (*dagnode).first->inputs[sd_assignment]);

			}
			else {//把上一条语句移动上来
				if ((*dagnode).first->relyNodes.size() != 0) {
					DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);
					last_state->relyNodes.clear();
					last_state->relyNodes.push_back((*dagnode).first->relyNodes[0]);
				}
				else {
					//dfa->topologicalSort((*dagnode).second);
					DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);
					last_state->relyNodes.clear();
				}
			}
			(*dagnode).first->relyNodes.clear();
			ischanged = true;
			break;
		}
		case OP_NODE: {
			//if (useless_nodes.find((*dagnode)) == useless_nodes.end()) break;

			bool finded = false;//是否有找到可以成为语句的结点
			vector<DAGNode*> following_node;

			vector<DAGNode*> temp_input = (*dagnode).first->inputs;
			for (int i = 0; i < temp_input.size(); i++) {
				if (!finded && useless_nodeType.find(temp_input[i]->nodeType) == useless_nodeType.end()) {
					temp = temp_input[i];
					following_node.push_back(temp_input[i]);
					finded = true;
				}
				else if (finded && useless_nodeType.find(temp_input[i]->nodeType) == useless_nodeType.end()) {
					following_node.push_back(temp_input[i]);
				}
				(*dagnode).second->dag->remove_edge((*dagnode).first, temp_input[i]);//移除输入与输出的关系
			}

			if (!finded) {
				if ((*dagnode).first->relyNodes.size() != 0) {
					DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);
					last_state->relyNodes.clear();
					last_state->relyNodes.push_back((*dagnode).first->relyNodes[0]);
				}
				else {
					//dfa->topologicalSort((*dagnode).second);
					DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);
					last_state->relyNodes.clear();
				}
			}
			else {
				DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);

				following_node[following_node.size() - 1]->relyNodes = (*dagnode).first->relyNodes;
				vector<DAGNode*> input_temp_rely;
				for (int j = following_node.size() - 1; j > 0; j--) {
					input_temp_rely.clear();
					input_temp_rely.push_back(following_node[j]);
					following_node[j - 1]->relyNodes = input_temp_rely;
				}
				last_state->relyNodes.clear();
				last_state->relyNodes.push_back(following_node[0]);
				//(*dagnode).first->relyNodes = following_node[0]->relyNodes;//当前应该是inputs[i]的依赖结点
				//copy_node((*dagnode).first, temp);
			}
			
			ischanged = true;
			break;
		}
		case LOAD: {
			//需要删除整条语句
			if (useless_nodeType.find((*dagnode).first->inputs[0]->nodeType) != useless_nodeType.end()) {
				if ((*dagnode).first->relyNodes.size() != 0) {
					DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);
					last_state->relyNodes.clear();
					last_state->relyNodes.push_back((*dagnode).first->relyNodes[0]);
				}
				else {
					//dfa->topologicalSort((*dagnode).second);
					DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);
					last_state->relyNodes.clear();
				}
			}
			else {
				DAGNode* last_state = last_rely((*dagnode).second->topoList, (*dagnode).first);
				(*dagnode).first->inputs[0]->relyNodes = (*dagnode).first->relyNodes;
				last_state->relyNodes.clear();
				last_state->relyNodes.push_back((*dagnode).first->inputs[0]);
				(*dagnode).second->dag->remove_edge((*dagnode).first, (*dagnode).first->inputs[0]);
			}
			ischanged = true;
			break;
		}
		default:
			cout << "dec error" << endl;
			break;
		}

		(*dagnode).first->relyNodes.clear();
		//dfa->topologicalSort((*dagnode).second);
	}
	return ischanged;
}


void eliminate_dead_code(DataFlowAnalysis* dfa, SymbolTable* sym) {
	bool changed = false;
	do {
		mark(dfa, sym);
		changed = sweep(dfa);
	} while (changed);
}


////要不直接遍历所有的use？注意不要忘记全局变量
//void mark(DataFlowAnalysis* dfa, SymbolTable* sym) {
//	used_var_set.clear();
//	//得到所有使用变量的集合
//	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
//		dfa->topologicalSort(*blockIt);
//		(*blockIt)->dag->nodes = (*blockIt)->topoList;
//		for (auto dagnode : (*blockIt)->topoList) {
//			string var_name;
//			Symbol* sd_left;
//			switch (dagnode->nodeType) {
//			case STORE:
//				if (dagnode->inputs[sd_assignment]->nodeType != CONSTANT) {
//					var_name = dagnode->inputs[sd_assignment]->nodeType == VALUE_NODE ? dagnode->inputs[sd_assignment]->name : ("%" + to_string(dagnode->inputs[sd_assignment]->resultId));
//					used_var_set.insert(var_name);
//				}
//
//				//判断是否为全局变量
//				if (dagnode->inputs[0]->name == ARRAYSUB_NODE) {//数组
//					used_var_set.insert("%" + to_string(dagnode->inputs[0]->resultId));
//					sd_left = dagnode->inputs[0]->inputs[0]->symbol;
//				}
//				else {
//					sd_left = dagnode->inputs[0]->symbol;
//				}
//				if (sym->isGlobal(sd_left)) {
//					used_var_set.insert(sd_left->name);
//				}
//
//				break;
//			case LOAD:
//				var_name = dagnode->inputs[0]->nodeType == VALUE_NODE ? dagnode->inputs[0]->name : ("%" + to_string(dagnode->inputs[0]->resultId));
//				used_var_set.insert(var_name);
//				break;
//			case OP_NODE:
//				for (int i = 0; i < dagnode->inputs.size(); i++) {
//					if (dagnode->inputs[i]->nodeType != CONSTANT) {
//						var_name = dagnode->inputs[i]->nodeType == VALUE_NODE ? dagnode->inputs[i]->name : ("%" + to_string(dagnode->inputs[i]->resultId));
//						used_var_set.insert(var_name);
//					}
//				}
//				break;
//			default:
//				break;
//			}
//		}
//	}
//}
//
//
//
//
////删除所有不在使用集合里面变量的赋值语句，记得改依赖关系,bool记录是否发生了消除
//bool sweep(DataFlowAnalysis* dfa) {
//	bool ischanged = false;
//	BlockManager* bM = new BlockManager();
//	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
//		for (auto dagnode = (*blockIt)->topoList.begin(); dagnode != (*blockIt)->topoList.end(); dagnode++) {
//			string assign_name;
//			DAGNode* temp = NULL;
//			switch ((*dagnode)->nodeType) {
//			case STORE: {
//				//获得store的左值
//				if ((*dagnode)->inputs[sd_variableName]->name == ARRAYSUB_NODE) {
//					assign_name = (*dagnode)->inputs[sd_variableName]->inputs[0]->name;
//				}
//				else {
//					assign_name = (*dagnode)->inputs[sd_variableName]->name;
//				}
//				//删除语句
//				if (used_var_set.find(assign_name) == used_var_set.end()) {//该变量未在后续使用到
//					if (useless_nodeType.find((*dagnode)->inputs[sd_assignment]->nodeType) == useless_nodeType.end()) {
//						temp = (*dagnode)->inputs[sd_assignment];
//						vector<DAGNode*> rely = (*dagnode)->relyNodes;
//						copy_node((*dagnode), temp);
//						(*dagnode)->relyNodes = rely;
//
//					}
//					else {//把下一条语句移动上来
//						if ((*dagnode)->relyNodes.size() != 0) {
//							temp = (*dagnode)->relyNodes[0];//如果有多条依赖关系，这里还需要改
//							copy_node((*dagnode), temp);
//						}
//						else {
//							int node_index = distance((*blockIt)->topoList.begin(), dagnode);
//							remove_rely((*blockIt)->topoList, node_index);
//						}
//					}
//					ischanged = true;
//				}
//
//				break;
//			}
//			case OP_NODE: {
//				assign_name = "%" + to_string((*dagnode)->resultId);
//				if (cse_op.find((*dagnode)->name) == cse_op.end() || used_var_set.find(assign_name) != used_var_set.end()) break;//判断是否在后续中有使用到
//
//				bool finded = false;//是否有找到可以成为语句的结点
//				vector<DAGNode*> following_node;
//				for (int i = 0; i < (*dagnode)->inputs.size(); i++) {
//					if (!finded && useless_nodeType.find((*dagnode)->inputs[i]->nodeType) == useless_nodeType.end()) {
//						temp = (*dagnode)->inputs[i];
//						following_node.push_back((*dagnode)->inputs[i]);
//						finded = true;
//					}else if (finded && useless_nodeType.find((*dagnode)->inputs[i]->nodeType) == useless_nodeType.end()) {
//						following_node.push_back((*dagnode)->inputs[i]);
//					}
//				}
//
//				if (!finded) {
//					if ((*dagnode)->relyNodes.size() != 0) {
//						temp = (*dagnode)->relyNodes[0];//如果有多条依赖关系，这里还需要改
//						copy_node((*dagnode), temp);
//					}
//					else {
//						int node_index = distance((*blockIt)->topoList.begin(), dagnode);
//						remove_rely((*blockIt)->topoList, node_index);
//					}
//				}
//				else {
//					following_node[following_node.size() - 1]->relyNodes = (*dagnode)->relyNodes;
//					vector<DAGNode*> input_temp_rely;
//					for (int j = following_node.size() - 1; j > 0; j--) {
//						input_temp_rely.clear();
//						input_temp_rely.push_back(following_node[j]);
//						following_node[j - 1]->relyNodes = input_temp_rely;
//					}
//					(*dagnode)->relyNodes = following_node[0]->relyNodes;//当前应该是inputs[i]的依赖结点
//					copy_node((*dagnode), temp);
//				}
//				ischanged = true;
//				break;
//			}
//			case LOAD: {
//				assign_name = "%" + to_string((*dagnode)->resultId);
//				if (used_var_set.find(assign_name) == used_var_set.end()) {//该变量未在后续使用到
//					if (useless_nodeType.find((*dagnode)->inputs[0]->nodeType) == useless_nodeType.end()) {
//						if ((*dagnode)->relyNodes.size() != 0) {
//							temp = (*dagnode)->relyNodes[0];//如果有多条依赖关系，这里还需要改
//							copy_node((*dagnode), temp);
//						}
//						else {
//							int node_index = distance((*blockIt)->topoList.begin(), dagnode);
//							remove_rely((*blockIt)->topoList, node_index);
//						}
//					}
//					else {
//						vector<DAGNode*> rely = (*dagnode)->relyNodes;
//						temp = (*dagnode)->inputs[0];
//						copy_node((*dagnode), temp);
//						(*dagnode)->relyNodes = rely;
//					}
//					ischanged = true;
//				}
//				break;
//			}
//			default:
//				break;
//			}
//		}
//	}
//	return ischanged;
//}  