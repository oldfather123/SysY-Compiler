#include "RegisterSpill.h"
#include <unordered_map>
#include <regex>
#include <unordered_set>

//计算溢出代价
InterNode* spill_cost(DataFlowAnalysis*& dfa, vector<InterNode*>& stack) {
	if (stack.size() == 0) return NULL;
	//初始化
	map<string, long> costs;
	map<string, int> name_index;
	map<string, vector<string>> inter_coal;
	for (int i = 0; i < stack.size(); i++) {
		vector<string> result;
		stringstream ss(stack[i]->name);
		string token;

		while (getline(ss, token, '|')) {
			costs.insert(make_pair(token, 0));
			result.push_back(token);
		}

		inter_coal.insert(make_pair(stack[i]->name, result));
	}

	//分析
	dfa->lv_traversal(1);

	for (int i = 0; i < dfa->live_varsSet.size(); i++) {
		if (costs.find(dfa->live_varsSet[i]) != costs.end()) {
			name_index.insert(make_pair(dfa->live_varsSet[i], i));
		}
	}
	//计算涉及到的每个变量的代价
	for (auto costIt = costs.begin(); costIt != costs.end(); costIt++) {
		long cost = 0;
		for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
			int def_use_num = (*blockIt)->dataInfo->liveDefs.test(name_index[(*costIt).first]) + (*blockIt)->dataInfo->liveUses.test(name_index[(*costIt).first]);
			cost += def_use_num * pow((*blockIt)->LoopLevel, 10);
		}
		costs[(*costIt).first] = cost;
	}

	long min_cost = LONG_MAX;
	long max_cost = 0;
	InterNode* min_result = stack[0];
	//找到最小的合并结点
	map<string, string> min_merge_re;
	for (int i = 0; i < stack.size(); i++) {
		long sum_cost = 0;
		for (int j = 0; j < inter_coal[stack[i]->name].size(); j++) {
			sum_cost += costs[inter_coal[stack[i]->name][j]];
			//if (max_cost < costs[inter_coal[stack[i]->name][j]]) {
			//	max_cost = costs[inter_coal[stack[i]->name][j]];
			//	//min_merge_re[stack[i]->name] = inter_coal[stack[i]->name][j];
			//}
		}
		int degree = stack[i]->real_nodes.size();
		if (degree > 0 && (dfa->max_cost.find(stack[i]->name) == dfa->max_cost.end() || (dfa->max_cost.find(stack[i]->name) != dfa->max_cost.end() && dfa->name_num[stack[i]->name] > 1))) {
			sum_cost = sum_cost / degree;
		}
		else {//如果度为0，则将其设为最大
			sum_cost = LONG_MAX;
		}
		if (sum_cost < min_cost && dfa->cannot_spill.find(stack[i]->name) == dfa->cannot_spill.end()) {//没有溢出过，且最小
			min_cost = sum_cost;
			min_result = stack[i];
		}
	}

	return min_result;
	//return min_merge_re[min_result];
}


