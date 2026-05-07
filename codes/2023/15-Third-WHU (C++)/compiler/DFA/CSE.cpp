#include "CSE.h"
#include<unordered_map>

map<string, map<BasicBlock*,long>> cse_exp_reg;//表达式--》块号，虚拟寄存器号
map<long, string> cse_reg_exp;

unordered_map<BasicBlock*, BasicBlock*> last_dom;
map<BasicBlock*, set<BasicBlock*>> doms;

map<string, vector<string>> cfg_globals;//获取每个CFG的全局变量的使用信息

unordered_map<BasicBlock*, int> block_index;//块以及块对应的位置

void cse_dfs(DataFlowAnalysis* dfa, BasicBlock* block, DAGNode* rootNode, unordered_set<string>& exp_gens, unordered_set<string>& diff_exp);

void print(string s) {
	//cout << s << endl;
}

//将某个节点替换为寄存器节点
void node_to_reg(DAGNode* node,BasicBlock* currentBlock, map<BasicBlock*, long> block_reg) {
	//查找合适的虚拟寄存器号
	int max_index = -1;//块的位置
	long old_id = node->resultId;
	for (auto& it : block_reg) {
		if (doms[currentBlock].find(it.first) != doms[currentBlock].end()) {
			if (block_index[it.first] > max_index) {
				node->resultId = it.second;
				max_index = block_index[it.first];
			}
		}
	}

	if (max_index == -1) {
		node->resultId = old_id;
		return;
	}

	//node->name = "%" + to_string(id);
	if (node->nodeType == LOAD && node->inputs[0]->nodeType == VALUE_NODE) {
		node->symbol = node->inputs[0]->symbol;
	}
	node->nodeType = REG_NODE;
	//找到支配点集当中合适的寄存器号
	vector<DAGNode*> temp = node->inputs;

	for (int i = 0; i < temp.size(); i++) {//移除该节点的输入边
		currentBlock->dag->remove_edge(node, temp[i]);
	}
}


unordered_set<string> bitSet_to_set(bitset<SIZE> &input_set, vector<string> &collection) {
	unordered_set<string> result;
	for (int i = 0; i < collection.size(); i++) {
		if(input_set.test(i))
			result.insert(collection[i]);
	}
	return result;
}
//恢复回边
void add_while_preedge(CFG* cfg) {
	for (int b_index = 0; b_index < cfg->basicBlocks.size(); b_index++) {
		if (cfg->basicBlocks[b_index]->label == BasicBlock::WHILEBODY || cfg->basicBlocks[b_index]->label == BasicBlock::IF_END_AND_WHILEBODY) {
			cfg->basicBlocks[b_index]->postBlock[0]->preBlock.push_back(cfg->basicBlocks[b_index]);
		}
	}
}
//删除回边
void de_while_preedge(CFG* cfg) {
	for (int b_index = 0; b_index < cfg->basicBlocks.size(); b_index++) {
		if (cfg->basicBlocks[b_index]->label == BasicBlock::WHILEBODY || cfg->basicBlocks[b_index]->label == BasicBlock::IF_END_AND_WHILEBODY) {
			BasicBlock* temp_whilecondition = cfg->basicBlocks[b_index]->postBlock[0];
			temp_whilecondition->preBlock.erase(find(temp_whilecondition->preBlock.begin(), temp_whilecondition->preBlock.end(), cfg->basicBlocks[b_index]));
		}
	}
}


//计算支配集
void cal_Dom(map<BasicBlock*, set<BasicBlock*>> &doms,CFG* cfg) {
	de_while_preedge(cfg);
	//初始化dom
	for (BasicBlock* b_iter : cfg->basicBlocks) {
		doms[b_iter].insert(b_iter);
	}
	for (int i = 0; i < cfg->basicBlocks.size(); i++)
	{
		vector<BasicBlock*> prenodes = cfg->basicBlocks[i]->preBlock;
		set<BasicBlock*> temp;
		if (!prenodes.empty())
		{
			//
			temp = doms[prenodes[0]];
			for (BasicBlock* pre_iter : prenodes) {
				set<BasicBlock*> temp_section;
				set_intersection(doms[pre_iter].begin(), doms[pre_iter].end(), temp.begin(), temp.end(), inserter(temp_section, temp_section.begin()));
				temp = temp_section;
			}
		}

		//加上自己
		temp.insert(cfg->basicBlocks[i]);
		doms[cfg->basicBlocks[i]] = temp;
	}
	add_while_preedge(cfg);
}


