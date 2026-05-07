#include"../IR/GCFG.h"
#include <set>
#include<cmath>

enum UnaryResult {
	EXP_NONE,
	EXP_ZHENG,
	EXP_FU,
	EXP_FEI
};

class ExpManager {
public:
	//运算符集合
	set<string> ava_opSet = { FLOAT_TO_INT,INT_TO_FLOAT,PLUS,SUBTRACT,MULTIPLY,DIVIDE,GREATER_THAN,GREATER_EQUAL,
		LESS_THAN,LESS_EQUAL,EQUAL,NOT_EQUAL,LOGICAL_NOT,LOGICAL_AND,LOGICAL_OR,REMAINDER };

	set<string> unary_operatior = { FLOAT_TO_INT,INT_TO_FLOAT, LOGICAL_NOT };

	//进行加法运算,在操作数都为常量的情况下
	Value exp_calculate_plus(DAGNode* node, Value value) {
		return value;
	}

	Value exp_calculate_plus(DAGNode* node, Value value1, Value value2) {
		Value newValue;
		if (node->resultType == i32) {
			int a = value1.iValue;
			int b = value2.iValue;
			newValue.iValue = a + b;
		}
		else if (node->resultType == f32) {
			float a = node->inputs[0]->value.fValue;
			float b = node->inputs[1]->value.fValue;
			newValue.fValue = a + b;
		}
		return newValue;
	}

	Value exp_calculate_sub(DAGNode* node, Value value) {
		Value newValue;
		newValue.iValue = -value.iValue;
		newValue.fValue = -value.fValue;
		return newValue;
	}

	//减法运算
	Value exp_calculate_sub(DAGNode* node, Value value1, Value value2) {
		Value newValue;
		if (node->resultType == i32) {
			int a = value1.iValue;
			int b = value2.iValue;
			newValue.iValue = a - b;
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			newValue.fValue = a - b;
		}
		return newValue;
	}

