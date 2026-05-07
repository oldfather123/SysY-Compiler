#include <algorithm>
#include "riscv_basic_block.hpp"
#include "riscv_function.hpp"

RiscvBasicBlock::RiscvBasicBlock(string name, int rbid, RiscvFunction* parent)
    : RiscvValue(RiscvTypeID::RLabelTy, name), parent(parent), rbid(rbid) {
    if(parent) {
        parent->add_rbb(this);
    }
}

void RiscvBasicBlock::add_rinst_front(RiscvInstruction* rinst) {
    if(rinst == nullptr) {
        return;
    }
    rinst_list.push_front(rinst);
}

void RiscvBasicBlock::add_rinst_back(RiscvInstruction* rinst) {
    if(rinst == nullptr) {
        return;
    }
    rinst_list.push_back(rinst);
}

void RiscvBasicBlock::add_rinst_before_rinst(RiscvInstruction* new_rinst, RiscvInstruction* rinst) {
    if(new_rinst == nullptr) {
        return;
    }
    auto it = find(rinst_list.begin(), rinst_list.end(), rinst);
    if(it != rinst_list.end()) { // 找到指定指令
        rinst_list.insert(it, new_rinst);
    } else { // 没有找到指定指令，则直接加入到最后
        add_rinst_back(new_rinst);
    }
}

void RiscvBasicBlock::add_rinst_after_rinst(RiscvInstruction* new_rinst, RiscvInstruction* rinst) {
    if(new_rinst == nullptr || rinst == nullptr) {
        return;
    }
    auto it = find(rinst_list.begin(), rinst_list.end(), rinst);
    if(it != rinst_list.end()) {
        if(next(it) == rinst_list.end()) {
            rinst_list.push_back(new_rinst);
        } else {
            rinst_list.insert(next(it), new_rinst);
        }
    } else {
        add_rinst_back(new_rinst);
    }
}

void RiscvBasicBlock::add_rinst_before_terminator(RiscvInstruction* rinst) {
    if(!rinst) {
        return;
    }
    if(rinst_list.empty()) {
        rinst_list.push_back(rinst);
    } else {
        RiscvInstruction* last_inst = rinst_list.back();
        if(last_inst->riid == RiscvInstID::RET) { // 返回指令，直接插入其前面
            add_rinst_before_rinst(rinst, last_inst);
        } else if(last_inst->riid == RiscvInstID::BGT) { // BGT 指令
            auto op0 = last_inst->get_op(0);
            if(op0 == nullptr) { // 如果 BGT 的第一个操作数为空，直接插入其前面
                add_rinst_before_rinst(rinst, last_inst);
            } else {
                if(rinst_list.size() < 2) {
                    cerr << "[Error] Not enough instructions in the block to add before BGT.\n";
                    exit(1);
                }
                RiscvInstruction* pre_inst = *prev(rinst_list.end(), 2);
                add_rinst_before_rinst(rinst, pre_inst);
            }
        } else {
            add_rinst_before_rinst(rinst, last_inst);
        }
    }
}

void RiscvBasicBlock::delete_rinst(RiscvInstruction* rinst) {
    if(rinst == nullptr) {
        return;
    }
    auto it = find(rinst_list.begin(), rinst_list.end(), rinst);
    if(it == rinst_list.end()) {
        cerr << "[Error] Instruction not found in the basic block.\n";
        exit(1);
    }
    rinst_list.erase(remove(rinst_list.begin(), rinst_list.end(), rinst), rinst_list.end());
}

RiscvInstruction* RiscvBasicBlock::get_rinst_by_index(int index) {
    if(index < 0 || index >= rinst_list.size()) {
        cerr << "[Error] Index out of bounds when accessing instruction in basic block.\n";
        exit(1);
    }
    auto it = rinst_list.begin();
    advance(it, index); // 将迭代器移动到目标位置
    return *it;
}