#include "BasicBlock.hpp"

#include "Function.hpp"
#include "Module.hpp"

#include <cassert>

BasicBlock::BasicBlock(Module *m, const std::string &name = "",
                       Function *parent = nullptr)
    : Value(m->get_label_type(), name), parent_(parent) {
    assert(parent && "currently parent should not be nullptr");
    parent_->add_basic_block(this);
}

Module *BasicBlock::get_module() { return get_parent()->get_parent(); }
void BasicBlock::erase_from_parent() { this->get_parent()->remove(this); }

bool BasicBlock::is_terminated() const {
    if (instr_list_.empty()) {
        return false;
    }
    switch (instr_list_.back().get_instr_type()) {
    case Instruction::ret:
    case Instruction::br:
        return true;
    default:
        return false;
    }
}

Instruction *BasicBlock::get_terminator() {
    assert(is_terminated() &&
           "Trying to get terminator from an bb which is not terminated");
    return &instr_list_.back();
}

void BasicBlock::add_instruction(Instruction *instr) {
    
    assert(not is_terminated() && "Inserting instruction to terminated bb");
    instr_list_.push_back(instr);
}

