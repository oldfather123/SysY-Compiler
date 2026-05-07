#include "RegisterAllocate.h"
#include "Generate.h"
#include "Register.h"
#include "../DFA/DFA.h"
#include "RegisterSpill.h"



//函数调用提前分配
void registerAssignFunctioncall(CFG* cfg, vector<DAGNode*> parameters) {
	for (int i = 0; i < cfg->basicBlocks.size(); i++) {
		for (int j = 0; j < cfg->basicBlocks[i]->topoList.size(); j++) {
			//遍历读到函数调用
			if (cfg->basicBlocks[i]->topoList[j]->inst == CALL) {
				DAGNode* call = cfg->basicBlocks[i]->topoList[j];

				//函数输入
				int stack = 0;
				//整形
				int count_i = 0;
				for (int k = 0; k < call->inputs.size(); k++) {
					if (call->inputs[k]->resultType != f32) {
						count_i++;
						//8个以内
						if (count_i <= 8) {
							//mv ax, s
							DAGNode* mv = new DAGNode(" ", OP_NODE);

							mv->inputs.push_back(parameters[0 + count_i - 1]);
							parameters[0 + count_i - 1]->outputs.push_back(mv);
							parameters[0 + count_i - 1]->outputs.push_back(call);
							mv->inputs.push_back(call->inputs[k]);
							//call->inputs[k]->outputs.push_back(mv);
							mv->inst = MV;
	
							*find(call->inputs[k]->outputs.begin(), call->inputs[k]->outputs.end(), call) = mv;
							call->inputs[k] = parameters[0 + count_i - 1];
							//插入call前面
							cfg->basicBlocks[i]->topoList.insert(cfg->basicBlocks[i]->topoList.begin() + j, mv);
							j = j + 1;
						}
						//8个以外
						else {
							break;
						}
					}
				}
				//浮点
				int count_f = 0;
				for (int k = 0; k < call->inputs.size(); k++) {
					if (call->inputs[k]->resultType == f32) {
						count_f++;
						//8个以内
						if (count_f <= 8) {
							//mv ax, s
							DAGNode* mv = new DAGNode(" ", OP_NODE);

							mv->inputs.push_back(parameters[8 + count_f - 1]);
							parameters[8 + count_f - 1]->outputs.push_back(mv);
							parameters[8 + count_f - 1]->outputs.push_back(call);
							mv->inputs.push_back(call->inputs[k]);
							//call->inputs[k]->outputs.push_back(mv);
							mv->inst = FMVS;
				
							*find(call->inputs[k]->outputs.begin(), call->inputs[k]->outputs.end(), call) = mv;
							call->inputs[k] = parameters[8 + count_f - 1];
							//插入call前面
							cfg->basicBlocks[i]->topoList.insert(cfg->basicBlocks[i]->topoList.begin() + j, mv);
							j = j + 1;
						}
						//8个以外
						else {
							break;
						}
					}
				}

				//函数调用有返回值
				if (call->outputs.size() != 0) {
					//mv x, a0
					DAGNode* mv = new DAGNode(" ", OP_NODE);

					DAGNode* reg1 = new DAGNode(" ", REG_NODE);
					mv->inputs.push_back(reg1);
					reg1->outputs.push_back(mv);

					//reg1代表函数输出
					reg1->resultId = call->resultId;

					if (call->resultType == f32) {
						mv->inst = FMVS;
						reg1->resultType = f32;
						mv->inputs.push_back(parameters[8]);
						parameters[8]->outputs.push_back(mv);
					}
					else {
						mv->inst = MV;
						reg1->resultType = i32;
						mv->inputs.push_back(parameters[0]);
						parameters[0]->outputs.push_back(mv);
					}
					//插入call后面
					cfg->basicBlocks[i]->topoList.insert(cfg->basicBlocks[i]->topoList.begin() + j + 1, mv);
				}

			}
			else if (cfg->basicBlocks[i]->topoList[j]->inst == RET) {
				if (cfg->basicBlocks[i]->topoList[j]->inputs.size() != 0) {
					//mv x, a0
					DAGNode* mv = new DAGNode(" ", OP_NODE);

					//reg1代表函数输出
					//reg1->resultId = cfg->basicBlocks[i]->topoList[j]->inputs[0]->resultId;

					if (cfg->basicBlocks[i]->topoList[j]->resultType == f32) {
						mv->inst = FMVS;
						mv->inputs.push_back(parameters[8]);
						parameters[8]->outputs.push_back(mv);
					}
					else {
						mv->inst = MV;
						mv->inputs.push_back(parameters[0]);
						parameters[0]->outputs.push_back(mv);
					}

					mv->inputs.push_back(cfg->basicBlocks[i]->topoList[j]->inputs[0]);
					cfg->basicBlocks[i]->topoList[j]->inputs[0]->outputs.push_back(mv);

					//插入ret前面
					cfg->basicBlocks[i]->topoList.insert(cfg->basicBlocks[i]->topoList.begin() + j, mv);
					j = j + 1;
				}
			}
		}
	}
}

