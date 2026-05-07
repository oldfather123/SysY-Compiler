#include "Linearize.h"
#include "RegisterAllocate.h"
#include "InstructionGenerate.h"
#include "../IR/DAG.h"
#include "../IR/BasicBlock.h"
#include "../IR/CFG.h"
#include "../IR/GCFG.h"

vector<DAGNode*> Linearize::linearize_work() {

	//加入end块id=-1
	BasicBlock* endblock = new BasicBlock(-1, nullptr);
	cfg->basicBlocks.push_back(endblock);
	DAGNode* nopnode = new DAGNode("%NOP", OP_NODE);
	endblock->topoList.push_back(nopnode);
	endblock->dag->rootNode = nopnode;
	endblock->dag->nodes.push_back(nopnode);




	first_travel();

	//在删除endbranch之前，删除掉move操作
	//避免branch到endbranch的空块，（被move遮挡，没有被passcontinue而分配了label，而后面全是空块则没有插入label_node
	//这样的话，只剩endbranck节点的块会被del_unreachable从Basicblocks里删除
	//del_regmove();
	second_travel();
	

	//连续无条件跳转的target传递
	pass_continue_branch();

	//会先处理掉只剩endbranch的块
	//接下来进行del_endbranch之后的块内必然还会剩下节点
	del_unreachblock();


	//删除尾部的无条件跳转到下一块的j指令
	del_enduncon_branch();


	change_doubleendbr();


	third_travel();

	//merge放最后保护label
	mergenodes();


	//指令窥孔
	mergeImmInstruction();

	//del_ldst();
	//del_ldst2();

	//删除end块
	cfg->basicBlocks.pop_back();

	return linear_nodes;
}




void Linearize::first_travel() {
	for (BasicBlock* b : cfg->basicBlocks) {

		vector<DAGNode*>& nodes = b->topoList;

		for (int node_index = 0; node_index < nodes.size(); node_index++) {
			DAGNode* node_iter = nodes[node_index];
			//拆开双分支的branch
			if (node_iter->name == BRANCH_NODE) {
				//记录块的live
				//for (int b_index : node_iter->targets) {
				//	block_label_live[b_index] += 1;
				//}

				if (node_iter->targets.size() == 2) {
					//把真假分支拆开
					//新建假节点
					DAGNode* false_branchnode = new DAGNode(node_iter, node_iter->symbol);
					false_branchnode->targets.push_back(node_iter->targets[1]);
					false_branchnode->inputs.clear();
					false_branchnode->inst = J;
					//保留真节点
					node_iter->targets.pop_back();
					//插入
					nodes.insert(nodes.begin() + node_index + 1, false_branchnode);
					node_index++;
				}
			}
			//拆开由load变成的单输入的move
			//并合并节点
			else if (node_iter->inst == MV || node_iter->inst == FMVS) {
				if (node_iter->inputs.size() == 1) {
					DAGNode* v_regnode = new DAGNode(node_iter, node_iter->symbol);
					v_regnode->nodeType = REG_NODE;
					//v_regnode->resultId = node_iter->resultId;
					node_iter->inputs.insert(node_iter->inputs.begin(), v_regnode);
					v_regnode->outputs.push_back(node_iter);
					//更换依赖关系
					for (DAGNode* rely_iter : node_iter->outputs) {
						vector<DAGNode*>::iterator temp_f = find(rely_iter->inputs.begin(), rely_iter->inputs.end(), node_iter);
						if (temp_f != rely_iter->inputs.end()) *temp_f = v_regnode;
					}
				}

				//合并move操作
				if (node_iter->inst == MV) {
					string reg1, reg2;
					if (node_iter->inputs[0]->nodeType == VALUE_NODE) reg1 = node_iter->inputs[0]->name;
					else if (node_iter->inputs[0]->nodeType == CONSTANT && node_iter->inputs[0]->name == "%zero") continue;
					else reg1 = "%" + to_string(node_iter->inputs[0]->resultId);

					if (node_iter->inputs[1]->nodeType == VALUE_NODE)  reg2 = node_iter->inputs[1]->name;
					else if (node_iter->inputs[1]->nodeType == CONSTANT && node_iter->inputs[1]->name == "%zero") continue;
					else reg2 = "%" + to_string(node_iter->inputs[1]->resultId);

					map<int, int>::const_iterator reg1_it = iregisters.find(ig->color[reg1]);
					map<int, int>::const_iterator reg2_it = iregisters.find(ig->color[reg2]);

					color_rec[reg1] = reg1_it->second;
					color_rec[reg2] = reg2_it->second;

					if ((*reg1_it).second == (*reg2_it).second) {
						//sen_iter->name = "%NOP";
						nodes.erase(nodes.begin() + node_index);
						node_index--;
					}
				}
				else if (node_iter->inst == FMVS) {
					string reg1, reg2;
					if (node_iter->inputs[0]->nodeType == VALUE_NODE) reg1 = node_iter->inputs[0]->name;
					else if (node_iter->inputs[0]->nodeType == CONSTANT && node_iter->inputs[0]->name == "%zero") continue;
					else	reg1 = "%" + to_string(node_iter->inputs[0]->resultId);

					if (node_iter->inputs[1]->nodeType == VALUE_NODE)  reg2 = node_iter->inputs[1]->name;
					else if (node_iter->inputs[1]->nodeType == CONSTANT && node_iter->inputs[1]->name == "%zero") continue;
					else reg2 = "%" + to_string(node_iter->inputs[1]->resultId);
					map<int, int>::const_iterator  reg1_it = fregisters.find(ig->color[reg1]);
					map<int, int>::const_iterator  reg2_it = fregisters.find(ig->color[reg2]);

					if ((*reg1_it).second == (*reg2_it).second) {
						//sen_iter->name = "%NOP";
						nodes.erase(nodes.begin() + node_index);
						node_index--;
					}
				}
			}
			//return返回值后面加cfg的end
			else if (node_iter->name == RETURN_NODE) {
				DAGNode* branchtoend = new DAGNode(BRANCH_NODE, OP_NODE);
				branchtoend->targets.push_back(-1);
				branchtoend->resultId = -1;
				branchtoend->inst = J;
				nodes.erase(nodes.begin() + node_index);
				nodes.insert(nodes.begin() + node_index, branchtoend);
				//block_label_live[-1] += 1;
				node_index++;
			}
		}
	}
}




