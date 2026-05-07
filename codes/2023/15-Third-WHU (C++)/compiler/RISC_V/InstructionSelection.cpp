#include "InstructionSelection.h"
#include "Instruction.h"
#include "../IR/definition.h"
#include "Register.h"
#include <cmath>

//bool round_float = false;

//判断是不是常数0，用寄存器0替换
bool isConstZero(DAGNode* dag) {
	if (dag->resultType != f32) {
		if (dag->name == FLOAT_TO_INT) {
			if (dag->inputs[0]->value.fValue == 0) {
				//dag->name = "%zero";
				//dag->nodeType = CONSTANT;
				return true;
			}
		}
		else if (dag->value.iValue == 0) {
			//dag->name = "%zero";
			//dag->nodeType = CONSTANT;
			return true;
		}
	}
	else {
		if (dag->name == INT_TO_FLOAT) {
			if (dag->inputs[0]->value.iValue == 0) {
				//dag->name = "%zero";
				//dag->nodeType = CONSTANT;

				//dag->inputs[0]->name = "%zero";
				//dag->inputs[0]->resultType = i32;
				//dag->inputs[0]->isConst = true;

				//dag->inst = FCVTSW;
				return true;
			}
		}
		else if (dag->value.fValue == 0) {
			//dag->name = "%zero";
			//dag->nodeType = CONSTANT;
			return true;
		}
	}
	return false;
}
 
//添加0寄存器节点
inline DAGNode* addi32Zero(DAGNode* dag) {
	DAGNode* zero = new DAGNode("%zero", CONSTANT);
	zero->isConst = false;
	zero->resultType = i32;
	zero->value.iValue = 0;
	zero->outputs.push_back(dag);
	return zero;
}

inline DAGNode* addf32Zero(DAGNode* dag, CFG* cfg) {
	DAGNode* fmv = new DAGNode("%Quit", OP_NODE);
	fmv->inst = FCVTSW;
	fmv->resultType = f32;
	fmv->inputs.push_back(addi32Zero(fmv));
	fmv->isConst = false;
	fmv->outputs.push_back(dag);
	fmv->resultId = cfg->getCount();
	return fmv;
}


//lui高位地址和数组全局地址等待公共子表达式优化
inline DAGNode* addlui(DAGNode* dag, CFG* cfg) {
	DAGNode* lui = new DAGNode("%Quit", OP_NODE);
	lui->inst = LUI;
	lui->resultType = i32;
	lui->inputs.push_back(dag->inputs[0]);
	lui->isConst = false;
	lui->resultId = cfg->getCount();

	lui->outputs.push_back(dag);
	for (int i = 0; i < dag->inputs[0]->outputs.size(); i++) {
		if (dag->inputs[0]->outputs[i] == dag) dag->inputs[0]->outputs[i] = lui;
	}
	return lui;
}

//判断常数是不是2的倍数
inline bool log2_int(int x) {
	return (x & (x - 1)) == 0;
}

inline bool isGlobalVal(DAGNode* dag, SymbolTable* GlobalSymbolTable) {
	if (dag->nodeType == REG_NODE) return false;
	if(GlobalSymbolTable->isGlobalSymbol(dag->symbol)) return true;
	else return false;
}

inline bool lwsdImm(CFG* cfg, DAGNode* array) {
	//整个栈没有超过最大立即数
	if (4 * (cfg->maxStackSize + 2 * icallee) <= 4096) {
		return true;
	}
	else {
		if (4 * (cfg->arrayStack[array->name].first + cfg->arrayStack[array->name].second + 2 * icallee) <= 4096) {
			return true;
		}
	}

	return false;

}

