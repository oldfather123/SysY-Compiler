#include"CostantPropagation.h"

map<string, vector<string>>global_vars;
vector<string> def_var;

set<string> CommonOPNameVec = { PLUS ,SUBTRACT,MULTIPLY,DIVIDE,GREATER_THAN,GREATER_EQUAL ,LESS_THAN ,LESS_EQUAL,EQUAL,NOT_EQUAL ,
										LOGICAL_NOT,LOGICAL_AND,LOGICAL_OR,REMAINDER,RETURN_NODE,BRANCH_NODE,ARRAY_INIT_NODE,IMPLICIT_INIT_NODE,
		ARRAYSUB_NODE,INT_TO_FLOAT ,FLOAT_TO_INT,ARRAY_TO_POINT };

//运算符集合
set<string> ava_opSet = { FLOAT_TO_INT,INT_TO_FLOAT,PLUS,SUBTRACT,MULTIPLY,DIVIDE,GREATER_THAN,GREATER_EQUAL,
	LESS_THAN,LESS_EQUAL,EQUAL,NOT_EQUAL,LOGICAL_NOT,LOGICAL_AND,LOGICAL_OR,REMAINDER };

//常量传播之基本块内
void cp_in_block(DataFlowAnalysis* dfa) {
	dfa->topologicalSort();
	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
		map<string, FoldingValue> constants;
		map<string, set<string>> arr_elements;
		string varName;
		for (auto dagnode : (*blockIt)->topoList) {
			switch (dagnode->nodeType)
			{
			case STORE:
				if (dagnode->inputs[0]->nodeType == VALUE_NODE && dagnode->inputs[1]->nodeType == CONSTANT) {
					FoldingValue value;
					value.type = FOLDING_C;
					value.valueType = dagnode->inputs[1]->resultType;
					value.constanValue = dagnode->inputs[1]->value;
					constants.insert(make_pair(dagnode->inputs[0]->name, value));
				}
				else if (dagnode->inputs[1]->nodeType == CONSTANT && dagnode->inputs[0]->name == ARRAYSUB_NODE && dagnode->inputs[0]->inputs[1]->nodeType == CONSTANT) {
					FoldingValue value;
					value.type = FOLDING_C;
					value.valueType = dagnode->inputs[1]->resultType;
					value.constanValue = dagnode->inputs[1]->value;
					varName = dagnode->inputs[0]->inputs[0]->name + "[]" + to_string(dagnode->inputs[0]->inputs[1]->value.iValue);
					constants.insert(make_pair(varName, value));
					arr_elements[dagnode->inputs[0]->inputs[0]->name].insert(varName);
				}
				else if (dagnode->inputs[1]->nodeType != CONSTANT) {
					if (dagnode->inputs[0]->nodeType == VALUE_NODE) {
						constants.erase(dagnode->inputs[0]->name);
					}
					else {
						for (auto name:arr_elements[dagnode->inputs[0]->inputs[0]->name]) {
							constants.erase(name);
						}
					}
				}
				break;
			case LOAD:
				if (dagnode->inputs[0]->nodeType == VALUE_NODE && constants.find(dagnode->inputs[0]->name)!=constants.end() ) {
					dagnode->nodeType = CONSTANT;
					dagnode->name = "";
					dagnode->value = constants[dagnode->inputs[0]->name].constanValue;
					dagnode->resultId = -1;
					(*blockIt)->dag->remove_edge(dagnode, dagnode->inputs[0]);
					//dagnode->inputs.clear();
				}
				else if (dagnode->inputs[0]->name == ARRAYSUB_NODE && dagnode->inputs[0]->inputs[1]->nodeType == CONSTANT) {
					varName = dagnode->inputs[0]->inputs[0]->name + "[]" + to_string(dagnode->inputs[0]->inputs[1]->value.iValue);
					if (constants.find(varName) != constants.end()) {
						dagnode->nodeType = CONSTANT;
						dagnode->name = "";
						dagnode->value = constants[varName].constanValue;
						dagnode->resultId = -1;
						(*blockIt)->dag->remove_edge(dagnode, dagnode->inputs[0]);
					}

				}
			case OP_NODE:
				if (ava_opSet.find(dagnode->name) != ava_opSet.end()) {
					ExpManager expM ;
					Value value;
					if (dagnode->inputs.size() == 1 && dagnode->inputs[0]->nodeType == CONSTANT) {
						value = expM.folding_calculate_collection(dagnode, dagnode->inputs[0]->value);
						dagnode->name = "";
						dagnode->value = value;
						dagnode->resultId = -1;
						dagnode->nodeType = CONSTANT;
						(*blockIt)->dag->remove_edge(dagnode, dagnode->inputs[0]);
					}
					else if(dagnode->inputs.size() == 2 && dagnode->inputs[0]->nodeType == CONSTANT && dagnode->inputs[1]->nodeType == CONSTANT) {
						value = expM.folding_calculate_collection(dagnode, dagnode->inputs[0]->value, dagnode->inputs[1]->value);
						dagnode->name = "";
						dagnode->value = value;
						dagnode->resultId = -1;
						dagnode->nodeType = CONSTANT;
						(*blockIt)->dag->remove_edge(dagnode, dagnode->inputs[1]);
						(*blockIt)->dag->remove_edge(dagnode, dagnode->inputs[0]);
					}
				}
			default:
				break;
			}
		}
	}
}



