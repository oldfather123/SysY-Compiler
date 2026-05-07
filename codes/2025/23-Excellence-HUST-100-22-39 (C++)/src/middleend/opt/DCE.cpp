#include "DCE.hpp"

bool DeadCodeElimination::has_side_effect(Instruction* inst) {
    switch(inst->iid) {
        case IRInstID::Ret:    // 控制流：返回
        case IRInstID::Br:     // 控制流：跳转
        case IRInstID::Call:   // 控制流：函数调用
        case IRInstID::Store:  // 内存：写内存
        case IRInstID::Alloca: // 内存：分配内存
            return true;
        default:
            return false; // 其他指令没有副作用
    }
}

bool DeadCodeElimination::is_dead_code(Instruction* inst) {
    if(has_side_effect(inst)) {
        return false; // 有副作用的指令不能删除
    }
    if(auto phi_inst = dynamic_cast<PhiInst*>(inst)) {
        // cerr << "Here is some Phi instruction with no or less operands\n";
        return phi_inst->operands.size() <= 2; // Phi指令没有操作数时可以删，只有一对操作数时需要进行替换再删
    }
    if(!inst->use_list.empty()) {
        return false; // 被使用的指令不能删除
    }
    return true;
}

vector<Instruction*> DeadCodeElimination::select_dead_code(Function* func) {
    vector<Instruction*> dead_code_candidates;
    for(auto& bb: func->bb_list) {
        for(auto& inst: bb->inst_list) {
            if(is_dead_code(inst)) {
                dead_code_candidates.push_back(inst);
            }
        }
    }
    return dead_code_candidates;
}

void DeadCodeElimination::execute() {
    for(auto func: m->func_list) {
        while(true) { // 循环删除死代码，直到没有更多的死代码
            vector<Instruction*> dead_code_candidates = select_dead_code(func);
            if(dead_code_candidates.empty()) {
                break; // 没有更多的死代码可以删除了
            }
            for(auto inst: dead_code_candidates) {
                // 特殊关照一下 Phi 指令
                if(auto phi_inst = dynamic_cast<PhiInst*>(inst)) {
                    if(phi_inst->operands.size() != 0) { // Phi 指令仅有一对操作数，则可用这对操作数的第一位替换掉所有引用该指令的地方
                        phi_inst->replace_use_with(phi_inst->get_op(0));
                    }
                }
                inst->parent->delete_inst(inst); // 删除指令
            }
        }
    }
}