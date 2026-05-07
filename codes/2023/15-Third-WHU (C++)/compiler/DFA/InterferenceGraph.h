#pragma once
#include"DataFlowAnalysis.h"
#include "../RISC_V/Register.h"
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include<utility>

const map<int, int> iregisters = { {1,a4}, {2,a5}, {3, a6},{4,a7},{5, a3},{6, a2}, {7, a1} ,{8, a0}, {9, t0},
								{10, t1}, {11,t2}, {12,t3}, {13,t4},{14,t5},{15, t6}, {16, s1}, {17,s2},
								{18,s3}, {19,s4}, {20,s5},{21,s6},{22,s7}, {23,s8},{24,s9}, {25,s10}, {26,s11} };
const map<int, int> fregisters = { {1,fa4}, {2,fa5}, {3, fa6},{4,fa7},{5, fa3},{6, fa2}, {7, fa1} ,{8, fa0}, {9, ft0},
								{10, ft1}, {11,ft2}, {12,ft3}, {13,ft4},{14,ft5},{15, ft6}, {16, ft7}, {17, ft8},
								{18, ft9}, {19, ft10}, {20, ft11},{21,fs0},{22,fs1}, {23,fs2},{24,fs3}, {25,fs4}, {26,fs5},
								{27,fs6}, {28, fs7}, {29, fs8}, {30,fs9},{31, fs10},{32, fs11} };

class InterNode;

struct pair_hash {
	template <class T1, class T2>
	std::size_t operator () (const std::pair<T1, T2>& p) const {
		auto h1 = std::hash<T1>{}(p.first);
		auto h2 = std::hash<T2>{}(p.second);
		return h1 ^ h2;
	}
};

struct pair_equal {
	template <class T1, class T2>
	bool operator () (const std::pair<T1, T2>& p1, const std::pair<T1, T2>& p2) const {
		return p1.first == p2.first && p1.second == p2.second;
	}
};


class InterNode {
public:
	string name;
	ResultType type;
	bool func = false;
	set<InterNode*> virtual_nodes;
	set<InterNode*> real_nodes;

	bool coalesce = false;
	InterNode* coalescenode;

	int pre_color = -1;
	bitset<8> pre_nums;

	int pre_protect = 0;

	bool contain_in_virtual(InterNode* node) {
		if (find(virtual_nodes.begin(), virtual_nodes.end(), node) == virtual_nodes.end()) {
			return false;
		}
		return true;
	}

	bool contain_in_real(InterNode* node) {
		if (find(real_nodes.begin(), real_nodes.end(), node) == real_nodes.end()) {
			return false;
		}
		return true;
	}

	void add_virtual(InterNode* end) {
		//以下这种情况不能合并，加实边
		//if (this->pre_color > 0 || end->pre_color > 0) {
		//	if (this->func || end->func) {
		//		remove_virtual(end);
		//		this->real_nodes.insert(end);
		//		end->real_nodes.insert(this);
		//		return;
		//	}
		//}
		if (this->real_nodes.find(end) != this->real_nodes.end()) {	
			remove_real(end);
		}
		this->virtual_nodes.insert(end);
		end->virtual_nodes.insert(this);
	}

	void add_real(InterNode* end) {
		//已有或类型不同
		//if (this->virtual_nodes.find(end) != this->virtual_nodes.end()) {//如果存在虚边，不加实边
		//	remove_virtual(end);
		//}
		this->real_nodes.insert(end);
		end->real_nodes.insert(this);
	}

	void remove_real(InterNode* node) {
		node->real_nodes.erase(this);
		this->real_nodes.erase(node);
	}

	void remove_virtual(InterNode* node) {
		node->virtual_nodes.erase(this);
		this->virtual_nodes.erase(node);
	}
	/*void remove_virtual(InterNode* end) {
		for (int i = 0; i < this->virtual_nodes.size(); i++) {
			if (end == this->virtual_nodes[i]) {
				this->virtual_nodes.erase(this->virtual_nodes.begin()+i);
				break;
			}
		}
	}*/
};

