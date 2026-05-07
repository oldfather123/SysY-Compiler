#include "../../include/Instruction.h"
#include "../../include/ir.h"
#include <assert.h>

int dfs_num = 0;

void LLVMIR::CFGInit() {
    for (auto &[defI, bb_map] : function_block_map) {
        //std::cout<<" ---------- a new cfg ---------- "<<std::endl;
        CFG *cfg = new CFG();
        cfg->block_map = &bb_map;
        cfg->function_def = defI;
		cfg->max_label = function_max_label[defI];
		cfg->max_reg = function_max_reg[defI];
        //重置tail_blockid（在SearchB中）
        cfg->BuildCFG();
        llvm_cfg[defI] = cfg;
    }
}

void LLVMIR::SyncMaxInfo() {
    for (auto &[defI, cfg] : llvm_cfg) {
        // 从CFG对象同步到LLVMIR
        function_max_label[defI] = cfg->max_label;
        function_max_reg[defI] = cfg->max_reg;
    }
}

void CFG::SearchB(LLVMBlock B){
    if(B->dfs_id!=0)return;
    B->dfs_id = ++dfs_num;//全局
    block_ids.insert(B->block_id);
    //遍历此基本块的所有指令
    for(auto it = B->Instruction_list.begin(); it != B->Instruction_list.end(); ++it){
        auto &intr = *it;
        //【1】无条件跳转指令
        if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_UNCOND){
            //（1）取出跳转指令的目标标签
            Operand operand = ((BrUncondInstruction*)intr)->GetDestLabel();
            int next_label = ((LabelOperand*)operand)->GetLabelNo();
            LLVMBlock next_block = (*block_map)[next_label];
            //（2）维护G/invG
            G[B->block_id].insert(next_block);
            invG[next_label].insert(B);
            //next_block->comment += ("L" + std::to_string(B->block_id) + ", ");
            //（3）递归调用，搜索它的目标块
            SearchB(next_block);
            //（4）删除当前块中跳转指令之后的所有指令
            if(std::next(it) != B->Instruction_list.end())
                B->Instruction_list.erase(std::next(it), B->Instruction_list.end());
            return;
        }
        //【2】条件跳转指令（与【1】类似，但是有两个目标块，都要维护）
        else if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::BR_COND){
            //（1）记录use_map
            auto use_regs = intr->GetNonResultOperands();
            for(auto &operand:use_regs){
                if(operand->GetOperandType()==BasicOperand::REG){
                    int regno=((RegOperand*)operand)->GetRegNo();
                    use_map[regno].push_back(intr);
                }
            }
            //（2）维护分支关系
            Operand operand1 = ((BrCondInstruction*)intr)->GetTrueLabel();
            Operand operand2 = ((BrCondInstruction*)intr)->GetFalseLabel();
            int true_label = ((LabelOperand*)operand1)->GetLabelNo();
            int false_label = ((LabelOperand*)operand2)->GetLabelNo();
            LLVMBlock true_block = (*block_map)[true_label];
            LLVMBlock false_block = (*block_map)[false_label];

            G[B->block_id].insert(true_block);
            invG[true_label].insert(B);

            G[B->block_id].insert(false_block);
            invG[false_label].insert(B);

            SearchB(true_block);
            SearchB(false_block);

            if(std::next(it) != B->Instruction_list.end())
                B->Instruction_list.erase(std::next(it), B->Instruction_list.end());
            return;
        }
        //ret指令
        else if(intr->GetOpcode()==BasicInstruction::LLVMIROpcode::RET){
            // (1) 记录use_map
            auto use_regs = intr->GetNonResultOperands();
            for(auto &operand:use_regs){
                if(operand->GetOperandType()==BasicOperand::REG){
                    int regno=((RegOperand*)operand)->GetRegNo();
                    use_map[regno].push_back(intr);
                }
            }
            //（2）删除当前块中跳转指令之后的所有指令
            if(std::next(it) != B->Instruction_list.end())
                B->Instruction_list.erase(std::next(it), B->Instruction_list.end());
            // (3) 标记非main函数的唯一ret_block
            if(function_def->GetFunctionName()!="main"){
                tail_blockid=B->block_id;
            }
            return;
        }else{
            //use_map
            auto use_regs = intr->GetNonResultOperands();
            for(auto &operand:use_regs){
                if(operand->GetOperandType()==BasicOperand::REG){
                    int regno=((RegOperand*)operand)->GetRegNo();
                    use_map[regno].push_back(intr);
                }
            }
            //def_map
            int def_regno=intr->GetDefRegno();
            if(def_regno!=-1){
                def_map[def_regno]=intr;
            }
        }
    }
}