//函数形参分配
void registerAssignFunctionParameters(CFG* cfg, SymbolTable* globalsymboltable, vector<DAGNode*> parameters) {
	vector<Symbol*> para = globalsymboltable->getFuncSymbols(cfg->name);
	int count_i = 0;
	for (int i = 0; i < para.size(); i++) {
		if (para[i]->type != f32) {
			count_i++;
			//8个以内
			if (count_i <= 8) {
				//mv ax, s
				DAGNode* mv = new DAGNode(" ", OP_NODE);

				DAGNode* parameteri = new DAGNode(para[i]->name, VALUE_NODE);
				parameteri->resultType = i32;

				mv->inputs.push_back(parameteri);
				parameteri->outputs.push_back(mv);
				mv->inputs.push_back(parameters[0 + count_i - 1]);
				parameters[0 + count_i - 1]->outputs.push_back(mv);
				mv->inst = MV;

				cfg->basicBlocks[0]->topoList.insert(cfg->basicBlocks[0]->topoList.begin(), mv);
			}
			//大于8个输入在数据流分析处理
			else break;
		}
	}

	int count_f = 0;
	for (int i = 0; i < para.size(); i++) {
		if (para[i]->type == f32) {
			count_f++;
			//8个以内
			if (count_f <= 8) {
				//mv ax, s
				DAGNode* mv = new DAGNode(" ", OP_NODE);

				DAGNode* parameteri = new DAGNode(para[i]->name, VALUE_NODE);
				parameteri->resultType = f32;

				mv->inputs.push_back(parameteri);
				parameteri->outputs.push_back(mv);
				mv->inputs.push_back(parameters[8 + count_f - 1]);
				parameters[8 + count_f - 1]->outputs.push_back(mv);
				mv->inst = FMVS;

				cfg->basicBlocks[0]->topoList.insert(cfg->basicBlocks[0]->topoList.begin(), mv);
			}
			//大于8个输入在数据流分析处理
			else break;
		}
	}
}


//找到最终结合的节点
InterNode* getCoalesceNode(InterNode* node) {
	if (node->coalesce == true) return getCoalesceNode(node->coalescenode);
	else return node;
}

//求节点的度数
inline int degree(InterNode* node) {
	//if (node->pre_color > 0) return node->real_nodes.size() + node->virtual_nodes.size();
	//else return node->real_nodes.size() + node->virtual_nodes.size() + node->pre_nums.count();
	if (node->pre_color > 0) return node->real_nodes.size();
	else if (node->func) return node->real_nodes.size() + node->pre_nums.count();
	else return node->real_nodes.size() + node->pre_nums.count() + node->pre_protect;
}

//查询n节点是否是y节点的临节点
inline bool edge(InterNode* n, InterNode* y) {
	//if (y->contain_in_real(n) || y->contain_in_virtual(n))return true;
	if (y->contain_in_real(n)) return true;
	return false;
}