class InterferenceGraph {
public:
	unordered_set<InterNode*> nodes;
	unordered_map<string, InterNode*> nodesMap;
	unordered_set<InterNode*> eraseMap;
	vector<InterNode*> stack;
	vector<InterNode*> spillstack;
	map<string, int> color;

	void clear() {
		this->nodes.clear();
		this->nodesMap.clear();
		this->spillstack.clear();
		this->stack.clear();
		this->color.clear();
	}
};

class IGManager {
private:
	DataFlowAnalysis* dfa;
public:
	InterferenceGraph* fGraph;
	InterferenceGraph* iGraph;
	unordered_set<ResultType> i32_set = { i32,iarray,farray };

	unordered_map<string, InterNode*> nodesMap;

	unordered_map<string, int> color;
	//标记在函数调用前最多需要保留几个寄存器的值
	int i32_max_func;
	int f32_max_func;

	//判断是否有函数调用
	bool hasFunc;

	unordered_map<string, DAGNode*> find_map;

	unordered_map<pair<string,string>,int,pair_hash,pair_equal> edge_index;
	int count = 0;//记录边的索引
	unordered_set<pair<string, string>, pair_hash, pair_equal> virtual_edges;

	IGManager(DataFlowAnalysis* d) :dfa(d) {
		fGraph = new InterferenceGraph();
		iGraph = new InterferenceGraph();
		hasFunc = false;
	}

	void print(string s) {
		//cout << s << endl;
	}

	void tag_block() {
		queue<BasicBlock*> q;
		q.push(dfa->cfg->entry);
		while (!q.empty()) {
			BasicBlock* block = q.front();
			if (block->postBlock.size() >= 2) {

			}
		}
	}

	//只加相应类型的边
	void add_edges(InterferenceGraph* ig,const set<string>& live_set,BasicBlock* block) {
		if (live_set.size() <= 1) return;
		vector<string> temp;
		for (auto it = live_set.begin(); it != live_set.end(); it++) {
			if (ig->nodesMap.find(*it) !=ig->nodesMap.end()) {
				temp.push_back(*it);
			}
		}
		int size = temp.size();
		int i_size = size - 1;
		for (int i = 0; i < i_size; i++) {
			for (int j = i + 1; j < size; j++) {
				pair<string, string> key(temp[i], temp[j]);
				if (temp[i] > temp[j]) {
					key = make_pair(temp[j], temp[i]);
				}
				if (edge_index.find(key) == edge_index.end()) {
					edge_index[key] = count;
					count++;
				}
				block->dataInfo->edges.set(edge_index[key]);
				ig->nodesMap[temp[i]]->add_real(ig->nodesMap[temp[j]]);
			}
		}
	}

	//新加入的元素和live_set里面的元素加边
	void add_edges(InterferenceGraph* ig, const set<string>& live_set, string newName, BasicBlock* block) {
		if (ig->nodesMap.find(newName) == ig->nodesMap.end()) return;
		for (auto& live_it : live_set) {
			if (ig->nodesMap.find(live_it) != ig->nodesMap.end() && newName != live_it) {
				pair<string, string> key(newName,live_it);
				if (newName > live_it) {
					key = make_pair(live_it, newName);
				}
				if (edge_index.find(key) == edge_index.end()) {
					edge_index[key] = count;
					count++;
				}
				block->dataInfo->edges.set(edge_index[key]);
				ig->nodesMap[newName]->add_real(ig->nodesMap[live_it]);
			}
		}
	}


