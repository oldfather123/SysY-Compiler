#include "DataFlowAnalysis.h"

void DataFlowAnalysis::topologicalSort() {
	for (auto currentBlock = this->cfg->basicBlocks.begin(); currentBlock != this->cfg->basicBlocks.end(); currentBlock++) {
		int n = (*currentBlock)->dag->nodes.size();
		if (n <= 0) continue;
		set<DAGNode*> temp;//便于查找
		(*currentBlock)->topoList.clear();
		topo_dfs(temp, (*currentBlock)->topoList, (*currentBlock)->dag->rootNode);
		vector<DAGNode*> topo_temp;
		for (int i = 0; i < (*currentBlock)->topoList.size(); i++) {
			if ((*currentBlock)->topoList[i]->nodeType == STORE || (*currentBlock)->topoList[i]->nodeType == LOAD ||
				(*currentBlock)->topoList[i]->nodeType == OP_NODE) {
				topo_temp.push_back((*currentBlock)->topoList[i]);
			}
		}
		(*currentBlock)->topoList = topo_temp;
	}
}

//拓扑排序
void DataFlowAnalysis::topologicalSort(BasicBlock* currentBlock) {
	int n = currentBlock->dag->nodes.size();
	if (n <= 0) return;
	set<DAGNode*> temp;//便于查找
	currentBlock->topoList.clear();
	topo_dfs(temp,currentBlock->topoList, currentBlock->dag->rootNode);
	/*if (currentBlock->topoList.size() != n) { 
		cout << currentBlock->id<<"拓扑排序没有排完，是否有不可达节点" << endl;
	}*/
}
//拓扑排序的深度优先
void  DataFlowAnalysis::topo_dfs(set<DAGNode*> &temp,vector<DAGNode*> &list,DAGNode* node) {
	for (int j = 0; j < node->relyNodes.size(); j++) {//先访问第一个语句
		topo_dfs(temp, list, node->relyNodes[j]);
	}
	if (node->nodeType == STORE || node->inst == MV) {
		for (auto nodeIt = node->inputs.rbegin(); nodeIt != node->inputs.rend(); nodeIt++) {
			topo_dfs(temp, list, *nodeIt);
		}
	}
	else{
		for (int i = 0; i < node->inputs.size(); i++) {
			topo_dfs(temp, list, node->inputs[i]);
		}
	}
	if (temp.find(node) == temp.end()) {
		list.push_back(node);
		temp.insert(node);
	}
}


//实现gen和kill集的实现
void DataFlowAnalysis::rd_traversal() {
	//完成定值的采集
	def_count = 0;//初始化
	def_var.clear();
	def_varMap.clear();
	for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); blockIt++) {
		topologicalSort(*blockIt);//拓扑排序
		for (auto dagnode : (*blockIt)->topoList) {
			if (dagnode->nodeType == VALUE_NODE || dagnode->nodeType == CONSTANT || (dagnode->name== BRANCH_NODE&&dagnode->resultId == -842150451)) {
				continue;
			}
			//defValue.push_back(dagnode->resultId);
			string varName;
			FoldingValue def_value;

			if (dagnode->nodeType == STORE) {
				if (dagnode->name == ARRAYSUB_NODE && dagnode->nodeType == OP_NODE) {
					varName = dagnode->inputs[sd_variableName]->inputs[0]->name;
				}
				else {
					varName = dagnode->inputs[sd_variableName]->name;//store节点的第一个输入
				}
			}
			else if (dagnode->nodeType == OP_NODE||dagnode->nodeType == LOAD) {
				varName = "%" + to_string(dagnode->resultId);
			}
			(*blockIt)->dataInfo->defGens.set(def_count, true);//将其加入当前基本块的gen集

			//将新定值加入
			if (def_varMap.find(varName) != def_varMap.end()) {
				def_varMap[varName].push_back(def_count);
			}
			else {
				vector<unsigned int> temp;
				temp.clear();
				temp.push_back(def_count);
				def_varMap.insert(make_pair(varName, temp));
				def_var.push_back(varName);//新变量
				(*blockIt)->dataInfo->variableList.push_back(varName);//将其加到基本块的变量集中
			}

			def_count++;//定值数量加一
		}
	}

	//打印变量表
	for (const auto& pair : def_varMap) {
		string s = "Var: " + pair.first + ", Value : ";
		for (int i = 0; i < pair.second.size(); i++) {
			s +=  to_string(pair.second[i]) + " ";
		}
		print(s);
		def_var.push_back(pair.first);//将变量加入
	}

	//计算kill集
	for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); blockIt++) {
		(*blockIt)->dataInfo->defKill.reset();
		for (string var : (*blockIt)->dataInfo->variableList) {
			for (long dNum : def_varMap.find(var)->second)
				(*blockIt)->dataInfo->defKill.set(dNum);
		}
		(*blockIt)->dataInfo->defKill = (*blockIt)->dataInfo->defKill & ~(*blockIt)->dataInfo->defGens;//前面的部分包括了生成的，所以这里去掉
		string gens = (*blockIt)->dataInfo->defGens.to_string();
		string kills = (*blockIt)->dataInfo->defKill.to_string();
		string s = "kill集：" +  kills.erase(0, SIZE - def_count + 1);
		print(s);
		s = "gens集：" + gens.erase(0, SIZE - def_count + 1);
		print(s);
	}

}