//计算保护寄存器的溢出代价
string spill_protect_cost(DataFlowAnalysis*& dfa, vector<string> protecct_nodes) {
	//初始化
	unordered_map<string, long> costs;
	unordered_map<string, int> name_index;
	unordered_map<string, vector<string>> inter_coal;
	for (int i = 0; i < protecct_nodes.size(); i++) {
		vector<string> result;
		stringstream ss(protecct_nodes[i]);
		string token;

		while (getline(ss, token, '|')) {
			costs.insert(make_pair(token, 0));
			result.push_back(token);
		}

		inter_coal.insert(make_pair(protecct_nodes[i], result));
	}

	for (int i = 0; i < dfa->live_varsSet.size(); i++) {
		if (costs.find(dfa->live_varsSet[i]) != costs.end()) {
			name_index.insert(make_pair(dfa->live_varsSet[i], i));
		}
	}
	//计算涉及到的每个变量的代价
	for (auto costIt = costs.begin(); costIt != costs.end(); costIt++) {
		long cost = 0;
		for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
			int def_use_num = (*blockIt)->dataInfo->liveDefs.test(name_index[(*costIt).first]) + (*blockIt)->dataInfo->liveUses.test(name_index[(*costIt).first]);
			cost += def_use_num * pow((*blockIt)->LoopLevel, 10);
		}
		costs[(*costIt).first] = cost;
	}

	long min_cost = LONG_MAX;
	string min_result;

	for (int i = 0; i < protecct_nodes.size(); i++) {
		long sum_cost = 0;
		for (int j = 0; j < inter_coal[protecct_nodes[i]].size(); j++) {
			sum_cost += costs[inter_coal[protecct_nodes[i]][j]];
		}
		if (sum_cost < min_cost) {
			min_cost = sum_cost;
			min_result = protecct_nodes[i];
		}
	}


	return min_result;
}