BasicBlock* Linearize::find_idblock(int bid) {
	for (BasicBlock* b_iter : cfg->basicBlocks) {
		if (b_iter->id == bid) {
			return b_iter;
		}
	}
	return nullptr;
}


void Linearize::pass_continue_branch() {
	bool branch_change = true;
	while (branch_change) {
		branch_change = false;
		//逆序迭代
		for (int b_index = cfg->basicBlocks.size() - 1; b_index > -1; b_index--) {
			BasicBlock* tempblock = cfg->basicBlocks[b_index];
			for (int sen_index = 0; sen_index < tempblock->topoList.size(); sen_index++) {
				if (tempblock->topoList[sen_index]->name == BRANCH_NODE) {
					//接着继续无条件跳转
					BasicBlock* temp_nextb = find_idblock(tempblock->topoList[sen_index]->targets[0]);
					DAGNode* temp_firstnode = temp_nextb->topoList[0];
					if (temp_firstnode->name == BRANCH_NODE && temp_firstnode->inputs.empty()) {
						branch_change = true;
						tempblock->topoList[sen_index]->targets[0] = temp_firstnode->targets[0];
						//减少中间块生命，增加目标块生命
						//block_label_live[temp_nextb->id]--;
						//block_label_live[temp_firstnode->targets[0]]++;
					}
				}
			}

		}
	}

}



void Linearize::del_unreachblock() {
	//第一个节点为无条件跳转的
	for (int b_index = 0; b_index < cfg->basicBlocks.size(); b_index++) {
		DAGNode* firstnode = cfg->basicBlocks[b_index]->topoList[0];
		if (firstnode->name == BRANCH_NODE && firstnode->inputs.empty()) {
			//并且减少里面所有branch节点带来的live影响
			//for (DAGNode* node_iter : cfg->basicBlocks[b_index]->topoList) {
			//	if (node_iter->name == BRANCH_NODE) {
			//		block_label_live[node_iter->targets[0]]--;
			//	}
			//}
			//
			cfg->basicBlocks[b_index]->label = BasicBlock::LabelType::NO_USE;
			cfg->basicBlocks.erase(cfg->basicBlocks.begin() + b_index);
			b_index--;

		}
	}
}