void DataFlowAnalysis::lv_traversal() {
	live_varsSet.clear();
	live_typeMap.clear();
	set<string> temp_live;//便于查找
	set<string> useBuff;//使用变量
	set<string> defBuff;//定义变量
	for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); blockIt++) {
		topologicalSort(*blockIt);//拓扑排序
		useBuff.clear();
		defBuff.clear();
		for (auto dagnode : (*blockIt)->topoList) {
			string name;//被赋值的名字
			string varName;
			string useName;
			switch (dagnode->nodeType)
			{
			case LOAD:
				name = "%" + to_string(dagnode->resultId);
				if (temp_live.find(name) == temp_live.end()) {
					temp_live.insert(name);
					live_varsSet.push_back(name);
					live_typeMap.insert(make_pair(name, dagnode->resultType));
				}

				if (dagnode->inputs.size() == 0) {//从栈中取值,在比较后面会用到
					defBuff.insert(name);
					break;
				}

				if (dagnode->inputs[0]->name == ARRAYSUB_NODE) {//数组
					useName = dagnode->inputs[0]->inputs[0]->name;
				}
				else {
					useName = dagnode->inputs[0]->name;
				}
				if (temp_live.find(useName) == temp_live.end()) {
					temp_live.insert(useName);
					live_varsSet.push_back(useName);
					live_typeMap.insert(make_pair(useName, dagnode->resultType));
				}

				if (defBuff.find(useName) == defBuff.end()) {
					useBuff.insert(useName);
				}

				defBuff.insert(name);//%x = load a;
				break;
			case OP_NODE:
				if (dagnode->name != BRANCH_NODE) {//不处理无条件跳转
					name = "%" + to_string(dagnode->resultId);
					if (temp_live.find(name) == temp_live.end()) {
						temp_live.insert(name);
						live_varsSet.push_back(name);
						live_typeMap.insert(make_pair(name, dagnode->resultType));
					}
					defBuff.insert(name);
				}

				for (int i = 0; i < dagnode->inputs.size(); i++) {//将该节点的输入加入到use集中
					string tempName;
					if (dagnode->inputs[i]->nodeType == OP_NODE || dagnode->inputs[i]->nodeType == LOAD||dagnode->inputs[i]->nodeType == REG_NODE) {
						tempName = "%" + to_string(dagnode->inputs[i]->resultId);
						if (defBuff.find(tempName) == defBuff.end()) {//说明使用(use)先于可能对它的定值(def)
							useBuff.insert(tempName);
						}
					}
					else if (dagnode->inputs[i]->nodeType == VALUE_NODE) {
						tempName = dagnode->inputs[i]->name;
						if (defBuff.find(tempName) == defBuff.end()) {//说明使用(use)先于可能对它的定值(def)
							useBuff.insert(tempName);
						}
					}
				}
				break;
			case STORE:
				if (dagnode->inputs.size() < 2) break;
				if (dagnode->inputs[sd_assignment]->nodeType != CONSTANT) {
					if (dagnode->inputs[sd_assignment]->nodeType == OP_NODE || dagnode->inputs[sd_assignment]->nodeType == LOAD || dagnode->inputs[sd_assignment]->nodeType == REG_NODE) {
						useName = "%" + to_string(dagnode->inputs[sd_assignment]->resultId);
					}
					else if (dagnode->inputs[sd_assignment]->nodeType == VALUE_NODE) {
						useName = dagnode->inputs[sd_assignment]->name;
					}
					if (defBuff.find(useName) == defBuff.end()) {//说明使用先于任何可能对它的定值
						useBuff.insert(useName);
					}
				}
				if (dagnode->inputs[sd_variableName]->name == ARRAYSUB_NODE) {//数组
					varName = dagnode->inputs[sd_variableName]->inputs[0]->name;
				}
				else if (dagnode->inputs[sd_variableName]->nodeType==VALUE_NODE) {
					varName = dagnode->inputs[sd_variableName]->name;
				}

				if (temp_live.find(varName) == temp_live.end()) {
					temp_live.insert(varName);
					live_varsSet.push_back(varName);
					live_typeMap.insert(make_pair(varName, dagnode->inputs[sd_variableName]->resultType));
				}
				defBuff.insert(varName);
				break;
			default:
				break;
			}

		}

		//初始化
		(*blockIt)->dataInfo->liveUses.reset();
		(*blockIt)->dataInfo->liveDefs.reset();

		//得出基本块的use和def集
		for (auto useIt = useBuff.begin(); useIt != useBuff.end(); ++useIt) {
			auto liveIt = find(live_varsSet.begin(), live_varsSet.end(), *useIt);
			if (liveIt != live_varsSet.end()) {
				int index = std::distance(live_varsSet.begin(), liveIt);
				(*blockIt)->dataInfo->liveUses.set(index);
			}
		}
		for (auto defIt = defBuff.begin(); defIt != defBuff.end(); ++defIt) {
			auto liveIt = find(live_varsSet.begin(), live_varsSet.end(), *defIt);
			if (liveIt != live_varsSet.end()) {
				int index = std::distance(live_varsSet.begin(), liveIt);
				(*blockIt)->dataInfo->liveDefs.set(index);
			}
		}
	}
}