//指令替换
void instructionReplace(DAGNode* dag, SymbolTable* GlobalSymbolTable, CFG* cfg) {
	if (dag->name == "%Quit" || dag->name == "%zero") {
		for (int i = 0; i < dag->relyNodes.size(); i++) {
			instructionReplace(dag->relyNodes[i], GlobalSymbolTable, cfg);
		}
		return;
	}
	else if (dag->name == "%PHI") {
		for (int i = 0; i < dag->inputs.size(); i++) {
			if(dag->inputs[i]->nodeType == CONSTANT) instructionReplace(dag->inputs[i], GlobalSymbolTable, cfg);
		}
		return;
	}

	switch (dag->nodeType) {
	case OP_NODE:
		//处理负常数情况
		if (dag->inputs.size() > 0) {
			if (dag->inputs[0]->name == SUBTRACT && dag->inputs.size() > 0 && dag->inputs[0]->inputs.size() == 1) {
				if (dag->inputs[0]->isConst) {
					dag->inputs[0] = dag->inputs[0]->inputs[0];
					if (dag->inputs[0]->resultType == i32) dag->inputs[0]->value.iValue = -dag->inputs[0]->value.iValue;
					else dag->inputs[0]->value.fValue = -dag->inputs[0]->value.fValue;
				}
			}
		}

		//输入是-a+b类型，转换成b-a
		if (dag->name == PLUS && dag->inputs[0]->name == SUBTRACT && dag->inputs.size() > 0 && dag->inputs[0]->inputs.size() == 1) {
			DAGNode* swap = dag->inputs[0];
			dag->inputs[0] = dag->inputs[1];
			dag->inputs[1] = swap->inputs[0];
			dag->name = SUBTRACT;
		}


		//"%ret"
		if (dag->name == RETURN_NODE) {
			dag->inst = RET;

			//if (dag->inputs.size() != 0) {
			//	dag->inputs[0]->allocate.ret = true;
			//}
		}
		//局部数组定义
		else if (dag->name == ARRAY_INIT_NODE) {
			break;
		}
		//数组使用
		else if (dag->name == ARRAYSUB_NODE) {
			if (isGlobalVal(dag->inputs[0], GlobalSymbolTable)) {
				dag->inputs[0] = addlui(dag, cfg);
				
				DAGNode* addi = new DAGNode("%Quit", OP_NODE);
				addi->inst = ADDI;
				addi->resultType = ipoint;
				addi->resultId = cfg->getCount();
				addi->inputs.push_back(dag->inputs[0]);
				addi->outputs.push_back(dag);
				for (int i = 0; i < dag->inputs[0]->outputs.size(); i++) {
					if (dag->inputs[0]->outputs[i] == dag) dag->inputs[0]->outputs[i] = addi;
				}
				dag->inputs[0] = addi;
			}
			else {
				if (dag->inputs[0]->resultType != ipoint && dag->inputs[0]->resultType != fpoint) {
					DAGNode* add = new DAGNode("%Quit", OP_NODE);
					add->inst = ADD;
					add->resultId = cfg->getCount();
					if (dag->resultType == ipoint) add->resultType = ipoint;
					else if (dag->resultType == fpoint) add->resultType = fpoint;
					add->inputs.push_back(dag->inputs[0]);
					add->outputs.push_back(dag);
					dag->inputs[0] = add;
					for (int i = 0; i < add->inputs[0]->outputs.size(); i++) {
						if (dag == add->inputs[0]->outputs[i]) add->inputs[0]->outputs[i] = add;

					}
				}
			}
			dag->inst = MV;

			if (dag->inputs.size() == 2) {
				dag->inst = ADD;
				DAGNode* slli = new DAGNode("%Quit", OP_NODE);
				slli->inst = SLLI;
				slli->value.iValue = 2;
				slli->resultType = i32;
				slli->resultId = cfg->getCount();
				slli->inputs.push_back(dag->inputs[1]);
				for (int i = 0; i < dag->inputs[1]->outputs.size(); i++) {
					if (dag->inputs[1]->outputs[i] == dag) dag->inputs[1]->outputs[i] = slli;
				}
				slli->outputs.push_back(dag);
				dag->inputs[1] = slli;

				for (int i = 0; i < dag->inputs[1]->inputs.size(); i++) {
					instructionReplace(dag->inputs[1]->inputs[i], GlobalSymbolTable, cfg);
				}
			}
		}
		//"%branch"
		else if (dag->name == BRANCH_NODE) {
			//无条件跳转
			if (dag->inputs.size() == 0) {
				dag->inst = J;
			}
			else if (dag->inputs[0]->name == SUBTRACT && dag->inputs[0]->inputs.size() == 1) {
				//-a类型输入
				dag->inst = BNEZ;
			}
			else if (dag->inputs[0]->name == LOGICAL_NOT) {
				dag->inst = BNEZ;
			}
			//条件表达式
// wait for optimize, 和常数比可以优化成先寄存器加/减然后和0比较
			//'>'
			else if (dag->inputs[0]->name == GREATER_THAN) {
				if (dag->inputs[0]->inputs[0]->resultType == i32) {
					//整数情况下删除操作节点
					dag->inputs.push_back(dag->inputs[0]->inputs[1]);
					for (int i = 0; i < dag->inputs[1]->outputs.size(); i++) {
						if (dag->inputs[1]->outputs[i] == dag->inputs[0]) {
							dag->inputs[1]->outputs[i] = dag;
						}
					}
					for (int i = 0; i < dag->inputs[0]->inputs[0]->outputs.size(); i++) {
						if (dag->inputs[0]->inputs[0]->outputs[i] == dag->inputs[0]) {
							dag->inputs[0]->inputs[0]->outputs[i] = dag;
						}
					}
					dag->inputs[0] = dag->inputs[0]->inputs[0];
					dag->inst = BGT;
					for (int i = 0; i < dag->inputs.size(); i++) {
						if (dag->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[i])) {
								if (i == 0) {
									dag->inst = BLTZ;
									dag->inputs[0] = dag->inputs[1];
								}
								else dag->inst = BGTZ;
							}
						}
					}
				}
				else if(dag->inputs[0]->inputs[0]->resultType == f32){
					dag->inst = BNEZ;
					dag->inputs[0]->inst = FGTS;
					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						if (dag->inputs[0]->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[0]->inputs[i])) {
								dag->inputs[0]->inputs[i] = addf32Zero(dag->inputs[0], cfg);
							}
						}
					}

					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						instructionReplace(dag->inputs[0]->inputs[i], GlobalSymbolTable, cfg);
					}

					for (int i = 0; i < dag->relyNodes.size(); i++) {
						instructionReplace(dag->relyNodes[i], GlobalSymbolTable, cfg);
					}
					return;
				}
			}
			//'>='
			else if (dag->inputs[0]->name == GREATER_EQUAL) {
				if (dag->inputs[0]->inputs[0]->resultType == i32) {
					//整数情况下删除操作节点
					dag->inputs.push_back(dag->inputs[0]->inputs[1]);
					for (int i = 0; i < dag->inputs[1]->outputs.size(); i++) {
						if (dag->inputs[1]->outputs[i] == dag->inputs[0]) {
							dag->inputs[1]->outputs[i] = dag;
						}
					}
					for (int i = 0; i < dag->inputs[0]->inputs[0]->outputs.size(); i++) {
						if (dag->inputs[0]->inputs[0]->outputs[i] == dag->inputs[0]) {
							dag->inputs[0]->inputs[0]->outputs[i] = dag;
						}
					}
					dag->inputs[0] = dag->inputs[0]->inputs[0];
					dag->inst = BGE;
					for (int i = 0; i < dag->inputs.size(); i++) {
						if (dag->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[i])) {
								if (i == 0) {
									dag->inst = BLEZ;
									dag->inputs[0] = dag->inputs[1];
								}
								else dag->inst = BGEZ;
							}
						}
					}
				}
				else if (dag->inputs[0]->inputs[0]->resultType == f32) {
					dag->inst = BNEZ;
					dag->inputs[0]->inst = FGES;
					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						if (dag->inputs[0]->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[0]->inputs[i])) {
								dag->inputs[0]->inputs[i] = addf32Zero(dag->inputs[0], cfg);
							}
						}
					}

					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						instructionReplace(dag->inputs[0]->inputs[i], GlobalSymbolTable, cfg);
					}

					for (int i = 0; i < dag->relyNodes.size(); i++) {
						instructionReplace(dag->relyNodes[i], GlobalSymbolTable, cfg);
					}
					return;
				}
			}
			//'<'
			else if (dag->inputs[0]->name == LESS_THAN) {
				if (dag->inputs[0]->inputs[0]->resultType == i32) {
					//整数情况下删除操作节点
					dag->inputs.push_back(dag->inputs[0]->inputs[1]);
					for (int i = 0; i < dag->inputs[1]->outputs.size(); i++) {
						if (dag->inputs[1]->outputs[i] == dag->inputs[0]) {
							dag->inputs[1]->outputs[i] = dag;
						}
					}
					for (int i = 0; i < dag->inputs[0]->inputs[0]->outputs.size(); i++) {
						if (dag->inputs[0]->inputs[0]->outputs[i] == dag->inputs[0]) {
							dag->inputs[0]->inputs[0]->outputs[i] = dag;
						}
					}
					dag->inputs[0] = dag->inputs[0]->inputs[0];
					dag->inst = BLT;
					for (int i = 0; i < dag->inputs.size(); i++) {
						if (dag->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[i])) {
								if (i == 0) {
									dag->inst = BGTZ;
									dag->inputs[0] = dag->inputs[1];
								}
								else dag->inst = BLTZ;
							}
						}
					}
				}
				else if (dag->inputs[0]->inputs[0]->resultType == f32) {
					dag->inst = BNEZ;
					dag->inputs[0]->inst = FLTS;
					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						if (dag->inputs[0]->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[0]->inputs[i])) {
								dag->inputs[0]->inputs[i] = addf32Zero(dag->inputs[0], cfg);
							}
						}
					}

					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						instructionReplace(dag->inputs[0]->inputs[i], GlobalSymbolTable, cfg);
					}

					for (int i = 0; i < dag->relyNodes.size(); i++) {
						instructionReplace(dag->relyNodes[i], GlobalSymbolTable, cfg);
					}
					return;
				}
			}
			//'<='
			else if (dag->inputs[0]->name == LESS_EQUAL) {
				if (dag->inputs[0]->inputs[0]->resultType == i32) {
					//整数情况下删除操作节点
					dag->inputs.push_back(dag->inputs[0]->inputs[1]);
					for (int i = 0; i < dag->inputs[1]->outputs.size(); i++) {
						if (dag->inputs[1]->outputs[i] == dag->inputs[0]) {
							dag->inputs[1]->outputs[i] = dag;
						}
					}
					for (int i = 0; i < dag->inputs[0]->inputs[0]->outputs.size(); i++) {
						if (dag->inputs[0]->inputs[0]->outputs[i] == dag->inputs[0]) {
							dag->inputs[0]->inputs[0]->outputs[i] = dag;
						}
					}
					dag->inputs[0] = dag->inputs[0]->inputs[0];
					dag->inst = BLE;
					for (int i = 0; i < dag->inputs.size(); i++) {
						if (dag->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[i])) {
								if (i == 0) {
									dag->inst = BGEZ;
									dag->inputs[0] = dag->inputs[1];
								}
								else dag->inst = BLEZ;
							}
						}
					}
				}
				else if (dag->inputs[0]->inputs[0]->resultType == f32) {
					dag->inst = BNEZ;
					dag->inputs[0]->inst = FLES;
					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						if (dag->inputs[0]->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[0]->inputs[i])) {
								dag->inputs[0]->inputs[i] = addf32Zero(dag->inputs[0], cfg);
							}
						}
					}
					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						instructionReplace(dag->inputs[0]->inputs[i], GlobalSymbolTable, cfg);
					}

					for (int i = 0; i < dag->relyNodes.size(); i++) {
						instructionReplace(dag->relyNodes[i], GlobalSymbolTable, cfg);
					}
					return;

				}
			}
			//'=='
			else if (dag->inputs[0]->name == EQUAL) {
				if (dag->inputs[0]->inputs[0]->resultType == i32) {
					//整数情况下删除操作节点
					dag->inputs.push_back(dag->inputs[0]->inputs[1]);
					for (int i = 0; i < dag->inputs[1]->outputs.size(); i++) {
						if (dag->inputs[1]->outputs[i] == dag->inputs[0]) {
							dag->inputs[1]->outputs[i] = dag;
						}
					}
					for (int i = 0; i < dag->inputs[0]->inputs[0]->outputs.size(); i++) {
						if (dag->inputs[0]->inputs[0]->outputs[i] == dag->inputs[0]) {
							dag->inputs[0]->inputs[0]->outputs[i] = dag;
						}
					}
					dag->inputs[0] = dag->inputs[0]->inputs[0];
					dag->inst = BEQ;
					for (int i = 0; i < dag->inputs.size(); i++) {
						if (dag->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[i])) {
								dag->inst = BEQZ;
								if (i == 0) {
									dag->inputs[0] = dag->inputs[1];
								}
								dag->inputs.pop_back();
							}
						}
					}
				}
				else if (dag->inputs[0]->inputs[0]->resultType == f32) {
					dag->inst = BNEZ;
					dag->inputs[0]->inst = FEQS;
					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						if (dag->inputs[0]->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[0]->inputs[i])) {
								dag->inputs[0]->inputs[i] = addf32Zero(dag->inputs[0], cfg);
							}
						}
					}

					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						instructionReplace(dag->inputs[0]->inputs[i], GlobalSymbolTable, cfg);
					}

					for (int i = 0; i < dag->relyNodes.size(); i++) {
						instructionReplace(dag->relyNodes[i], GlobalSymbolTable, cfg);
					}
					return;
				}
			}
			//'!='
			else if (dag->inputs[0]->name == NOT_EQUAL) {
				if (dag->inputs[0]->inputs[0]->resultType == i32) {
					//整数情况下删除操作节点
					dag->inputs.push_back(dag->inputs[0]->inputs[1]);
					for (int i = 0; i < dag->inputs[1]->outputs.size(); i++) {
						if (dag->inputs[1]->outputs[i] == dag->inputs[0]) {
							dag->inputs[1]->outputs[i] = dag;
						}
					}
					for (int i = 0; i < dag->inputs[0]->inputs[0]->outputs.size(); i++) {
						if (dag->inputs[0]->inputs[0]->outputs[i] == dag->inputs[0]) {
							dag->inputs[0]->inputs[0]->outputs[i] = dag;
						}
					}
					dag->inputs[0] = dag->inputs[0]->inputs[0];
					dag->inst = BNE;
					for (int i = 0; i < dag->inputs.size(); i++) {
						if (dag->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[i])) {
								dag->inst = BNEZ;
								if (i == 0) {
									dag->inputs[0] = dag->inputs[1];
								}
							}
						}
					}
				}
				else if (dag->inputs[0]->inputs[0]->resultType == f32) {
					dag->inst = BEQZ;
					dag->inputs[0]->inst = FEQS;
					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						if (dag->inputs[0]->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[0]->inputs[i])) {
								dag->inputs[0]->inputs[i] = addf32Zero(dag->inputs[0], cfg);
							}
						}
					}
					for (int i = 0; i < dag->inputs[0]->inputs.size(); i++) {
						instructionReplace(dag->inputs[0]->inputs[i], GlobalSymbolTable, cfg);
					}

					for (int i = 0; i < dag->relyNodes.size(); i++) {
						instructionReplace(dag->relyNodes[i], GlobalSymbolTable, cfg);
					}
					return;
				}
			}
			//常量节点,理论上不会出现
			else  if(dag->inputs[0]->isConst){
				//常数默认直接跳转
				dag->inst = J;
				//常数假跳转否节点，将目标节点调换
				if ((dag->inputs[0]->resultType == i32 && dag->inputs[0]->value.iValue == 0)||
					(dag->inputs[0]->resultType == i32 && dag->inputs[0]->value.fValue == 0)) {
					int temp = dag->targets[0];
					dag->targets[0] = dag->targets[1];
					dag->targets[1] = temp;
				}
			}
			else {
				//变量输入
				dag->inst = BNEZ;
			}
		}
		//intTofloat
		else if (dag->name == INT_TO_FLOAT) {
			dag->inst = FCVTSW;
			if (dag->inputs[0]->isConst) {
				if (isConstZero(dag->inputs[0])) {
					dag->inst = FMVS;
					dag->inputs[0] = addi32Zero(dag);
				}
				//DAGNode* loadnode = new DAGNode("%Quit", OP_NODE);
				//loadnode->inst = LI;
				//loadnode->resultType = i32;
				//loadnode->inputs.push_back(dag->inputs[0]);
				//dag->inputs[0] = loadnode;
			}
		}
		//floatToint
		else if (dag->name == FLOAT_TO_INT) {
			dag->inst = FCVTWS;
			//round_float = true;
			if (dag->inputs[0]->isConst) {
				if (isConstZero(dag->inputs[0])) {
					dag->isConst = false;
					dag->nodeType = CONSTANT;
					dag->name = "%zero";
					dag->resultType = i32;
					dag->value.iValue = 0;
					dag->inst = null;
					dag->inputs.clear();
				}
				else {
					dag->inst = LI;
					dag->value.iValue = (int)dag->inputs[0]->value.fValue;
					//dag->resultType = i32;
				}
			}
		}
		//'+'
		else if (dag->name == PLUS) {
			//+a和+常数类型
			if (dag->inputs.size() == 1) {
				if (dag->inputs[0]->isConst) {
					if (dag->resultType != f32) {
						DAGNode* newnode = new DAGNode("%Quit", OP_NODE);
						newnode->resultType = i32;
						newnode->inst = LI;
						newnode->value.iValue = dag->inputs[0]->value.iValue;
						newnode->resultId = cfg->getCount();

						newnode->outputs = dag->outputs;
						*dag = *newnode;
						for (int i = 0; i < dag->inputs.size(); i++) {
							for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
								if (newnode == dag->inputs[i]->outputs[j]) {
									dag->inputs[i]->outputs[j] = dag;
								}
							}
						}
					}
					else if (dag->resultType == f32) {
						DAGNode* loadnode = new DAGNode("%Quit", OP_NODE);
						loadnode->inst = LI;
						loadnode->resultType = i32;
						loadnode->value.iValue = f32Toi32(dag->inputs[0]->value.fValue);
						loadnode->resultId = cfg->getCount();

						DAGNode* fmv = new DAGNode("%Quit", OP_NODE);
						fmv->resultType = f32;
						fmv->inst = FMVWX;
						fmv->inputs.push_back(loadnode);
						fmv->resultId = cfg->getCount();

						loadnode->outputs.push_back(fmv);
						fmv->outputs = dag->outputs;

						*dag = *fmv;

						for (int i = 0; i < dag->inputs.size(); i++) {
							for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
								if (fmv == dag->inputs[i]->outputs[j]) {
									dag->inputs[i]->outputs[j] = dag;
								}
							}
						}
					}
				}
			}

			//两个操作数均为寄存器类型
			if (!dag->inputs[0]->isConst && !dag->inputs[1]->isConst) {
				if (dag->resultType == i32) dag->inst = ADDW;
				else if (dag->resultType == f32) dag->inst = FADDS;
			}
			//两个操作数均为常数,在数据流分析之后不会出现这种情况
			else if (dag->inputs[0]->isConst && dag->inputs[1]->isConst) {
				if (dag->inputs[0]->resultType == i32 && dag->inputs[0]->resultType == i32) {
					dag->inst = ADDW;
				}
				else if (dag->inputs[0]->resultType == f32 || dag->inputs[0]->resultType == f32)dag->inst = FADDS;

				dag->isConst = false;
			}
			//两个操作数其中一个为常数
			else {
				if (dag->resultType == i32) {
					dag->inst = ADDIW;
					for (int i = 0; i < dag->inputs.size(); i++) {
						if (dag->inputs[i]->isConst ) {
							//加0
							if (isConstZero(dag->inputs[i])) {
								DAGNode* rely;
								if (i == 0) {
									rely = dag->inputs[1];
									*dag = *dag->inputs[1];
								}
								else {
									rely = dag->inputs[0];
									*dag = *dag->inputs[0];
								}

								*dag = *rely;
								for (int i = 0; i < dag->inputs.size(); i++) {
									for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
										if (rely == dag->inputs[i]->outputs[j]) {
											dag->inputs[i]->outputs[j] = dag;
										}
									}
								}
								instructionReplace(dag, GlobalSymbolTable, cfg);
								return;
							}
							else if (isLegalImm(IType, dag->inputs[i]->value.iValue)) {
								dag->value.iValue = dag->inputs[i]->value.iValue;
								if (i == 0) dag->inputs[0] = dag->inputs[1];
								dag->inputs.pop_back();
							}
							else dag->inst = ADDW;
						}
					}
				}
				else if (dag->resultType == f32) {
					dag->inst = FADDS;
					//for (int i = 0; i < dag->inputs.size(); i++) {
					//	if (dag->inputs[i]->isConst) {
					//		//加0
					//		if (isConstZero(dag->inputs[i])) {
					//			DAGNode* rely;
					//			if (i == 0) {
					//				rely = dag->inputs[1];
					//				*dag = *dag->inputs[1];
					//			}
					//			else {
					//				rely = dag->inputs[0];
					//				*dag = *dag->inputs[0];
					//			}

					//			*dag = *rely;
					//			for (int i = 0; i < dag->inputs.size(); i++) {
					//				for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
					//					if (rely == dag->inputs[i]->outputs[j]) {
					//						dag->inputs[i]->outputs[j] = dag;
					//					}
					//				}
					//			}
					//			instructionReplace(dag, GlobalSymbolTable, cfg);
					//			return;
					//		}
					//	}
					//}
				}
			}
		}
		//'-'
		else if (dag->name == SUBTRACT) {
			//不考虑常数，常数在入口处处理了
			//将-a转换成0-a
			if (dag->inputs.size() == 1) {
				if (dag->inputs[0]->resultType == i32) {
					dag->resultType = i32;
					dag->inst = SUBW;
					dag->inputs.push_back(dag->inputs[0]);
					dag->inputs[0] = addi32Zero(dag);
				}
				else if (dag->inputs[0]->resultType == f32) {
					dag->resultType = f32;
					dag->inst = FSUBS;
					dag->inputs.push_back(dag->inputs[0]);
					dag->inputs[0] = addf32Zero(dag, cfg);
				}
			}
			//两个操作数均为寄存器类型
			else if (!dag->inputs[0]->isConst && !dag->inputs[1]->isConst) {
				if (dag->resultType == i32) dag->inst = SUBW;
				else if (dag->resultType == f32) dag->inst = FSUBS;
			}
			//两个操作数均为常数,在数据流分析之后不会出现这种情况
			else if (dag->inputs[0]->isConst && dag->inputs[1]->isConst) {
				if (dag->inputs[0]->resultType == i32 && dag->inputs[0]->resultType == i32) {
					dag->inst = SUBW;
				}
				else if (dag->inputs[0]->resultType == f32 || dag->inputs[0]->resultType == f32)dag->inst = FSUBS;
			}
			//两个操作数其中一个为常数
			else {
				if (dag->resultType == i32) {
					dag->inst = SUBW;
					if (dag->inputs[1]->isConst) {
						//减0
						if (isConstZero(dag->inputs[1])) {
							dag->inputs[1] = addi32Zero(dag);
							dag->inst = SUBW;
								/*else {
									DAGNode* rely = dag->inputs[0];
									*dag = *rely;
									for (int i = 0; i < dag->inputs.size(); i++) {
										for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
											if (rely == dag->inputs[i]->outputs[j]) {
												dag->inputs[i]->outputs[j] = dag;
											}
										}
									}
									instructionReplace(dag, GlobalSymbolTable, cfg);
									return;
								}*/
						}
						else if (isLegalImm(IType, -dag->inputs[1]->value.iValue)) {
							dag->value.iValue = -dag->inputs[1]->value.iValue;//减立即数等于加立即数相反数
							dag->inputs.pop_back();
							dag->inst = ADDIW;
						}
					}
				}
				else if (dag->resultType == f32) {
					dag->inst = FSUBS;
					for (int i = 0; i < dag->inputs.size(); i++) {
						if (dag->inputs[i]->isConst) {
							if (isConstZero(dag->inputs[i])) {
								dag->inputs[i] = addf32Zero(dag, cfg);
								//dag->inst = SUBW;
							}
							//else {
							//	DAGNode* rely = dag->inputs[0];
							//	*dag = *rely;
							//	for (int i = 0; i < dag->inputs.size(); i++) {
							//		for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
							//			if (rely == dag->inputs[i]->outputs[j]) {
							//				dag->inputs[i]->outputs[j] = dag;
							//			}
							//		}
							//	}
							//	instructionReplace(dag, GlobalSymbolTable, cfg);
							//	return;
							//}
						}
					}
				}
			}
		}
		//'*'
		else if (dag->name == MULTIPLY) {
			if (dag->resultType == i32) dag->inst = MULW;
			else if (dag->resultType == f32) dag->inst = FMULS;
			//两个操作数其中一个为常数
			for (int i = 0; i < dag->inputs.size(); i++) {
				if (dag->inputs[i]->isConst) {
					if (dag->resultType != f32) {
						//乘0
						if (isConstZero(dag->inputs[i])) {
							dag->name = "%zero";
							dag->nodeType = CONSTANT;
						}
						//waitForOptimize 
						else {
							//乘负常数
							if (dag->inputs[i]->value.iValue < 0) {
								if (log2_int(-dag->inputs[i]->value.iValue)) {
									dag->inst = NEGW;
									DAGNode* slli = new DAGNode("%Quit", OP_NODE);
									slli->resultType = i32;
									slli->inst = SLLIW;
									slli->resultId = cfg->getCount();
									if (i == 0) slli->inputs.push_back(dag->inputs[1]);
									else if (i == 1) slli->inputs.push_back(dag->inputs[0]);
									slli->value.iValue = (int)log2(-dag->inputs[i]->value.iValue);

									dag->inputs.pop_back();
									dag->inputs[0] = slli;
									slli->outputs.push_back(dag);

									instructionReplace(slli->inputs[0], GlobalSymbolTable, cfg);
								}
							}
							//乘正常数
							else {
								if (log2_int(dag->inputs[i]->value.iValue)) {
									if (dag->inputs[i]->value.iValue == 1) {
										dag->inst = MV;
										if (i == 0) dag->inputs[0] = dag->inputs[1];
										dag->inputs.pop_back();
									}
									else {
										dag->inst = SLLIW;
										dag->value.iValue = (int)log2(dag->inputs[i]->value.iValue);

										if (i == 0) dag->inputs[0] = dag->inputs[1];
										dag->inputs.pop_back();
									}
								}
							}
						}
					}
					//乘浮点0
					else if (dag->resultType == f32) {
						//if (isConstZero(dag->inputs[i])) {
						//	*dag = *addf32Zero(dag->outputs[0], cfg);//不知会不会有问题
						//	//dag->nodeType = CONSTANT;
						//}
					}
				}
			}
		}
		//'/'
		else if (dag->name == DIVIDE){
			if (dag->resultType == i32) {
				dag->inst = DIVW;
				/*
				//两个操作数其中一个为常数
				for (int i = 0; i < dag->inputs.size(); i++) {
					if (dag->inputs[i]->isConst) {
						//除负常数
						if (dag->inputs[i]->value.iValue < 0) {
							if (log2_int(-dag->inputs[i]->value.iValue)) {
								dag->inst = NEGW;
								DAGNode* srai = new DAGNode("%Quit", OP_NODE);
								srai->resultType = i32;
								srai->inst = SRAIW;
								srai->resultId = cfg->getCount();
								if (i == 0) srai->inputs.push_back(dag->inputs[1]);
								else if (i == 1) srai->inputs.push_back(dag->inputs[0]);
								srai->value.iValue = (int)log2(-dag->inputs[i]->value.iValue);

								dag->inputs.pop_back();
								dag->inputs[0] = srai;
								srai->outputs.push_back(dag);

								instructionReplace(srai->inputs[0], GlobalSymbolTable, cfg);
							}
						}
						//除正常数
						else {
							if (log2_int(dag->inputs[i]->value.iValue)) {
								dag->inst = SRAIW;
								dag->value.iValue = (int)log2(dag->inputs[i]->value.iValue);

								if (i == 0) dag->inputs[0] = dag->inputs[1];
								dag->inputs.pop_back();
							}
						}

					}
				}
				*/
			}
			else if (dag->resultType == f32) dag->inst = FDIVS;
		}
		//'%'
		else if (dag->name == REMAINDER) {
			dag->inst = REMW;

		}
		//'!'
		else if (dag->name == LOGICAL_NOT) {
			if (dag->inputs[0]->resultType != f32) {
				dag->inst = SEQZ;
				dag->resultType = i32;
			}
			else {
				dag->inst = FEQS;
				dag->inputs.push_back(addf32Zero(dag, cfg));
			}
		}
		//'>'
		else if (dag->name == GREATER_THAN) {
			dag->resultType = i32;
			if (dag->inputs[0]->resultType != f32) {
				dag->inst = SGT;

			}
			else {
				dag->inst = FGTS;
			}
		}
		//'>='
		else if (dag->name == GREATER_EQUAL) {
			dag->resultType = i32;
			if (dag->inputs[0]->resultType != f32) {
				dag->inst = SLT;

				DAGNode* seqz = new DAGNode("%Quit", OP_NODE);
				seqz->inst = SEQZ;
				seqz->resultId = cfg->getCount();
				seqz->inputs.push_back(dag);
				seqz->outputs = dag->outputs;

				dag->outputs.clear();
				dag->outputs.push_back(seqz);
				for (int i = 0; i < seqz->outputs[0]->inputs.size(); i++) {
					if (dag == seqz->outputs[0]->inputs[i]) seqz->outputs[0]->inputs[i] = seqz;
				}
			}
			else {
				dag->inst = FGES;
			}
		}
		//'<'
		else if (dag->name == LESS_THAN) {
			dag->resultType = i32;
			if (dag->inputs[0]->resultType != f32) {
				dag->inst = SLT;

			}
			else {
				dag->inst = FGTS;
				DAGNode* swap = dag->inputs[0];
				dag->inputs[0] = dag->inputs[1];
				dag->inputs[1] = swap;
			}
		}
		//'<='
		else if (dag->name == LESS_EQUAL) {
			dag->resultType = i32;
			if (dag->inputs[0]->resultType != f32) {
				dag->inst = SGT;

				DAGNode* seqz = new DAGNode("%Quit", OP_NODE);
				seqz->inst = SEQZ;
				seqz->resultId = cfg->getCount();
				seqz->inputs.push_back(dag);
				seqz->outputs = dag->outputs;

				dag->outputs.clear();
				dag->outputs.push_back(seqz);
				for (int i = 0; i < seqz->outputs[0]->inputs.size(); i++) {
					if (dag == seqz->outputs[0]->inputs[i]) seqz->outputs[0]->inputs[i] = seqz;
				}
			}
			else {
				dag->inst = FGES;
				DAGNode* swap = dag->inputs[0];
				dag->inputs[0] = dag->inputs[1];
				dag->inputs[1] = swap;
			}
		}
		//'=='
		else if (dag->name == EQUAL) {
			dag->resultType = i32;
			if (dag->inputs[0]->resultType != f32) {
				dag->inst = SUBW;

				DAGNode* seqz = new DAGNode("%Quit", OP_NODE);
				seqz->inst = SEQZ;
				seqz->resultId = cfg->getCount();
				seqz->inputs.push_back(dag);
				seqz->outputs = dag->outputs;

				dag->outputs.clear();
				dag->outputs.push_back(seqz);
				for (int i = 0; i < seqz->outputs[0]->inputs.size(); i++) {
					if (dag == seqz->outputs[0]->inputs[i]) seqz->outputs[0]->inputs[i] = seqz;
				}
			}
			else {
				dag->inst = FEQS;
				for (int i = 0; i < dag->inputs.size(); i++) {
					if (dag->inputs[i]->isConst) {
						if (isConstZero(dag->inputs[i])) {
							dag->inputs[i] = addf32Zero(dag, cfg);
						}
					}
				}
			}
		}
		//'!='
		else if (dag->name == NOT_EQUAL) {
			dag->resultType = i32;
			if (dag->inputs[0]->resultType != f32) {
				dag->inst = SUBW;

				DAGNode* snez = new DAGNode("%Quit", OP_NODE);
				snez->resultType = i32;
				snez->inst = SNEZ;
				snez->resultId = cfg->getCount();
				snez->inputs.push_back(dag);
				snez->outputs = dag->outputs;

				dag->outputs.clear();
				dag->outputs.push_back(snez);
				for (int i = 0; i < snez->outputs[0]->inputs.size(); i++) {
					if (dag == snez->outputs[0]->inputs[i]) snez->outputs[0]->inputs[i] = snez;
				}
			}
			else {
				dag->inst = FEQS;

				DAGNode* snez = new DAGNode("%Quit", OP_NODE);
				snez->resultType = i32;
				snez->inst = SNEZ;
				snez->resultId = cfg->getCount();
				snez->inputs.push_back(dag);
				snez->outputs = dag->outputs;
				dag->outputs.clear();
				dag->outputs.push_back(snez);

				for (int i = 0; i < snez->outputs[0]->inputs.size(); i++) {
					if (dag == snez->outputs[0]->inputs[i]) snez->outputs[0]->inputs[i] = snez;
				}
			}
		
		}

		//函数调用
		else if (dag->name != " ") {
			dag->inst = CALL;
			int count_I = 0;
			int count_F = 0;
			int stack = 0;
			for (int i = 0; i < dag->inputs.size(); i++) {
				if (dag->inputs[i]->resultType == iarray || dag->inputs[i]->resultType == farray) {
					DAGNode* add = new DAGNode("%Quit", OP_NODE);
					add->inst = ADD;
					add->resultId = cfg->getCount();
					if (dag->inputs[i]->resultType == iarray) add->resultType = ipoint;
					else if (dag->inputs[i]->resultType == farray) add->resultType = fpoint;
					add->inputs.push_back(dag->inputs[i]);
					add->outputs.push_back(dag);
					dag->inputs[i] = add;
					for (int j = 0; j < add->inputs[0]->outputs.size(); j++) {
						if (dag->inputs[i] == add->inputs[0]->outputs[j]) add->inputs[0]->outputs[j] = add;
					}
				}

				if (dag->inputs[i]->resultType != f32) {
					count_I++;
					if (count_I > 8) {
						DAGNode* sw = new DAGNode("%Quit", STORE);
						sw->resultType = i32;
						sw->inst = SD;
						sw->inputs.push_back(dag->inputs[i]);
						sw->outputs.push_back(dag);
			
						for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
							if (dag->inputs[i]->outputs[j] == dag) {
								dag->inputs[i]->outputs[j] = sw;
								break;
							}
						}
						dag->inputs[i] = sw;

						if (4 * stack >= 2048) {
							DAGNode* li = new DAGNode("%Quit", OP_NODE);
							li->inst = LI;
							li->resultType = i32;
							li->value.iValue = 4 * stack;
							li->resultId = cfg->getCount();

							sw->inputs.push_back(sw->inputs[0]);
							sw->inputs[0] = li;
							li->outputs.push_back(sw);
						}
						else {
							sw->value.iValue = stack;
						}
						stack += 2;
						cfg->funcStackSize += 2;

						for (int j = 0; j < dag->inputs[i]->inputs.size(); j++) {
							instructionReplace(dag->inputs[i]->inputs[j], GlobalSymbolTable, cfg);
						}
					}
				}
			}
			for (int i = 0; i < dag->inputs.size(); i++) {
				if (dag->inputs[i]->resultType == f32) {
					count_F++;
					if (count_F > 8) {
						DAGNode* sw = new DAGNode("%Quit", STORE);
						sw->resultType = f32;
						sw->inst = FSD;
						sw->inputs.push_back(dag->inputs[i]);
						sw->outputs.push_back(dag);

						for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
							if (dag->inputs[i]->outputs[j] == dag) {
								dag->inputs[i]->outputs[j] = sw;
								break;
							}
						}
						dag->inputs[i] = sw;

						if (4 * stack >= 2048) {
							DAGNode* li = new DAGNode("%Quit", OP_NODE);
							li->inst = LI;
							li->resultType = i32;
							li->value.iValue = 4 * stack;
							li->resultId = cfg->getCount();

							sw->inputs.push_back(sw->inputs[0]);
							sw->inputs[0] = li;
							li->outputs.push_back(sw);
						}
						else {
							sw->value.iValue = stack;
						}
						stack += 2;
						cfg->funcStackSize += 2;

						for (int j = 0; j < dag->inputs[i]->inputs.size(); j++) {
							instructionReplace(dag->inputs[i]->inputs[j], GlobalSymbolTable, cfg);
						}
					}
				}
			}
		}
		break;
	case CONSTANT:
		//常数转换成LI指令
		//将常数的数值存在LI指令DAG的value中
		if (dag->resultType != f32) {
			for (int i = 0; i < dag->outputs.size(); i++) {
				if (dag->outputs[i]->name == ARRAY_INIT_NODE) continue;
				DAGNode* newnode = new DAGNode("%Quit", OP_NODE);
				newnode->resultType = i32;
				newnode->inst = LI;
				newnode->value.iValue = dag->value.iValue;
				newnode->resultId = cfg->getCount();

				newnode->outputs.push_back(dag->outputs[i]);


				for (int j = 0; j < dag->outputs[i]->inputs.size(); j++) {
					if (dag->outputs[i]->inputs[j] == dag) {
						dag->outputs[i]->inputs[j] = newnode;
						break;
					}
				}
			}
		}
		else if (dag->resultType == f32) {
			for (int i = 0; i < dag->outputs.size(); i++) {
				if (dag->outputs[i]->name == ARRAY_INIT_NODE) continue;
				DAGNode* loadnode = new DAGNode("%Quit", OP_NODE);
				loadnode->inst = LI;
				loadnode->resultType = i32;
				loadnode->resultId = cfg->getCount();

				DAGNode* fmv = new DAGNode("%Quit", OP_NODE);
				fmv->resultType = f32;
				fmv->inst = FMVWX;
				fmv->inputs.push_back(loadnode);
				fmv->resultId = cfg->getCount();

				loadnode->outputs.push_back(fmv);

				if (dag->value.fValue >= 0) {
					loadnode->value.iValue = f32Toi32(dag->value.fValue);

					fmv->outputs.push_back(dag->outputs[i]);
					for (int j = 0; j < dag->outputs[i]->inputs.size(); j++) {
						if (dag->outputs[i]->inputs[j] == dag) {
							dag->outputs[i]->inputs[j] = fmv;
							break;
						}
					}
				}
				else {
					DAGNode* fnegs = new DAGNode("%Quit", OP_NODE);
					fnegs->resultId = cfg->getCount();
					fnegs->resultType = f32;
					fnegs->inst = FNEGS;
					loadnode->value.iValue = f32Toi32(-dag->value.fValue);

					fnegs->inputs.push_back(fmv);
					fmv->outputs.push_back(fnegs);

					fnegs->outputs.push_back(dag->outputs[i]);

					for (int j = 0; j < dag->outputs[i]->inputs.size(); j++) {
						if (dag->outputs[i]->inputs[j] == dag) {
							dag->outputs[i]->inputs[j] = fnegs;
							break;
						}
					}
				}
			}
		}
		else if (dag->resultType == stringT) {
			dag->inst = LUI;
			dag->nodeType = OP_NODE;

			DAGNode* addi = new DAGNode("%Quit", OP_NODE);
			addi->inst = ADDI;
			addi->resultType = i32;
			addi->resultId = cfg->getCount();

			addi->inputs.push_back(dag);
			addi->outputs = dag->outputs;
			
			dag->outputs[0]->inputs[0] = addi;
			dag->outputs[0] = addi;

			return;
		}
		break;
	case STORE:
		//遇到%PHI节点跳过
		if (dag->inputs[1]->name == "%PHI") {
			for (int i = 0; i < dag->inputs[1]->inputs.size(); i++) {
				if(dag->inputs[1]->inputs[i]->nodeType == CONSTANT) instructionReplace(dag->inputs[1]->inputs[i], GlobalSymbolTable, cfg);
			}
			break;
		}

		else if (dag->inputs[1]->name == ARRAY_INIT_NODE) {
			//数组清空节点添加
			DAGNode* callmemset = new DAGNode("memset", OP_NODE);
			callmemset->inst = CALL;
			callmemset->resultType = i32;
			callmemset->resultId = cfg->getCount();

			DAGNode* arrayaddress = new DAGNode("%Quit", OP_NODE);
			arrayaddress->inputs.push_back(dag->inputs[0]);
			dag->inputs[0]->outputs.push_back(arrayaddress);
			arrayaddress->inst = ADD;
			if (dag->inputs[1]->resultType == iarray) arrayaddress->resultType = ipoint;
			else if (dag->inputs[1]->resultType == farray)  arrayaddress->resultType = fpoint;
			arrayaddress->resultId = cfg->getCount();

			DAGNode* zero = new DAGNode("%Quit", OP_NODE);
			zero->inst = LI;
			zero->resultType = i32;
			zero->resultId = cfg->getCount();
			zero->value.iValue = 0;

			DAGNode* size = new DAGNode("%Quit", OP_NODE);
			size->inst = LI;
			size->resultType = i32;
			size->resultId = cfg->getCount();
			size->value.iValue = 4 * cfg->arrayStack[dag->inputs[0]->name].second;

			arrayaddress->outputs.push_back(callmemset);
			zero->outputs.push_back(callmemset);
			size->outputs.push_back(callmemset);
			callmemset->inputs.push_back(arrayaddress);
			callmemset->inputs.push_back(zero);
			callmemset->inputs.push_back(size);

			callmemset->relyNodes = dag->relyNodes;
			dag->relyNodes.clear();
			dag->relyNodes.push_back(callmemset);
			
			if (dag->inputs[0]->resultType == iarray) dag->inst = SW;
			else if (dag->inputs[0]->resultType == farray) dag->inst = FSW;

			//if (dag->inputs[1]->inputs.size() != 0) {
			//	dag->inputs[1]->resultType = i32;
			//}

			//拆开initlist
			for (int i = 0; i < dag->inputs[1]->inputs.size(); i++) {
				DAGNode* store = new DAGNode("%Quit", STORE);
				if (dag->inputs[1]->inputs[i]->resultType != f32) store->inst = SW;
				else store->inst = FSW;
				
				DAGNode* addi = new DAGNode("%Quit", OP_NODE);
				addi->inst = ADDI;
				addi->resultType = ipoint;
				addi->resultId = cfg->getCount();
				addi->value.iValue = 4 * dag->inputs[1]->offset[i];
				addi->inputs.push_back(dag->inputs[0]);
				dag->inputs[0]->outputs.push_back(addi);
				addi->outputs.push_back(store);
				
				store->inputs.push_back(addi);
				store->inputs.push_back(dag->inputs[1]->inputs[i]);
				for (int j = 0; j < dag->inputs[1]->inputs[i]->outputs.size(); j++) {
					if (dag->inputs[1] == dag->inputs[1]->inputs[i]->outputs[j]) dag->inputs[1]->inputs[i]->outputs[j] = store;
				}

				if (store->inputs[1]->nodeType == CONSTANT) {
					DAGNode* li = new DAGNode("%Quit", OP_NODE);
					li->inst = LI;
					li->resultType = i32;
					li->resultId = cfg->getCount();
					if (dag->inst == SW) li->value.iValue = store->inputs[1]->value.iValue;
					else if (dag->inst == FSW) li->value.iValue = f32Toi32(store->inputs[1]->value.fValue);
					store->inputs[1] = li;
				}
				else instructionReplace(store->inputs[1], GlobalSymbolTable, cfg);

				store->relyNodes = dag->relyNodes;
				dag->relyNodes.clear();
				dag->relyNodes.push_back(store);
			}

			//左侧数组添加add
			instructionReplace(dag->inputs[0], GlobalSymbolTable, cfg);

			DAGNode* rely = dag->relyNodes[0];
			*dag = *rely;
			for (int i = 0; i < dag->inputs.size(); i++) {
				for (int j = 0; j < dag->inputs[i]->outputs.size(); j++) {
					if (rely == dag->inputs[i]->outputs[j]) {
						dag->inputs[i]->outputs[j] = dag;
					}
				}
			}
		}
		else if (dag->inputs[0]->name == ARRAYSUB_NODE) {
			if (dag->inputs[0]->resultType == ipoint) dag->inst = SW;
			else if (dag->inputs[0]->resultType == fpoint) dag->inst = FSW;
		}
		else {
			dag->resultType = dag->inputs[0]->resultType;
			//全局变量
			if (isGlobalVal(dag->inputs[0], GlobalSymbolTable)) {
				//添加LUI
				dag->inputs[0] = addlui(dag, cfg);
				if (dag->inputs[0]->resultType == i32) dag->inst = P_SW;
				else if (dag->inputs[0]->resultType == f32) dag->inst = P_FSW;
			}
			else {
				dag->nodeType = OP_NODE;
				if (dag->inputs[0]->resultType != f32) dag->inst = MV;
				else if (dag->inputs[0]->resultType == f32) dag->inst = FMVS;

				if (dag->inputs.size() == 2 && (dag->inputs[1]->resultType == iarray || dag->inputs[1]->resultType == farray)) {
					DAGNode* add = new DAGNode("%Quit", OP_NODE);
					add->inst = ADD;
					add->resultId = cfg->getCount();
					if (dag->inputs[1]->resultType == iarray) add->resultType = ipoint;
					else if (dag->inputs[1]->resultType == farray) add->resultType = fpoint;
					add->inputs.push_back(dag->inputs[1]);
					add->outputs.push_back(dag);
					dag->inputs[1] = add;
					for (int i = 0; i < add->inputs[0]->outputs.size(); i++) {
						if (dag == add->inputs[0]->outputs[i]) add->inputs[0]->outputs[i] = add;
					}
				}
			}
		}

		dag->value.iValue = 4 * dag->value.iValue;

		if (!isLegalImm(SType, dag->value.iValue)) {

			DAGNode* add = new DAGNode("%Quit", OP_NODE);
			add->inst = ADD;
			add->resultId = cfg->getCount();
			if (dag->inputs[1]->resultType == ipoint) add->resultType = ipoint;
			else if (dag->inputs[1]->resultType == fpoint) add->resultType = fpoint;
			else add->resultType = i32;

			add->inputs.push_back(dag->inputs[0]);
			add->outputs.push_back(dag);
			dag->inputs[0] = add;
			for (int i = 0; i < add->inputs[0]->outputs.size(); i++) {
				if (dag == add->inputs[0]->outputs[i]) add->inputs[0]->outputs[i] = add;
			}
				
			DAGNode* li = new DAGNode("%Quit", OP_NODE);
			li->inst = LI;
			li->resultId = cfg->getCount();
			li->resultType = i32;
			li->value.iValue = dag->value.iValue;
			add->inputs.push_back(li);
			li->outputs.push_back(add);

			dag->value.iValue = 0;

			instructionReplace(add->inputs[0], GlobalSymbolTable, cfg);
		}
		break;
	case LOAD:
		if (dag->inputs[0]->name == ARRAYSUB_NODE) {
			if (dag->inputs[0]->resultType == ipoint) dag->inst = LW;
			else if (dag->inputs[0]->resultType == fpoint) dag->inst = FLW;
		}
		else {
			dag->resultType = dag->inputs[0]->resultType;
			if (isGlobalVal(dag->inputs[0], GlobalSymbolTable)) {
				//添加LUI
				dag->inputs[0] = addlui(dag, cfg);
				if (dag->inputs[0]->resultType == i32) dag->inst = P_LW;
				else if (dag->inputs[0]->resultType == f32) dag->inst = P_FLW;
			}
			else {
				dag->nodeType = OP_NODE;
				if (dag->inputs[0]->resultType == i32) dag->inst = MV;
				else if (dag->inputs[0]->resultType == f32) dag->inst = FMVS;
			}
		}

		dag->value.iValue = 4 * dag->value.iValue;

		if (!isLegalImm(IType, dag->value.iValue)) {

			DAGNode* add = new DAGNode("%Quit", OP_NODE);
			add->inst = ADD;
			add->resultId = cfg->getCount();
			if (dag->inputs[0]->resultType == ipoint) add->resultType = ipoint;
			else if (dag->inputs[0]->resultType == fpoint) add->resultType = fpoint;
			else add->resultType = i32;

			add->inputs.push_back(dag->inputs[0]);
			add->outputs.push_back(dag);
			dag->inputs[0] = add;
			for (int i = 0; i < add->inputs[0]->outputs.size(); i++) {
				if (dag == add->inputs[0]->outputs[i]) add->inputs[0]->outputs[i] = add;
			}

			DAGNode* li = new DAGNode("%Quit", OP_NODE);
			li->inst = LI;
			li->resultId = cfg->getCount();
			li->resultType = i32;
			li->value.iValue = dag->value.iValue;
			add->inputs.push_back(li);
			li->outputs.push_back(add);

			dag->value.iValue = 0;

			instructionReplace(add->inputs[0], GlobalSymbolTable, cfg);
		}
		break;
	case VALUE_NODE:
		//数组类型添加add节点
		return;
		break;
	case REG_NODE:
		//return;
		break;
	default:
		break;
	}

	for (int i = 0; i < dag->inputs.size(); i++) {
		instructionReplace(dag->inputs[i], GlobalSymbolTable, cfg);
	}

	for (int i = 0; i < dag->relyNodes.size(); i++) {
		instructionReplace(dag->relyNodes[i], GlobalSymbolTable, cfg);
	}

}


//指令选择，传入GCFG和全局符号表
void instructionSelection(GCFG* gcfg, SymbolTable* GlobalSymbolTable) {
	map<string, CFG*>::iterator it;
	for (it = gcfg->controlFlow.begin(); it != gcfg->controlFlow.end(); it++) {
		int n = it->second->basicBlocks.size();
		for (int i = 0; i < n; i++) {
			if (it->second->basicBlocks[i]->dag->nodes.size() == 0) break;
			else {
				instructionReplace(it->second->basicBlocks[i]->dag->rootNode, GlobalSymbolTable, it->second);
			}
		}
	}
}