//store常量状态计算
void store_merge(BasicBlock* block, DAGNode* node) {
	if (node->inputs[sd_variableName]->nodeType == OP_NODE && node->name == ARRAYSUB_NODE) {//处理数组赋值
		block->dataInfo->foldingMap[node->inputs[sd_variableName]->inputs[0]->name].type = NAC;
		return;
	}
	if (node->inputs[sd_assignment]->name == ARRAY_INIT_NODE) {//处理数组的初始化
		block->dataInfo->foldingMap[node->inputs[sd_variableName]->name].type = NAC;
		return;
	}
	string assignName;
	string varName = node->inputs[sd_variableName]->name;
	DAGNode* subNode = node->inputs[sd_assignment];

	switch (subNode->nodeType)
	{
	case LOAD:
	case OP_NODE:
		/*if (subNode->name == ARRAY_INIT_NODE) {
			array_init(block, subNode, varName);
			break;
		}*/
		assignName = "%" + to_string(subNode->resultId);
		block->dataInfo->foldingMap[varName] = block->dataInfo->foldingMap[assignName];
		break;
	case CONSTANT:
		block->dataInfo->foldingMap[varName].type = FOLDING_C;
		block->dataInfo->foldingMap[varName].valueType = subNode->resultType;
		block->dataInfo->foldingMap[varName].constanValue = subNode->value;
		break;
	default:
		break;
	}
}
//load节点常量状态计算
void load_merge(BasicBlock* block, DAGNode* node) {
	string varName = "%" + to_string(node->resultId);
	string assignName;
	if (node->inputs[0]->nodeType == VALUE_NODE) {
		assignName = node->inputs[0]->name;
	}
	else {//数组或寄存器
		assignName = "%" + to_string(node->inputs[0]->resultId);
	}
	block->dataInfo->foldingMap[varName] = block->dataInfo->foldingMap[assignName];
}