void spill_replaceIR(string merged_node, CFG* cfg, int i_or_f) {


	INSTRUCTIONS ifLD;
	INSTRUCTIONS ifSD;
	if (i_or_f == 0) {
		ifLD = LD;
		ifSD = SD;
	}
	else if (i_or_f == 1) {
		ifLD = FLD;
		ifSD = FSD;
	}


	//得到需要被替换的变量
	vector<int> indexes;
	vector<string> varnodes;
	vector<int> regnodes;
	char split_ch = '|';

	int pos = merged_node.find(split_ch);
	int lastpos = 0;
	while (pos != std::string::npos) {
		string tempnode = merged_node.substr(lastpos, pos - lastpos);
		if (tempnode.empty()) continue;//空节点
		else if (tempnode[0] == '%') regnodes.push_back(stoi(tempnode.substr(1)));//寄存器节点
		else  varnodes.push_back(tempnode);//变量节点

		lastpos = pos + 1;
		pos = merged_node.find(split_ch, pos + 1);
	}
	string tempnode = merged_node.substr(lastpos, pos - lastpos);
	if (tempnode.empty());//空节点
	else if (tempnode[0] == '%') regnodes.push_back(stoi(tempnode.substr(1)));//寄存器节点
	else  varnodes.push_back(tempnode);//变量节点





	map<string, int> var_loc;
	map<int, int> reg_loc;
	//替换所有节点，遍历dag
	for (int b_index = 0; b_index < cfg->basicBlocks.size(); b_index++) {
		BasicBlock* b = cfg->basicBlocks[b_index];
		vector<DAGNode*>& nodes = b->topoList;
		//只有load a和reg的使用形式是需要load的
		//其他都是store回去
		for (int sen_index = 0; sen_index < nodes.size(); sen_index++) {
			DAGNode* top = nodes[sen_index];
			//move操作
			if (top->inst == MV || top->inst == FMVS) {
				//可能是两个inputs，也可能是一个inputs
				if (top->inputs.size() == 1) {
					//对于input的变量
					//原本是load指令，且被load的变量确实是需要被替换的变量节点，需要替换成从内存load
					if (find(varnodes.begin(), varnodes.end(), top->inputs[0]->name) != varnodes.end()) {
						DAGNode* mem_load = new DAGNode(top->inputs[0], top->inputs[0]->symbol);
						mem_load->name = "%spill";
						mem_load->nodeType = LOAD;
						mem_load->resultId = cfg->getCount();//每个load都是重新分配的寄存器，且会被马上释放
						if (var_loc.find(top->inputs[0]->name) == var_loc.end()) {
							//cout << "spill:load before store" << top->inputs[0]->name << endl;
							mem_load->value.iValue = cfg->funcStackSize;
							var_loc[top->inputs[0]->name] = cfg->funcStackSize;
							cfg->funcStackSize += 2;
						}
						else {
							mem_load->value.iValue = var_loc[top->inputs[0]->name];
						}

						mem_load->inst = ifLD;

						//超立即数的访问
						if (mem_load->value.iValue * 4 >= 2048) {
							DAGNode* load_imm = new DAGNode("%spill", LOAD);
							load_imm->resultType = i32;
							load_imm->resultId = cfg->getCount();
							load_imm->value.iValue = 4 * mem_load->value.iValue;
							load_imm->inst = LI;
							//依赖关系
							mem_load->inputs.push_back(load_imm);
							load_imm->outputs.push_back(mem_load);
							//插入节点
							nodes.insert(nodes.begin() + sen_index, load_imm);
							sen_index++;
						}

						//更换依赖关系
						//假设只有一个依赖关系
						top->inputs[0] = mem_load;


						//可能存在报错
						//在move前load进来
						nodes.insert(nodes.begin() + sen_index, mem_load);
						sen_index++;

					}

					//对于输出的寄存器id
					if (find(regnodes.begin(), regnodes.end(), top->resultId) != regnodes.end()) {
						DAGNode* mem_store = new DAGNode(" ", STORE);
						mem_store->name = "%spill";
						//记录地址
						if (reg_loc.find(top->resultId) == reg_loc.end()) {
							mem_store->value.iValue = cfg->funcStackSize;
							reg_loc[top->resultId] = cfg->funcStackSize;
							cfg->funcStackSize += 2;
						}
						else {
							mem_store->value.iValue = reg_loc[top->resultId];
						}
						//更新load的id
						//top->resultId = cfg->getCount();
						//加一个寄存器节点存储
						//DAGNode* new_leftreg = new DAGNode(top->inputs[0], top->inputs[0]->symbol);
						//new_leftreg->nodeType = REG_NODE;
						//new_leftreg->resultId = cfg->getCount();
						//top->inputs.insert(top->inputs.begin(), new_leftreg);
						//new_leftreg->outputs.push_back(top);

						//给store存的节点
						//DAGNode* temp_reg = new_leftreg;
						//更换依赖关系
						//for (DAGNode* rely_iter : top->outputs) {
						//	*find(rely_iter->inputs.begin(), rely_iter->inputs.end(), top) = temp_reg;
						//}
						//分配指令
						mem_store->inst = ifSD;

						//超立即数的访问
						if (mem_store->value.iValue * 4 >= 2048) {
							DAGNode* load_imm = new DAGNode("%spill", LOAD);
							load_imm->resultType = i32;
							load_imm->resultId = cfg->getCount();
							load_imm->value.iValue = 4 * mem_store->value.iValue;
							load_imm->inst = LI;
							//依赖关系
							mem_store->inputs.insert(mem_store->inputs.begin(), load_imm);
							load_imm->outputs.push_back(mem_store);
							//插入节点
							nodes.insert(nodes.begin() + sen_index + 1, load_imm);
							sen_index++;
						}

						//依赖关系
						mem_store->inputs.push_back(top);
						top->outputs.push_back(mem_store);
						//toplist
						nodes.insert(nodes.begin() + sen_index + 1, mem_store);
						b->dag->nodes.push_back(mem_store);

						sen_index++;
					}

				}

				else if (top->inputs.size() == 2) {
					//就是move操作，一般不会作为其他节点的输入
					//move节点没有resultid，store只和左节点有关系
					DAGNode* leftnode = top->inputs[0];
					DAGNode* rightnode = top->inputs[1];
					if (leftnode->nodeType == VALUE_NODE) {
						//插入store语句
						if (find(varnodes.begin(), varnodes.end(), leftnode->name) != varnodes.end()) {
							DAGNode* mem_store = new DAGNode(" ", STORE);
							mem_store->name = "%spill";
							//记录地址
							if (var_loc.find(leftnode->name) == var_loc.end()) {
								mem_store->value.iValue = cfg->funcStackSize;
								var_loc[leftnode->name] = cfg->funcStackSize;
								cfg->funcStackSize += 2;
							}
							else {
								mem_store->value.iValue = var_loc[leftnode->name];
							}

							//分配指令
							mem_store->inst = ifSD;

							//需要新建寄存器节点，并替换位置
							DAGNode* new_leftreg = new DAGNode(leftnode, leftnode->symbol);
							new_leftreg->nodeType = REG_NODE;
							new_leftreg->resultId = cfg->getCount();
							top->inputs[0] = new_leftreg;
							new_leftreg->outputs.push_back(top);

							//给store存的节点
							DAGNode* temp_reg = new DAGNode(new_leftreg, new_leftreg->symbol);

							//依赖关系
							mem_store->inputs.push_back(temp_reg);
							temp_reg->outputs.push_back(mem_store);


							//超立即数的访问
							if (mem_store->value.iValue * 4 >= 2048) {
								DAGNode* load_imm = new DAGNode("%spill", LOAD);
								load_imm->resultType = i32;
								load_imm->resultId = cfg->getCount();
								load_imm->value.iValue = 4 * mem_store->value.iValue;
								load_imm->inst = LI;
								//依赖关系
								mem_store->inputs.insert(mem_store->inputs.begin(), load_imm);
								load_imm->outputs.push_back(mem_store);
								//插入节点
								nodes.insert(nodes.begin() + sen_index + 1, load_imm);
								sen_index++;
							}

							//toplist
							nodes.insert(nodes.begin() + sen_index + 1, mem_store);
							b->dag->nodes.push_back(mem_store);

							sen_index++;
						}
					}

					else if (leftnode->nodeType == REG_NODE) {
						if (find(regnodes.begin(), regnodes.end(), leftnode->resultId) != regnodes.end()) {
							DAGNode* mem_store = new DAGNode(" ", STORE);
							mem_store->name = "%spill";
							//记录地址
							if (reg_loc.find(leftnode->resultId) == reg_loc.end()) {
								mem_store->value.iValue = cfg->funcStackSize;
								reg_loc[leftnode->resultId] = cfg->funcStackSize;
								cfg->funcStackSize += 2;
							}
							else {
								mem_store->value.iValue = reg_loc[leftnode->resultId];
							}

							//分配指令
							mem_store->inst = ifSD;
							//需要新建寄存器节点，并替换位置
							DAGNode* new_leftreg = new DAGNode(leftnode, leftnode->symbol);
							new_leftreg->nodeType = REG_NODE;
							new_leftreg->resultId = cfg->getCount();
							top->inputs[0] = new_leftreg;
							new_leftreg->outputs.push_back(top);

							//给store存的节点
							DAGNode* temp_reg = new DAGNode(new_leftreg, new_leftreg->symbol);

							//依赖关系
							mem_store->inputs.push_back(temp_reg);
							temp_reg->outputs.push_back(mem_store);

							//超立即数的访问
							if (mem_store->value.iValue * 4 >= 2048) {
								DAGNode* load_imm = new DAGNode("%spill", LOAD);
								load_imm->resultType = i32;
								load_imm->resultId = cfg->getCount();
								load_imm->value.iValue = 4 * mem_store->value.iValue;
								load_imm->inst = LI;
								//依赖关系
								mem_store->inputs.insert(mem_store->inputs.begin(), load_imm);
								load_imm->outputs.push_back(mem_store);
								//插入节点
								nodes.insert(nodes.begin() + sen_index + 1, load_imm);
								sen_index++;
							}

							//toplist
							nodes.insert(nodes.begin() + sen_index + 1, mem_store);
							b->dag->nodes.push_back(mem_store);

							sen_index++;
						}
					}

					//右节点可能是reg。如果是其他op，则由暂存的id提供结果
					//双输入的move不应该作为input
					if (rightnode->nodeType == REG_NODE || rightnode->nodeType == OP_NODE || rightnode->nodeType == LOAD) {
						if (find(regnodes.begin(), regnodes.end(), rightnode->resultId) != regnodes.end()) {
							//使用到reg
							DAGNode* mem_load = new DAGNode(rightnode, rightnode->symbol);
							mem_load->name = "%spill";
							mem_load->nodeType = LOAD;
							mem_load->resultId = cfg->getCount();
							if (reg_loc.find(rightnode->resultId) == reg_loc.end()) {
								//cout << "spill:move load before store " << rightnode->resultId << endl;
								mem_load->value.iValue = cfg->funcStackSize;
								reg_loc[rightnode->resultId] = cfg->funcStackSize;
								cfg->funcStackSize += 2;
							}
							else {
								mem_load->value.iValue = reg_loc[rightnode->resultId];
							}
							mem_load->inst = ifLD;

							//超立即数的访问
							if (mem_load->value.iValue * 4 >= 2048) {
								DAGNode* load_imm = new DAGNode("%spill", LOAD);
								load_imm->resultType = i32;
								load_imm->resultId = cfg->getCount();
								load_imm->value.iValue = 4 * mem_load->value.iValue;
								load_imm->inst = LI;
								//依赖关系
								mem_load->inputs.push_back(load_imm);
								load_imm->outputs.push_back(mem_load);
								//插入节点
								nodes.insert(nodes.begin() + sen_index, load_imm);
								sen_index++;
							}


							//使用之前插入
							nodes.insert(nodes.begin() + sen_index, mem_load);
							sen_index++;//回到当前语句
							//替换右节点
							top->inputs[1] = mem_load;
							//更换依赖关系

						}
					}
				}

			}

			else if (top->nodeType == OP_NODE || top->nodeType == LOAD || top->nodeType == STORE) {
				//为其他操作的值，进行溢出/存入的寄存器
				//先看源操作数
				DAGNode* leftnode = top;
				for (int input_index = 0; input_index < top->inputs.size(); input_index++) {
					DAGNode* rightnode = top->inputs[input_index];
					//右节点可能是reg
					//右节点可能是sw
					//or arraysub
					if (rightnode->nodeType == REG_NODE || rightnode->nodeType == OP_NODE || rightnode->nodeType == LOAD) {
						if (find(regnodes.begin(), regnodes.end(), rightnode->resultId) != regnodes.end()) {
							//使用到reg
							DAGNode* mem_load = new DAGNode(rightnode, rightnode->symbol);
							mem_load->name = "%spill";
							mem_load->nodeType = LOAD;
							mem_load->resultId = cfg->getCount();
							if (reg_loc.find(rightnode->resultId) == reg_loc.end()) {
								//cout << "spill:opnode load before store " << rightnode->resultId << endl;
								mem_load->value.iValue = cfg->funcStackSize;
								reg_loc[rightnode->resultId] = cfg->funcStackSize;
								cfg->funcStackSize += 2;
							}
							else {
								mem_load->value.iValue = reg_loc[rightnode->resultId];
							}

							mem_load->inst = ifLD;

							//超立即数的访问
							if (mem_load->value.iValue * 4 >= 2048) {
								DAGNode* load_imm = new DAGNode("%spill", LOAD);
								load_imm->resultType = i32;
								load_imm->resultId = cfg->getCount();
								load_imm->value.iValue = 4 * mem_load->value.iValue;
								load_imm->inst = LI;
								//依赖关系
								mem_load->inputs.push_back(load_imm);
								load_imm->outputs.push_back(mem_load);
								//插入节点
								nodes.insert(nodes.begin() + sen_index, load_imm);
								sen_index++;
							}

							//使用之前插入
							nodes.insert(nodes.begin() + sen_index, mem_load);
							sen_index++;//回到当前语句
							//更换依赖关系
							if (leftnode == top)
								top->inputs[input_index] = mem_load;
							else
								leftnode->inputs[0] = mem_load;

						}
					}
					else if (rightnode->nodeType == VALUE_NODE) {
						if (find(varnodes.begin(), varnodes.end(), rightnode->name) != varnodes.end()) {
							DAGNode* mem_load = new DAGNode(rightnode, rightnode->symbol);
							mem_load->name = "%spill";
							mem_load->nodeType = LOAD;
							mem_load->resultId = cfg->getCount();//每个load都是重新分配的寄存器，且会被马上释放
							if (var_loc.find(rightnode->name) == var_loc.end()) {
								//cout << "spill:load before store" << top->inputs[0]->name << endl;
								mem_load->value.iValue = cfg->funcStackSize;
								var_loc[rightnode->name] = cfg->funcStackSize;
								cfg->funcStackSize += 2;
							}
							else {
								mem_load->value.iValue = var_loc[rightnode->name];
							}

							mem_load->inst = ifLD;

							//超立即数的访问
							if (mem_load->value.iValue * 4 >= 2048) {
								DAGNode* load_imm = new DAGNode("%spill", LOAD);
								load_imm->resultType = i32;
								load_imm->resultId = cfg->getCount();
								load_imm->value.iValue = 4 * mem_load->value.iValue;
								load_imm->inst = LI;
								//依赖关系
								mem_load->inputs.push_back(load_imm);
								load_imm->outputs.push_back(mem_load);
								//插入节点
								nodes.insert(nodes.begin() + sen_index, load_imm);
								sen_index++;
							}

							//更换依赖关系
							//假设只有一个依赖关系
							top->inputs[input_index] = mem_load;


							//可能存在报错
							//在move前load进来
							nodes.insert(nodes.begin() + sen_index, mem_load);
							sen_index++;

						}
					}
				}

				//再看存回目的寄存器
				leftnode = top;
				if (find(regnodes.begin(), regnodes.end(), leftnode->resultId) != regnodes.end()) {
					DAGNode* mem_store = new DAGNode(" ", STORE);
					mem_store->name = "%spill";
					//记录地址
					if (reg_loc.find(leftnode->resultId) == reg_loc.end()) {
						mem_store->value.iValue = cfg->funcStackSize;
						reg_loc[leftnode->resultId] = cfg->funcStackSize;
						cfg->funcStackSize += 2;
					}
					else {
						mem_store->value.iValue = reg_loc[leftnode->resultId];
					}

					//重新分配虚拟存储器编号
					//leftnode->resultId = cfg->getCount();

					//分配指令
					mem_store->inst = ifSD;
					//依赖关系
					mem_store->inputs.push_back(leftnode);
					leftnode->outputs.push_back(mem_store);

					//超立即数的访问
					if (mem_store->value.iValue * 4 >= 2048) {
						DAGNode* load_imm = new DAGNode("%spill", LOAD);
						load_imm->resultType = i32;
						load_imm->resultId = cfg->getCount();
						load_imm->value.iValue = 4 * mem_store->value.iValue;
						load_imm->inst = LI;
						//依赖关系
						mem_store->inputs.insert(mem_store->inputs.begin(), load_imm);
						load_imm->outputs.push_back(mem_store);
						//插入节点
						nodes.insert(nodes.begin() + sen_index + 1, load_imm);
						sen_index++;
					}

					//toplist
					nodes.insert(nodes.begin() + sen_index + 1, mem_store);
					b->dag->nodes.push_back(mem_store);
					sen_index++;
				}
			}

		}
	}


}