//为寄存器分配
void DataFlowAnalysis::lv_traversal(bool ig) {
	if (!ig) return;
	live_varsSet.clear();
	live_typeMap.clear();
	array_set.clear();
	cannot_spill = { "f%i0" , "f%f0","f%i1" , "f%f1","f%i2" , "f%f2", "f%i3" , "f%f3", "f%i4" , 
		"f%f4" ,"f%i5" , "f%f5" ,"f%i6" , "f%f6","f%i7" , "f%f8" };
	max_cost.clear();
	set<string> temp_live;//便于查找
	set<string> useBuff;//使用变量
	set<string> defBuff;//定义变量
	for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); blockIt++) {
		useBuff.clear();
		defBuff.clear();
		for (int k = 0; k < (*blockIt)->topoList.size(); k++) {
			string name;//被赋值的名字
			string varName;
			string useName;
			switch ((*blockIt)->topoList[k]->nodeType)
			{
			case LOAD:
				varName = "%" + to_string((*blockIt)->topoList[k]->resultId);
				if (temp_live.find(varName) == temp_live.end()) {
					temp_live.insert(varName);
					live_varsSet.push_back(varName);
					live_typeMap.insert(make_pair(varName, (*blockIt)->topoList[k]->resultType));
					name_num[varName] = 0;
				}
				if ((*blockIt)->topoList[k]->inputs.size() == 0) {
					defBuff.insert(varName);
					break;
				}
				useName = (*blockIt)->topoList[k]->inputs[0]->nodeType == VALUE_NODE ? (*blockIt)->topoList[k]->inputs[0]->name : ("%" + to_string((*blockIt)->topoList[k]->inputs[0]->resultId));
				if (temp_live.find(useName) == temp_live.end()) {
					temp_live.insert(useName);
					live_varsSet.push_back(useName);
					live_typeMap.insert(make_pair(useName, (*blockIt)->topoList[k]->inputs[0]->resultType));
					name_num[useName] = 0;
				}
				if (defBuff.find(useName) == defBuff.end() && (*blockIt)->topoList[k]->inputs[0]->resultType != iarray && (*blockIt)->topoList[k]->inputs[0]->resultType != farray)
					useBuff.insert(useName);
				defBuff.insert(varName);

				//不可以溢出
				cannot_spill.insert(varName);
				if ((*blockIt)->topoList[k]->inputs[0]->nodeType == VALUE_NODE || (*blockIt)->topoList[k]->inputs[0]->name!=ARRAYSUB_NODE) {
					cannot_spill.insert(useName);
				}
				//记录每个变量的出现次数
				name_num[useName]++;
				break;
			case OP_NODE:
				varName = "";
				useName = "";
				if ((*blockIt)->topoList[k]->inst == MV || (*blockIt)->topoList[k]->inst == FMVS) {
					if ((*blockIt)->topoList[k]->inputs.size() == 1) {
						varName = "%" + to_string((*blockIt)->topoList[k]->resultId);
						useName = (*blockIt)->topoList[k]->inputs[0]->nodeType == VALUE_NODE ? (*blockIt)->topoList[k]->inputs[0]->name : ("%" + to_string((*blockIt)->topoList[k]->inputs[0]->resultId));
						if (temp_live.find(varName) == temp_live.end()) {
							temp_live.insert(varName);
							live_varsSet.push_back(varName);
							live_typeMap.insert(make_pair(varName, (*blockIt)->topoList[k]->resultType));
							name_num[varName] = 0;
						}
						if (temp_live.find(useName) == temp_live.end()) {
							temp_live.insert(useName);
							live_varsSet.push_back(useName);
							live_typeMap.insert(make_pair(useName, (*blockIt)->topoList[k]->inputs[0]->resultType));
							name_num[useName] = 0;
						}
					}
					else if ((*blockIt)->topoList[k]->inputs.size() == 2) {
						varName = (*blockIt)->topoList[k]->inputs[0]->nodeType == VALUE_NODE ? (*blockIt)->topoList[k]->inputs[0]->name : ("%" + to_string((*blockIt)->topoList[k]->inputs[0]->resultId));
						useName = (*blockIt)->topoList[k]->inputs[1]->nodeType == VALUE_NODE ? (*blockIt)->topoList[k]->inputs[1]->name : ("%" + to_string((*blockIt)->topoList[k]->inputs[1]->resultId));
						if (temp_live.find(varName) == temp_live.end()) {
							temp_live.insert(varName);
							live_varsSet.push_back(varName);
							live_typeMap.insert(make_pair(varName, (*blockIt)->topoList[k]->inputs[0]->resultType));
							name_num[varName] = 0;
						}
						if (temp_live.find(useName) == temp_live.end()) {
							temp_live.insert(useName);
							live_varsSet.push_back(useName);
							live_typeMap.insert(make_pair(useName, (*blockIt)->topoList[k]->inputs[1]->resultType));
							name_num[useName] = 0;
						}
					}
					if (defBuff.find(useName) == defBuff.end())
						useBuff.insert(useName);
					defBuff.insert(varName);

					//记录每个变量的出现次数
					name_num[useName]++;
				}
				else {
					varName = "%" + to_string((*blockIt)->topoList[k]->resultId);


					for (int i = 0; i < (*blockIt)->topoList[k]->inputs.size(); i++) {
						if ((*blockIt)->topoList[k]->inst == LUI || (*blockIt)->topoList[k]->inputs[i]->nodeType == CONSTANT) continue;//忽略全局

						useName = (*blockIt)->topoList[k]->inputs[i]->nodeType == VALUE_NODE ? (*blockIt)->topoList[k]->inputs[i]->name : ("%" + to_string((*blockIt)->topoList[k]->inputs[i]->resultId));
						
						//生命周期短无限代价，后期做了公共子表达式要改
						if (k > 0 && (*blockIt)->topoList[k]->inputs[i]->nodeType == OP_NODE &&(*blockIt)->topoList[k - 1]->resultId == (*blockIt)->topoList[k]->inputs[i]->resultId){
							max_cost.insert(useName);
						}
						
						if (defBuff.find(useName) == defBuff.end() && (*blockIt)->topoList[k]->inputs[i]->resultType != iarray && (*blockIt)->topoList[k]->inputs[i]->resultType != farray
							&& (*blockIt)->topoList[k]->inputs[i]->inst != SD && (*blockIt)->topoList[k]->inputs[i]->inst != FSD) {
							useBuff.insert(useName);
							if (temp_live.find(useName) == temp_live.end()) {
								temp_live.insert(useName);
								live_varsSet.push_back(useName);
								live_typeMap.insert(make_pair(useName, (*blockIt)->topoList[k]->inputs[i]->resultType));
								name_num[useName] = 0;
							}
						}
						name_num[useName]++;
					}

					if (temp_live.find(varName) == temp_live.end() && (*blockIt)->topoList[k]->name != BRANCH_NODE && (*blockIt)->topoList[k]->name != RETURN_NODE) {
						temp_live.insert(varName);
						live_varsSet.push_back(varName);
						live_typeMap.insert(make_pair(varName, (*blockIt)->topoList[k]->resultType));
						defBuff.insert(varName);
						name_num[varName] = 0;
					}
				}
				break;
			case STORE:
				if ((*blockIt)->topoList[k]->inputs.size() == 1 && (*blockIt)->topoList[k]->inputs[0]->nodeType != CONSTANT) {//只有use？
					useName = (*blockIt)->topoList[k]->inputs[0]->nodeType == VALUE_NODE ? (*blockIt)->topoList[k]->inputs[0]->name : ("%" + to_string((*blockIt)->topoList[k]->inputs[0]->resultId));
					if (defBuff.find(useName) == defBuff.end()&& (*blockIt)->topoList[k]->inputs[0]->resultType != iarray && (*blockIt)->topoList[k]->inputs[0]->resultType != farray)
						useBuff.insert(useName);
					if (temp_live.find(useName) == temp_live.end()) {
						temp_live.insert(useName);
						live_varsSet.push_back(useName);
						live_typeMap.insert(make_pair(useName, (*blockIt)->topoList[k]->inputs[0]->resultType));
						name_num[useName] = 0;
					}
					//不可以溢出
					cannot_spill.insert(useName);
					name_num[useName]++;
				}
				else if ((*blockIt)->topoList[k]->inputs.size() == 2) {
					if ((*blockIt)->topoList[k]->inputs[0]->nodeType != CONSTANT)//对于store而言，两个都是使用
					{
						varName = (*blockIt)->topoList[k]->inputs[0]->nodeType == VALUE_NODE ? (*blockIt)->topoList[k]->inputs[0]->name : ("%" + to_string((*blockIt)->topoList[k]->inputs[0]->resultId));
						if (temp_live.find(varName) == temp_live.end()) {
							temp_live.insert(varName);
							live_varsSet.push_back(varName);
							live_typeMap.insert(make_pair(varName, (*blockIt)->topoList[k]->inputs[0]->resultType));
							name_num[varName] = 0;
						}
						if (defBuff.find(varName) == defBuff.end()&& (*blockIt)->topoList[k]->inputs[0]->resultType != iarray && (*blockIt)->topoList[k]->inputs[0]->resultType != farray)
							useBuff.insert(varName);
					}

					if ((*blockIt)->topoList[k]->inputs[1]->nodeType != CONSTANT) {
						useName = (*blockIt)->topoList[k]->inputs[1]->nodeType == VALUE_NODE ? (*blockIt)->topoList[k]->inputs[1]->name : ("%" + to_string((*blockIt)->topoList[k]->inputs[1]->resultId));

						if (defBuff.find(useName) == defBuff.end() && (*blockIt)->topoList[k]->inputs[1]->resultType != iarray && (*blockIt)->topoList[k]->inputs[1]->resultType != farray)
							useBuff.insert(useName);
						cannot_spill.insert(useName);
						if (temp_live.find(useName) == temp_live.end()) {
							temp_live.insert(useName);
							live_varsSet.push_back(useName);
							live_typeMap.insert(make_pair(useName, (*blockIt)->topoList[k]->inputs[1]->resultType));
							name_num[useName] = 0;
						}
						name_num[useName]++;
					}
				}
				break;
			default:
				break;
			}

		}

		//初始化
		(*blockIt)->dataInfo->liveUses.reset();
		(*blockIt)->dataInfo->liveDefs.reset();

		//得出基本块的use和def集
		for (auto useIt = useBuff.begin(); useIt != useBuff.end(); ++useIt) {
			auto liveIt = find(live_varsSet.begin(), live_varsSet.end(), *useIt);
			if (liveIt != live_varsSet.end()) {
				int index = std::distance(live_varsSet.begin(), liveIt);
				(*blockIt)->dataInfo->liveUses.set(index);
			}
		}
		for (auto defIt = defBuff.begin(); defIt != defBuff.end(); ++defIt) {
			auto liveIt = find(live_varsSet.begin(), live_varsSet.end(), *defIt);
			if (liveIt != live_varsSet.end()) {
				int index = std::distance(live_varsSet.begin(), liveIt);
				(*blockIt)->dataInfo->liveDefs.set(index);
			}
		}
	}
}