//可能有多层尾部无条件跳转
void Linearize::del_enduncon_branch() {

	for (int b_index = 0; b_index < cfg->basicBlocks.size(); b_index++) {
		if (cfg->basicBlocks[b_index]->topoList.empty()) continue;//已被删完
		DAGNode* tempend = cfg->basicBlocks[b_index]->topoList.back();
		//无条件跳转
		if (tempend->name == BRANCH_NODE) {
			if (tempend->inputs.empty()) {
				//马上跳转到下一块
				if (tempend->targets[0] == cfg->basicBlocks[b_index + 1]->id) {
					cfg->basicBlocks[b_index]->topoList.pop_back();
					//block_label_live[tempend->targets[0]]--;//失活branch对块的影响
					b_index--;//可能有多层无条件跳转
				}
			}
		}
	}

}



INSTRUCTIONS anti_br(INSTRUCTIONS br) {
	INSTRUCTIONS ans;
	switch (br)
	{
		//
	case BEQ:ans = BNE; break;
	case BNE:ans = BEQ; break;
	case BLT:ans = BGE; break;
	case BGE:ans = BLT; break;
	case BLTU:ans = BGEU; break;
	case BGEU:ans = BLTU; break;
	case BGT:ans = BLE; break;
	case BLE:ans = BGT; break;
	case BGTU:ans = BLEU; break;
	case BLEU:ans = BGTU; break;

		//
	case BEQZ:ans = BNEZ; break;
	case BNEZ:ans = BEQZ; break;
	case BLEZ:ans = BGTZ; break;
	case BGEZ:ans = BLTZ; break;
	case BLTZ:ans = BGEZ; break;
	case BGTZ:ans = BLEZ; break;

		//
	default:
		ans = br;
		break;
	}
	return ans;
}


//可能有多层尾部双跳转
void Linearize::change_doubleendbr() {

	for (int b_index = 0; b_index < cfg->basicBlocks.size(); b_index++) {
		vector<DAGNode*>& nodes = cfg->basicBlocks[b_index]->topoList;
		if (nodes.size()<2) continue;//已被删完
		else {
			DAGNode* tempend = nodes[nodes.size() - 1];
			DAGNode* tempeve = nodes[nodes.size() - 2];
			//尾部两个跳转
			if (tempend->name == BRANCH_NODE && tempeve->name == BRANCH_NODE) {
				if (tempeve->inputs.empty()) continue;//无条件跳转，换不得
				int nextid = cfg->basicBlocks[b_index + 1]->id;
				//有一个无条件跳转才行
				//经过delend之后end不可能是直接到后块的
				if (tempend->inputs.empty() && tempeve->targets[0] == nextid) {
					tempeve->targets[0] = tempend->targets[0];
					tempeve->inst = anti_br(tempeve->inst);
					nodes.pop_back();
					//block_label_live[nextid]--;
				}
			}
		}
	}

}



void Linearize::mergenodes() {
	//最后一个是end块
	for (int b_index = 0; b_index < cfg->basicBlocks.size()-1; b_index++) {
		BasicBlock* tempblock = cfg->basicBlocks[b_index];
		if (tempblock->id == -1) continue;

		//live为0的不可达块已经被删除
		if (block_label_live[tempblock->id] > 0) {
			//if (block_label_live[tempblock->id == -1]) continue;
			if (tempblock->topoList.empty()) {
				//blockend无条件跳转
				block_label[tempblock->id] = ".Label_" + cfg->name + "_B" + to_string(blockcount);
				//nodes的index对应的块名不变
				// 当前块合后一块的名字相同，位置相同，被一起使用（如果后一个块live）
				//blockcount++
			}
			else {
				//给块命名
				block_label[tempblock->id] = ".Label_" + cfg->name + "_B" + to_string(blockcount);
				DAGNode* labelnode = new DAGNode(".Label_" + cfg->name + "_B" + to_string(blockcount), VALUE_NODE);
				//给label记录语句位置
				//label_location[nodescount] = cfg->name + "_B" + to_string(blockcount);
				//label_trigger.insert(nodescount);
				//插入label节点
				linear_nodes.push_back(labelnode);
				nodescount++;
				blockcount++;
			}
		}
		//插入节点
		linear_nodes.insert(linear_nodes.end(), tempblock->topoList.begin(), tempblock->topoList.end());
		nodescount += tempblock->topoList.size();
	}

	if (block_label_live[-1] > 0) {
		DAGNode* endlabel = new DAGNode(".Label_" + cfg->name + "_END", VALUE_NODE);
		block_label[-1] = ".Label_" + cfg->name + "_END";
		linear_nodes.push_back(endlabel);
		nodescount++;
	}





}