InterNode* reg_spill(DataFlowAnalysis* dfa, vector<InterNode*>& stack, int i_or_f) {

	InterNode* spillnode = spill_cost(dfa, stack);

	spill_replaceIR(spillnode->name, dfa->cfg, i_or_f);

	return spillnode;
}



//溢出保护寄存器
void choose_protect(DataFlowAnalysis*& dfa, int reg_num, set<InterNode*> prot_set, int i_or_f, set<string>& spilled) {
	unordered_map<InterNode*, int> prot_index;
	int count = 0;
	vector<InterNode*> temp_prot(prot_set.begin(), prot_set.end());
	for (auto& it : temp_prot) {
		if (spilled.find(it->name) == spilled.end()) {
			prot_index[it] = count;
			count++;
		}
		else {
			prot_set.erase(it);
		}
	}
	//不需要溢出
	if (prot_index.size() <= reg_num) return;

	//计算合并
	for (auto& f32 : prot_set) {
		for (auto& vir_f32 : f32->virtual_nodes) {
			if (vir_f32->func) {
				prot_index[vir_f32] = prot_index[f32];
			}
		}
	}
	//合并
	int max_merge = -1;
	map<int, string> merge_map;
	for (auto& mapIt : prot_index) {
		if (merge_map.find(mapIt.second) == merge_map.end()) {
			merge_map[mapIt.second] = mapIt.first->name;
		}
		else {
			merge_map[mapIt.second] += "|" + mapIt.first->name;
		}
	}
	vector<string> pro_merge_vec;
	for (auto& it : merge_map) {
		pro_merge_vec.push_back(it.second);
	}
	max_merge = pro_merge_vec.size();
	//溢出
	while (max_merge >= reg_num) {
		string spill_node = spill_protect_cost(dfa, pro_merge_vec);
		spilled.insert(spill_node);
		spill_replaceIR(spill_node, dfa->cfg, i_or_f);
		max_merge--;
		pro_merge_vec.erase(remove(pro_merge_vec.begin(), pro_merge_vec.end(), spill_node), pro_merge_vec.end());
	}

}

