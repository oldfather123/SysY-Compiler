
#include "MachineBasicBlock.hpp"
#include "MachineFunction.hpp"
#include "MachineInstr.hpp"
#include "Operand.hpp"
#include "exitcode.hpp"
#include <list>
#include <memory>
MachineBasicBlock::MachineBasicBlock(BasicBlock *IR_bb,
                                     std::weak_ptr<MachineFunction> parent,
                                     std::string name) {
    this->IR_bb = IR_bb;
    this->parent = parent;
    if (IR_bb != nullptr) {
        this->name = parent.lock()->get_name() + "_" + IR_bb->get_name();
    } else {
        this->name = parent.lock()->get_name() + name;
    }
}

std::vector<std::weak_ptr<MachineBasicBlock>> &
MachineBasicBlock::get_succ_basic_blocks() {
    return succs;
}

std::vector<std::weak_ptr<MachineBasicBlock>> &
MachineBasicBlock::get_pre_basic_blocks() {
    return preds;
}

std::weak_ptr<MachineFunction> MachineBasicBlock::get_parent() const {
    return parent;
}

void MachineBasicBlock::insert_instr(
    std::shared_ptr<MachineInstr> instr,
    std::list<std::shared_ptr<MachineInstr>>::iterator it) {
    instrs.insert(it, instr);
}

std::list<std::shared_ptr<MachineInstr>>::iterator
MachineBasicBlock::erase_instr(
    std::list<std::shared_ptr<MachineInstr>>::iterator it) {
    return instrs.erase(it);
}

std::list<std::shared_ptr<MachineInstr>>::iterator
MachineBasicBlock::get_instrs_begin() {
    return instrs.begin();
}

std::list<std::shared_ptr<MachineInstr>>::iterator
MachineBasicBlock::get_instrs_end() {
    return instrs.end();
}

void MachineBasicBlock::clear_instrs() { instrs.clear(); }
void MachineBasicBlock::add_succ_basic_block(
    std::weak_ptr<MachineBasicBlock> succ) {
    ASSERT(succ.lock() != nullptr && "succ is nullptr");
    succs.push_back(succ);
}
void MachineBasicBlock::add_pre_basic_block(
    std::weak_ptr<MachineBasicBlock> pred) {
    ASSERT(pred.lock() != nullptr && "pred is nullptr");
    preds.push_back(pred);
}
std::list<std::shared_ptr<MachineInstr>> &MachineBasicBlock::get_instrs() {
    return instrs;
}
BasicBlock *MachineBasicBlock::get_IR_basic_block() const {
    ASSERT(IR_bb != nullptr && "IR_basic_block is nullptr");
    return IR_bb;
}
std::string MachineBasicBlock::get_name() const { return name; }
std::string MachineBasicBlock::print() const {
    std::string ret;
    ret += get_name() + ":\n";
    for (auto &instr : instrs) {
        if (instr->print() != "")
            ret += "\t" + instr->print();
    }
    return ret;
}

RegisterSet MachineBasicBlock::get_def() {
    RegisterSet uses;
    RegisterSet defs;
    for (auto &instr : instrs) {
        for (auto &use : instr->get_use()) {
            uses.insert(use);
        }
        for (auto &def : instr->get_def()) {
            if (uses.find(def) == uses.end()) {
                defs.insert(def);
            }
        }
    }
    return defs;
}

RegisterSet MachineBasicBlock::get_use() {
    RegisterSet uses;
    RegisterSet defs;
    for (auto &instr : instrs) {
        for (auto &use : instr->get_use()) {
            if (defs.find(use) == defs.end()) {
                uses.insert(use);
            }
        }
        for (auto &def : instr->get_def()) {
            defs.insert(def);
        }
    }
    return uses;
}