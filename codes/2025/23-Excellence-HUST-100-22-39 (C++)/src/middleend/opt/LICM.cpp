#include <set>
#include <stack>
#include <algorithm>
#include "LICM.hpp"

void LICM::init_loop_invar(Function* func, LoopPtr loop) {
    for(auto bb: func->bb_list) {
        if(find(loop->body.begin(), loop->body.end(), bb) == loop->body.end()) {
            for(auto inst: bb->inst_list) { // 如果当前基本块不在循环体中，则将其所有指令标记为循环不变量
                loop->is_invar[inst] = true;
            }
        } else {
            for(auto inst: bb->inst_list) { // 如果当前基本块在循环体中，则将其所有指令暂时标记为非循环不变量
                loop->is_invar[inst] = false;
            }
        }
    }
}

void LICM::execute() {
    for(auto& [func, loops]: loops) {
        for(auto& loop: *loops) {
            vector<Instruction*> invars;
            init_loop_invar(func, loop); // 初始化循环不变量
            for(auto bb: loop->body) {
                if(!loop->is_always_executed(bb)) { // 如果基本块不在循环中总是执行，则跳过
                    continue;
                }
                for(auto inst: bb->inst_list) {
                    if(!loop->is_invariant(inst)) { // 如果不是循环不变量，则跳过
                        loop->is_invar[inst] = false;
                        continue;
                    }
                    loop->is_invar[inst] = true;
                    invars.push_back(inst);
                }
            }
            // 批量修改
            for(auto inst: invars) {
                inst->parent->remove_inst(inst); // 从原基本块中移除指令
                loop->pre_header->add_inst_before_terminator(inst); // 将循环不变量指令添加到 pre_header
            }
        }
    }
}