//赋值语句节点常量状态计算
void op_merge(BasicBlock* block, DAGNode* node) {
	if (node->name == BRANCH_NODE && node->inputs.size() == 0 || node->name == ARRAYSUB_NODE || node->name == ARRAY_INIT_NODE) return;//无条件跳转语句
	if (CommonOPNameVec.find(node->name) == CommonOPNameVec.end()) {//发生函数调用,将所有全局变量置为NAC
		for (int i = 0; i < global_vars[node->name].size(); i++) {
			if (find(def_var.begin(), def_var.end(), global_vars[node->name][i]) != def_var.end()) {
				block->dataInfo->foldingMap[global_vars[node->name][i]].type = NAC;
			}
		}
	}

	ExpManager* expMan = new ExpManager();
	string varName = "%" + to_string(node->resultId);
	block->dataInfo->foldingMap[varName].valueType = node->resultType;//常量的值
	std::set<string>::iterator it = expMan->ava_opSet.find(node->name);

	if (it == expMan->ava_opSet.end()) {//当前节点为函数调用节点
		block->dataInfo->foldingMap[varName].type = NAC;
		return;
	}
	bool single = node->inputs.size() == 1 ? 1 : 0;//是否只有一个操作数
	if (single) {//只有一个操作数
		string assignName;
		switch (node->inputs[0]->nodeType) {
		case CONSTANT:
			block->dataInfo->foldingMap[varName].type = FOLDING_C;
			block->dataInfo->foldingMap[varName].constanValue = expMan->folding_calculate_collection(node, node->inputs[0]->value);//计算完的结果存回去
			break;
		case LOAD:
		case OP_NODE:
			assignName = "%" + to_string(node->inputs[0]->resultId);
			if (block->dataInfo->foldingMap[assignName].type != FOLDING_C) {//子节点的分析结果不是常量
				block->dataInfo->foldingMap[varName] = block->dataInfo->foldingMap[assignName];
			}
			else {
				block->dataInfo->foldingMap[varName].type = FOLDING_C;
				block->dataInfo->foldingMap[varName].constanValue = expMan->folding_calculate_collection(node,
					block->dataInfo->foldingMap[assignName].constanValue);//计算完的结果存回去
			}
			break;
		default:
			break;
		}
	}
	else {//有两个操作数
		if (node->inputs[0]->nodeType == CONSTANT && node->inputs[1]->nodeType == CONSTANT) {//op节点的操作数都为常数
			block->dataInfo->foldingMap[varName].type = FOLDING_C;
			block->dataInfo->foldingMap[varName].constanValue = expMan->folding_calculate_collection(node, node->inputs[0]->value,
				node->inputs[1]->value);
		}
		else if (node->inputs[0]->nodeType == CONSTANT && node->inputs[1]->nodeType != CONSTANT) {//op的左节点为常数，右节点不是
			string inputRight = "%" + to_string(node->inputs[1]->resultId);
			if (block->dataInfo->foldingMap[inputRight].type == FOLDING_C) {
				block->dataInfo->foldingMap[varName].type = FOLDING_C;
				block->dataInfo->foldingMap[varName].constanValue = expMan->folding_calculate_collection(node, node->inputs[0]->value,
					block->dataInfo->foldingMap[inputRight].constanValue);
			}
			else if (block->dataInfo->foldingMap[inputRight].type == NAC) {
				block->dataInfo->foldingMap[varName].type = NAC;
			}
			else {
				block->dataInfo->foldingMap[varName].type = UNDEF;
			}
		}
		else if (node->inputs[0]->nodeType != CONSTANT && node->inputs[1]->nodeType == CONSTANT) {//op的右节点是常数
			string inputLeft = "%" + to_string(node->inputs[0]->resultId);
			if (block->dataInfo->foldingMap[inputLeft].type == FOLDING_C) {
				block->dataInfo->foldingMap[varName].type = FOLDING_C;
				block->dataInfo->foldingMap[varName].constanValue = expMan->folding_calculate_collection(node,
					block->dataInfo->foldingMap[inputLeft].constanValue, node->inputs[1]->value);
			}
			else if (block->dataInfo->foldingMap[inputLeft].type == NAC) {
				block->dataInfo->foldingMap[varName].type = NAC;
			}
			else {
				block->dataInfo->foldingMap[varName].type = UNDEF;
			}
		}
		else if (node->inputs[0]->nodeType != CONSTANT && node->inputs[1]->nodeType != CONSTANT) {//左右节点都不是常数
			string inputLeft = "%" + to_string(node->inputs[0]->resultId), inputRight = "%" + to_string(node->inputs[1]->resultId);
			if (block->dataInfo->foldingMap[inputLeft].type == NAC || block->dataInfo->foldingMap[inputRight].type == NAC) {
				block->dataInfo->foldingMap[varName].type = NAC;
			}
			else if (block->dataInfo->foldingMap[inputLeft].type == FOLDING_C && block->dataInfo->foldingMap[inputRight].type == FOLDING_C) {
				block->dataInfo->foldingMap[varName].type = FOLDING_C;
				block->dataInfo->foldingMap[varName].constanValue = expMan->folding_calculate_collection(node,
					block->dataInfo->foldingMap[inputLeft].constanValue, block->dataInfo->foldingMap[inputRight].constanValue);
			}
			else {
				block->dataInfo->foldingMap[varName].type = UNDEF;
			}
		}

	}
	delete expMan;
}

//根据当前节点的属性给节点赋常量状态
void assign_state(BasicBlock* block, DAGNode* node) {
	if (node->nodeType == CONSTANT || node->nodeType == VALUE_NODE)
		return;
	string varName, assignName;
	if (node->nodeType == STORE) {
		store_merge(block, node);
	}
	else if (node->nodeType == OP_NODE) {
		op_merge(block, node);
	}
	else if (node->nodeType == LOAD) {
		load_merge(block, node);
	}
}