void Linearize::second_travel() {


	//计算节点名到物理寄存器编号
	//for (auto color_iter : ig->color) {
	//	n2r[color_iter.first] = ig->registers[color_iter.second];
	//}


	//for (int sen_index = 0; sen_index < linear_nodes.size(); sen_index++) {
	for (int b_index = 0; b_index < cfg->basicBlocks.size(); b_index++) {
		for (int sen_index = 0; sen_index < cfg->basicBlocks[b_index]->topoList.size(); sen_index++) {
			vector<DAGNode*>& nodes = cfg->basicBlocks[b_index]->topoList;
			DAGNode* sen_iter = nodes[sen_index];
			//合并干涉图节点的move操作
			if (sen_iter->inst == MV) {
				string reg1, reg2;
				if (sen_iter->inputs[0]->nodeType == VALUE_NODE) reg1 = sen_iter->inputs[0]->name;
				else if (sen_iter->inputs[0]->nodeType == CONSTANT && sen_iter->inputs[0]->name == "%zero") continue;
				else reg1 = "%" + to_string(sen_iter->inputs[0]->resultId);

				if (sen_iter->inputs[1]->nodeType == VALUE_NODE)  reg2 = sen_iter->inputs[1]->name;
				else if (sen_iter->inputs[1]->nodeType == CONSTANT && sen_iter->inputs[1]->name == "%zero") continue;
				else reg2 = "%" + to_string(sen_iter->inputs[1]->resultId);

				map<int,int>::const_iterator reg1_it = iregisters.find(ig->color[reg1]);
				map<int,int>::const_iterator reg2_it = iregisters.find(ig->color[reg2]);

				color_rec[reg1] = reg1_it->second;
				color_rec[reg2] = reg2_it->second;

				if ((*reg1_it).second == (*reg2_it).second) {
					//sen_iter->name = "%NOP";
					nodes.erase(nodes.begin() + sen_index);
					sen_index--;
				}
			}
			else if (sen_iter->inst == FMVS) {
				string reg1, reg2;
				if (sen_iter->inputs[0]->nodeType == VALUE_NODE) reg1 = sen_iter->inputs[0]->name;
				else if (sen_iter->inputs[0]->nodeType == CONSTANT && sen_iter->inputs[0]->name == "%zero") continue;
				else	reg1 = "%" + to_string(sen_iter->inputs[0]->resultId);

				if (sen_iter->inputs[1]->nodeType == VALUE_NODE)  reg2 = sen_iter->inputs[1]->name;
				else if (sen_iter->inputs[1]->nodeType == CONSTANT && sen_iter->inputs[1]->name == "%zero") continue;
				else reg2 = "%" + to_string(sen_iter->inputs[1]->resultId);
				map<int,int>::const_iterator  reg1_it = fregisters.find(ig->color[reg1]);
				map<int,int>::const_iterator  reg2_it = fregisters.find(ig->color[reg2]);

				if ((*reg1_it).second == (*reg2_it).second) {
					//sen_iter->name = "%NOP";
					nodes.erase(nodes.begin() + sen_index);
					sen_index--;
				}
			}
			//删除ret节点
			else if (sen_iter->name == RETURN_NODE) {
				nodes.erase(nodes.begin() + sen_index);
				sen_index--;
			}
		}

	}

}