//提前溢出保护寄存器
void pre_spill_protect(DataFlowAnalysis*& dfa, IGManager*& ig) {
	dfa->liveVariablesAnalysis(1);
	//创建干涉图节点
	for (int i = 0; i < dfa->live_varsSet.size(); i++) {
		if (dfa->live_typeMap[dfa->live_varsSet[i]] == iarray || dfa->live_typeMap[dfa->live_varsSet[i]] == farray) continue;
		InterNode* newNode = new InterNode();
		newNode->name = dfa->live_varsSet[i];
		newNode->type = dfa->live_typeMap[dfa->live_varsSet[i]];
		if (dfa->live_typeMap[dfa->live_varsSet[i]] == f32) {
			ig->fGraph->nodes.insert(newNode);
			ig->fGraph->nodesMap[dfa->live_varsSet[i]] = newNode;
		}
		else {
			ig->iGraph->nodes.insert(newNode);
			ig->iGraph->nodesMap[dfa->live_varsSet[i]] = newNode;
		}
		/*nodesMap[dfa->live_varsSet[i]] = newNode;*/
	}
	pre_tra(dfa, ig->iGraph, 0);
	pre_tra(dfa, ig->fGraph, 1);
}

void pre_tra(DataFlowAnalysis*& dfa, InterferenceGraph*& graph, int i_or_f) {
	if (graph->nodesMap.size() == 0) return;
	int reg_num = i_or_f ? fcallee : icallee;
	vector<set<InterNode*>> spill_prot;
	set<string> spilled;

	regex pattern("f%[i|f][0-7]");
	//加边
	for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
		vector<string> temp = (*blockIt)->dataInfo->bitSet_to_vector((*blockIt)->dataInfo->liveOutSet, dfa->live_varsSet);//标志当前活跃的变量
		set<string> live_set(temp.begin(), temp.end());
		//倒序遍历
		for (auto dagNode = (*blockIt)->topoList.rbegin(); dagNode != (*blockIt)->topoList.rend(); dagNode++) {
			string useName;
			string defName;
			switch ((*dagNode)->nodeType) {
			case LOAD:
				useName = "";
				defName = "%" + to_string((*dagNode)->resultId);
				if ((*dagNode)->inputs.size() > 0 && (*dagNode)->inputs[0]->resultType != iarray && (*dagNode)->inputs[0]->resultType != farray) {
					useName = (*dagNode)->inputs[0]->nodeType == VALUE_NODE ? (*dagNode)->inputs[0]->name : ("%" + to_string((*dagNode)->inputs[0]->resultId));
					live_set.insert(useName);
				}
				live_set.erase(defName);
				break;
			case STORE:
				useName = "";
				for (int i = 0; i < (*dagNode)->inputs.size(); i++) {
					if ((*dagNode)->inputs[i]->nodeType != CONSTANT && (*dagNode)->inputs[i]->resultType != iarray && (*dagNode)->inputs[i]->resultType != farray) {//只有use？
						useName = (*dagNode)->inputs[i]->nodeType == VALUE_NODE ? (*dagNode)->inputs[i]->name : ("%" + to_string((*dagNode)->inputs[i]->resultId));
						live_set.insert(useName);
					}
				}
				break;
			case OP_NODE:
				useName = "";
				defName = "";
				if ((*dagNode)->inst == MV || (*dagNode)->inst == FMVS) {
					if ((*dagNode)->inputs.size() == 1) {
						defName = "%" + to_string((*dagNode)->resultId);
						useName = (*dagNode)->inputs[0]->nodeType == VALUE_NODE ? (*dagNode)->inputs[0]->name : ("%" + to_string((*dagNode)->inputs[0]->resultId));
					}
					else if ((*dagNode)->inputs.size() == 2) {
						defName = (*dagNode)->inputs[0]->nodeType == VALUE_NODE ? (*dagNode)->inputs[0]->name : ("%" + to_string((*dagNode)->inputs[0]->resultId));
						useName = (*dagNode)->inputs[1]->nodeType == VALUE_NODE ? (*dagNode)->inputs[1]->name : ("%" + to_string((*dagNode)->inputs[1]->resultId));
					}
					live_set.erase(defName);
					live_set.insert(useName);
					//if (graph->nodesMap.find(defName) != graph->nodesMap.end() && graph->nodesMap.find(useName) != graph->nodesMap.end()) {
					//	graph->nodesMap[defName]->add_virtual(graph->nodesMap[useName]); //添加虚边；
					//}
				}
				else {
					defName = "%" + to_string((*dagNode)->resultId);
					if ((*dagNode)->inst == LUI) {
						live_set.erase(defName);
						break;
					}
					//判断是不是函数调用
					if ((*dagNode)->inst == CALL) {
						live_set.erase(defName);
						//int temp_i32 = 0;
						set<InterNode*> protect;
						for (auto& it : live_set) {
							if (!regex_match(it, pattern) && graph->nodesMap.find(it) != graph->nodesMap.end()) {
								protect.insert(graph->nodesMap[it]);
							}
						}
						if (protect.size() > reg_num) {
							spill_prot.push_back(protect);
							//choose_protect(dfa, reg_num, protect, i_or_f);
						}
					}
					for (int i = 0; i < (*dagNode)->inputs.size(); i++) {
						if ((*dagNode)->inputs[i]->nodeType == CONSTANT || (*dagNode)->inputs[i]->resultType == iarray || (*dagNode)->inputs[i]->resultType == farray
							|| (*dagNode)->inputs[i]->inst == SD || (*dagNode)->inputs[i]->inst == FSD) continue;
						useName = (*dagNode)->inputs[i]->nodeType == VALUE_NODE ? (*dagNode)->inputs[i]->name : ("%" + to_string((*dagNode)->inputs[i]->resultId));
						live_set.insert(useName);
					}
					live_set.erase(defName);
				}
				break;
			default:
				break;
			}
		}

		for (int i = 0; i < spill_prot.size(); i++) {
			choose_protect(dfa, reg_num, spill_prot[i], i_or_f, spilled);
		}
	}
}