//当一个变量有两个常量状态时，需要进行合并，交汇运算
FoldingValue meet_func(FoldingValue value1, FoldingValue value2) {
	FoldingValue reValue;
	if (value1.type == NAC || value2.type == NAC) {
		reValue.type = NAC;
	}
	else if (value1.type == UNDEF && value2.type == FOLDING_C) {
		reValue.type = FOLDING_C;
		reValue.valueType = value2.valueType;
		reValue.constanValue = value2.constanValue;
	}
	else if (value2.type == UNDEF && value1.type == FOLDING_C) {
		reValue.type = FOLDING_C;
		reValue.valueType = value1.valueType;
		reValue.constanValue = value1.constanValue;
	}
	else if (value1.type == FOLDING_C && value2.type == FOLDING_C) {
		if (value1.value_equal(value1, value2)) {
			reValue.type = FOLDING_C;
			reValue.valueType = value1.valueType;
			reValue.constanValue = value1.constanValue;
		}
		else {//两个常量值不相等
			reValue.type = NAC;
		}
	}
	else {
		reValue.type = UNDEF;
	}

	return reValue;
}

//常量传播初始化 
void constant_init(DataFlowAnalysis* dfa) {
	dfa->rd_traversal();
	def_var = dfa->def_var;
	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
		for (int i = 0; i < dfa->def_var.size(); i++) {
			FoldingValue folding_value;
			folding_value.type = UNDEF;
			(*blockIt)->dataInfo->foldingMap.insert(make_pair(dfa->def_var[i], folding_value));
		}
	}
	for (auto dagnode : dfa->cfg->entry->topoList) {//对第一个基本块进行初始化
		assign_state(dfa->cfg->entry, dagnode);
	}
}


//转移函数,根据输入map修改
void folding_transfer(BasicBlock* block) {
	for (auto dagnode : block->topoList) {//对第一个基本块进行初始化
		assign_state(block, dagnode);
	}
}

//设置每个基本块的输入map,主要把后续块的状态和此前状态合并
void merge_block_map(map<string, FoldingValue>& map1, map<string, FoldingValue> map2) {
	for (auto mapIt = map1.begin(); mapIt != map1.end(); mapIt++) {
		(*mapIt).second = meet_func((*mapIt).second, map2[(*mapIt).first]);
	}
}


//根据id找到当前基本块
BasicBlock* find_block_id(DataFlowAnalysis* dfa, int id) {
	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
		if ((*blockIt)->id == id) {
			return (*blockIt);
		}
	}
}

//判断两个map是否相等
bool map_equal(DataFlowAnalysis* dfa, map<string, FoldingValue> map1, map<string, FoldingValue> map2) {
	for (int i = 0; i < dfa->def_var.size(); i++) {//变量集合是map的key
		if (!map1[dfa->def_var[i]].equal(map2[dfa->def_var[i]]))
			return false;
	}
	return true;
}

//分析常量值
void constant_traversal(DataFlowAnalysis* dfa) {
	constant_init(dfa);
	bool change = false;
	int n = dfa->cfg->basicBlocks.size();
	queue<int> worklist;
	vector<int> visited(n, 0);
	int count = 0;
	do {
		change = false;
		count++;
		for (int i = 0; i < n; i++) {
			if (dfa->cfg->basicBlocks[i]->preBlock.size() <= 0) {
				folding_transfer(dfa->cfg->basicBlocks[i]);
				continue;
			}
			map<string, FoldingValue> tempMap = dfa->cfg->basicBlocks[i]->preBlock[0]->dataInfo->foldingMap;
			for (auto preBlockIt = dfa->cfg->basicBlocks[i]->preBlock.begin(); preBlockIt != dfa->cfg->basicBlocks[i]->preBlock.end(); preBlockIt++) {
				merge_block_map(tempMap, (*preBlockIt)->dataInfo->foldingMap);
			}
			map<string, FoldingValue> oldMap = dfa->cfg->basicBlocks[i]->dataInfo->foldingMap;
			dfa->cfg->basicBlocks[i]->dataInfo->foldingMap = tempMap;
			folding_transfer(dfa->cfg->basicBlocks[i]);
			if (!map_equal(dfa, oldMap, dfa->cfg->basicBlocks[i]->dataInfo->foldingMap))
				change = true;
		}
	} while (change == true);
}

bool isZero(DAGNode* node) {
	if (node->nodeType != CONSTANT) return false;
	if (node->resultType == i32) {
		return !node->value.iValue;
	}
	else {
		return !node->value.fValue;
	}
	return false;
}

void remove_postBlock(BasicBlock* pre, BasicBlock* post) {
	auto postIt = std::remove(pre->postBlock.begin(), pre->postBlock.end(), post);
	pre->postBlock.erase(postIt, pre->postBlock.end());
	auto preIt = remove(post->preBlock.begin(), post->preBlock.end(), pre);
	post->preBlock.erase(preIt, post->preBlock.end());
}