void CFG::BuildCFG() {
    G.clear();
    invG.clear();
    block_ids.clear();
    use_map.clear();
    def_map.clear();
    
    // 重置dfs编号
    dfs_num = 0;
    for (auto [id, block] : *block_map) {
        block->dfs_id = 0;
    }

    // 深度优先遍历块内的所有指令，遇到跳转指令记录前置块和后缀块
    // 如果跳转/返回指令之后还有指令，全部删除
    // SearchB()进行递归调用
    LLVMBlock start_block = (*block_map)[0];
    SearchB(start_block);
}

std::set<LLVMBlock> CFG::GetPredecessor(LLVMBlock B) { return invG[B->block_id]; }

std::set<LLVMBlock> CFG::GetPredecessor(int bbid) { return invG[bbid]; }

std::set<LLVMBlock> CFG::GetSuccessor(LLVMBlock B) { return G[B->block_id]; }

std::set<LLVMBlock> CFG::GetSuccessor(int bbid) { return G[bbid]; }

void CFG::GetSSAGraphAllSucc(std::set<int>& succs, int regno){ 
    if(succs.count(regno) > 0) return;
    auto it = SSA_Graph.find(regno);
    if(it != SSA_Graph.end()){
        for(auto &s:it->second){
            succs.insert(s);
            GetSSAGraphAllSucc(succs,s);
        }
    }
    return ;
}

LLVMBlock CFG::GetNewBlock() {
	max_label++;
    (*block_map)[max_label] = new BasicBlock(max_label);
    return GetBlockWithId(max_label);
}

LLVMBlock CFG::GetBlockWithId(int id) {
	if(id <= max_label) return (*block_map)[id];
	else return nullptr;
}

/* what func do: [froms -> to] change to [froms -> mid -> to] 
-------- before replace --------
from.x.block:
	br label.to
to.block:
	%t1 = phi [%i1, from1], [%i2, form2], [%l1, latch1], [%l2, latch2]
-------- after  replace --------
from.x.block:
	br label.mid
mid.block:
	%t_new = phi [%i1, from1], [%i2, form2]
	br label.to
to.block:
	%t1 = phi [%t_new, mid] [%l1, latch1], [%l2, latch2]
	; delete phi, and continue to use t1
*/
void CFG::replaceSuccessors(std::set<LLVMBlock> froms, LLVMBlock to, LLVMBlock mid) {
	auto mid_label_op = GetNewLabelOperand(mid->block_id);
	// 1. replace all froms br label
	for(auto from : froms) {
		auto inst = *prev(from->Instruction_list.end());
		if (auto br_uncond_inst = dynamic_cast<BrUncondInstruction*>(inst)) {
			auto br_label = (LabelOperand*)(br_uncond_inst->GetDestLabel());
			if (br_label->GetLabelNo() == to->block_id) {
				br_uncond_inst->ChangeDestLabel(mid_label_op);
			}
		} else if (auto br_cond_inst = dynamic_cast<BrCondInstruction*>(inst)) {
			auto ture_label = (LabelOperand*)(br_cond_inst->GetTrueLabel());
			auto false_label = (LabelOperand*)(br_cond_inst->GetFalseLabel());
			if (ture_label->GetLabelNo() == to->block_id) {
				br_cond_inst->ChangeTrueLabel(mid_label_op);
			}
			if (false_label->GetLabelNo() == to->block_id) {
				br_cond_inst->ChangeFalseLabel(mid_label_op);
			} 
		}
	}

	// 2. error : mov all phi inst [to -> mid], and delete phi inst at to.block
	// ! Tip : may be phi inst args is come from latches.
	// ! Tip : may be generate phi inst with only one pred, optimize it.
	// Todo : call EOPPhi() to elim one-pred phi inst.
	for (auto inst : to->Instruction_list) {
		if (auto phi_inst = dynamic_cast<PhiInstruction*>(inst)) {
			std::vector<std::pair<Operand, Operand>> phi_list_new;  // store phi_list that mid.block create
			std::vector<std::pair<Operand, Operand>> phi_list_left; // store phi_list that to.block left
			for(auto phi_pair : phi_inst->GetPhiList()) {
				auto label_op = (LabelOperand*)phi_pair.first;
				int label_no = label_op->GetLabelNo();
				LLVMBlock label_block = (*block_map)[label_no];
				if(froms.count(label_block)) {
					phi_list_new.push_back(phi_pair);
				} else {
					phi_list_left.push_back(phi_pair);
				}
			}
			// phi_list_new empty means all phi_label is come from latches, no need to update.
			if(!phi_list_new.empty()) {
				RegOperand* t_new = GetNewRegOperand(++max_reg);
				// std::cerr << "generate new regop with regNo: " << max_reg << std::endl;
				PhiInstruction* phi_inst_new = new PhiInstruction(phi_inst->GetResultType(), t_new, phi_list_new);
				mid->InsertInstruction(1, phi_inst_new);

				phi_list_left.push_back(std::make_pair(mid_label_op, t_new));
				phi_inst->SetPhiList(phi_list_left);
			}
		}
	}

	// 3. mid.block insert br_inst to to.block
	auto br_inst = new BrUncondInstruction(GetNewLabelOperand(to->block_id));
	mid->InsertInstruction(1, br_inst);

	// 4. rebuild cfg, quicker than a lot of addEdge, delEdge operations.
	BuildCFG();
}