//获取某个块前面
unordered_map<BasicBlock*, BasicBlock*> calcu_last_dom(map<BasicBlock*, set<BasicBlock*>> &doms, CFG* cfg) {
	block_index.clear();//初始化

	//获得基本块在排序当中的位置关系
	for (int i = 0; i < cfg->basicBlocks.size(); i++) {
		block_index[cfg->basicBlocks[i]] = i;
	}
	unordered_map<BasicBlock*, BasicBlock*> result;
	for (auto mapIt = doms.begin(); mapIt != doms.end(); mapIt++) {
		BasicBlock* last = *((*mapIt).second.begin());
		result[(*mapIt).first] = last;
		int max = 0;
		for (auto setIt : (*mapIt).second) {
			if (block_index[setIt] > max && block_index[setIt] < block_index[(*mapIt).first]) {
				result[(*mapIt).first] = setIt;
				max = (*setIt).id;
			}
			/*if ((*setIt).id > max && (*setIt).id < (*mapIt).first->id) {
				result[(*mapIt).first] = setIt;
				max = (*setIt).id;
			}*/
		}
	}
	return result;
}

void exp_elimination(DataFlowAnalysis* dfa, SymbolTable* sym, bool mir, map<string, vector<string>> &globals) {
	cse_exp_reg.clear();
	cse_reg_exp.clear();
	cfg_globals = globals;
	//branch_move();
	if (mir) {
		dfa->availableExpressionsAnalysis(sym, 1);
	}
	else {
		dfa->availableExpressionsAnalysis(sym, 0);
	}
	cse_reg_exp = dfa->reg_exp;

	doms.clear();
	cal_Dom(doms, dfa->cfg);
	//calcu_dom(dfa->cfg, doms);
	
	last_dom = calcu_last_dom(doms,dfa->cfg);

	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
		unordered_set<string> exp_gens = bitSet_to_set(last_dom[(*blockIt)]->dataInfo->AvaInSet, dfa->ava_exp_vec);
		unordered_set<string> diff_exp;
		cse_dfs(dfa, (*blockIt), (*blockIt)->dag->rootNode, exp_gens,diff_exp);
	}
}


//判断cse_exp_reg当中存在的map当中是否是当前块的支配点集
bool contain_in_doms(string exp, BasicBlock* block, long id) {
	for (auto& it : cse_exp_reg[exp]) {
		if (doms[block].find(it.first) != doms[block].end()) {
			cse_exp_reg[exp][block] = id;
			return true;
		}
	}
	return false;
}

//遍历语句
void cse_dfs(DataFlowAnalysis* dfa, BasicBlock* block, DAGNode* rootNode,unordered_set<string> &exp_gens, unordered_set<string>& diff_exp) {
	//先访问前面的语句
	if (rootNode->relyNodes.size() != 0) {
		cse_dfs(dfa, block, rootNode->relyNodes[0], exp_gens, diff_exp);
	}
	if (rootNode->nodeType == CONSTANT || rootNode->nodeType == VALUE_NODE || rootNode->name == "%PHI") return;

	//先访问前面的语句
	/*if (rootNode->relyNodes.size() != 0) {
		cse_dfs(dfa, block, rootNode->relyNodes[0], exp_gens, diff_exp);
	}*/
	/*if (rootNode->nodeType == OP_NODE && (rootNode->name == BRANCH_NODE || rootNode->name == RETURN_NODE)) {
		for (int i = 0; i < rootNode->inputs.size(); i++) {
			cse_dfs(dfa, block, rootNode->inputs[i], exp_gens, diff_exp);
		}
	}*/
	AvaExp* avaM = new AvaExp();
	string exp;
	switch (rootNode->nodeType) {
	case STORE:
		for (auto nodeIt = rootNode->inputs.rbegin(); nodeIt != rootNode->inputs.rend(); nodeIt++) {
			cse_dfs(dfa, block, (*nodeIt), exp_gens, diff_exp);
		}
		if (rootNode->inputs[0]->nodeType == VALUE_NODE) {
			string varName = rootNode->inputs[0]->name;
			unordered_set<string> temp = exp_gens;
			for (auto& it : temp) {
				if (avaM->contain_operand(it, varName)) {
					exp_gens.erase(it);
					cse_exp_reg.erase(it);
				}
			}
		}
		break;
	case OP_NODE:
		//跳过函数调用等
		if (dfa->ava_opSet.find(rootNode->name) == dfa->ava_opSet.end()){
			for (int i = 0; i < rootNode->inputs.size(); i++) {
				cse_dfs(dfa, block, rootNode->inputs[i], exp_gens, diff_exp);
			}
			break;
		}
	case LOAD:
		if (cse_reg_exp.find(rootNode->resultId) == cse_reg_exp.end()) {
			for (int i = 0; i < rootNode->inputs.size(); i++) {
				cse_dfs(dfa, block, rootNode->inputs[i], exp_gens, diff_exp);
			}
			break;
		}
		//cout << "没有在reg_exp找到该表达式，错误" << endl;
		exp = cse_reg_exp[rootNode->resultId]; //找到该节点的表达式

		//可以替换
		if (exp_gens.find(exp) != exp_gens.end() && cse_exp_reg.find(exp) != cse_exp_reg.end() &&
			(last_dom[block]->dataInfo->AvaOutSet.test(dfa->ava_exp_index[exp]) 
				|| block->dataInfo->AvaGens.test(dfa->ava_exp_index[exp]))) {
			/*bool is_phi = false;*/
			if ((diff_exp.size() > 15 && diff_exp.find(exp) == diff_exp.end())) break;
			node_to_reg(rootNode, block, cse_exp_reg[exp]);
			diff_exp.insert(exp);
		}
		else {
			//遍历底下的点
			for (int i = 0; i < rootNode->inputs.size(); i++) {
				cse_dfs(dfa, block, rootNode->inputs[i], exp_gens, diff_exp);
			}
			//由该块产生
			if ((block == dfa->cfg->entry || block != dfa->cfg->entry && !last_dom[block]->dataInfo->AvaOutSet.test(dfa->ava_exp_index[exp]) && block != dfa->cfg->entry)
				&& block->dataInfo->AvaGens.test(dfa->ava_exp_index[exp]) 
				&& exp_gens.find(exp) == exp_gens.end()) {

				//维护表达式和寄存器的对应关系
				cse_exp_reg[exp][block] = rootNode->resultId;

				//if (!contain_in_doms(exp,block, rootNode->resultId)) {
				//	cse_exp_reg[exp][block] = rootNode->resultId;//维护表达式和寄存器的对应关系
				//}
				exp_gens.insert(exp);
			}
		}
		break;
	default:
		break;
	}
	delete avaM;
}