//判断是否已经包含当前表达式，若已经包含，则返回其索引
//int DataFlowAnalysis::isContainExp(AvaExp* exp) {
//	for (int i = 0; i < ava_expSet.size(); i++) {
//		if (exp->equal(exp, ava_expSet[i])) return i;
//	}
//	return -1;
//}

//填充表达式中的操作数
//void DataFlowAnalysis::fill_operand(AvaExp* exp, DAGNode* input, int index) {//index标识填充的是左右哪个操作数
//	if (index == 0) {
//		if (input->nodeType == CONSTANT) {
//			if (input->resultType == i32) {
//				exp->left.intValue = input->value.iValue;
//				exp->mode[0] = INT_AVA;
//			}
//			if (input->resultType == f32) { 
//				exp->left.floatValue = input->value.fValue; 
//				exp->mode[0] = FLOAT_AVA;
//			}
//		}
//		if (input->nodeType == OP_NODE || input->nodeType == LOAD) {
//			exp->left.longValue = input->resultId;
//			exp->mode[0] = VREG_AVA;
//		}
//	}
//	else {
//		if (input->nodeType == CONSTANT) {
//			if (input->resultType == i32) {
//				exp->right.intValue = input->value.iValue;
//				exp->mode[index] = INT_AVA;
//			}
//			if (input->resultType == f32) {
//				exp->right.floatValue = input->value.fValue;
//				exp->mode[index] = FLOAT_AVA;
//			}
//		}
//		if (input->nodeType == OP_NODE || input->nodeType == LOAD) {
//			exp->right.longValue = input->resultId;
//			exp->mode[index] = VREG_AVA;
//		}
//	}
//}
// 

