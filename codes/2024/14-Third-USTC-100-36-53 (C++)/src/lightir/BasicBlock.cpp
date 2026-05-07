#include "BasicBlock.hpp"
#include "Function.hpp"
#include "IRprinter.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "exitcode.hpp"

#include <iostream>

BasicBlock::BasicBlock(Module *m, const std::string &name = "",
                       Function *parent = nullptr)
    : Value(m->get_label_type(), name), parent_(parent) {
    ASSERT(parent && "currently parent should not be nullptr");
    parent_->add_basic_block(this);
}

Module *BasicBlock::get_module() { return get_parent()->get_parent(); }
void BasicBlock::erase_from_parent() { this->get_parent()->remove(this); delete this;}

bool BasicBlock::is_terminated() const {
    if (instr_list_.empty())
        return false;
    switch (instr_list_.back().get_instr_type()) {
    case Instruction::ret:
    case Instruction::br:
        return true;
    default:
        return false;
    }
}

Instruction *BasicBlock::get_terminator() {
    ASSERT(is_terminated() &&
           "Trying to get terminator from an bb which is not terminated");
    return &instr_list_.back();
}

void BasicBlock::add_instruction(Instruction *instr) {
    ASSERT(not is_terminated() && "Inserting instruction to terminated bb");
    instr_list_.push_back(instr);
    instr->set_parent(this);
}

void BasicBlock::add_instr_before_terminator(Instruction *instr) {
    assert(this->is_terminated());
    for (auto it = this->get_instructions().begin();
         it != this->get_instructions().end(); ++it) {
        if (it->is_terminator()) {
            this->get_instructions().insert(it, instr);
            break;
        }
    }
    instr->set_parent(this);
}

std::string BasicBlock::print() {
    std::string bb_ir;
    bb_ir += this->get_name();
    bb_ir += ":";
    // print prebb
    if (!this->get_pre_basic_blocks().empty()) {
        bb_ir += "                                                ; preds = ";
    }
    for (auto bb : this->get_pre_basic_blocks()) {
        if (bb != *this->get_pre_basic_blocks().begin())
            bb_ir += ", ";
        bb_ir += print_as_op(bb, false);
    }

    // print prebb
    if (!this->get_parent()) {
        bb_ir += "\n";
        bb_ir += "; Error: Block without parent!";
    }
    bb_ir += "\n";
    for (auto &instr : this->get_instructions()) {
        bb_ir += "  ";
        bb_ir += instr.print();
        bb_ir += "\n";
    }

    return bb_ir;
}
