#include<iostream>
#include <string>
#include<algorithm>
#include "../IR/definition.h"
#include <sstream>
using namespace std;

#define DFA_EXP_FUNC "false"//表示表达式中存在函数调用

//可用表达式
class AvaExp {
public:
	//符合交换律的操作符
	set<string> commutative_set = { PLUS,EQUAL,MULTIPLY,NOT_EQUAL,LOGICAL_AND,LOGICAL_OR };
	//所有操作符
	set<string> opSet = { PLUS,SUBTRACT,MULTIPLY,DIVIDE,REMAINDER ,ARRAYSUB_NODE};

	string get_exp_string(DAGNode* node, map<long, string>& reg_exp,SymbolTable* &sym) {
		string temp, operand1, operand2;
		switch (node->nodeType)
		{
		case LOAD://load可能没有接任何东西?
			if (node->inputs.size() > 0 && node->inputs[0]->nodeType == VALUE_NODE) {
				temp = input_to_string(node->inputs[0], reg_exp, sym);
				if (temp != DFA_EXP_FUNC) {
					reg_exp.insert(make_pair(node->resultId, temp));
				}
			}
			else {
				temp = DFA_EXP_FUNC;
			}
			break;
		case OP_NODE:
			if (opSet.find(node->name) != opSet.end()) {

				if (node->inputs.size() == 1) {
					operand1 = input_to_string(node->inputs[0], reg_exp,sym);
					/*if (node->name == SUBTRACT) {
						temp = SINGLE_SUBTRACT;
						temp += " " + operand1;
					}
					else {*/
					temp = node->name + " " + operand1;
					/*}*/
				}
				else if (node->inputs.size() == 2) {
					operand1 = input_to_string(node->inputs[0], reg_exp,sym);
					operand2 = input_to_string(node->inputs[1], reg_exp,sym);
					if (operand1 <= operand2) {
						temp = operand1 + " " + operand2 + " " + node->name; //采用逆波兰表达式
					}
					else {
						temp = operand2 + " " + operand1 + " " + node->name; //采用逆波兰表达式
					}
				}
				if (operand1 == DFA_EXP_FUNC || operand2 == DFA_EXP_FUNC) {
					temp = DFA_EXP_FUNC;
					break;//不处理函数调用以及相关表达式
				} 
				reg_exp.insert(make_pair(node->resultId,temp));
			}
			else {
				temp = DFA_EXP_FUNC;
			}
			break;
		default:
			temp = DFA_EXP_FUNC;
			break;
		}
		return temp;
	}


	void print(string s) {
		//cout << s << endl;
	}

	//将该节点的输入节点转成字符串
	string input_to_string(DAGNode* node, map<long, string>& reg_exp, SymbolTable*& sym) {
		string result;
		switch (node->nodeType)
		{
		case LOAD:
			//if (reg_exp.find(node->resultId) == reg_exp.end()) {
			//	if (node->inputs[0]->nodeType == VALUE_NODE) {
			//		result = node->inputs[0]->name;
			//		reg_exp.insert(make_pair(node->resultId,result));
			//	}
			//	else {//数组
			//		result = DFA_EXP_FUNC;
			//		/*result = "ld" + reg_exp[node->resultId];
			//		reg_exp.insert(make_pair(node->resultId,result));*/
			//	}
			//}
		case REG_NODE:
		case OP_NODE:
			if (reg_exp.find(node->resultId) != reg_exp.end()) {
				result = reg_exp[node->resultId];
			}
			else {
				//print("在reg_exp的map查找这里出错了");
				result = DFA_EXP_FUNC;//遇到函数调用了
			}
			break;
		case CONSTANT:
			if (node->resultType == i32) {
				result = to_string(node->value.iValue);
			}
			else if (node->resultType == f32) {
				result = to_string(node->value.fValue);
			}
			break;
		case VALUE_NODE:
			//非数组的全局变量
			if (node->resultType != iarray && node->resultType != farray && sym->isGlobal(node->symbol)) {
				result = DFA_EXP_FUNC;
			}
			else {
				result = node->name;
			}
			break;
		default:
			break;
		}
		return result;
	}

