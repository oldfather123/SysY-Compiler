#include "StackFrameExtension.h"

set<BasicBlock::LabelType> condition_label = { BasicBlock::LabelType::WHILE_CONDITION, BasicBlock::LabelType::IF_BEGIN, BasicBlock::LabelType::IF_END_AND_WHILEBODY };

//计算支配点集
void calcu_dom(CFG* cfg, map<BasicBlock*, set<BasicBlock*>>& doms) {
	size_t n = cfg->basicBlocks.size();
	for (size_t i = 0; i < n; i++) {
		set<BasicBlock*> temp_dom = { cfg->basicBlocks[i] };
		doms.insert(make_pair(cfg->basicBlocks[i], temp_dom));
	}

	bool changed = true;
	while (changed) {
		changed = false;
		for (size_t i = 0; i < n; i++) {
			set<BasicBlock*> temp = calcu_dom_inters(cfg->basicBlocks[i]->preBlock, doms);
			temp.insert(cfg->basicBlocks[i]);
			if (temp != doms[cfg->basicBlocks[i]]) {
				changed = true;
				doms[cfg->basicBlocks[i]] = temp;
			}
		}
	}
}

//计算当前基本块的前置块的支配集的交集
set<BasicBlock*> calcu_dom_inters(vector<BasicBlock*> pres, map<BasicBlock*, set<BasicBlock*>> doms) {
	set<BasicBlock*> pre_dom;
	if (pres.size() >= 1) {
		pre_dom = doms[pres[0]];
		for (size_t i = 1; i < pres.size(); i++) {
			set_intersection(pre_dom.begin(), pre_dom.end(), doms[pres[i]].begin(), doms[pres[i]].end(), std::inserter(pre_dom, pre_dom.begin()));
		}
	}

	return pre_dom;
}

bool get_use(DAGNode* node, string name) {
	for (int i = 0; i < node->inputs.size(); i++) {
		if (node->inputs[i]->nodeType != CONSTANT && node->inputs[i]->inst != SW) {
			string useNmae = node->inputs[i]->nodeType == VALUE_NODE ? node->inputs[i]->name : ("%" + to_string(node->inputs[i]->resultId));
			if (useNmae == name)
				return true;
		}
	}
	/*if (node->name == name) {
		return true;
	}
	for (int i = 0; i < node->inputs.size(); i++) {
		if (get_use(node->inputs[i], name))
			return true;
	}*/

	return false;
}

//在基本块中找到需要插入的下一条语句
int find_Insert(BasicBlock* block, string name) {
	for (int i = 0; i < block->topoList.size(); i++) {
		if (get_use(block->topoList[i], name)) {
			return i;
		}
	}
	return 0;
	/*DAGNode* first_state = block->dag->rootNode;
	DAGNode* current_state = block->dag->rootNode;
	while (current_state->relyNodes.size() != 0) {
		current_state = current_state->relyNodes[0];
		if (get_use(current_state, name)) {
			first_state = current_state;
		}
	}
	return first_state;*/
}