//求一个节点能否和另一个合并
bool George(InterNode* A, InterNode* B, int K) {
	if (A->real_nodes.find(B) != A->real_nodes.end()) return false;

//	if (A->func && B->func) return false;

	for (auto it = A->real_nodes.begin(); it != A->real_nodes.end(); it++) {
		if (degree(*it) < K || edge(*it, B)) {
			continue;
		}
		return false;
	}

	//for (auto it = A->virtual_nodes.begin(); it != A->virtual_nodes.end(); it++) {
	//	if (degree(*it) < K || edge(*it, B) || (*it) == B) {
	//		continue;
	//	}
	//	coalesceable = false;
	//	break;
	//}

	if (A->pre_color > 0 || B->pre_color > 0) {
		if (A->func || B->func) return false;
	}

	return true;
}

//将一个节点的所有临边删除
void remove_node(InterNode* n) {
	for (auto it = n->real_nodes.begin(); it != n->real_nodes.end(); it++) {
		(*it)->real_nodes.erase(n);
	}

	for (auto it = n->virtual_nodes.begin(); it != n->virtual_nodes.end(); it++) {
		(*it)->virtual_nodes.erase(n);
	}
}

//放回节点，后续coloring需要做修改以着色
void rejoin_node(InterNode* n) {
	for (auto it = n->real_nodes.begin(); it != n->real_nodes.end(); it++) {
		(*it)->real_nodes.insert(n);
	}

	for (auto it = n->virtual_nodes.begin(); it != n->virtual_nodes.end(); it++) {
		(*it)->virtual_nodes.insert(n);
	}
}

//将A合并到B，默认A为小度节点
bool coalesceInterNodes(InterNode* A, InterNode* B, InterferenceGraph* graph, int K) {
	bool coalesceable = true;

	if (degree(A) >= degree(B)) coalesceable = George(B, A, K);//是否可以合并
	else coalesceable = George(A, B, K);

	//可以结合
	if (!coalesceable) {
		return coalesceable;
	}

	//A和B结合后的节点
	InterNode* newnode = new InterNode();
	newnode->name = A->name + "|" + B->name;
	newnode->type = A->type;

	newnode->func = A->func || B->func;

	newnode->pre_color = A->pre_color > B->pre_color ? A->pre_color : B->pre_color;
	newnode->pre_nums = A->pre_nums | B->pre_nums;
	newnode->pre_protect = A->pre_protect + B->pre_protect;

	//将A，B改为已合并
	A->coalesce = true;
	B->coalesce = true;
	A->coalescenode = newnode;
	B->coalescenode = newnode;

	//删除A，B的所有边
	remove_node(A);
	A->virtual_nodes.erase(B);
	remove_node(B);

	//将A，B合并
	set_union(A->real_nodes.begin(), A->real_nodes.end(), B->real_nodes.begin(), B->real_nodes.end(), inserter(newnode->real_nodes, newnode->real_nodes.begin()));
	set_union(A->virtual_nodes.begin(), A->virtual_nodes.end(), B->virtual_nodes.begin(), B->virtual_nodes.end(), inserter(newnode->virtual_nodes, newnode->virtual_nodes.begin()));


	//同时存在实边和虚边取虚边
	//for (auto it_v = newnode->virtual_nodes.begin(); it_v != newnode->virtual_nodes.end(); it_v++) {
	//	if (newnode->real_nodes.find(*it_v) != newnode->real_nodes.end()) {
	//		(*it_v)->real_nodes.erase(newnode);
	//		newnode->real_nodes.erase(*it_v);
	//	}
	//}
		//同时存在实边和虚边取实边
	for (auto it_r = newnode->real_nodes.begin(); it_r != newnode->real_nodes.end(); it_r++) {
		if (newnode->virtual_nodes.find(*it_r) != newnode->virtual_nodes.end()) {
			(*it_r)->virtual_nodes.erase(newnode);
			newnode->virtual_nodes.erase(*it_r);
		}
	}


	//对合并后的节点的临边添加该节点
	for (auto it = newnode->real_nodes.begin(); it != newnode->real_nodes.end(); it++) {
		(*it)->real_nodes.insert(newnode);
	}
	for (auto it = newnode->virtual_nodes.begin(); it != newnode->virtual_nodes.end(); it++) {
		(*it)->virtual_nodes.insert(newnode);
	}
	
	graph->eraseMap.insert(A);
	graph->eraseMap.insert(B);
	graph->nodes.insert(newnode);
	graph->nodesMap.insert(pair<string, InterNode*>(newnode->name, newnode));

	return coalesceable;
}