void CFG::replaceSuccessor(LLVMBlock from, LLVMBlock to, LLVMBlock mid) {
    // 1. 修改from块的跳转指令，将目标从to改为mid
    auto& from_instructions = from->Instruction_list;
    if (!from_instructions.empty()) {
        auto last_inst = from_instructions.back();
        if (last_inst->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_UNCOND) {
            // 无条件跳转
            auto br_uncond = dynamic_cast<BrUncondInstruction*>(last_inst);
            if (br_uncond) {
                auto label_op = br_uncond->GetDestLabel();
                if (label_op->GetOperandType() == BasicOperand::LABEL) {
                    int label_no = ((LabelOperand*)label_op)->GetLabelNo();
                    if (label_no == to->block_id) {
                        // 创建新的标签操作数，指向mid块
                        auto new_label = GetNewLabelOperand(mid->block_id);
                        br_uncond->SetTarget(new_label);
                    }
                }
            }
        } else if (last_inst->GetOpcode() == BasicInstruction::LLVMIROpcode::BR_COND) {
            // 条件跳转
            auto br_cond = dynamic_cast<BrCondInstruction*>(last_inst);
            if (br_cond) {
                auto true_label = br_cond->GetTrueLabel();
                auto false_label = br_cond->GetFalseLabel();
                
                // 检查并修改true分支
                if (true_label->GetOperandType() == BasicOperand::LABEL) {
                    int label_no = ((LabelOperand*)true_label)->GetLabelNo();
                    if (label_no == to->block_id) {
                        auto new_label = GetNewLabelOperand(mid->block_id);
                        br_cond->SetTrueLabel(new_label);
                    }
                }
                
                // 检查并修改false分支
                if (false_label->GetOperandType() == BasicOperand::LABEL) {
                    int label_no = ((LabelOperand*)false_label)->GetLabelNo();
                    if (label_no == to->block_id) {
                        auto new_label = GetNewLabelOperand(mid->block_id);
                        br_cond->SetFalseLabel(new_label);
                    }
                }
            }
        }
    }
    
    // 2. 修改to块中phi指令的前驱映射，将from改为mid
    for (auto inst : to->Instruction_list) {
        if (inst->isPhi()) {
            auto phi_inst = dynamic_cast<PhiInstruction*>(inst);
            if (phi_inst) {
                auto phi_list = phi_inst->GetPhiList();
                std::vector<std::pair<Operand, Operand>> new_phi_list;
                
                for (auto& [label_op, value_op] : phi_list) {
                    if (label_op->GetOperandType() == BasicOperand::LABEL) {
                        int label_no = ((LabelOperand*)label_op)->GetLabelNo();
                        if (label_no == from->block_id) {
                            // 将前驱从from改为mid
                            auto new_label = GetNewLabelOperand(mid->block_id);
                            new_phi_list.push_back({new_label, value_op});
                        } else {
                            new_phi_list.push_back({label_op, value_op});
                        }
                    } else {
                        new_phi_list.push_back({label_op, value_op});
                    }
                }
                
                phi_inst->SetPhiList(new_phi_list);
            }
        }
    }
}

void CFG::addEdge(LLVMBlock from, LLVMBlock to) {
    G[from->block_id].insert(to);
    invG[to->block_id].insert(from);
}

void CFG::delEdge(LLVMBlock from, LLVMBlock to) {
    int from_id = from->block_id;
    int to_id = to->block_id;

    auto &succ_set = G[from_id];
    succ_set.erase(to);

    auto &pred_set = invG[to_id];
    pred_set.erase(from);
}

void CFG::display(bool reverse) {
    std::cout << "\n=== Control Flow Graph Information ===" << std::endl;
    if (!reverse) {
        // 显示后继块信息
        for(size_t i = 0; i < this->G.size(); i++) {
            if(this->G[i].empty()) continue;
            std::cout << "Block " << i << " 的后继块: ";
            for(auto succ : this->G[i]) {
                std::cout << succ->block_id << " ";
            }
            std::cout << std::endl;
        }
    } else {
        // 显示前驱块信息
        for(size_t i = 0; i < this->invG.size(); i++) {
            if(this->invG[i].empty()) continue;
            std::cout << "Block " << i << " 的前驱块: ";
            for(auto pred : this->invG[i]) {
                std::cout << pred->block_id << " ";
            }
            std::cout << std::endl;
        }
    }
    std::cout << "=== End of Control Flow Graph ===\n" << std::endl;
}

void CFG::reSetBlockID() {
    for (auto& [block_id, block] : *block_map) {
        for (Instruction inst : block->Instruction_list) {
            inst->SetBlockID(block_id);
        }
    }
}