//在可用表达式的vector中查找相应的表达式
//int DataFlowAnalysis::get_exp_index(string exp) {
//	/*for (int i = 0; i < ava_exp_vec.size(); i++) {
//		if (exp == ava_exp_vec[i]) {
//			return i;
//		}
//	}*/
//	return 0;
//}


//可用表达式遍历分析
void DataFlowAnalysis::ae_traversal(SymbolTable* &sym) {
	AvaExp* expM = new AvaExp();
	ava_exp_index.clear();
	ava_count = 0;
	ava_exp_set.clear();
	ava_exp_vec.clear();
	reg_exp.clear();
	topologicalSort();
	for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); blockIt++) {
		unordered_set<string> exp_gens_buff;//生成的表达式的合集
		for (auto dagnode : (*blockIt)->topoList) {
			/*if (dagnode->nodeType == VALUE_NODE) {
				name_sympol_map.insert(make_pair( dagnode->name, dagnode->symbol));
				continue;
			}eles*/
			if (dagnode->nodeType == STORE && dagnode->inputs.size()>1&&dagnode->inputs[0]->nodeType==VALUE_NODE){
				string varName = dagnode->inputs[0]->name;
				unordered_set<string> temp = exp_gens_buff;
				for (auto &it : temp) {
					if (expM->contain_operand(it, varName))
						exp_gens_buff.erase(it);
				}
			}
			string expS = expM->get_exp_string(dagnode,reg_exp,sym);


			if (expS == DFA_EXP_FUNC) continue;//不做处理

			exp_gens_buff.insert(expS);
			if (ava_exp_set.find(expS) == ava_exp_set.end()) {//将新的表达式加入集合中
				//加入新的表达式
				ava_exp_index[expS] = ava_count;
				ava_count++;
				ava_exp_set.insert(expS);
				ava_exp_vec.push_back(expS);
			}
		}
		
		(*blockIt)->dataInfo->AvaGens.reset();
		for (auto expIt = exp_gens_buff.begin(); expIt != exp_gens_buff.end(); expIt++) {
			if (ava_exp_index.find(*expIt) == ava_exp_index.end()) {
				cout << "ava error: 出现未知的表达式" << endl;
			}
			//int index = get_exp_index(*expIt);
			(*blockIt)->dataInfo->AvaGens.set(ava_exp_index[*expIt]);//设置生成集
		}
	}
	//填充kill
	for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); blockIt++) {
		(*blockIt)->dataInfo->AvaKills.reset();
		for (auto dagnode : (*blockIt)->topoList) {
			if (dagnode->nodeType != STORE) continue;
			string varName;//找到被赋值的变量名
			if (dagnode->name == ARRAYSUB_NODE && dagnode->nodeType == OP_NODE) {
				varName = dagnode->inputs[sd_variableName]->inputs[0]->name;
			}
			else {
				varName = dagnode->inputs[sd_variableName]->name;//store节点的第一个输入
			}

			//设置kill集
			for (auto mapIt = ava_exp_index.begin(); mapIt != ava_exp_index.end(); mapIt++) {
				if (expM->contain_operand(mapIt->first, varName)) {
					(*blockIt)->dataInfo->AvaKills.set(mapIt->second);
				}
			}

			/*for (int i = 0; i < ava_exp_vec.size(); i++) {
				if (expM->contain_operand(ava_exp_vec[i], varName)) {
					(*blockIt)->dataInfo->AvaKills.set(i);
				}
			}*/
			/*if (dagnode->nodeType != OP_NODE || ava_opSet.find(dagnode->name) == ava_opSet.end()) continue;
			for (int i = 0; i < ava_expSet.size(); i++) {
				if (ava_expSet[i]->containVar(ava_expSet[i], dagnode->resultId))
					(*blockIt)->dataInfo->AvaKills.set(i);
			}*/
			/*if (dagnode->nodeType == STORE) {
			也许不用考虑，因为store都是与a,b等用户自定义变量，而操作符并不会与这些变量相连
			}*/
		}
	}
	delete expM;
}

