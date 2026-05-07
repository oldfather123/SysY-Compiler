#include "InstructionGenerate.h"
#include "Generate.h"
#include "Register.h"
#include "Instruction.h"
#include <fstream>

//单条指令生成，stacktop是临时变量开始位置距sp寄存器位置
void instructionDAGToAssemblerCode(DAGNode* dag, IGManager* ig, CFG* cfg, Linearize* linearize, int stack_val, int stack_protect) {
	//输出寄存器
	int rd;
	if (dag->resultType == f32) {
		auto reg = fregisters.find(ig->color["%" + to_string(dag->resultId)]);
		if (reg != fregisters.end()) rd = (*reg).second;
	}
	else {
		auto reg = iregisters.find(ig->color["%" + to_string(dag->resultId)]);
		if (reg != iregisters.end()) rd = (*reg).second;
	}

	//输入寄存器
	vector<int> inputs;
	for (int i = 0; i < dag->inputs.size(); i++) {
		if (dag->inputs[i]->nodeType == VALUE_NODE) {
			if (dag->inputs[i]->resultType == f32) {
				auto reg = fregisters.find(ig->color[dag->inputs[i]->name]);
				if (reg != fregisters.end()) inputs.push_back((*reg).second);
			}
			else {
				if (dag->inputs[i]->name == "%zero") {
					inputs.push_back(0);
					continue;
				}
				auto reg = iregisters.find(ig->color[dag->inputs[i]->name]);
				if (reg != iregisters.end()) inputs.push_back((*reg).second);
			}
		}
		else {
			if (dag->inputs[i]->resultType == f32) {
				auto reg = fregisters.find(ig->color["%" + to_string(dag->inputs[i]->resultId)]);
				if (reg != fregisters.end()) inputs.push_back((*reg).second);
			}
			else {
				if (dag->inputs[i]->name == "%zero") {
					inputs.push_back(0);
					continue;
				}
				auto reg = iregisters.find(ig->color["%" + to_string(dag->inputs[i]->resultId)]);
				if (reg != iregisters.end()) inputs.push_back((*reg).second);
			}
		}
	}

	switch (dag->inst) {
		case CALL://函数调用
			if (dag->name == "starttime") {
				dag->name = "_sysy_starttime";
				gene(instructions[LI] + "\t" + REG_I[a0] + ", 0");
			}
			else if (dag->name == "stoptime") {
				dag->name = "_sysy_stoptime";
				gene(instructions[LI] + "\t" + REG_I[a0] + ", 0");
			}
			gene(instructions[dag->inst] + "\t" + dag->name);
			break;
		case RET://函数返回

			break;
		case J://无条件跳转
			gene(instructions[JAL] + "\t" + REG_I[zero] + ", " + linearize->block_label[dag->targets[0]]);
			break;
			//B型指令
			//寄存器比较
		case BEQ:
		case BNE:
		case BLT:
		case BGE:
		case BLTU:
		case BGEU:
		case BGT:
		case BLE:
		case BGTU:
		case BLEU:
			gene(instructions[dag->inst] + "\t" + REG_I[inputs[0]] + ", " + REG_I[inputs[1]] + ", " + linearize->block_label[dag->targets[0]]);
			break;
			//零比较
		case BEQZ:
		case BNEZ:
		case BLEZ:
		case BGEZ:
		case BLTZ:
		case BGTZ:
			gene(instructions[dag->inst] + "\t" + REG_I[inputs[0]] + ", " + linearize->block_label[dag->targets[0]]);
			break;

			//R形指令
				//RV32I
		case SUB:
		case SLL:
		case SLT:
		case SLTU:
		case XOR:
		case SRL:
		case SRA:
		case OR:
		case AND:
		case SLLW:
		case SRLW:
		case SRAW:
		case ADDW:
		case SUBW:
			//RV32M
		case MUL:
		case MULH:
		case MULHU:
		case MULHSU:
		case MULW:
		case DIV:
		case DIVU:
		case DIVW:
		case DIVUW:
		case REM:
		case REMU:
		case REMW:
		case REMUW:
		case SGT:
			gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]] + ", " + REG_I[inputs[1]]);
			break;
		case ADD:
			//全局数组
			//if (dag->inputs[0]->inst == ADDI) {
			//	gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]] + ", " + REG_I[inputs[1]]);
			//}
			if (dag->inputs.size() == 1) {
				if (cfg->arrayStack.find(dag->inputs[0]->name) != cfg->arrayStack.end()) {
					int arraybegin = stack_val - 4 * (cfg->arrayStack[dag->inputs[0]->name].first + cfg->arrayStack[dag->inputs[0]->name].second);

					if (arraybegin < 2048) gene(instructions[ADDI] + "\t" + REG_I[rd] + ", " + REG_I[sp] + ", " + to_string(arraybegin));
					else if (stack_val + stack_protect - arraybegin >= 0 && stack_val + stack_protect - arraybegin < 2048) gene(instructions[ADDI] + "\t" + REG_I[rd] + ", " + REG_I[s0] + ", -" + to_string(stack_val + stack_protect - arraybegin));
					else {
						gene(instructions[LI] + "\t" + REG_I[rd] + ", " + to_string(arraybegin));
						gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[rd] + ", " + REG_I[sp]);
					}
				}
				else {
					gene(instructions[LUI] + "\t" + REG_I[rd] + ", %hi(" + dag->inputs[0]->name + ")");
					gene(instructions[ADDI] + "\t" + REG_I[rd] + +", " + REG_I[rd] + ", %lo(" + dag->inputs[0]->name + ")");
				}
			}
			////用枚举类型数值判断
			//else if (dag->inputs[0]->resultType > 1) {
			//	int arraybegin = stacktop - 4 * (cfg->arrayStack[dag->inputs[0]->name].first + cfg->arrayStack[dag->inputs[0]->name].second);

			//	if (arraybegin < 2048) gene(instructions[ADDI] + "\t" + REG_I[rd] + ", " + REG_I[sp] + ", " + to_string(arraybegin));
			//	else if (stacktop - arraybegin < 2048) gene(instructions[ADDI] + "\t" + REG_I[rd] + ", " + REG_I[s0] + ", -" + to_string(arraybegin));
			//	else {
			//		gene(instructions[LI] + "\t" + REG_I[rd] + ". " + ", " + to_string(arraybegin));
			//		gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[rd] + ", " + REG_I[sp]);
			//	}

			//	gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[rd] + ", " + REG_I[inputs[1]]);
			//}
			else {
				gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]] + ", " + REG_I[inputs[1]]);
			}
			break;

			//I型指令
		case SLLI:
		case SLLIW:
		case SRAI:
		case SRAIW:
		case SRLI:
		case SRLIW:
		case ANDI:
		case ORI:
		case XORI:
		case SLTI:
		case SLTIU:
			gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]] + ", " + to_string(dag->value.iValue));
			break;
		case ADDI:
			//导全局数组首地址
			if (dag->inputs[0]->inst == LUI) gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]] + +", " + "%lo(" + dag->inputs[0]->inputs[0]->name + ")");
			//形参数组
			else if (dag->inputs[0]->nodeType == VALUE_NODE) {
				int begin = stack_val - 4 * (cfg->arrayStack[dag->inputs[0]->name].first + cfg->arrayStack[dag->inputs[0]->name].second);
				if(begin + dag->value.iValue < 2048) gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[sp] + ", " + to_string(begin + dag->value.iValue));
				else {
					gene(instructions[LI] + "\t" + REG_I[rd] + ", " + to_string(begin + dag->value.iValue));
					gene(instructions[ADD] + "\t" + REG_I[rd] + ", " + REG_I[sp] + ", " + REG_I[rd]);
				}
			} 
			//else gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]] + ", " + to_string(dag->value.iValue));
			break;
		case ADDIW:
			if(isLegalImm(IType, dag->value.iValue)) gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]] + ", " + to_string(dag->value.iValue));
			else {
				gene(instructions[LI] + "\t" + REG_I[rd] + ", " + to_string(dag->value.iValue));
				gene(instructions[ADDW] + "\t" + REG_I[rd] + ", " + REG_I[rd] + ", " + REG_I[inputs[0]]);
			}
			break;

			//立即数指令
		case LI:
			/*
			if (dag->value.iValue == 0) { dag->resultId = 0; break; }

			if(dag->name == "argu_li")			//暂时，后续修改
				gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + to_string(-dag->value.iValue));
			else */gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + to_string(dag->value.iValue));
			break;

			//空指令
		case NOP:
			gene(instructions[dag->inst]);
			break;
			//移动指令
		case MV:
			if(dag->inputs.size() == 1) gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]]);
			else gene(instructions[dag->inst] + "\t" + REG_I[inputs[0]] + ", " + REG_I[inputs[1]]);
			break;
			//0比较指令
		case NOT:
		case NEG:
		case NEGW:
		case SEXTW:
		case SEQZ:
		case SNEZ:
		case SLTZ:
		case SGTZ:
			gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_I[inputs[0]]);
			break;

			//浮点R型
		case FADDS:
		case FSUBS:
		case FMULS:
		case FDIVS:
		case FADDD:
		case FSUBD:
		case FMULD:
		case FDIVD:
			gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + REG_F[inputs[0]] + ", " + REG_F[inputs[1]]);
			break;

			//平方根指令
		case FSQRTS:
		case FSQRTD:
			gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + REG_F[inputs[0]]);
			break;

			//浮点多算术操作指令
		case FMADDS:
		case FMSUBS:
		case FNMADDS:
		case FNMSUBS:
		case FMADDD:
		case FMSUBD:
		case FNMADDD:
		case FNMSUBD:
			break;

			//浮点最大最小值指令
		case FMINS:
		case FMAXS:
		case FMIND:
		case FMAXD:
			break;

			//浮点比较指令
		case FEQS:
		case FLTS:
		case FLES:
		case FGTS:
		case FGES:
		case FEQD:
		case FLTD:
		case FLED:
		case FGTD:
		case FGED:
			gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_F[inputs[0]] + ", " + REG_F[inputs[1]]);
			break;

		case FNEGS:
		case FNEGD:
			gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + REG_F[inputs[0]]);
			break;

			//浮点分类指令
		case FCLASSS:
		case FCLASSD:
			break;

			// 浮点注入指令
		case FSGNJS:
		case FSGNJNS:
		case FSGNJXS:
		case FSGNJD:
		case FSGNJND:
		case FSGNJXD:
			break;

			//浮点转换指令
				//整形转浮点
		case FCVTSW:
		case FCVTDW:
		case FCVTSWU:
		case FCVTDWU:
			gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + REG_I[inputs[0]]);
			break;
			//浮点转整形
		case FCVTWS:
		case FCVTWD:
		case FCVTWUD:
		case FCVTWUS:
			gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_F[inputs[0]] + ", rtz");
			break;
			//单精度双精度转换
		case FCVTSD:
		case FCVTDS:
			break;

			//浮点搬运指令
		case FMVXW:
			gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + REG_F[inputs[0]]);
			break;
		case FMVWX:
			gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + REG_I[inputs[0]]);
			break;

		case FMVS:
			gene(instructions[dag->inst] + "\t" + REG_F[inputs[0]] + ", " + REG_F[inputs[1]]);
			break;

			//内存访问指令
		case LW:
			//if (dag->inputs[0]->resultType == iarray) {
			//	gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + to_string(stacktop - 4 * (cfg->arrayStack[dag->inputs[0]->name].first + cfg->arrayStack[dag->inputs[0]->name].second) + dag->inputs[0]->value.iValue) + "(" + REG_I[sp] + ")");
			//}
			//else if (dag->value.iValue <= 0) gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + to_string(-dag->value.iValue * 4) + "(" + REG_I[sp] + ")");
			//else gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", 0(" + REG_I[inputs[0]] + ")");
			/*if(dag->inputs.size() == 0) gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + to_string(-4 * dag->value.iValue) + "(" + REG_I[s0] + ")");
			else */gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + to_string(dag->value.iValue) + "(" + REG_I[inputs[0]] + ")");
			break;
		case FLW:
			//if (dag->inputs[0]->resultType == farray) {
			//	gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + to_string(stacktop - 4 * (cfg->arrayStack[dag->inputs[0]->name].first + cfg->arrayStack[dag->inputs[0]->name].second) + dag->inputs[0]->value.iValue) + "(" + REG_I[sp] + ")");
			//}
			//else if (dag->value.iValue <= 0) gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + to_string(-dag->value.iValue * 4) + "(" + REG_I[sp] + ")");
			//else gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", 0(" + REG_I[inputs[0]] + ")");
			gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + to_string(dag->value.iValue) + "(" + REG_I[inputs[0]] + ")");
			break;
		case SW:
			//if (dag->inputs[1]->name == ARRAY_INIT_NODE) {
			//	int last = 0;
			//	for (int i = 0; i < dag->inputs[1]->offset.size(); i++) {
			//		bool add = false;//是否超立即数
			//		int offset = 4 * dag->inputs[1]->offset[i];
			//		//超常量范围
			//		if (offset >= 2048 + last) {
			//			//加偏移
			//			add = true;
			//			gene(instructions[LI] + "\t" + REG_I[inputs[1]] + ", " + to_string(offset - last));
			//			gene(instructions[ADD] + "\t" + REG_I[inputs[0]] + ", " + REG_I[inputs[0]] + ", " + REG_I[inputs[1]]);
			//			last = offset;
			//		}
			//		if(dag->resultType )
			//		//根据情况生成
			//		if (dag->inputs[1]->inputs[i]->nodeType != CONSTANT) {
			//			//是否加上偏移
			//			if (add) {
			//				gene(instructions[SW] + "\t" + REG_I[ig->registers[ig->color["%" + to_string(dag->inputs[1]->inputs[i]->resultId)]]] + ", 0(" + REG_I[inputs[0]] + ")");
			//			}
			//			else gene(instructions[SW] + "\t" + REG_I[ig->registers[ig->color["%" + to_string(dag->inputs[1]->inputs[i]->resultId)]]] + ", " + to_string(offset - last) + "(" + REG_I[inputs[0]] + ")");
			//		}
			//		else {
			//			gene(instructions[LI] + "\t" + REG_I[inputs[1]] + ", " + to_string(dag->inputs[1]->inputs[i]->value.iValue));
			//			//是否加上偏移
			//			if (add) {
			//				gene(instructions[SW] + "\t" + REG_I[inputs[1]] + ", 0(" + REG_I[inputs[0]] + ")");
			//			}
			//			else gene(instructions[SW] + "\t" + REG_I[inputs[1]] + ", " + to_string(offset- last) + "(" + REG_I[inputs[0]] + ")");
			//		}
			//	}
			//}
			//if (dag->inputs[0]->resultType == iarray) {
			//	gene(instructions[dag->inst] + "\t" + REG_I[inputs[1]] + ", " + to_string(stacktop - 4 * (cfg->arrayStack[dag->inputs[0]->name].first + cfg->arrayStack[dag->inputs[0]->name].second) + dag->inputs[0]->value.iValue) + "(" + REG_I[sp] + ")");
			//}
			//else if (dag->value.iValue <= 0) gene(instructions[dag->inst] + "\t" + REG_I[inputs[0]] + ", " + to_string(-dag->value.iValue * 4) + "(" + REG_I[sp] + ")");
			//else gene(instructions[dag->inst] + "\t" + REG_I[inputs[1]] + ", 0(" + REG_I[inputs[0]] + ")");
			/*if(dag->inputs.size() == 1) gene(instructions[dag->inst] + "\t" + REG_I[inputs[1]] + ", " + to_string(-4 * dag->value.iValue) + "(" + REG_I[sp] + ")");
			else */gene(instructions[dag->inst] + "\t" + REG_I[inputs[1]] + ", " + to_string(dag->value.iValue) + "(" + REG_I[inputs[0]] + ")");
			break;
		case FSW:
			//if (dag->inputs[1]->name == ARRAY_INIT_NODE) {
			//	int last = 0;
			//	for (int i = 0; i < dag->inputs[1]->offset.size(); i++) {
			//		bool add = false;//是否超立即数
			//		int offset = 4 * dag->inputs[1]->offset[i];
			//		//超常量范围
			//		if (offset >= 2048 + last) {
			//			//加偏移
			//			add = true;
			//			gene(instructions[LI] + "\t" + REG_I[inputs[1]] + ", " + to_string(offset - last));
			//			gene(instructions[ADD] + "\t" + REG_I[inputs[0]] + ", " + REG_I[inputs[0]] + ", " + REG_I[inputs[1]]);
			//			last = offset;
			//		}
			//		//根据情况生成
			//		if (dag->inputs[1]->inputs[i]->nodeType != CONSTANT) {
			//			//是否加上偏移
			//			if (add) {
			//				gene(instructions[SW] + "\t" + REG_I[ig->registers[ig->color["%" + to_string(dag->inputs[1]->inputs[i]->resultId)]]] + ", 0(" + REG_I[inputs[0]] + ")");
			//			}
			//			else gene(instructions[SW] + "\t" + REG_I[ig->registers[ig->color["%" + to_string(dag->inputs[1]->inputs[i]->resultId)]]] + ", " + to_string(offset - last) + "(" + REG_I[inputs[0]] + ")");
			//		}
			//		else {
			//			gene(instructions[LI] + "\t" + REG_I[inputs[1]] + ", " + to_string(dag->inputs[1]->inputs[i]->value.iValue));
			//			//是否加上偏移
			//			if (add) {
			//				gene(instructions[SW] + "\t" + REG_I[inputs[1]] + ", 0(" + REG_I[inputs[0]] + ")");
			//			}
			//			else gene(instructions[SW] + "\t" + REG_I[inputs[1]] + ", " + to_string(offset - last) + "(" + REG_I[inputs[0]] + ")");
			//		}
			//	}
			//}
			//if (dag->inputs[0]->resultType == farray) {
			//	gene(instructions[dag->inst] + "\t" + REG_F[inputs[1]] + ", " + to_string(stacktop - 4 * (cfg->arrayStack[dag->inputs[0]->name].first + cfg->arrayStack[dag->inputs[0]->name].second) + dag->inputs[0]->value.iValue) + "(" + REG_I[sp] + ")");
			//}
			//else if (dag->value.iValue <= 0) gene(instructions[dag->inst] + "\t" + REG_F[inputs[0]] + ", " + to_string(-dag->value.iValue * 4) + "(" + REG_I[sp] + ")");
			//else gene(instructions[dag->inst] + "\t" + REG_F[inputs[1]] + ", 0(" + REG_I[inputs[0]] + ")");
			gene(instructions[dag->inst] + "\t" + REG_F[inputs[1]] + ", " + to_string(dag->value.iValue) + "(" + REG_I[inputs[0]] + ")");
			break;
		case P_LW:
			gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", %lo(" + dag->inputs[0]->inputs[0]->name + ")(" + REG_I[inputs[0]] + ")");
			break;
		case P_SW:
			gene(instructions[dag->inst] + "\t" + REG_I[inputs[1]] + ", %lo(" + dag->inputs[0]->inputs[0]->name + ")(" + REG_I[inputs[0]] + ")");
			break;
		case P_FLW:
			gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", %lo(" + dag->inputs[0]->inputs[0]->name + ")(" + REG_I[inputs[0]] + ")");
			break;
		case P_FSW:
			gene(instructions[dag->inst] + "\t" + REG_F[inputs[1]] + ", %lo(" + dag->inputs[0]->inputs[0]->name + ")(" + REG_I[inputs[0]] + ")");
			break;
			//全局地址计算
		case LUI:
			gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", %hi(" + dag->inputs[0]->name + ")");
			break;
		case SD:
			if (dag->inputs.size() == 1) {
				if (dag->value.iValue < 2048)gene(instructions[dag->inst] + "\t" + REG_I[inputs[0]] + ", " + to_string(4 * dag->value.iValue) + "(" + REG_I[sp] + ")");
				else cout << "stack overflow" << endl;
			} 
			else {
				if (dag->inputs[0]->inst == LI) gene(instructions[ADD] + "\t" + REG_I[inputs[0]] + ", " + REG_I[sp] + ", " + REG_I[inputs[0]]);
				gene(instructions[dag->inst] + "\t" + REG_I[inputs[1]] + ", 0(" + REG_I[inputs[0]] + ")");
			}
			break;
		case LD:
			if (dag->inputs.size() == 0) {
				if (dag->name == "%spill") {
					if (4 * dag->value.iValue < 2048) gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + to_string(4 * dag->value.iValue) + "(" + REG_I[sp] + ")");
					else cout << "stack overflow" << endl;
	
				} 
				else {
					if (4 * dag->value.iValue > -2048)gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", " + to_string(-4 * dag->value.iValue) + "(" + REG_I[s0] + ")");
					else cout << "stack overflow" << endl;
				} 
			} 
			else {
				if (dag->inputs[0]->inst == LI) {
					if (dag->name == "%spill") {
						gene(instructions[ADD] + "\t" + REG_I[inputs[0]] + ", " + REG_I[sp] + ", " + REG_I[inputs[0]]);
					}
					else {
						gene(instructions[ADD] + "\t" + REG_I[inputs[0]] + ", " + REG_I[s0] + ", " + REG_I[inputs[0]]);
					}
				}
				gene(instructions[dag->inst] + "\t" + REG_I[rd] + ", 0(" + REG_I[inputs[0]] + ")");
			}
			break;
		case FLD:
			if (dag->inputs.size() == 0) {
				if (dag->name == "%spill") {
					if (4 * dag->value.iValue < 2048) gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + to_string(4 * dag->value.iValue) + "(" + REG_I[sp] + ")");
					else cout << "stack overflow" << endl;

				}
				else {
					if (4 * dag->value.iValue > -2048)gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", " + to_string(-4 * dag->value.iValue) + "(" + REG_I[s0] + ")");
					else cout << "stack overflow" << endl;
				}
			}
			else {
				if (dag->inputs[0]->inst == LI) gene(instructions[ADD] + "\t" + REG_I[inputs[0]] + ", " + REG_I[sp] + ", " + REG_I[inputs[0]]);
				gene(instructions[dag->inst] + "\t" + REG_F[rd] + ", 0(" + REG_I[inputs[0]] + ")");
			}
			break;
		case FSD:
			if (dag->inputs.size() == 1) {
				if (dag->value.iValue < 2048)gene(instructions[dag->inst] + "\t" + REG_F[inputs[0]] + ", " + to_string(4 * dag->value.iValue) + "(" + REG_I[sp] + ")");
				else cout << "stack overflow" << endl;
			}
			else {
				if (dag->inputs[0]->inst == LI) gene(instructions[ADD] + "\t" + REG_I[inputs[0]] + ", " + REG_I[sp] + ", " + REG_I[inputs[0]]);
				gene(instructions[dag->inst] + "\t" + REG_F[inputs[1]] + ", 0(" + REG_I[inputs[0]] + ")");
			}
			break;
	
		default:
			cout << "unselected node" << "   inst:" << instructions[dag->inst] << endl;
			break;
	}
}


