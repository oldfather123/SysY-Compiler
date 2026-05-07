#include "opt.hpp"

void remove_phi_in_succ(BasicBlock* bb, BasicBlock* succ_bb) {
    unordered_set<Instruction*> phi_to_delete;
    for(auto inst: succ_bb->inst_list) {
        if(auto phi_inst = dynamic_cast<PhiInst*>(inst)) { // 遍历找到 Phi 指令
            for(int i = 1; i < phi_inst->operands.size(); i += 2) {
                if(phi_inst->get_op(i) == bb) {
                    phi_inst->remove_ops(i - 1, i);
                    break;
                }
            }
            if(phi_inst->parent->pre_bbs.size() == 1) {
                if(phi_inst->operands.size() == 2) {
                    Value* only_val = phi_inst->get_op(0);
                    phi_inst->replace_use_with(only_val);
                    phi_to_delete.insert(phi_inst);
                } else {
                    cerr << "[ERROR] remove_phi_in_succ: PhiInst has more than one operand after removing.\n";
                    exit(1);
                }
            }
        }
    }
    for(auto inst: phi_to_delete) {
        succ_bb->delete_inst(inst);
    }
}

void dfs_cfg(BasicBlock* bb, unordered_set<BasicBlock*>& visited) {
    if(!bb) {
        cerr << "[ERROR] dfs_cfg: BasicBlock is null.\n";
        exit(1);
    }
    visited.insert(bb);
    for(auto succ_bb: bb->succ_bbs) {
        if(!visited.count(succ_bb)) {
            dfs_cfg(succ_bb, visited);
        }
    }
}

void delete_unreachable_blocks(Function* func) {
    unordered_set<BasicBlock*> visited;
    unordered_set<BasicBlock*> bb_to_delete;
    for(auto bb: func->bb_list) {
        if(bb->name == "label_entry") {
            dfs_cfg(bb, visited);
            break;
        }
    }
    for(auto bb: func->bb_list) {
        if(!visited.count(bb)) {
            bb_to_delete.insert(bb);
        }
    }
    for(auto bb: bb_to_delete) {
        func->remove_bb(bb);
        for(auto suc: bb->succ_bbs) {
            remove_phi_in_succ(bb, suc);
        }
    }
}