//常量传播折叠
void constant_propagation_folding(DataFlowAnalysis* dfa, map<string, vector<string>> cfg_global) {
	global_vars = cfg_global;
	cp_in_block(dfa);
	constant_traversal(dfa);//常量传播分析
	vector<DAGNode*> tempNewNodes;
	BlockManager* bM = new BlockManager();
	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
		tempNewNodes.clear();//初始化
		for (int i = 0; i < (*blockIt)->topoList.size(); i++) {
			if ((*blockIt)->topoList[i]->nodeType == CONSTANT || (*blockIt)->topoList[i]->nodeType == VALUE_NODE|| (*blockIt)->topoList[i]->nodeType == STORE) {
				continue;
			}
			//改变跳转
			if ((*blockIt)->topoList[i]->name == BRANCH_NODE || (*blockIt)->topoList[i]->name == RETURN_NODE) {
				if ((*blockIt)->topoList[i]->targets.size() == 2 && (*blockIt)->topoList[i]->inputs[0]->nodeType == CONSTANT) {
					if (isZero((*blockIt)->topoList[i]->inputs[0])) {//对于跳转进行的修改
						(*blockIt)->topoList[i]->targets.erase((*blockIt)->topoList[i]->targets.begin());//移除true分支
						BasicBlock* truePost = (*blockIt)->postBlock[0];
						remove_postBlock(*blockIt, truePost);
					}
					else {
						(*blockIt)->topoList[i]->targets.erase((*blockIt)->topoList[i]->targets.begin() + 1);//移除false分支
						BasicBlock* falsePost = (*blockIt)->postBlock[1];
						remove_postBlock(*blockIt, falsePost);
					}
					vector<DAGNode*> tempInputs = (*blockIt)->topoList[i]->inputs;
					for (int j = 0; j < tempInputs.size(); j++) {
						(*blockIt)->dag->remove_edge((*blockIt)->topoList[i], tempInputs[j]);
					}
				}
				continue;
			}

			//load和op的处理一样，store不需要处理，store的右边就是load或op
			string varName = "%" + to_string((*blockIt)->topoList[i]->resultId);
			if ((*blockIt)->dataInfo->foldingMap[varName].type == FOLDING_C) {
				(*blockIt)->topoList[i]->nodeType = CONSTANT;
				(*blockIt)->topoList[i]->value = (*blockIt)->dataInfo->foldingMap[varName].constanValue;
				vector<DAGNode*> tempInputs = (*blockIt)->topoList[i]->inputs;
				for (int j = 0; j < tempInputs.size(); j++) {
					(*blockIt)->dag->remove_edge((*blockIt)->topoList[i], tempInputs[j]);
				}
			//	//建立新节点
			//	DAGNode* newNode = new DAGNode(" ", CONSTANT);
			//	newNode->resultType = (*blockIt)->dag->nodes[i]->resultType;
			//	newNode->value = (*blockIt)->dataInfo->foldingMap[varName].constanValue;
			//	//暂时加入缓冲区，防止影响当前循环
			//	tempNewNodes.push_back(newNode);
			//	//添加边，移除旧边
			//	vector<DAGNode*> tempOutput = (*blockIt)->dag->nodes[i]->outputs;
			//	for (auto outputIt = tempOutput.begin(); outputIt != tempOutput.end(); outputIt++) {
			//		if ((*outputIt)->name == BRANCH_NODE) {
			//			(*outputIt)->inputs.clear();
			//			if (isZero(newNode)) {//对于跳转进行的修改
			//				(*outputIt)->targets.erase((*outputIt)->targets.begin());//移除true分支
			//				BasicBlock* truePost = (*blockIt)->postBlock[0];
			//				remove_postBlock(*blockIt, truePost);
			//			}
			//			else { 
			//				(*outputIt)->targets.erase((*outputIt)->targets.begin() + 1);//移除false分支
			//				BasicBlock* falsePost = (*blockIt)->postBlock[1];
			//				remove_postBlock(*blockIt, falsePost);
			//			}
			//			continue;
			//		}
			//		(*blockIt)->dag->add_edge((*outputIt), newNode);
			//		(*blockIt)->dag->remove_edge((*outputIt), (*blockIt)->dag->nodes[i]);
			//	}
			}
		}
		//新节点加入dag->nodes
		//(*blockIt)->dag->nodes.insert((*blockIt)->dag->nodes.end(), tempNewNodes.begin(), tempNewNodes.end());
		//移除不可达节点
		bM->remove_unreachableNodes((*blockIt));
	}
	delete bM;
}