//对一个IGManger里面的一个图进行合并
bool coalesceGraph(InterferenceGraph* graph, int K) {
	bool coalesce = false;
	for (auto it_n = graph->nodes.begin(); it_n != graph->nodes.end(); it_n++) {
		if (graph->eraseMap.find(*it_n) != graph->eraseMap.end()) continue;
		for (auto it = (*it_n)->virtual_nodes.begin(); it != (*it_n)->virtual_nodes.end(); it++) {
			if (graph->eraseMap.find(*it) != graph->eraseMap.end()) break;
			if (coalesceInterNodes(*it_n, *it, graph, K)) {
				coalesce = true;
				break;
			}
		}
	}

	//删除节点
	for (auto it = graph->eraseMap.begin(); it != graph->eraseMap.end(); it++) {
		graph->nodes.erase(*it);
	}

	graph->eraseMap.clear();

	return coalesce;
}

//尽可能合并一个干涉图的节点
void coalesceAsPossible(InterferenceGraph* graph, int K) {
	bool havechanged = true;
	while (havechanged) {
		havechanged = coalesceGraph(graph, K);
	}
}

//简化一个图
bool simplifyGraph(InterferenceGraph* graph, int K) {
	bool simplifiable = false;

	bool ending = false;
	while (!ending) {
		ending = true;
		for (auto it = graph->nodes.begin(); it != graph->nodes.end(); ) {
			//删除小度节点
			if (degree(*it) < K) {
				if ((*it)->pre_color > 0 && (*it)->pre_color <= 8) {
					for (auto it_r = (*it)->real_nodes.begin(); it_r != (*it)->real_nodes.end(); it_r++) {
						(*it_r)->pre_nums.set((*it)->pre_color - 1);
					}
					//for (auto it_v = (*it)->virtual_nodes.begin(); it_v != (*it)->virtual_nodes.end(); it_v++) {
					//	(*it_v)->pre_nums.set((*it)->pre_color - 1);
					//}
				}
				else if ((*it)->func) {
					for (auto it_r = (*it)->real_nodes.begin(); it_r != (*it)->real_nodes.end(); it_r++) {
						(*it_r)->pre_protect++;
					}
				}

				InterNode* current = *it;
				it = next(it);
				graph->stack.push_back(current);
				remove_node(current);
				graph->nodes.erase(current);
				ending = false;
				simplifiable = true;
				//break;
			}
			if (it != graph->nodes.end()) it++;
		}
	}

	//返回图是否还能被优化
	return simplifiable;
}

//冰冻虚拟边
//bool Freeze(InterferenceGraph* graph, int K) {
//	for (auto it = graph->nodes.begin(); it != graph->nodes.end(); it++) {
//		//当边it_n-it没被加进去
//		if ((*it)->virtual_nodes.size() != 0) {
//			//边两边都是小度节点
//			if (degree(*it) <= K && degree(*(*it)->virtual_nodes.begin()) <= K) {
//				(*(*it)->virtual_nodes.begin())->virtual_nodes.erase(*it);
//				(*(*it)->virtual_nodes.begin())->virtual_nodes.erase(*(*it)->virtual_nodes.begin());
//				return true;
//				break;
//			}
//		}
//	}
//	return false;
//
//	//bool freeze = false;
//	//for (auto it = graph->nodes.begin(); it != graph->nodes.end();) {
//	//	bool deleteedge = false;
//	//	//当边it_n-it没被加进去
//	//	for (auto it_v = (*it)->virtual_nodes.begin(); it_v != (*it)->virtual_nodes.end(); it_v++) {
//	//		//边两边都是小度节点
//	//		if (degree(*it) < K && degree(*it_v) < K) {
//	//			(*it_v)->virtual_nodes.erase(*it);
//	//			(*it)->virtual_nodes.erase(*it_v);
//	//			deleteedge = true;
//	//			freeze = true;
//	//			break;
//	//		}
//	//	}
//	//	if (!deleteedge) it++;
//
//	//}
//	//return freeze;
//}