	//更改整型的干涉图
	void create_i32G() {
		// 获取当前时间点
		//auto start = std::chrono::high_resolution_clock::now();

		iGraph->nodes.clear();
		iGraph->nodesMap.clear();
		dfa->liveVariablesAnalysis(1);//活跃变量分析
		i32_max_func = 0;
		if (dfa->cfg->basicBlocks.size() <= 0)   return;

		//边的数量清0
		count = 0;
		edge_index.clear();
		virtual_edges.clear();

		//创建干涉图节点
		for (int i = 0; i < dfa->live_varsSet.size(); i++) {
			if (dfa->live_typeMap[dfa->live_varsSet[i]] == iarray || dfa->live_typeMap[dfa->live_varsSet[i]] == farray) continue;
			InterNode* newNode = new InterNode();
			newNode->name = dfa->live_varsSet[i];
			newNode->type = dfa->live_typeMap[dfa->live_varsSet[i]];
			if (dfa->live_typeMap[dfa->live_varsSet[i]] != f32) {
				iGraph->nodes.insert(newNode);
				iGraph->nodesMap[dfa->live_varsSet[i]] = newNode;
			}
		}

		ExpManager* expM = new ExpManager();

		//设置每个块的虚边信息
		queue<BasicBlock*> q;
		map<BasicBlock*, int> degree;
		map<BasicBlock*, bool> visited;
		for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
			if ((*blockIt)->postBlock.size() == 0 || (*blockIt)->label == BasicBlock::LabelType::WHILEBODY || (*blockIt)->label == BasicBlock::LabelType::IF_END_AND_WHILEBODY) {
				q.push((*blockIt));
			}
			degree[(*blockIt)] = (*blockIt)->postBlock.size();
			visited[(*blockIt)] = false;
		}
		//加边
		while (!q.empty()) {
			BasicBlock* block = q.front();
			q.pop();
			visited[block] = true;
			for (int i = 0; i < block->preBlock.size(); i++) {
				degree[block->preBlock[i]]--;
				if (degree[block->preBlock[i]] <= 0 && !visited[block->preBlock[i]])
					q.push(block->preBlock[i]);
			}

			bitset<10*SIZE> edge_bit;
			edge_bit.reset();
			for (int i = 0; i < block->postBlock.size(); i++) {
				edge_bit |= block->postBlock[i]->dataInfo->edges;
			}
			block->dataInfo->edges = edge_bit;
			print("块：" + to_string(block->id));

			//生成活跃集
			vector<string> temp = block->dataInfo->bitSet_to_vector(block->dataInfo->liveOutSet, dfa->live_varsSet);//标志当前活跃的变量
			set<string> live_set(temp.begin(), temp.end());

			add_edges(iGraph,live_set, block);
			//倒序遍历
			for (auto dagNode = block->topoList.rbegin(); dagNode != block->topoList.rend(); dagNode++) {
				string useName;
				string defName;
				switch ((*dagNode)->nodeType) {
				case LOAD:
					useName = "";
					defName = "%" + to_string((*dagNode)->resultId);
					add_edges(iGraph, live_set, defName, block);
					/*live_set.insert(defName);
					add_edges(iGraph, live_set, i32);*/
					live_set.erase(defName);
					if ((*dagNode)->inputs.size() > 0 && (*dagNode)->inputs[0]->resultType != iarray && (*dagNode)->inputs[0]->resultType != farray) {
						useName = (*dagNode)->inputs[0]->nodeType == VALUE_NODE ? (*dagNode)->inputs[0]->name : ("%" + to_string((*dagNode)->inputs[0]->resultId));
						add_edges(iGraph, live_set, useName, block);
						live_set.insert(useName);
						//find_map[useName] = (*dagNode)->inputs[0];
					}
					//find_map[defName] = *dagNode;
					break;
				case STORE:
					useName = "";
					for (int i = 0; i < (*dagNode)->inputs.size(); i++) {
						if ((*dagNode)->inputs[i]->nodeType != CONSTANT && (*dagNode)->inputs[i]->resultType != iarray && (*dagNode)->inputs[i]->resultType != farray) {//只有use？
							useName = (*dagNode)->inputs[i]->nodeType == VALUE_NODE ? (*dagNode)->inputs[i]->name : ("%" + to_string((*dagNode)->inputs[i]->resultId));
							add_edges(iGraph, live_set, useName, block);
							live_set.insert(useName);
							
							//find_map[useName] = (*dagNode)->inputs[i];
						}
					}

					//add_edges(iGraph,live_set,i32);
					break;
				case OP_NODE:
					useName = "";
					defName = "";
					if ((*dagNode)->inst == MV || (*dagNode)->inst == FMVS) {
						if ((*dagNode)->inputs.size() == 1) {
							defName = "%" + to_string((*dagNode)->resultId);
							useName = (*dagNode)->inputs[0]->nodeType == VALUE_NODE ? (*dagNode)->inputs[0]->name : ("%" + to_string((*dagNode)->inputs[0]->resultId));
							//find_map[defName] = *dagNode;
							//find_map[useName] = (*dagNode)->inputs[0];
						}
						else if ((*dagNode)->inputs.size() == 2) {
							defName = (*dagNode)->inputs[0]->nodeType == VALUE_NODE ? (*dagNode)->inputs[0]->name : ("%" + to_string((*dagNode)->inputs[0]->resultId));
							useName = (*dagNode)->inputs[1]->nodeType == VALUE_NODE ? (*dagNode)->inputs[1]->name : ("%" + to_string((*dagNode)->inputs[1]->resultId));
							//处理预着色结点
							if ((*dagNode)->inputs[0]->precolor > 0 && iGraph->nodesMap.find((*dagNode)->inputs[0]->name) != iGraph->nodesMap.end()) {
								iGraph->nodesMap[(*dagNode)->inputs[0]->name]->pre_color = (*dagNode)->inputs[0]->precolor;
							}
							else if ((*dagNode)->inputs[1]->precolor > 0 && iGraph->nodesMap.find((*dagNode)->inputs[1]->name) != iGraph->nodesMap.end()) {
								iGraph->nodesMap[(*dagNode)->inputs[1]->name]->pre_color = (*dagNode)->inputs[1]->precolor;
							}
							//find_map[defName] = (*dagNode)->inputs[0];
							//find_map[useName] = (*dagNode)->inputs[1];
						}
						add_edges(iGraph, live_set, defName, block);
						live_set.erase(defName);
						add_edges(iGraph, live_set, useName, block);
						live_set.insert(useName);

						//处理虚边
						if (iGraph->nodesMap.find(defName) != iGraph->nodesMap.end() && iGraph->nodesMap.find(useName) != iGraph->nodesMap.end()) {
							pair<string, string> key(defName, useName);
							if (defName > useName) {
								key = make_pair(useName, defName);
							}
							if (edge_index.find(key) == edge_index.end()) {
								edge_index[key] = count;
								count++;
							}
							block->dataInfo->edges.reset(edge_index[key]);//虚边置0
							virtual_edges.insert(key);
							//iGraph->nodesMap[defName]->add_virtual(iGraph->nodesMap[useName]); //添加虚边；
						}
					}
					else {
						defName = "%" + to_string((*dagNode)->resultId);
						add_edges(iGraph, live_set, defName, block);
						live_set.erase(defName);
						//忽略全局
						if ((*dagNode)->inst == LUI) {
							break;
						}

						//判断是不是函数调用
						if ((*dagNode)->inst == CALL) {
							live_set.erase(defName);
							//int temp_i32 = 0;
							hasFunc = true;
							for (auto it : live_set) {
								if (it != "f%i0" && it != "f%f0" && iGraph->nodesMap.find(it)!=iGraph->nodesMap.end()) {
									iGraph->nodesMap[it]->func = true;
								}
							}
							//i32_max_func = max(temp_i32, i32_max_func);
						}

						for (int i = 0; i < (*dagNode)->inputs.size(); i++) {
							if ((*dagNode)->inputs[i]->nodeType == CONSTANT || (*dagNode)->inputs[i]->resultType == iarray || (*dagNode)->inputs[i]->resultType == farray
								|| (*dagNode)->inputs[i]->inst == SD || (*dagNode)->inputs[i]->inst == FSD) continue; 
							useName = (*dagNode)->inputs[i]->nodeType == VALUE_NODE ? (*dagNode)->inputs[i]->name : ("%" + to_string((*dagNode)->inputs[i]->resultId));
							add_edges(iGraph, live_set, useName, block);
							live_set.insert(useName);
							//find_map[useName] = (*dagNode)->inputs[i];
						}
					}
					/*add_edges(iGraph,live_set,i32);*/
					break;
				default:
					break;
				}
			}
		}