//对于MIR的公共子表达式消除
void DataFlowAnalysis::ae_traversal(bool mir) {
	if (!mir) return;
	ava_exp_index.clear();
	ava_count = 0;
	ava_exp_set.clear();
	ava_exp_vec.clear();
	reg_exp.clear();
	topologicalSort();
	for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); blockIt++) {
		unordered_set<string> exp_gens_buff;//生成的表达式的合集
		for (auto dagnode : (*blockIt)->topoList) {
			if (dagnode->inst == LUI) {
				string expS = dagnode->inputs[0]->name + "lui";
				exp_gens_buff.insert(expS);
				if (ava_exp_set.find(expS) == ava_exp_set.end()) {//将新的表达式加入集合中
					//加入新的表达式
					ava_exp_index[expS] = ava_count;
					ava_count++;
					ava_exp_set.insert(expS);
					ava_exp_vec.push_back(expS);
				}
			}
		}

		//设置生成集
		(*blockIt)->dataInfo->AvaGens.reset();
		for (auto expIt = exp_gens_buff.begin(); expIt != exp_gens_buff.end(); expIt++) {
			if (ava_exp_index.find(*expIt) == ava_exp_index.end()) {
				cout << "ava error: 出现未知的表达式" << endl;
			}
			(*blockIt)->dataInfo->AvaGens.set(ava_exp_index[*expIt]);
		}
	}
}