int coloringProtect(CFG* cfg, IGManager* ig, InterferenceGraph* graph) {
	int func_max = 0;

	for (int i = graph->stack.size() - 1; i >= 0; i--) {
		set<int> a;

		if (graph->stack[i]->pre_nums.count()) {
			for (int j = 1; j <= 8; j++) {
				if (graph->stack[i]->pre_nums[j - 1]) {
					a.insert(j);
				}
			}
		}

		//求与it_n相关的保护节点
		if (graph->stack[i]->func) {
			for (auto it = graph->stack[i]->real_nodes.begin(); it != graph->stack[i]->real_nodes.end(); it++) {
				if ((*it)->func && graph->color.find((*it)->name) != graph->color.end()) {
					a.insert(graph->color[(*it)->name]);
				}
			}

			//for (auto it = graph->stack[i]->virtual_nodes.begin(); it != graph->stack[i]->virtual_nodes.end(); it++) {
			//	if ((*it)->func && graph->color.find((*it)->name) != graph->color.end()) {
			//		a.insert(graph->color[(*it)->name]);
			//	}
			//}

			//igraph
 			if (ig->iGraph == graph) {
				for (int j = 1; j <= icallee + 1; j++) {
					if (j == icallee + 1) {
						//cout << "register insufficient int" << endl;
						spill_replaceIR(ig->iGraph->stack[i]->name, cfg, 0);
						//ig->iGraph->eraseProtectMap.insert(ig->iGraph->stack[i]);
						//ig->iGraph->nodesMap.erase(ig->iGraph->stack[i]->name);
						return j;
					}
					if (a.count(j + icaller) == 1) continue;
					graph->color[graph->stack[i]->name] = j + icaller;
					func_max = j > func_max ? j : func_max;
					break;
				}
			}
			//fgraph
			else {
				for (int j = 1; j <= fcallee + 1; j++) {
					if (j == fcallee + 1) {
						//cout << "register insufficient float" << endl;
						spill_replaceIR(ig->fGraph->stack[i]->name, cfg, 1);
						//ig->fGraph->eraseProtectMap.insert(ig->fGraph->stack[i]);
						//ig->fGraph->nodesMap.erase(ig->fGraph->stack[i]->name);
						return j;
					}
					if (a.count(j + fcaller) == 1) continue;
					graph->color[graph->stack[i]->name] = j + fcaller;
					func_max = j > func_max ? j : func_max;
					break;
				}
			}
		}
	}

	return func_max;
}

//弹出节点着色
bool coloring(InterferenceGraph* graph, InterNode* node, int K) {
	//已经着色
	if (graph->color.find(node->name) != graph->color.end()) return true;

	set<int> a;

	if (node->pre_nums.count()) {
		for (int i = 1; i <= 8; i++) {
			if (node->pre_nums[i - 1]) {
				a.insert(i);
			}
		}
	}

	for (auto it = node->real_nodes.begin(); it != node->real_nodes.end(); it++) {
		if (graph->color.find((*it)->name) != graph->color.end()) {
			a.insert(graph->color[(*it)->name]);
		}
	}

	//for (auto it = node->virtual_nodes.begin(); it != node->virtual_nodes.end(); it++) {
	//	if (graph->color.find((*it)->name) != graph->color.end()) {
	//		a.insert(graph->color[(*it)->name]);
	//	}
	//}

	while (1) {
		for (int i = 1; i <= K; i++) {
			if (a.count(i) == 1) continue;
			graph->color[node->name] = i;
			return true;
		}		
		break;
	}
	//while (1) {
	//	//igraph
	//	if (ig->iGraph == graph) {
	//		for (int i = 1; i <= K; i++) {
	//			if (a.count(i) == 1) continue;
	//			graph->color[node->name] = i;
	//			return true;
	//		}
	//	}
	//	//fgraph
	//	else {
	//		for (int i = 1; i <= K; i++) {
	//			if (a.count(i) == 1) continue;
	//			graph->color[node->name] = i;
	//			return true;
	//		}
	//	}
	//	if (K >= icallee + icaller) return false;
	//	K++;
	//}

	return false;
}