//unordered_set<string> exp_gens;
//unordered_set<string> diff_exp;
//
//for (auto dagnode = (*blockIt)->topoList.begin(); dagnode != (*blockIt)->topoList.end(); dagnode++) {
//	string exp;
//	switch ((*dagnode)->nodeType) {
//	case STORE:
//		if ((*dagnode)->inputs[0]->nodeType == VALUE_NODE) {
//			string varName = (*dagnode)->inputs[0]->name;
//			unordered_set<string> temp = exp_gens;
//			for (auto& it : temp) {
//				if (avaM->contain_operand(it, varName)) {
//					exp_gens.erase(it);
//					cse_exp_reg.erase(it);
//				}
//			}
//		}
//		break;
//	case OP_NODE:
//		if (dfa->ava_opSet.find((*dagnode)->name) == dfa->ava_opSet.end()) break;//跳过函数调用等
//	case LOAD:
//		if (cse_reg_exp.find((*dagnode)->resultId) == cse_reg_exp.end())
//			break;
//		//cout << "没有在reg_exp找到该表达式，错误" << endl;
//		exp = cse_reg_exp[(*dagnode)->resultId]; //找到该节点的表达式
//
//		//可以替换
//		if (exp_gens.find(exp) != exp_gens.end() && cse_exp_reg.find(exp) != cse_exp_reg.end() &&
//			(last_dom[(*blockIt)]->dataInfo->AvaOutSet.test(dfa->ava_exp_index[exp]) || (*blockIt)->dataInfo->AvaGens.test(dfa->ava_exp_index[exp]))) {
//			bool is_phi = false;
//			for (int i = 0; i < (*dagnode)->outputs.size(); i++) {
//				if ((*dagnode)->outputs[i]->name == "%PHI") {
//					is_phi = true;
//					break;
//				}
//			}
//			if (is_phi || (diff_exp.size() > 15 && diff_exp.find(exp) == diff_exp.end())) break;
//			node_to_reg((*dagnode), cse_exp_reg[exp]);
//			diff_exp.insert(exp);
//			vector<DAGNode*> temp = (*dagnode)->inputs;
//
//			//维护diff_exp，以达到更好的效果
//			for (int i = 0; i < temp.size(); i++) {
//				if (temp[i]->nodeType == OP_NODE || temp[i]->nodeType == VALUE_NODE) {
//
//				}
//			}
//
//			for (int i = 0; i < temp.size(); i++) {//移除该节点的输入边
//				(*blockIt)->dag->remove_edge((*dagnode), temp[i]);
//			}
//		}//由该块产生
//		else if ((!(*blockIt)->dataInfo->AvaInSet.test(dfa->ava_exp_index[exp]) || (*blockIt)->label == BasicBlock::WHILE_CONDITION)
//			&& (*blockIt)->dataInfo->AvaGens.test(dfa->ava_exp_index[exp]) && exp_gens.find(exp) == exp_gens.end()) {
//			cse_exp_reg[exp] = (*dagnode)->resultId;//维护表达式和寄存器的对应关系
//			exp_gens.insert(exp);
//		}
//		break;
//	default:
//		break;
//	}
//}


