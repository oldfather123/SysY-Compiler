#include "CSE.hpp"

bool CSE::is_available_for_cse(Instruction* inst) {
    return inst->is_add() || inst->is_sub() || inst->is_mul() || inst->is_div() || inst->is_rem() ||    // 整型二元运算
            inst->is_fadd() || inst->is_fsub() || inst->is_fmul() || inst->is_fdiv() ||                 // 浮点型二元运算
            // inst->is_sitofp() || inst->is_fptosi() || inst->is_zext() || inst->is_bitcast() ||          // 类型转换
            inst->is_shl() || inst->is_lshr() ||inst->is_ashr() ||                                      // 位运算
            inst->is_icmp() || inst->is_fcmp() || inst->is_gep();                                       // 比较和 GEP 指令
    // 不可以进行 CSE 的指令类型：ret, br, store, alloca, phi
    // 经过一定分析后可以进行 CSE 的指令类型：load(确认地址不变且无 alias), call(确认为纯函数)
}

void CSE::run_cse_in_block(BasicBlock* bb) {
    // cerr << "\n====================  BasicBlock: " << bb->name << "  ====================\n";
    unordered_map<ExpKey, Value*> exp_map; // 开始前清空上一次操作带来的数据
    vector<Instruction*> inst_to_delete; // 需要删除的指令
    for(auto inst: bb->inst_list) {
        if(is_available_for_cse(inst)) {
            // cerr << "[Info] found CSE candidate: " << inst->print() << "\n";
            ExpKey key(inst->iid, inst->operands);
            auto it = exp_map.find(key);
            if(it != exp_map.end()) {
                // cerr << "[Info] already in the exp_map\n";
                Value* existing_value = it->second;
                if(existing_value == inst) {
                    // cerr << "[ERROR] inst " << inst->print() << " already exists in exp_map with the same value, please check SSA!\n";
                }
                // cerr << "[Info] replacing " << inst->print() << " with " << existing_value->print() << "\n";
                inst_to_delete.push_back(inst);
                inst->replace_use_with(existing_value); // 替换所有使用该指令的引用
            } else {
                // cerr << "[Info] inserting new expression: " << inst->print() << "\n";
                exp_map[key] = inst;
            }
        }
    }
    if(exp_map.size() <= 1) { // 无需进行 CSE
        // cerr << "[Info] zero or only one expression in exp_map, no CSE needed.\n";
        return;
    }
    // cerr << "[Info] current exp_map size: " << exp_map.size() << "\n";
    for(auto inst: inst_to_delete) {
        // cerr << "[Info] deleting instruction: " << inst->print() << "\n";
        if(!bb->delete_inst(inst)) {
            // cerr << "[Error] failed to delete instruction: " << inst->print() << "\n";
        }
    }
    return;
}

void CSE::run_global_cse(Function* func) {
    // cerr << "\n====================  Function: " << func->name << "  ====================\n";
    unordered_map<ExpKey, vector<ExpInfo>> exp_map; // 存储全局公共子表达式
    for(auto bb: func->bb_list) {
        for(auto inst: bb->inst_list) {
            if(is_available_for_cse(inst)) {
                ExpKey key(inst->iid, inst->operands);
                ExpInfo exp_info(inst, bb, key);
                // cerr << "[Info] global CSE candidate: " << inst->print() << "\n";
                if(exp_map.find(key) != exp_map.end()) {
                    // cerr << "[Info] already in the exp_map: " << inst->print() << "\n";
                } else {
                    // cerr << "[Info] inserting new expression: " << inst->print() << "\n";
                }
                exp_map[key].push_back(exp_info);
            }
        }
    }
}

void CSE::execute() {
    for(auto func: m->func_list) {
        if(func->bb_list.empty()) {
            continue; // 跳过函数声明
        }
        // cerr << "\n---------------------------------------------  Function: " << func->name << "  ---------------------------------------------\n";
        for(auto bb: func->bb_list) {
            run_cse_in_block(bb);
        }
        // run_global_cse(func);
    }
}