//弹出栈顶节点
bool select(IGManager* ig, InterferenceGraph* graph, int &K) {
	//graph->color.clear();
	//添加参数节点颜色
	for (int i = 0; i < graph->stack.size(); i++) {
		if (graph->stack[i]->pre_color > 0) graph->color[graph->stack[i]->name] = graph->stack[i]->pre_color;
	}

	//着色
	while (graph->stack.size() != 0) {
		if (!coloring(graph, graph->stack[graph->stack.size() - 1], K)) {
			K++;
			if (ig->iGraph == graph) {
				if (K > icallee + icaller) return false;
			}
			else if (ig->fGraph == graph) {
				if (K > fcallee + fcaller) return false;
			}
			continue;
		}

		graph->stack.pop_back();
	}

	return true;
}

//在进入寄存器着色前先行溢出
void spillBeforeColoring(DataFlowAnalysis* dfa, IGManager* ig) {
	//整形
	bool end = false;
	while (!end) {
		end = true;
		ig->iGraph->clear();
		ig->create_i32G();
		//Freeze(ig->iGraph, icallee + icaller);

		bool simplifiable = true;
		while (simplifiable) {
			simplifiable = simplifyGraph(ig->iGraph, icallee + icaller);
		}

		while (ig->iGraph->nodes.size() != 0) {
			vector<InterNode*> spill(ig->iGraph->nodes.begin(), ig->iGraph->nodes.end());
			InterNode* current = reg_spill(dfa, spill, 0);

			end = false;
			remove_node(current);
			ig->iGraph->nodes.erase(current);
			bool simplifiable = true;
			while (simplifiable) {
				//coalesceAsPossible(ig->iGraph, icallee + icaller);
				simplifiable = simplifyGraph(ig->iGraph, icallee + icaller);
			}
		}
	}

	//浮点
	end = false;
	while (!end) {
		end = true;
		ig->fGraph->clear();
		ig->create_f32G();

		bool simplifiable = true;
		while (simplifiable) {
			simplifiable = simplifyGraph(ig->fGraph, fcallee + fcaller);
		}

		while (ig->fGraph->nodes.size() != 0) {
			vector<InterNode*> spill(ig->fGraph->nodes.begin(), ig->fGraph->nodes.end());
			InterNode* current = reg_spill(dfa, spill, 1);

			end = false;
			remove_node(current);
			ig->fGraph->nodes.erase(current);
			bool simplifiable = true;
			while (simplifiable) {
				//coalesceAsPossible(ig->fGraph, fcallee + fcaller);
				simplifiable = simplifyGraph(ig->fGraph, fcallee + fcaller);
			}
		}
	}
}

