#include <algorithm>
#include "ir_basic_block.hpp"
#include "ir_module.hpp"
#include "ir_function.hpp"
#include "ir_instruction.hpp"

BasicBlock::BasicBlock(Module* module, const string& name, Function* parent)
    : Value(module->label_type, name), parent(parent) {
    parent->add_bb(this);
}

bool BasicBlock::add_inst_front(Instruction* inst) {
    if(!inst->pos_in_bb.empty()) {
        return false; // 指令已经在其他基本块中
    }
    inst_list.push_front(inst);
    inst->pos_in_bb.emplace_back(inst_list.begin());
    inst->parent = this;
    return true;
}

bool BasicBlock::add_inst_back(Instruction* inst) {
    if(!inst->pos_in_bb.empty()) {
        return false; // 指令已经在其他基本块中
    }
    inst_list.push_back(inst);
    inst->pos_in_bb.emplace_back(--inst_list.end());
    inst->parent = this;
    return true;
}

bool BasicBlock::add_inst_before_inst(Instruction* new_inst, Instruction* inst) {
    if(!inst || inst->pos_in_bb.size() != 1 || inst->parent != this) {
        return false; // 指令不属于当前基本块
    }
    if(!new_inst->pos_in_bb.empty()) {
        return false; // 新指令已经在其他基本块中
    }
    if(inst_list.empty()) {
        return false; // 基本块中没有指令
    }
    auto it = inst->pos_in_bb.back();
    inst_list.emplace(it, new_inst);
    new_inst->pos_in_bb.emplace_back(--it);
    new_inst->parent = this;
    return true;
}

bool BasicBlock::add_inst_before_terminator(Instruction* new_inst) {
    if(!new_inst->pos_in_bb.empty()) {
        return false; // 新指令已经在其他基本块中
    }
    if(inst_list.empty()) {
        return false; // 基本块中没有指令
    }
    auto terminator = get_terminator();
    if(!terminator) {
        return false; // 基本块没有终结指令
    }
    return add_inst_before_inst(new_inst, terminator);
}

void BasicBlock::add_pre_basic_block(BasicBlock* bb) {
    pre_bbs.push_back(bb);
}

void BasicBlock::add_succ_basic_block(BasicBlock* bb) {
    succ_bbs.push_back(bb);
}

void BasicBlock::remove_pre_basic_block(BasicBlock* bb) {
    pre_bbs.erase(remove(pre_bbs.begin(), pre_bbs.end(), bb), pre_bbs.end());
}

void BasicBlock::remove_succ_basic_block(BasicBlock* bb) {
    succ_bbs.erase(remove(succ_bbs.begin(), succ_bbs.end(), bb), succ_bbs.end());
}

bool BasicBlock::is_dominated_by(BasicBlock* bb) {
    if(this == bb) {
        return true;
    }
    BasicBlock *cur = this->idom;
    while(cur) {
        if(cur == bb) {
            return true;
        }
        cur = cur->idom;
    }
    return false;
}

Instruction* BasicBlock::get_terminator() {
    if(inst_list.empty())
        return nullptr;
    switch(inst_list.back()->iid) {
        case IRInstID::Ret:
        case IRInstID::Br:
            return inst_list.back();
        default:
            return nullptr;
    }
}

bool BasicBlock::remove_inst(Instruction* inst) {
    if((!inst) || inst->pos_in_bb.size() != 1 || inst->parent != this) {
        return false;
    }
    this->inst_list.erase(inst->pos_in_bb.back());
    inst->pos_in_bb.clear();
    inst->parent = nullptr;
    return true;
}

bool BasicBlock::delete_inst(Instruction* inst) {
    if((!inst) || inst->pos_in_bb.size() != 1 || inst->parent != this) {
        return false;
    }
    this->inst_list.erase(inst->pos_in_bb.back());
    inst->remove_use_of_ops();
    inst->pos_in_bb.clear();
    inst->parent = nullptr;
    return true;
}