void Linearize::del_ldst() {
	/*
		<color, addr>
	*/
	map<int, int> load_rec;
	map<int, int> li_rec;
	map<int, int>  store_rec;

	//注意数组地址运算
	for (int s_index = 0; s_index < linear_nodes.size(); s_index++) {
		DAGNode* sen_iter = linear_nodes[s_index];
		int stack_addr;
		int regcolor;
		//立即数load
		if (sen_iter->inst == LI) {
			stack_addr = sen_iter->value.iValue;//实际上是立即数值
			regcolor= color_rec["%" + to_string(sen_iter->resultId)];
			if (li_rec.find(regcolor) != li_rec.end()) {
				//该寄存器已经预先存入立即数
				if (li_rec[regcolor] == stack_addr) {
					//重复LOAD
					linear_nodes.erase(linear_nodes.begin() + s_index);
					s_index--;
					continue;
				}
				else {
					//更新寄存器内容
					li_rec[regcolor] = stack_addr;
				}
			}
			else {
				//更新寄存器内容
				li_rec[regcolor] = stack_addr;
			}
		}
		//内存load
		else if (sen_iter->nodeType == LOAD) {
			//先不做数组的
			regcolor = color_rec["%" + to_string(sen_iter->resultId)];
			if (sen_iter->inputs.empty()) {
				//不需要立即数指令
				stack_addr = sen_iter->value.iValue*4;

			}
			else if (sen_iter->inputs.size() == 1 && sen_iter->inputs[0]->inst == LI) {
				//需要立即数指令
				stack_addr = sen_iter->inputs[0]->value.iValue;
			}
			else {
				//接的数组运算
				//判断修改
				if (load_rec.find(regcolor) != load_rec.end() || li_rec.find(regcolor) != li_rec.end()) {
					//清除记录
					load_rec.clear();
					store_rec.clear();
					li_rec.clear();
				}
				continue;
			}

			//寄存器是否有当前栈地址值
			if (load_rec.find(regcolor) != load_rec.end() && store_rec.find(stack_addr)!=store_rec.end()
				&& load_rec[regcolor] == stack_addr && store_rec[stack_addr] == regcolor) {
					//预先load进来了
					linear_nodes.erase(linear_nodes.begin() + s_index);
					s_index--;
					continue;
			}
			else {
				load_rec[regcolor] = stack_addr;
				store_rec[stack_addr] = regcolor;
			}
		}
		//内存store
		else if (sen_iter->nodeType == STORE) {
			//先不做数组的
			DAGNode* srcnode=nullptr;
			//算地址
			if (sen_iter->inputs.size()==1) {
				//不需要立即数指令
				stack_addr = sen_iter->value.iValue * 4;
				srcnode = sen_iter->inputs[0];

			}
			else if (sen_iter->inputs.size() == 2 && sen_iter->inputs[0]->inst==LI) {
				//需要立即数指令
				stack_addr = sen_iter->inputs[0]->value.iValue;
				srcnode = sen_iter->inputs[1];
			}
			else {
				//存回数组
				srcnode = sen_iter->inputs[1];
				//找寄存器
				if (srcnode->nodeType == VALUE_NODE) {
					regcolor = color_rec[srcnode->name];
				}
				else {
					regcolor = color_rec["%" + to_string(srcnode->resultId)];
				}
				//判断修改
				if (load_rec.find(regcolor) != load_rec.end() || li_rec.find(regcolor) != li_rec.end()) {
					//清除记录
					load_rec.clear();
					store_rec.clear();
					li_rec.clear();
				}
				continue;
			}



			//找寄存器
			if (srcnode->nodeType == VALUE_NODE) {
				regcolor = color_rec[srcnode->name];
			}
			else {
				regcolor = color_rec["%" + to_string(srcnode->resultId)];
			}

			//栈上内容
			if (load_rec.find(regcolor) != load_rec.end() && store_rec.find(stack_addr) != store_rec.end()
				&& load_rec[regcolor] == stack_addr && store_rec[stack_addr] == regcolor) {
				//预先store回去了
				linear_nodes.erase(linear_nodes.begin() + s_index);
				s_index--;
				continue;
			}
			else {
				load_rec[regcolor] = stack_addr;
				store_rec[stack_addr] = regcolor;
			}
		}
		//跳转/Label/调用
		else if (sen_iter->name == BRANCH_NODE
			|| sen_iter->nodeType==VALUE_NODE  //其实是Label节点
			|| sen_iter->inst==CALL) {
			//清除记录
			load_rec.clear();
			store_rec.clear();
			li_rec.clear();
		}
		//move和其他操作节点
		else if (sen_iter->nodeType==OP_NODE) {
			DAGNode* srcnode = sen_iter->inputs[0];
			if (sen_iter->inst == MV || sen_iter->inst == FMVS) {
				if (srcnode->nodeType == VALUE_NODE) {
					regcolor = color_rec[srcnode->name];
				}
				else {
					regcolor = color_rec["%" + to_string(srcnode->resultId)];
				}
			}
			else {
				srcnode = sen_iter;
				regcolor = color_rec["%" + to_string(srcnode->resultId)];
			}

			//判断修改
			if (load_rec.find(regcolor) != load_rec.end() || li_rec.find(regcolor)!= li_rec.end()) {
				//清除记录
				load_rec.clear();
				store_rec.clear();
				li_rec.clear();
			}

		}
	}

}