extern ofstream output;

void instructionGenerate(GCFG* gcfg, DFA* dfa, Linear* linear, string targetname) {
	output.open(targetname);

	for (auto it = gcfg->controlFlow.begin(); it != gcfg->controlFlow.end(); it++) {
		string functionname = (*it).first;
		CFG* cfg = (*it).second;
		IGManager* ig = dfa->igs[cfg];
		Linearize* linearize = linear->linear[cfg];

		int stack_protect = 8 * (ig->i32_max_func + ig->f32_max_func + 1);
		int stack_t = 0;
		int stack_val = (cfg->maxStackSize + cfg->funcStackSize) * 4;
		int stack = stack_val + stack_protect;
		bool reg_ra = false;
		//bool reg_s0 = false;

		gene(".globl\t" + functionname);
		gene(".align\t2");
		gene(".type\t" + functionname + ",@function");
		//添加函数名
		addlabel(functionname);


		if (ig->hasFunc) {
			stack_protect += 8;
			stack_t += 8;
			stack += 8;
			reg_ra = true;
		}
		////是否能够通过立即数分配栈空间
		//if (stack >= 2048) {
		//	stack_protect += 8;
		//	stack += 8;
		//	reg_s0 = true;
		//}

		stack = (((stack - 1) >> 4) + 1) << 4;

		//if (round_float && functionname == "main") {
		//	gene("li\tt0, 1");
		//	gene("fsrm\tt0, t0");
		//}

		//是否需要s0寄存器进行栈访存的栈指针移动和s0和ra寄存器存储
		if (stack >= 2048) {
			gene("addi\tsp, sp, -" + to_string(stack_protect));
			stack_t = stack_protect - 8;
			gene("sd\ts0, " + to_string(stack_t) + "(sp)");
			gene("addi\ts0, sp, " + to_string(stack_protect));
			if (reg_ra) {
				stack_t -= 8;
				gene("sd\tra, " + to_string(stack_t) + "(sp)");
			}
		}
		else {
			gene("addi\tsp, sp, -" + to_string(stack));
			stack_t = stack - 8;
			gene("sd\ts0, " + to_string(stack_t) + "(sp)");
			if (reg_ra) {
				stack_t -= 8;
				gene("sd\tra, " + to_string(stack_t) + "(sp)");
			}
		}

		//存储保护寄存器
		for (int i = 1; i <= ig->i32_max_func; i++) {
			stack_t -= 8;
			gene("sd\t" + REG_I[iregisters.find(i + icaller)->second] + ", " + to_string(stack_t) + "(sp)");

		}
		for (int i = 1; i <= ig->f32_max_func; i++) {
			stack_t -= 8;
			gene("fsd\t" + REG_F[fregisters.find(i + fcaller)->second] + ", " + to_string(stack_t) + "(sp)");
		}

		//无法栈立即数分配情况
		if (stack >= 2048) {
			gene("li\tt0, " + to_string(-stack_val));
			gene("add\tsp, sp, t0");
		}
		else {
			gene("addi\ts0, sp, " + to_string(stack));
		}
		


		//生成正文代码
		for (int i = 0; i < linearize->linear_nodes.size(); i++) {
			//if(linearize->label_trigger.find(i) != linearize->label_trigger.end()) addlabel((*linearize->label_trigger.find(i)).second);

			if (linearize->linear_nodes[i]->name.find(".Label_") != linearize->linear_nodes[i]->name.npos) {
				addlabel(linearize->linear_nodes[i]->name);
				continue;
			}

			//if (linearize->linear_nodes[i]->name == "%NOP") continue;

			instructionDAGToAssemblerCode(linearize->linear_nodes[i], ig, cfg, linearize, stack_val, stack_protect);
		}



		//无法栈立即数分配情况
		if (stack >= 2048) {
			gene("li\tt0, " + to_string(stack_val));
			gene("add\tsp, sp, t0");
		}


		//存储保护寄存器
		for (int i = ig->f32_max_func; i > 0; i--) {
			gene("fld\t" + REG_F[fregisters.find(i + fcaller)->second] + ", " + to_string(stack_t) + "(sp)");
			stack_t += 8;
		}
		for (int i = ig->i32_max_func; i > 0 ; i--) {
			gene("ld\t" + REG_I[iregisters.find(i + icaller)->second] + ", " + to_string(stack_t) + "(sp)");
			stack_t += 8;
		}

		//函数结束
		if (stack >= 2048) {
			if (reg_ra) {
				gene("ld\tra, " + to_string(stack_t) + "(sp)");
				stack_t += 8;
			}
			gene("ld\ts0, " + to_string(stack_t) + "(sp)");
			gene("addi\tsp, sp, " + to_string(stack_protect));
			stack_t += 8;
		}
		else {
			if (reg_ra) {
				gene("ld\tra, " + to_string(stack_t) + "(sp)");
				stack_t += 8;
			} 
			gene("ld\ts0, " + to_string(stack_t) + "(sp)");
			gene("addi\tsp, sp, " + to_string(stack));
		}

		gene("ret");

		generateWithoutTab("");
	}
}