//对一个函数进行寄存器分配
void registerAllocateCFG(CFG* cfg, IGManager* ig, DataFlowAnalysis* dfa, SymbolTable* globalsymboltable) {
	vector<DAGNode*> parametersreg;//函数形参寄存器，0-7是整形，8-15是浮点
	for (int i = 0; i < 8; i++) {
		DAGNode* Fi = new DAGNode("f%i" + to_string(i), VALUE_NODE);
		if(i <= 3) 	Fi->precolor = 8 - i;
		else Fi->precolor = i - 3;
		Fi->resultType = i32;
		Fi->isConst = false;
		parametersreg.push_back(Fi);
	}
	for (int i = 0; i < 8; i++) {
		DAGNode* Fi = new DAGNode("f%f" + to_string(i), VALUE_NODE);
		if (i <= 3) Fi->precolor = 8 - i;
		else Fi->precolor = i - 3;
		Fi->resultType = f32;
		Fi->isConst = false;
		parametersreg.push_back(Fi);
	}

	registerAssignFunctioncall(cfg, parametersreg);//函数调用提前分配
	registerAssignFunctionParameters(cfg, globalsymboltable, parametersreg);//函数形参提前分配

	spillBeforeColoring(dfa, ig);

	pre_spill_protect(dfa, ig);

	bool complete = false;
	//对igraph进行分配
	while (!complete) {
		ig->iGraph->clear();
		ig->create_i32G();
		int K = icallee + icaller;
		while (ig->iGraph->nodes.size() != 0) {
			bool simplifiable = true;
			while (simplifiable) {
				//simplifiable = false;
				coalesceAsPossible(ig->iGraph, K);
				simplifiable = simplifyGraph(ig->iGraph, K);
			}
		}
		//复原栈上节点
		for (int i = ig->iGraph->stack.size() - 1; i >= 0; i--) {
			if (ig->iGraph->stack[i]->func) {
				rejoin_node(ig->iGraph->stack[i]);
			}
		}
		K = coloringProtect(cfg, ig, ig->iGraph) + icaller;
		if (K == icallee + icaller + 1) continue;

		complete = select(ig, ig->iGraph, K);

		if (!complete) {
			cout << "error" << endl;
		}

		ig->i32_max_func = K - icaller;
	}

	complete = false;
	//对fgraph进行分配
	while (!complete) {
		ig->fGraph->clear();
		ig->create_f32G();
		int K = fcallee + fcaller;
		while (ig->fGraph->nodes.size() != 0) {
			bool simplifiable = true;
			while (simplifiable) {
				//simplifiable = false;
				coalesceAsPossible(ig->fGraph, K);
				simplifiable = simplifyGraph(ig->fGraph, K);
			}
		}
		//复原栈上节点
		for (int i = ig->fGraph->stack.size() - 1; i >= 0; i--) {
			if (ig->fGraph->stack[i]->func) {
				rejoin_node(ig->fGraph->stack[i]);
			}
		}
		K = coloringProtect(cfg, ig, ig->fGraph) + fcaller;
		if (K == fcallee + fcaller + 1) continue;

		complete = select(ig, ig->fGraph, K);

		if (!complete) {
			cout << "error float" << endl;
		}

		ig->f32_max_func = K - fcaller;
	}

	/*
	bool complete = false;
	//对igraph进行分配
	while (!complete) {
		ig->iGraph->clear();
		ig->create_i32G();
		int K = icallee + icaller;
		while (ig->iGraph->nodes.size() != 0) {
			bool freezeableable = true;
			while (freezeableable) {
				bool simplifiable = true;
				while (simplifiable) {
					//simplifiable = false;
					coalesceAsPossible(ig->iGraph, K);
					simplifiable = simplifyGraph(ig->iGraph, K);
				}
				freezeableable = Freeze(ig->iGraph, K);
			}
			//if (ig->iGraph->nodes.size() != 0) {
			//	ig->iGraph->stack.push_back(*(ig->iGraph->nodes.begin()));
			//	remove_node(*(ig->iGraph->nodes.begin()));
			//	ig->iGraph->nodes.erase(*(ig->iGraph->nodes.begin()));
			//}
		}
		//复原栈上节点
		for (int i = ig->iGraph->stack.size() - 1; i >= 0; i--) {
			if (ig->iGraph->stack[i]->func) {
				rejoin_node(ig->iGraph->stack[i]);
			}
			//rejoin_node(ig->iGraph->stack[i]);
			//ig->iGraph->nodes.insert(ig->iGraph->stack[i]);
		}
		K = coloringProtect(ig, ig->iGraph) + icaller;
		//if (K > icallee + icaller) {
		//	//溢出保护
		//	//cout << "spill1";
		//	reg_spill(dfa, ig->iGraph->protectstack, 0);
		//	continue;
		//}

		complete = select(ig, ig->iGraph, K);

		if (!complete) {
			cout << "error" << endl;
		}

		//if (!complete) {
		//	//溢出普通
		//	//cout << "spill";
		//	reg_spill(dfa, ig->iGraph->spillstack, 0);
		//	continue;
		//}

		ig->i32_max_func = K - icaller;
	}

	complete = false;
	//对fgraph进行分配
	while (!complete) {
		ig->fGraph->clear();
		ig->create_f32G();
		int K = fcallee + fcaller;
		while (ig->fGraph->nodes.size() != 0) {
			bool freezeableable = true;
			while (freezeableable) {
				bool simplifiable = true;
				while (simplifiable) {
					//simplifiable = false;
					coalesceAsPossible(ig->fGraph, K);
					simplifiable = simplifyGraph(ig->fGraph, K);
				}
				freezeableable = Freeze(ig->fGraph, K);
			}
			//if (ig->fGraph->nodes.size() != 0) {
			//	ig->fGraph->stack.push_back(*(ig->fGraph->nodes.begin()));
			//	remove_node(*(ig->fGraph->nodes.begin()));
			//	ig->fGraph->nodes.erase(*(ig->fGraph->nodes.begin()));
			//}
		}
		//复原栈上节点
		for (int i = ig->fGraph->stack.size() - 1; i >= 0; i--) {
			if (ig->fGraph->stack[i]->func) {
				rejoin_node(ig->fGraph->stack[i]);
			}
		}
		K = coloringProtect(ig, ig->fGraph) + fcaller;
		//if (K > fcallee + fcaller) {
		//	//溢出保护
		//	//cout << "spill1";
		//	reg_spill(dfa, ig->fGraph->protectstack, 1);
		//	continue;
		//}

		complete = select(ig, ig->fGraph, K);

		if (!complete) {
			//溢出普通
			cout << "error float" << endl;
			//reg_spill(dfa, ig->fGraph->spillstack, 1);
			continue;
		}

		ig->f32_max_func = K - fcaller;
	}
	*/

	ig->color.clear();
	ig->nodesMap.clear();
	for (auto it = ig->iGraph->nodesMap.begin(); it != ig->iGraph->nodesMap.end(); it++) {
		ig->nodesMap[(*it).first] = (*it).second;
	}
	for (auto it = ig->fGraph->nodesMap.begin(); it != ig->fGraph->nodesMap.end(); it++) {
		ig->nodesMap[(*it).first] = (*it).second;
	}


	for (auto it = ig->nodesMap.begin(); it != ig->nodesMap.end(); it++) {
		InterNode* coalescenode = getCoalesceNode((*it).second);

		if (coalescenode->type == f32) {
			//if (ig->fGraph->eraseMap.find(coalescenode) != ig->fGraph->eraseMap.end()) continue;
			if (ig->fGraph->color.find(coalescenode->name) == ig->fGraph->color.end()) continue;
			ig->color[(*it).first] = ig->fGraph->color[coalescenode->name];
		}
		else {
			//if (ig->iGraph->eraseMap.find(coalescenode) != ig->iGraph->eraseMap.end()) continue;
			if (ig->iGraph->color.find(coalescenode->name) == ig->iGraph->color.end()) continue;
			ig->color[(*it).first] = ig->iGraph->color[coalescenode->name];
		}
	}

	//ig->color = ig->iGraph->color + ig->fGraph->color;

}

//图着色寄存器分配与指派
void graphColoringRegisterAllocation(GCFG* gcfg, DFA* dfa, SymbolTable* globalsymboltable) {
	for (auto it = gcfg->controlFlow.begin(); it != gcfg->controlFlow.end(); it++) {
		registerAllocateCFG(it->second, dfa->igs[it->second], dfa->cfg_dfa[it->second], globalsymboltable);
		//registerAssignment(it->second, dfa->igs[it->second], globalsymboltable);
	}
}