//BasicBlock* find_if_end(BasicBlock* begin) {
//	BasicBlock* post = begin;
//	int count = 0;//记录当前路径的嵌套的if
//	while ((post->label != BasicBlock::IF_END && post->label != BasicBlock::IF_END_AND_WHILEBODY) || count > 0 ) {
//		post = post->postBlock[0];
//		if (post->label == BasicBlock::IF_BEGIN) {
//			count++;
//		}
//		else if ((post->label == BasicBlock::IF_END || post->label == BasicBlock::IF_END_AND_WHILEBODY)) {
//			count--;
//		}
//	}
//	return post;//似乎不会为空？
//}

////记录一个节点是不是被拷贝了，如果拷贝了，就返回拷贝后的节点，没有的话返回nullptr
//map<DAGNode*, DAGNode*> hasCopiedNode;
//DAGNode* judgeIfCopied(DAGNode* node) {
//	auto it = hasCopiedNode.find(node);
//	if (it != hasCopiedNode.end()) {
//		return it->second;
//	}
//	return nullptr;
//}

////拷贝一段dagnode
////mode 0 为block拷贝时，mode 1 为afterblock拷贝时
//DAGNode* copyDagnode(DAGNode* copyRoot)
//{
//	DAGNode* newNode = judgeIfCopied(copyRoot);
//	if (!newNode) {
//		//没有拷贝过，拷贝本节点
//		if (copyRoot->symbol) {
//			newNode = new DAGNode(copyRoot, copyRoot->symbol);
//		}
//		else {
//			newNode = new DAGNode(copyRoot, nullptr);
//		}

//		//加入已经拷贝过的map
//		hasCopiedNode[copyRoot] = newNode;
//	}
//	else {
//		return newNode;
//	}

//	//拷贝输入节点
//	if (copyRoot->inputs.size() != 0) {
//		for (size_t i = 0; i < copyRoot->inputs.size(); i++) {
//			DAGNode* returnNode = copyDagnode(copyRoot->inputs[i]);
//			newNode->inputs.push_back(returnNode);
//			returnNode->outputs.push_back(newNode);
//		}
//	}

//	return newNode;
//}

////将分支中都存在的表达式提前到分支的条件块中
//void advance_exp(string exp, BasicBlock* if_begin, int index) {
//	//从任何一条路径找到当前exp所在的位置，post指向其基本块的位置
//	BasicBlock* post = if_begin->postBlock[0];
//	while (!post->dataInfo->AvaOutSet[index]) {
//		post = post->postBlock[0];
//	}

//	DAGNode* exp_start_node = NULL;

//	for (int i = 0; i < post->topoList.size(); i++) {
//		if (post->topoList[i]->nodeType == OP_NODE || post->topoList[i]->nodeType == LOAD) {
//			if (dfa->reg_exp[post->topoList[i]->resultId] == exp) {
//				exp_start_node = copyDagnode(post->topoList[i]);
//				break;
//			}
//		}
//	}

//	//将新的DAGnode插入到条件语句前面
//	if (exp_start_node == NULL) {
//		print("表达式插入错误");
//	}
//	else {
//		exp_start_node->relyNodes = if_begin->dag->rootNode->relyNodes;
//		if_begin->dag->rootNode->relyNodes.clear();
//		if_begin->dag->rootNode->relyNodes.push_back(exp_start_node);
//	}
//}


//void branch_move() {
//	dfa->availableExpressionsAnalysis();
//	int exp_num = dfa->ava_exp_vec.size();
//	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
//		bitset<SIZE> if_begin_set;
//		bitset<SIZE> if_end_set;
//		bitset<SIZE> joint_set;
//		if ((*blockIt)->label == BasicBlock::IF_BEGIN) {
//			if_begin_set = (*blockIt)->dataInfo->AvaOutSet;//检查if开始时的可用表达式
//			BasicBlock* temp = find_if_end((*blockIt));
//			if_end_set = temp->dataInfo->AvaInSet;//检查if结束的可用表达式
//			joint_set = if_end_set & ~if_begin_set;
//			for (int i = 0; i < exp_num; i++) {
//				if (joint_set[i]) {
//					advance_exp(dfa->ava_exp_vec[i], (*blockIt), i);//将公共子表达式提前
//				}
//			}
//		}
//	}
//}