void Linearize::del_ldst2() {

	//addrreg和immreg不冲突，否则置reg值为-1，直到再次被赋值
	//重新记录新寄存器时要考虑是否冲突
	//假设：若寄存器编号不为-1，则表示记录正确值
	int last_reg=-1;
	int last_addr=-1;
	int last_imm_reg=-1;
	int last_imm=-1;
	

	//注意数组地址运算
	for (int s_index = 0; s_index < linear_nodes.size(); s_index++) {
		DAGNode* sen_iter = linear_nodes[s_index];
		int stack_addr;
		int regcolor;
		//立即数load
		if (sen_iter->inst == LI) {
			stack_addr = sen_iter->value.iValue;//实际上是立即数值
			regcolor = color_rec["%" + to_string(sen_iter->resultId)];
			if (regcolor==last_imm_reg ) {
				if (last_imm == stack_addr) {
					//该寄存器已经预先存入立即数
					linear_nodes.erase(linear_nodes.begin() + s_index);
					s_index--;
					continue;
				}
				else {
					last_imm = stack_addr;
				}
			}
			else {
				//更新寄存器内容
				last_imm_reg = regcolor;
				last_imm = stack_addr;
				if (last_reg == regcolor)
					last_reg = -1;
			}
		}
		//内存load
		else if (sen_iter->nodeType == LOAD) {
			//先不做数组的
			regcolor = color_rec["%" + to_string(sen_iter->resultId)];
			if (sen_iter->inputs.empty()) {
				//不需要立即数指令
				stack_addr = sen_iter->value.iValue * 4;

			}
			else if (sen_iter->inputs.size() == 1 && sen_iter->inputs[0]->inst == LI) {
				//需要立即数指令
				stack_addr = sen_iter->inputs[0]->value.iValue;
			}
			else {
				//接的数组运算
				//判断修改
				if (regcolor == last_reg)
					last_reg = -1;
				if (regcolor == last_imm_reg)
					last_imm_reg = -1;
				continue;
			}

			//寄存器是否有当前栈地址值
			if (regcolor==last_reg) {
				if (last_addr == stack_addr) {
					//预先load进来了
					linear_nodes.erase(linear_nodes.begin() + s_index);
					s_index--;
					continue;
				}
				else {
					last_addr = stack_addr;
				}

			}
			else {
				last_reg = regcolor;
				last_addr = stack_addr;
				if (last_imm_reg == regcolor)
					last_imm_reg = -1;
			}
		}
		//内存store
		else if (sen_iter->nodeType == STORE) {
			//先不做数组的
			DAGNode* srcnode = nullptr;
			//算地址
			if (sen_iter->inputs.size() == 1) {
				//不需要立即数指令
				stack_addr = sen_iter->value.iValue * 4;
				srcnode = sen_iter->inputs[0];

			}
			else if (sen_iter->inputs.size() == 2 && sen_iter->inputs[0]->inst == LI) {
				//需要立即数指令
				stack_addr = sen_iter->inputs[0]->value.iValue;
				srcnode = sen_iter->inputs[1];
			}
			else {
				//存回数组
				srcnode = sen_iter->inputs[1];
				//找寄存器
				if (srcnode->nodeType == VALUE_NODE) {
					regcolor = color_rec[srcnode->name];
				}
				else {
					regcolor = color_rec["%" + to_string(srcnode->resultId)];
				}
				//判断修改
								//接的数组运算
				//判断修改
				if (regcolor == last_reg)
					last_reg = -1;
				if (regcolor == last_imm_reg)
					last_imm_reg = -1;
				continue;
			}



			//找寄存器
			if (srcnode->nodeType == VALUE_NODE) {
				regcolor = color_rec[srcnode->name];
			}
			else {
				regcolor = color_rec["%" + to_string(srcnode->resultId)];
			}

			//栈上内容
			if (regcolor==last_reg) {
				if (last_addr == stack_addr) {
					//预先store回去了
					linear_nodes.erase(linear_nodes.begin() + s_index);
					s_index--;
					continue;
				}
				else {
					last_addr = stack_addr;
				}

			}
			else {
				last_reg = regcolor;
				last_addr = stack_addr;
				if (last_imm_reg == regcolor)
					last_imm_reg = -1;
			}
		}
		//跳转/Label/调用
		else if (sen_iter->name == BRANCH_NODE
			|| sen_iter->nodeType == VALUE_NODE  //其实是Label节点
			|| sen_iter->inst == CALL) {
			//清除记录
			last_reg = -1;
			last_imm_reg = -1;
		}
		//move和其他操作节点
		else if (sen_iter->nodeType == OP_NODE) {
			DAGNode* srcnode = sen_iter->inputs[0];
			if (sen_iter->inst == MV || sen_iter->inst == FMVS) {
				if (srcnode->nodeType == VALUE_NODE) {
					regcolor = color_rec[srcnode->name];
				}
				else {
					regcolor = color_rec["%" + to_string(srcnode->resultId)];
				}
			}
			else {
				srcnode = sen_iter;
				regcolor = color_rec["%" + to_string(srcnode->resultId)];
			}

			//判断修改
			if (regcolor == last_reg)
				last_reg = -1;
			if (regcolor == last_imm_reg)
				last_imm_reg = -1;
		}
	}

}