		for (auto &v_edge : virtual_edges) {
			if (!dfa->cfg->entry->dataInfo->edges.test(edge_index[v_edge])) {
				iGraph->nodesMap[v_edge.first]->add_virtual(iGraph->nodesMap[v_edge.second]); //添加虚边；
			}
		}

		delete expM;

		//// 获取当前时间点
		//auto end = std::chrono::high_resolution_clock::now();

		//// 计算执行时间（以毫秒为单位）
		//auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		//std::cout << "Execution time: " << duration << " milliseconds" << std::endl;
	}

	//更改浮点类型的干涉图
	void create_f32G() {
		fGraph->nodes.clear();
		fGraph->nodesMap.clear();
		dfa->liveVariablesAnalysis(1);//活跃变量分析
		f32_max_func = 0;
		if (dfa->cfg->basicBlocks.size() <= 0)   return;


		//边的数量清0
		count = 0;
		edge_index.clear();
		virtual_edges.clear();

		//创建干涉图节点
		for (int i = 0; i < dfa->live_varsSet.size(); i++) {
			if (dfa->live_typeMap[dfa->live_varsSet[i]] == iarray || dfa->live_typeMap[dfa->live_varsSet[i]] == farray) continue;
			InterNode* newNode = new InterNode();
			newNode->name = dfa->live_varsSet[i];
			newNode->type = dfa->live_typeMap[dfa->live_varsSet[i]];
			if (dfa->live_typeMap[dfa->live_varsSet[i]] == f32) {
				fGraph->nodes.insert(newNode);
				fGraph->nodesMap[dfa->live_varsSet[i]] = newNode;
			}
			/*nodesMap[dfa->live_varsSet[i]] = newNode;*/
		}

		ExpManager* expM = new ExpManager();

		//设置每个块的虚边信息
		queue<BasicBlock*> q;
		map<BasicBlock*, int> degree;
		map<BasicBlock*, bool> visited;
		for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
			if ((*blockIt)->postBlock.size() == 0 || (*blockIt)->label == BasicBlock::LabelType::WHILEBODY || (*blockIt)->label == BasicBlock::LabelType::IF_END_AND_WHILEBODY) {
				q.push((*blockIt));
			}
			degree[(*blockIt)] = (*blockIt)->postBlock.size();
			visited[(*blockIt)] = false;
		}
		//加边
		while (!q.empty()) {
			BasicBlock* block = q.front();
			q.pop();
			visited[block] = true;
			for (int i = 0; i < block->preBlock.size(); i++) {
				degree[block->preBlock[i]]--;
				if (degree[block->preBlock[i]] <= 0 && !visited[block->preBlock[i]])
					q.push(block->preBlock[i]);
			}

			bitset<10*SIZE> edge_bit;
			edge_bit.reset();
			for (int i = 0; i < block->postBlock.size(); i++) {
				edge_bit |= block->postBlock[i]->dataInfo->edges;
			}
			block->dataInfo->edges = edge_bit;
			print("块：" + to_string(block->id));

			//加边
			vector<string> temp = block->dataInfo->bitSet_to_vector(block->dataInfo->liveOutSet, dfa->live_varsSet);//标志当前活跃的变量
			set<string> live_set(temp.begin(), temp.end());
			//print("块：" + to_string((*blockIt)->id));

			add_edges(fGraph, live_set, block);
			//倒序遍历
			for (auto dagNode = block->topoList.rbegin(); dagNode != block->topoList.rend(); dagNode++) {
				string useName;
				string defName;
				switch ((*dagNode)->nodeType) {
				case LOAD:
					useName = "";
					defName = "%" + to_string((*dagNode)->resultId);
					add_edges(fGraph, live_set, defName, block);
					/*live_set.insert(defName);
					add_edges(iGraph, live_set, i32);*/
					live_set.erase(defName);
					if ((*dagNode)->inputs.size() > 0 && (*dagNode)->inputs[0]->resultType != iarray && (*dagNode)->inputs[0]->resultType != farray) {
						useName = (*dagNode)->inputs[0]->nodeType == VALUE_NODE ? (*dagNode)->inputs[0]->name : ("%" + to_string((*dagNode)->inputs[0]->resultId));
						add_edges(fGraph, live_set, useName, block);
						live_set.insert(useName);
					}
					break;
				case STORE:
					useName = "";
					for (int i = 0; i < (*dagNode)->inputs.size(); i++) {
						if ((*dagNode)->inputs[i]->nodeType != CONSTANT && (*dagNode)->inputs[i]->resultType != iarray && (*dagNode)->inputs[i]->resultType != farray) {//只有use？
							useName = (*dagNode)->inputs[i]->nodeType == VALUE_NODE ? (*dagNode)->inputs[i]->name : ("%" + to_string((*dagNode)->inputs[i]->resultId));
							add_edges(fGraph, live_set, useName, block);
							live_set.insert(useName);
						}
					}
					/*add_edges(fGraph,live_set, f32);*/
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
							//处理预着色结点
							if ((*dagNode)->inputs[0]->precolor > 0 && fGraph->nodesMap.find((*dagNode)->inputs[0]->name) != fGraph->nodesMap.end()) {
								fGraph->nodesMap[(*dagNode)->inputs[0]->name]->pre_color = (*dagNode)->inputs[0]->precolor;
							}
							else if ((*dagNode)->inputs[1]->precolor > 0 && fGraph->nodesMap.find((*dagNode)->inputs[1]->name) != fGraph->nodesMap.end()) {
								fGraph->nodesMap[(*dagNode)->inputs[1]->name]->pre_color = (*dagNode)->inputs[1]->precolor;
							}
						}
						add_edges(fGraph, live_set, defName, block);
						live_set.erase(defName);
						add_edges(fGraph, live_set, useName, block);
						live_set.insert(useName);
						//处理虚边
						if (fGraph->nodesMap.find(defName) != fGraph->nodesMap.end() && fGraph->nodesMap.find(useName) != fGraph->nodesMap.end()) {
							pair<string, string> key(defName, useName);
							if (defName > useName) {
								key = make_pair(useName, defName);
							}
							if (edge_index.find(key) == edge_index.end()) {
								edge_index[key] = count;
								count++;
							}
							block->dataInfo->edges.reset(edge_index[key]);//虚边置0
							virtual_edges.insert(key);
						}
					}
					else {
						defName = "%" + to_string((*dagNode)->resultId);
						add_edges(fGraph, live_set, defName, block);
						/*live_set.insert(defName);
						add_edges(fGraph, live_set, f32);*/
						live_set.erase(defName);
						//忽略全局
						if ((*dagNode)->inst == LUI) {
							break;
						}
						//判断是不是函数调用
						if ((*dagNode)->inst == CALL) {
							//int temp_f32 = 0;
							hasFunc = true;
							for (auto it : live_set) {
								if (it != "f%i0" && it != "f%f0" && fGraph->nodesMap.find(it) != fGraph->nodesMap.end()) {
									fGraph->nodesMap[it]->func = true;
								}
							}
						}
						for (int i = 0; i < (*dagNode)->inputs.size(); i++) {
							if ((*dagNode)->inputs[i]->nodeType == CONSTANT || (*dagNode)->inputs[i]->resultType == iarray || (*dagNode)->inputs[i]->resultType == farray
								|| (*dagNode)->inputs[i]->inst == SD || (*dagNode)->inputs[i]->inst == FSD) continue;
							useName = (*dagNode)->inputs[i]->nodeType == VALUE_NODE ? (*dagNode)->inputs[i]->name : ("%" + to_string((*dagNode)->inputs[i]->resultId));
							add_edges(fGraph, live_set, useName, block);
							live_set.insert(useName);
						}
					}
					/*add_edges(fGraph,live_set, f32);*/
					break;
				default:
					break;
				}
			}
		}

		for (auto v_edge : virtual_edges) {
			//在最开始块判断是否可以加虚边
			if (!dfa->cfg->entry->dataInfo->edges.test(edge_index[v_edge])) {
				fGraph->nodesMap[v_edge.first]->add_virtual(fGraph->nodesMap[v_edge.second]); //添加虚边；
			}
		}


		delete expM;
	}

};