	//乘法运算
	Value exp_calculate_mul(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			int a = value1.iValue;
			int b = value2.iValue;
			value.iValue = a * b;
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.fValue = a * b;
		}
		return value;
	}

	//除法运算
	Value exp_calculate_divide(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			int a = value1.iValue;
			int b = value2.iValue;
			value.iValue = a / b;
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.fValue = a / b;
		}
		return value;
	}


	//取余运算
	Value exp_calculate_remainder(DAGNode* node, Value value1, Value value2) {
		Value value;
		int a = value1.iValue;
		int b = value2.iValue;
		value.iValue = a % b;
		return value;
	}

	//大于：>
	Value exp_calculate_greaterThan(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				int b = value2.iValue;
				value.iValue = a > b;
			}
			else {
				float a = value1.fValue;
				float b = value2.fValue;
				value.iValue = a > b;
			}
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.iValue = a > b;
		}
		return value;
	}

	//大于等于：>=
	Value exp_calculate_greaterEqual(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				int b = value2.iValue;
				value.iValue = a >= b;
			}
			else {
				float a = value1.fValue;
				float b = value2.fValue;
				value.iValue = a >= b;
			}	
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.iValue = a >= b;
		}
		return value;
	}

	//小于：<
	Value exp_calculate_less(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				int b = value2.iValue;
				value.iValue = a < b;
			}
			else {
				float a = value1.fValue;
				float b = value2.fValue;
				value.iValue = a < b;
			}
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.iValue = a < b;
		}
		return value;
	}

	//小于等于：<=
	Value exp_calculate_lessEqual(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				int b = value2.iValue;
				value.iValue = a <= b;
			}
			else {
				float a = value1.fValue;
				float b = value2.fValue;
				value.iValue = a <= b;
			}
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.iValue = a <= b;
		}
		return value;
	}

	//等于：==
	Value exp_calculate_equal(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				int b = value2.iValue;
				value.iValue = a == b;
			}
			else {
				float a = value1.fValue;
				float b = value2.fValue;
				value.iValue = a == b;
			}
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.iValue = (a == b);
		}
		return value;
	}

	//等于：!=
	Value exp_calculate_notEqual(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				int b = value2.iValue;
				value.iValue = a != b;
			}
			else {
				float a = value1.fValue;
				float b = value2.fValue;
				value.iValue = a != b;
			}
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.iValue = a != b;
		}
		return value;
	}

	//逻辑非：!
	Value exp_calculate_logicalNot(DAGNode* node, Value value1) {
		Value newValue;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				newValue.iValue = !a;
			}
			else {
				float a = value1.fValue;
				newValue.iValue = !a;
			}
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			newValue.iValue = !a;
		}
		return newValue;
	}
	//逻辑与
	Value exp_calculate_and(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				int b = value2.iValue;
				value.iValue = a && b;
			}
			else {
				float a = value1.fValue;
				float b = value2.fValue;
				value.iValue = a && b;
			}
			
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.iValue = a && b;
		}
		return value;
	}


	//逻辑或
	Value exp_calculate_or(DAGNode* node, Value value1, Value value2) {
		Value value;
		if (node->resultType == i32) {
			if (node->inputs[0]->resultType == i32) {
				int a = value1.iValue;
				int b = value2.iValue;
				value.iValue = a || b;
			}
			else {
				float a = value1.fValue;
				float b = value2.fValue;
				value.iValue = a || b;
			}
		}
		else if (node->resultType == f32) {
			float a = value1.fValue;
			float b = value2.fValue;
			value.iValue = a || b;
		}
		return value;
	}


	//浮点转整数
	Value exp_float_T_int(DAGNode* node, Value value1) {
		Value newValue;
		newValue.iValue = (int)std::trunc(value1.fValue);
		return newValue;
	}

	//整数转浮点
	Value exp_int_T_float(DAGNode* node, Value value1) {
		Value newValue;
		newValue.fValue = (float)value1.iValue;
		return newValue;
	}



	//重载，该函数负责计算单目运算符
	Value folding_calculate_collection(DAGNode* opNode, Value valueLeft) {
		Value newValue;
		string nodename = opNode->name;
		/*bool isSingle = opNode->inputs.size() == 1 ? 1 : 0;*/
		if (nodename == PLUS) {//加
			newValue = exp_calculate_plus(opNode, valueLeft);
		}
		else if (nodename == SUBTRACT) {//减
			newValue = exp_calculate_sub(opNode, valueLeft);
		}
		else if (nodename == LOGICAL_NOT) {//逻辑非
			newValue = exp_calculate_logicalNot(opNode, valueLeft);
		}
		else if (nodename == FLOAT_TO_INT) {//浮点转整数
			newValue = exp_float_T_int(opNode, valueLeft);
		}
		else if (nodename == INT_TO_FLOAT) {//整数转浮点
			newValue = exp_int_T_float(opNode, valueLeft);
		}
		return newValue;
	}


	//该函数负责双目运算符的运算
	Value folding_calculate_collection(DAGNode* opNode, Value valueLeft, Value valueRight) {
		Value newValue;
		string nodename = opNode->name;
		if (nodename == PLUS) {//加
			newValue = exp_calculate_plus(opNode, valueLeft, valueRight);
		}
		else if (nodename == SUBTRACT) {//减
			newValue = exp_calculate_sub(opNode, valueLeft, valueRight);
		}
		else if (nodename == MULTIPLY) {//乘
			newValue = exp_calculate_mul(opNode, valueLeft, valueRight);
		}
		else if (nodename == DIVIDE) {//除
			newValue = exp_calculate_divide(opNode, valueLeft, valueRight);
		}
		else if (nodename == REMAINDER) {//取余
			newValue = exp_calculate_remainder(opNode, valueLeft, valueRight);
		}
		else if (nodename == GREATER_THAN) {//大于
			newValue = exp_calculate_greaterThan(opNode, valueLeft, valueRight);
		}
		else if (nodename == GREATER_EQUAL) {//大于等于
			newValue = exp_calculate_greaterEqual(opNode, valueLeft, valueRight);
		}
		else if (nodename == LESS_THAN) {//小于
			newValue = exp_calculate_less(opNode, valueLeft, valueRight);
		}
		else if (nodename == LESS_EQUAL) {//小于等于
			newValue = exp_calculate_lessEqual(opNode, valueLeft, valueRight);
		}
		else if (nodename == EQUAL) {//等于
			newValue = exp_calculate_equal(opNode, valueLeft, valueRight);
		}
		else if (nodename == NOT_EQUAL) {//不等于
			newValue = exp_calculate_notEqual(opNode, valueLeft, valueRight);
		}
		else if (nodename == LOGICAL_AND) {//与
			newValue = exp_calculate_and(opNode, valueLeft, valueRight);
		}
		else if (nodename == LOGICAL_OR) {//或
			newValue = exp_calculate_or(opNode, valueLeft, valueRight);
		}
		return newValue;
	}

	//消除块内的常量表达式的接口
	void calculate_constantExp(GCFG* gcfg) {
		for (const auto& pair : gcfg->controlFlow) {
			for (auto iter = pair.second->basicBlocks.begin(); iter != pair.second->basicBlocks.end(); iter++) {
				if ((*iter)->dag->nodes.size() != 0)
					eliminate_constantExp((*iter)->dag->rootNode, (*iter)->dag);
			}
		}
	}
	//消除常量表达式的递归函数
	int eliminate_constantExp(DAGNode* root, DAG* dag) {
		if (root->nodeType == CONSTANT || root->nodeType == VALUE_NODE) {
			return root->nodeType;
		}
		bool isConstantExp = true;
		vector<DAGNode*> tempNodes;
		tempNodes.insert(tempNodes.end(), root->inputs.begin(), root->inputs.end());
		tempNodes.insert(tempNodes.end(), root->relyNodes.begin(), root->relyNodes.end());
		for (auto nodeIt = tempNodes.begin(); nodeIt != tempNodes.end(); nodeIt++) {
			int resultType = eliminate_constantExp(*nodeIt, dag);
			if (resultType != CONSTANT) {//如果计算的返回结果不为常数，则说明该表达式不是常量
				isConstantExp = false;
			}
		}

		if (isConstantExp) {
			std::set<string>::iterator it = ava_opSet.find(root->name);
			Value newValue;//节点的新值
			if (it == ava_opSet.end())//不在运算符表中
			{
				return -1;
			}
			else {
				if (root->inputs.size() == 1)
					newValue = folding_calculate_collection(root, root->inputs[0]->value);
				else
					newValue = folding_calculate_collection(root, root->inputs[0]->value, root->inputs[1]->value);
				transform_to_constantnode(dag, root, newValue);
				return CONSTANT;
			}
		}
		return -1;
	}


	//将op_node转换为常量节点
	void transform_to_constantnode(DAG* dag, DAGNode* node, Value nodeValue) {
		node->name = " ";
		node->nodeType = CONSTANT;
		node->value = nodeValue;
		node->isConst = true;

		vector<DAGNode*> tempInputs = node->inputs;
		for (int i = 0; i < tempInputs.size(); i++) {
			dag->remove_edge(node, tempInputs[i]);//移除边
			dag->remove_node(tempInputs[i]);//如果没有与当前常量值相连边的话，删除该常量节点
		}
	}


	void elimanate_UnaryExp(GCFG* gcfg)
	{
		for (const auto& pair : gcfg->controlFlow) {
			for (auto iter = pair.second->basicBlocks.begin(); iter != pair.second->basicBlocks.end(); iter++) {
				if ((*iter)->dag->rootNode != NULL)
					eliminate_UnaryExp((*iter)->dag->rootNode, (*iter)->dag);
			}
		}
	}

	//消除单目运算符表达式的递归函数
	int eliminate_UnaryExp(DAGNode* root, DAG* dag) {
		if (root->relyNodes.size() != 0) {
			for (size_t i = 0; i < root->relyNodes.size(); i++) {
				int result = eliminate_UnaryExp(root->relyNodes[i], dag);
				if (result == EXP_ZHENG) {
					DAGNode* toDelete = root->relyNodes[i];
					vector<DAGNode*> temp = toDelete->relyNodes;

					toDelete->relyNodes.pop_back();
					root->relyNodes[i] = root->relyNodes[i]->inputs[0];
					root->relyNodes[i]->relyNodes = temp;

					dag->remove_node(toDelete);
				}
			}
		}
		if (root->nodeType == DAGNodeType::CONSTANT || root->nodeType == DAGNodeType::LOAD || root->nodeType == DAGNodeType::VALUE_NODE) {
			return EXP_NONE;
		}
		else if (root->inputs.size() == 0) {
			return EXP_NONE;
		}
		else if (root->inputs.size() > 2) {
			for (size_t i = 0; i < root->inputs.size(); i++) {
				int result = eliminate_UnaryExp(root->inputs[i], dag);
				if (result == EXP_ZHENG) {
					DAGNode* toDelete = root->inputs[i];
					root->inputs[i] = toDelete->inputs[0];
					toDelete->inputs[0]->outputs.push_back(root);
					dag->remove_edge(toDelete, toDelete->inputs[0]);
					dag->remove_edge(root, toDelete);
					dag->remove_node(toDelete);
				}
			}
			return EXP_NONE;
		}

		if (root->inputs.size() == 2) {
			int leftResult = eliminate_UnaryExp(root->inputs[0], dag);
			int rightResult = eliminate_UnaryExp(root->inputs[1], dag);

			//双负处理，单负只处理右
			if (rightResult == EXP_FU) {
				if (root->name == PLUS) {
					root->name = SUBTRACT;
					DAGNode* toDelete = root->inputs[1];
					root->inputs[1] = toDelete->inputs[0];
					toDelete->inputs[0]->outputs.push_back(root);
					dag->remove_edge(toDelete, toDelete->inputs[0]);
					dag->remove_edge(root, toDelete);
					dag->remove_node(toDelete);
				}
				else if (root->name == SUBTRACT) {
					root->name = PLUS;
					DAGNode* toDelete = root->inputs[1];
					root->inputs[1] = toDelete->inputs[0];
					toDelete->inputs[0]->outputs.push_back(root);
					dag->remove_edge(toDelete, toDelete->inputs[0]);
					dag->remove_edge(root, toDelete);
					dag->remove_node(toDelete);
				}

				//两边都是负
				if (leftResult == EXP_FU) {
					if (root->name == MULTIPLY || root->name == DIVIDE || root->name == EQUAL || root->name == NOT_EQUAL) {
					}
					else if (root->name == GREATER_THAN) {
						root->name = LESS_THAN;
					}
					else if (root->name == GREATER_EQUAL) {
						root->name = LESS_EQUAL;
					}
					else if (root->name == LESS_THAN) {
						root->name = GREATER_THAN;

					}
					else if (root->name == LESS_EQUAL) {
						root->name = GREATER_EQUAL;
					}
					else if (root->name == REMAINDER) {//****不知道%怎么处理
					}
					DAGNode* leftToDelete = root->inputs[0];
					root->inputs[0] = leftToDelete->inputs[0];
					leftToDelete->inputs[0]->outputs.push_back(root);
					dag->remove_edge(leftToDelete, leftToDelete->inputs[0]);
					dag->remove_edge(root, leftToDelete);
					dag->remove_node(leftToDelete);

					DAGNode* rightToDelete = root->inputs[1];
					root->inputs[1] = rightToDelete->inputs[0];
					rightToDelete->inputs[0]->outputs.push_back(root);
					dag->remove_edge(rightToDelete, rightToDelete->inputs[0]);
					dag->remove_edge(root, rightToDelete);
					dag->remove_node(rightToDelete);
				}
			}

			//处理正号
			if (leftResult == EXP_ZHENG) {
				DAGNode* leftToDelete = root->inputs[0];
				root->inputs[0] = leftToDelete->inputs[0];
				leftToDelete->inputs[0]->outputs.push_back(root);
				dag->remove_edge(leftToDelete, leftToDelete->inputs[0]);
				dag->remove_edge(root, leftToDelete);
				dag->remove_node(leftToDelete);
			}
			if (rightResult == EXP_ZHENG) {
				DAGNode* rightToDelete = root->inputs[1];
				root->inputs[1] = rightToDelete->inputs[0];
				rightToDelete->inputs[0]->outputs.push_back(root);
				dag->remove_edge(root, rightToDelete);
				dag->remove_edge(rightToDelete, rightToDelete->inputs[0]);
				dag->remove_node(rightToDelete);
			}
			return EXP_NONE;
		}

		//接下来处理正和负，但还包含其他的
		if (root->name != PLUS && root->name != SUBTRACT && root->name != LOGICAL_NOT) {
			int result = eliminate_UnaryExp(root->inputs[0], dag);
			if (result == EXP_ZHENG) {
				DAGNode* toDelete = root->inputs[0];
				dag->add_edge(root, toDelete->inputs[0]);
				dag->remove_edge(root, toDelete);
				dag->remove_edge(toDelete, toDelete->inputs[0]);
				dag->remove_node(toDelete);
			}
			return EXP_NONE;
		}

		//只剩下了正和负
		int result = eliminate_UnaryExp(root->inputs[0], dag);
		if (root->name == PLUS) {
			if (result == EXP_FU) {
				root->name = SUBTRACT;
				DAGNode* toDelete = root->inputs[0];
				dag->add_edge(root, toDelete->inputs[0]);
				dag->remove_edge(root, toDelete);
				dag->remove_edge(toDelete, toDelete->inputs[0]);
				dag->remove_node(toDelete);
				return EXP_FU;
			}
			else if (result == EXP_ZHENG) {
				DAGNode* toDelete = root->inputs[0];
				dag->add_edge(root, toDelete->inputs[0]);
				dag->remove_edge(root, toDelete);
				dag->remove_edge(toDelete, toDelete->inputs[0]);
				dag->remove_node(toDelete);
				return EXP_ZHENG;
			}
			return EXP_ZHENG;
		}


		if (root->name == SUBTRACT) {
			if (result == EXP_FU) {
				root->name = PLUS;
				DAGNode* toDelete = root->inputs[0];
				dag->add_edge(root, toDelete->inputs[0]);
				dag->remove_edge(root, toDelete);
				dag->remove_edge(toDelete, toDelete->inputs[0]);
				dag->remove_node(toDelete);
				return EXP_ZHENG;
			}
			else if (result == EXP_ZHENG) {
				DAGNode* toDelete = root->inputs[0];
				dag->add_edge(root, toDelete->inputs[0]);
				dag->remove_edge(root, toDelete);
				dag->remove_edge(toDelete, toDelete->inputs[0]);
				dag->remove_node(toDelete);
				return EXP_FU;
			}
			return EXP_FU;
		}

		if (root->name == LOGICAL_NOT) {
			if (result == EXP_FU || result == EXP_ZHENG) {
				DAGNode* toDelete = root->inputs[0];
				dag->add_edge(root, toDelete->inputs[0]);
				dag->remove_edge(root, toDelete);
				dag->remove_edge(toDelete, toDelete->inputs[0]);
				dag->remove_node(toDelete);
				return EXP_FEI;
			}
			if (result == EXP_FEI) {
				root->name = PLUS;
				DAGNode* toDelete = root->inputs[0];
				dag->add_edge(root, toDelete->inputs[0]);
				dag->remove_edge(root, toDelete);
				dag->remove_edge(toDelete, toDelete->inputs[0]);
				dag->remove_node(toDelete);
				return EXP_ZHENG;
			}

			return EXP_FEI;
		}
		return EXP_NONE;
	}

};