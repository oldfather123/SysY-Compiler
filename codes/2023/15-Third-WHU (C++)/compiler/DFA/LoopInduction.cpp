#include "LoopInduction.h"

//判断是不是函数调用
//数组也做不了
unordered_set<string> CommonOPNameVec_copy =
{ PLUS ,SUBTRACT,MULTIPLY,DIVIDE,GREATER_THAN,GREATER_EQUAL ,LESS_THAN ,LESS_EQUAL,EQUAL,NOT_EQUAL ,
LOGICAL_NOT,LOGICAL_AND,LOGICAL_OR,REMAINDER,IMPLICIT_INIT_NODE,
INT_TO_FLOAT ,FLOAT_TO_INT,ARRAY_TO_POINT,
BRANCH_NODE,RETURN_NODE};
//PHI,store_para


DAGNode* LoopInduction::struct_copy(DAGNode* n) {

	if (n->nodeType == OP_NODE && CommonOPNameVec_copy.find(n->name) == CommonOPNameVec_copy.end()) {
		//函数或者数组或PHI
		return nullptr;
	}

	if (n->nodeType == VALUE_NODE && symboltable->isGlobalSymbol(n->symbol)) {
		//全局变量
		return nullptr;
	}

	DAGNode* temp_n = new DAGNode(n, n->symbol);
	temp_n->resultId = cfg->getCount();

	for (DAGNode* iter : n->inputs) {
		DAGNode* temp_iter = struct_copy(iter);
		if (temp_iter == nullptr) {
			return nullptr;
		}
		temp_n->inputs.push_back(temp_iter);
		temp_iter->outputs.push_back(temp_n);
	}
	return temp_n;
}




void LoopInduction::varassign_detect(DAGNode* node,DAGNode* last) {


	if (node->nodeType == STORE) {

		//形参无初始化
		//右值
		if (node->name != "store_para") {
			if (node->inputs[1]->name == "%PHI") {
				return;
			}
			varassign_detect(node->inputs[1],node);
		}

		//左值
		DAGNode* leftnode = node->inputs[0];
		if (node->name!="store_para" && node->inputs[1]->name==ARRAY_INIT_NODE) {
			//数组初始化，则左值无处理
		}
		else if (leftnode->name == ARRAYSUB_NODE) {
			//数组需要考虑地址计算
			if (leftnode->inputs.size() > 1) {
				varassign_detect(leftnode->inputs[1],leftnode);
			}
		}
		else {
			//变量
			if (symboltable->isGlobal(leftnode->symbol)) {
				//不记录全局变量
			}
			else if (leftnode->symbol->type == iarray || leftnode->symbol->type == farray
				|| leftnode->symbol->type == ipoint || leftnode->symbol->type == fpoint) {
				//不记录数组
			}
			else {
				//记录普通变量
				string var_name = leftnode->name;
				if (node->name != "store_para") {
					DAGNode* temp_copy= struct_copy(node->inputs[1]);
					if (temp_copy != nullptr) {
						var_assign[var_name] = temp_copy;
					}
				}
			}

		}

	}
	else if (node->nodeType == LOAD) {
		//要用到变量了
		//可以被替换的时候，表达式就一定是原子了
		//可能是数组
		DAGNode* varnode = node->inputs[0];
		if (varnode->nodeType == VALUE_NODE) {
			if (var_assign.find(varnode->name) != var_assign.end()) {
				//可以替换了
				DAGNode* replace_node = struct_copy(var_assign[varnode->name]);
				*find(last->inputs.begin(), last->inputs.end(), node) = replace_node;
				replace_node->outputs.push_back(last);
			}
		}
		else if (varnode->nodeType == OP_NODE) {
			for (DAGNode* input_iter : varnode->inputs) {
				varassign_detect(input_iter, varnode);
			}
		}


	}
	else if (node->nodeType == OP_NODE) {
		//当前没有move操作，直接往下读即可
		for (DAGNode* op_iter : node->inputs) {
			varassign_detect(op_iter,node);
		}
	}
	else if (node->nodeType == CONSTANT) {
		//常量无处理
	}
	else if (node->nodeType == VALUE_NODE) {
		//数组名/指针作为函数调用的参数
		//不会被传播
	}

}


void LoopInduction::forward_replace() {
	for (BasicBlock* b_iter : cfg->basicBlocks) {
		vector<DAGNode*> nextsen;
		//找到第一条语句节点
		DAGNode* firstnode = b_iter->dag->rootNode;
		nextsen.insert(nextsen.begin(), firstnode);
		while (!firstnode->relyNodes.empty()) {
			firstnode = firstnode->relyNodes[0];
			nextsen.insert(nextsen.begin(), firstnode);
		}
		//拓扑遍历,初始化是按语句顺序记录,单边依赖
		//忽略全局变量
		for (int sen_index = 0; sen_index < nextsen.size(); sen_index++) {
			varassign_detect(nextsen[sen_index], nullptr);
		}

	}
}