//到达定值分析
void DataFlowAnalysis::reachingDefinationAnalysis() {
	rd_traversal();
	bool changed = true;
	int count = 1;
	print("到达定值分析");
	while (changed)
	{
		changed = false;
		string s = "第" + to_string(count) + "次迭代";
		//print(s);
		for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); blockIt++) {
			string preInSet = (*blockIt)->dataInfo->defInSet.to_string();
			setDefIn(*blockIt);
			(*blockIt)->dataInfo->setDefOut();
			string inSet = (*blockIt)->dataInfo->defInSet.to_string();
			string outSet = (*blockIt)->dataInfo->defOutSet.to_string();
			if (preInSet.compare(inSet)) {
				changed = true;
			}
			s= inSet.erase(0, SIZE - def_count + 1) + " "+outSet.erase(0, SIZE - def_count + 1);
			print(s);
		}
		count++;
	}
}

//活跃变量分析
void DataFlowAnalysis::liveVariablesAnalysis(bool ig) {
	if (ig)
		lv_traversal(ig);
	else
		lv_traversal();
	bool changed = true;
	int count = 1;
	int n = live_varsSet.size();

	//初始化：
	for (auto blockIt = cfg->basicBlocks.rbegin(); blockIt != cfg->basicBlocks.rend(); ++blockIt) {
		(*blockIt)->dataInfo->liveInSet.reset();
		(*blockIt)->dataInfo->liveOutSet.reset();
	}
	while (changed)
	{
		changed = false;
		for (auto blockIt = cfg->basicBlocks.rbegin(); blockIt != cfg->basicBlocks.rend(); ++blockIt) {
			bitset<SIZE> preOutSet = (*blockIt)->dataInfo->liveOutSet;
			setLiveOut(*blockIt);
			(*blockIt)->dataInfo->setLiveIn();
			/*string inSet = (*blockIt)->dataInfo->liveInSet.to_string();
			string outSet = (*blockIt)->dataInfo->liveOutSet.to_string();*/
			if (preOutSet != (*blockIt)->dataInfo->liveOutSet) {//比较是否发生变化
				changed = true;
			}
			/*print("in:   ");
			(*blockIt)->dataInfo->Display((*blockIt)->dataInfo->liveInSet, live_varsSet);
			print("out: ");
			(*blockIt)->dataInfo->Display((*blockIt)->dataInfo->liveOutSet, live_varsSet);*/
		}
		count++;
	}
}