//处理一个函数多余的形参
void stack_frame_extension(DataFlowAnalysis* dfa, SymbolTable* sym) {
	vector<Symbol*> temp = sym->getFuncSymbols(dfa->cfg->name);
	int i32_para = 0;
	int f32_para = 0;

	vector<string> i32_stack_para;
	vector<string> f32_stack_para;

	map<string, Symbol*> name_sym;

	for (int i = 0; i < temp.size(); i++) {
		if (temp[i]->type == f32) {
			f32_para++;
		}
		else {
			i32_para++;
		}

		if (i32_para > 8 && temp[i]->type != f32) {
			i32_stack_para.push_back(temp[i]->name);
			name_sym.insert(make_pair(temp[i]->name, temp[i]));
		}
		else if (f32_para > 8 && temp[i]->type == f32) {
			f32_stack_para.push_back(temp[i]->name);
			name_sym.insert(make_pair(temp[i]->name, temp[i]));
		}
	}

	if (i32_para <= 8 && f32_para <= 8) return;//无需操作

	vector<string> stack_para(i32_stack_para.begin(), i32_stack_para.end());
	stack_para.insert(stack_para.end(), f32_stack_para.begin(), f32_stack_para.end());//所有要放进栈中的形参

	//map<BasicBlock*, set<BasicBlock*>> doms;
	//calcu_dom(dfa->cfg, doms);

	//dfa->topologicalSort();
	dfa->lv_traversal(1);//分析

	for (int i = 0; i < stack_para.size(); i++) {
		int index = find_inVec(dfa->live_varsSet, stack_para[i]);
		if (index < 0) continue;

		//找到所有使用的块
		vector<BasicBlock*> para_blocks;
		int min_id = dfa->cfg->basicBlocks.size();
		BasicBlock* min_para_block = dfa->cfg->entry;//id最小的块
		for (int j = 0; j < dfa->cfg->basicBlocks.size(); j++) {
			if (dfa->cfg->basicBlocks[j]->dataInfo->liveUses[index]) {
				para_blocks.push_back(dfa->cfg->basicBlocks[j]);
				/*if (dfa->cfg->basicBlocks[j]->id < min_id) {
					min_id = dfa->cfg->basicBlocks[j]->id;
					min_para_block = dfa->cfg->basicBlocks[j];
				}*/
			}
		}

		for (auto para_it : para_blocks) {
			//找到插入的点
			int insert_index = find_Insert(para_it, stack_para[i]);
			//组装插入点
			DAGNode* load_node = new DAGNode(ARGUMENT_SPILL, LOAD);
			load_node->resultId = dfa->cfg->getCount();
			load_node->resultType = name_sym[stack_para[i]]->type;
			if (load_node->resultType == f32) {
				load_node->inst = FLD;
			}
			else {
				load_node->inst = LD;
			}
			load_node->value.iValue = -2 * i;

			DAGNode* var_node = new DAGNode(stack_para[i], VALUE_NODE);
			var_node->resultType = name_sym[stack_para[i]]->type;

			DAGNode* mov_node = new DAGNode(" ", OP_NODE);
			mov_node->resultId = dfa->cfg->getCount();
			mov_node->resultType = name_sym[stack_para[i]]->type;
			if (mov_node->resultType == f32) {
				mov_node->inst = FMVS;
			}
			else {
				mov_node->inst = MV;
			}

			para_it->dag->add_edge(mov_node, var_node);//加边
			para_it->dag->add_edge(mov_node, load_node);

			para_it->topoList.insert(para_it->topoList.begin() + insert_index, mov_node);
			para_it->topoList.insert(para_it->topoList.begin() + insert_index, load_node);


			if (load_node->value.iValue * 4 <= -2048) {
				DAGNode* li_node = new DAGNode("%argu_li", OP_NODE);
				li_node->resultId = dfa->cfg->getCount();
				li_node->resultType = i32;
				li_node->inst = LI;
				li_node->value.iValue = -load_node->value.iValue * 4;
				para_it->dag->add_edge(load_node, li_node);
				
				para_it->topoList.insert(para_it->topoList.begin() + insert_index, li_node);
			}
			//开始插
		}

		//找到插入的块
		/*set<BasicBlock*> inter_set = calcu_dom_inters(para_blocks, doms);
		set<BasicBlock*>::iterator inter_it = inter_set.begin();
		BasicBlock* insert_block = *inter_it;
		for (; inter_it != inter_set.end(); inter_it++) {
			if ((*inter_it)->id > insert_block->id) {
				insert_block = *inter_it;
			}
		}*/

		////组装插入点
		//DAGNode* load_node = new DAGNode(ARGUMENT_SPILL, LOAD);
		//load_node->resultId = dfa->cfg->getCount();
		//load_node->resultType = name_sym[stack_para[i]]->type;
		//if (load_node->resultType == f32) {
		//	load_node->inst = FLD;
		//}
		//else {
		//	load_node->inst = LD;
		//}
		//load_node->value.iValue = -2 * i;

		//DAGNode* var_node = new DAGNode(stack_para[i], VALUE_NODE);
		//var_node->resultType = name_sym[stack_para[i]]->type;

		//DAGNode* mov_node = new DAGNode(" ", OP_NODE);
		//mov_node->resultId = dfa->cfg->getCount();
		//mov_node->resultType = name_sym[stack_para[i]]->type;
		//if (mov_node->resultType == f32) {
		//	mov_node->inst = FMVS;
		//}
		//else {
		//	mov_node->inst = MV;
		//}
		////如果即将插入的块是条件块，那么就将该语句插入到条件块的前面
		///*if (condition_label.find(insert_block->label) != condition_label.end()) {
		//	insert_block = insert_block->preBlock[0]->label == BasicBlock::LabelType::WHILEBODY ? insert_block->preBlock[1] : insert_block->preBlock[0];
		//}*/

		//insert_block->dag->add_edge(mov_node, var_node);//加边
		//insert_block->dag->add_edge(mov_node, load_node);

		//insert_block->dag->nodes.push_back(load_node);//加节点
		//insert_block->dag->nodes.push_back(var_node);
		//insert_block->dag->nodes.push_back(mov_node);

		//if (min_para_block->id == insert_block->id) {//将mov语句插入合适的程序点
		//	DAGNode* temp = find_Insert(min_para_block,stack_para[i]);
		//	mov_node->relyNodes = temp->relyNodes;
		//	vector<DAGNode*> rely;
		//	rely.push_back(mov_node);
		//	temp->relyNodes = rely;
		//}  
		//else {
		//	mov_node->relyNodes = insert_block->dag->rootNode->relyNodes;
		//	vector<DAGNode*> rely;
		//	rely.push_back(mov_node);
		//	insert_block->dag->rootNode->relyNodes = rely;
		//}
		//vector<DAGNode*> rely = insert_block->dag->rootNode->relyNodes;//加依赖边
		//insert_block->dag->rootNode->relyNodes.clear();
		//insert_block->dag->rootNode->relyNodes.push_back(mov_node);
		//mov_node->relyNodes = rely;
	}

}


int find_inVec(vector<string> live_vec, string para) {
	for (int i = 0; i < live_vec.size(); i++) {
		if (live_vec[i] == para) {
			return i;
		}
	}
	return -1;//没有在后续使用
}