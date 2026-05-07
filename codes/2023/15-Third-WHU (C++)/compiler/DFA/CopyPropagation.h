
#include "DataFlowAnalysis.h"

class CopyPropagation {
private:
	DataFlowAnalysis* dfa;
public:
	vector<string> cp_vec;
	set<string> cp_set;

	map<string, string> copy_map;

	CopyPropagation(DataFlowAnalysis* s) :dfa(s) {}

	//得出gen,kill集
	void cp_traversal() {
		dfa->topologicalSort();
		for (auto blockIt = dfa->cfg->basicBlocks.begin(); blockIt != dfa->cfg->basicBlocks.end(); blockIt++) {
			set<string> gen_buff;
			string varName;
			string useName;
			string copyS;
			for (auto dagnode : (*blockIt)->topoList) {

				if (dagnode->nodeType == STORE && dagnode->inputs[sd_variableName]->nodeType != OP_NODE
					&& dagnode->inputs[sd_assignment]->nodeType != CONSTANT && dagnode->inputs[sd_assignment]->nodeType != OP_NODE) {
					varName = dagnode->inputs[sd_variableName]->nodeType == VALUE_NODE ? dagnode->inputs[sd_variableName]->name : ("%" + to_string(dagnode->inputs[sd_variableName]->resultId));
					useName = dagnode->inputs[sd_assignment]->nodeType == LOAD ? dagnode->inputs[sd_assignment]->inputs[0]->name : ("%" + to_string(dagnode->inputs[sd_assignment]->resultId));
					copyS = varName + " " + useName;

					//移除可复写语句
					set<string> temp = gen_buff;
					for (auto it : temp) {
						if (contain_opernads(it, varName)) {
							gen_buff.erase(it);
						}
					}
				}



				//if (dagnode->inst == MV || dagnode->inst == FMVS) {
				//	if (dagnode->inputs.size() == 1) {
				//		varName = "%" + to_string(dagnode->resultId);
				//		useName = dagnode->inputs[0]->nodeType == VALUE_NODE ? dagnode->inputs[0]->name : ("%" + to_string(dagnode->inputs[0]->resultId));
				//	}
				//	else if (dagnode->inputs.size() == 2) {
				//		varName = dagnode->inputs[0]->nodeType == VALUE_NODE ? dagnode->inputs[0]->name : ("%" + to_string(dagnode->inputs[0]->resultId));
				//		useName = dagnode->inputs[1]->nodeType == VALUE_NODE ? dagnode->inputs[1]->name : ("%" + to_string(dagnode->inputs[1]->resultId));
				//	}
				//	copyS = varName + " " + useName;

				//	//移除可复写语句
				//	set<string> temp = gen_buff;
				//	for (auto it:temp) {
				//		if (contain_opernads(it, varName)) {
				//			gen_buff.erase(it);
				//		}
				//	}
				//}

			}
		}
	}
	//复写传播分析
	void cp_analysis() {

	}


	//判断是否具有该变量
	bool contain_opernads(string cp, string s) {
		stringstream ss(cp);
		string part;
		while (ss >> part) {
			if (s == part)
				return true;
		}
		return false;
	}

	bool LocalCopyPropagation() {

	}



};