	bool contain_operand(string exp, string varName) {
		stringstream ss(exp);
		string part;

		// 将表达式分割为操作数和运算符，只保留操作符
		while (ss >> part) {
			if (varName == part) {
				return true;
			}
		}
		return false;
	
	}

};


//enum avaExp_Mode { INT_AVA, FLOAT_AVA, VREG_AVA, EMPTY_AVA };//EMPTY表示
//可用表达式的数据结构
//class AvaExp {
//public:
//	string op;
//	struct operand {
//		int intValue;
//		float floatValue;
//		long longValue;//虚拟寄存器号
//	};
//	avaExp_Mode mode[2];//左右的数据类型
//	operand left;//左操作数
//	operand right;//右操作数
//
//
//	bool operandEqual(avaExp_Mode mode, operand x, operand y) {
//		switch (mode)
//		{
//		case INT_AVA:
//			if (x.intValue == y.intValue) return true;
//			break;
//		case FLOAT_AVA:
//			if (x.floatValue == y.floatValue) return true;
//			break;
//		case VREG_AVA:
//			if (x.longValue == y.longValue) return true;
//			break;
//		case EMPTY_AVA:
//			return true;
//			break;
//		default:
//			break;
//		}
//		return false;
//	}
//
//	//判断两个表达式是否相等
//	bool equal(AvaExp* e1, AvaExp* e2) {
//		if (e1->op != e2->op) return false;
//		bool l = false, r = false;//标志左右操作数是否相同
//		if (e1->op == PLUS || e1->op == MULTIPLY || e1->op == EQUAL || e1->op == NOT_EQUAL || e1->op == LOGICAL_AND || e1->op == LOGICAL_OR || e1->op == LOGICAL_NOT) {//只有单个操作数的
//			if (e1->mode[0] == e2->mode[0] && e1->mode[1] == e2->mode[1]) {
//				l = operandEqual(e1->mode[0], e1->left, e2->left);
//				r = operandEqual(e1->mode[1], e1->right, e2->right);
//			}
//			if (e1->mode[0] == e2->mode[1] && e1->mode[1] == e2->mode[0]) {
//				l = operandEqual(e1->mode[0], e1->left, e2->right);
//				r = operandEqual(e1->mode[1], e1->right, e2->left);
//			}
//		}
//		else {
//			if (e1->mode[0] == e2->mode[0] && e1->mode[1] == e2->mode[1]) {
//				l = operandEqual(e1->mode[0], e1->left, e2->left);
//				r = operandEqual(e1->mode[1], e1->right, e2->right);
//			}
//		}
//		return r&&l;
//	}
//
//	bool containVar(AvaExp* e1, long resultId) {
//		if (e1->mode[0] == VREG_AVA && e1->left.longValue == resultId) return true;
//		if (e1->mode[1] == VREG_AVA && e1->right.longValue == resultId) return true;
//		return false;
//	}
//
//	string toString(AvaExp* e1) {
//		string temp = "";
//		string ls = "";
//		string rs = "";
//		switch (e1->mode[0])
//		{
//		case INT_AVA:
//			ls = "i" + to_string(e1->left.intValue);
//			break;
//		case FLOAT_AVA:
//			ls = "f" + to_string(e1->left.floatValue);
//			break;
//		case VREG_AVA:
//			ls = "%" + to_string(e1->left.longValue);
//			break;
//		default:
//			break;
//		}
//		switch (e1->mode[1])
//		{
//		case INT_AVA:
//			rs = "i" + to_string(e1->right.intValue);
//			break;
//		case FLOAT_AVA:
//			rs = "f" + to_string(e1->right.floatValue);
//			break;
//		case VREG_AVA:
//			rs += "%" + to_string(e1->right.longValue);
//			break;
//		default:
//			break;
//		}
//		temp = ls + " " + op + " " + rs;
//		return temp;
//	}
//};