//可用表达式分析
void DataFlowAnalysis::availableExpressionsAnalysis(SymbolTable*& sym, bool mir) {
	if (mir) {
		ae_traversal(1);
	}
	else {
		ae_traversal(sym);
	}
	bool changed = true;
	int count = 1;
	int n = ava_exp_index.size();
	print( "可用表达式分析" );

	//打印可用表达式
	/*for (int i = 0; i < ava_exp_vec.size(); i++) {
		print(ava_exp_vec[i]);
	}*/
	//初始化
	//cfg->entry->dataInfo->AvaOutSet.reset();
	while (changed)
	{
		changed = false;
		/*string tempS = "第" + to_string(count) + "次迭代";
		print(tempS);*/
		for (auto blockIt = cfg->basicBlocks.begin(); blockIt != cfg->basicBlocks.end(); ++blockIt) {
			bitset<SIZE> preOutSet = (*blockIt)->dataInfo->AvaOutSet;
			setAvaIn(*blockIt);
			(*blockIt)->dataInfo->setAvaOut();
			if (preOutSet != (*blockIt)->dataInfo->AvaOutSet) {
				changed = true;
			}
			/*print("in:   ");
			(*blockIt)->dataInfo->Display((*blockIt)->dataInfo->AvaInSet, ava_exp_vec);
			print("out: ");
			(*blockIt)->dataInfo->Display((*blockIt)->dataInfo->AvaOutSet, ava_exp_vec);*/
		}
		count++;
	}
}