void Linearize::mergeImmInstruction() {
	//遍历各节点
	for (int index = 0; index < linear_nodes.size() - 1; index++) {
		//两个addi看一个的输入是否是另一个输出,如果是则合并
		if (linear_nodes[index]->inst == ADDIW) {
			if (linear_nodes[index + 1]->inst == ADDIW) {
				DAGNode* addi1 = linear_nodes[index];
				DAGNode* addi2 = linear_nodes[index + 1];

				int rd_addi1 = 0;
				int input_addi2 = 0;

				//计算addi1的输出和addi2的输入是否相等
				auto reg = iregisters.find(ig->color["%" + to_string(addi1->resultId)]);
				if (reg != iregisters.end()) rd_addi1 = (*reg).second;

				if (addi2->inputs[0]->nodeType == VALUE_NODE) {
					auto reg = iregisters.find(ig->color[addi2->inputs[0]->name]);
					if (reg != iregisters.end()) input_addi2 = (*reg).second;
				}
				else {
					auto reg = iregisters.find(ig->color["%" + to_string(addi2->inputs[0]->resultId)]);
					if (reg != iregisters.end()) input_addi2 = (*reg).second;
				}

				if (rd_addi1 == input_addi2) {
					int imm = addi1->value.iValue + addi2->value.iValue;
					addi2->value.iValue = imm;
					addi2->inputs[0] = addi1->inputs[0];
					linear_nodes.erase(linear_nodes.begin() + index);
					index--;
				}
			}
		}

		//立即数指令立即数为常数0时候，如果输入和输出的寄存器一样，删除这条语句，否则改成move
		if (linear_nodes[index]->inst == ADDIW || linear_nodes[index]->inst == SLLIW || linear_nodes[index]->inst == SRLIW || linear_nodes[index]->inst == SRAIW) {
			if (linear_nodes[index]->value.iValue == 0) {
				int rd = 0;
				int input = 0;

				//计算节点的输入和输出寄存器是否一样
				auto reg = iregisters.find(ig->color["%" + to_string(linear_nodes[index]->resultId)]);
				if (reg != iregisters.end()) rd = (*reg).second;

				if (linear_nodes[index]->inputs[0]->nodeType == VALUE_NODE) {
					auto reg = iregisters.find(ig->color[linear_nodes[index]->inputs[0]->name]);
					if (reg != iregisters.end()) input = (*reg).second;
				}
				else {
					auto reg = iregisters.find(ig->color["%" + to_string(linear_nodes[index]->inputs[0]->resultId)]);
					if (reg != iregisters.end()) input = (*reg).second;
				}

				if (rd == input && rd != 0) {
					linear_nodes.erase(linear_nodes.begin() + index);
				}
				else {
					linear_nodes[index]->inst = MV;
				}

			}
		}

		//
	}
}



void Linearize::third_travel() {

	
	for (int b_index = 0; b_index < cfg->basicBlocks.size(); b_index++) {
		for (int sen_index = 0; sen_index < cfg->basicBlocks[b_index]->topoList.size(); sen_index++) {
			vector<DAGNode*>& nodes = cfg->basicBlocks[b_index]->topoList;
			DAGNode* sen_iter = nodes[sen_index];
			//记录块的生命周期
			if (sen_iter->name == BRANCH_NODE) {
				block_label_live[sen_iter->targets[0]]++;
			}
		}

	}

}