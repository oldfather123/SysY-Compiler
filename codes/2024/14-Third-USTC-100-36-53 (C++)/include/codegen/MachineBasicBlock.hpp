#pragma once

#include "BasicBlock.hpp"

#include <algorithm>
#include <list>
#include <memory>
#include <unordered_set>
#include <vector>

class Register;
class MachineBasicBlock;
class Operand;
class MachineInstr;
class MachineFunction;
using RegisterSet = std::unordered_set<std::shared_ptr<Register>>;

class MachineBasicBlock
    : public std::enable_shared_from_this<MachineBasicBlock> {
  public:
    MachineBasicBlock(BasicBlock *IR_bb, std::weak_ptr<MachineFunction> parent,
                      std::string name = "");

    void insert_instr(std::shared_ptr<MachineInstr> instr,
                      std::list<std::shared_ptr<MachineInstr>>::iterator it);
    std::list<std::shared_ptr<MachineInstr>>::iterator
    erase_instr(std::list<std::shared_ptr<MachineInstr>>::iterator it);
    std::list<std::shared_ptr<MachineInstr>>::iterator get_instrs_begin();
    std::list<std::shared_ptr<MachineInstr>>::iterator get_instrs_end();
    void clear_instrs();
    template <typename T>
    std::list<std::shared_ptr<MachineInstr>>::iterator find_instr(T pred) {
        return std::find_if(instrs.begin(), instrs.end(), pred);
    }

    void add_succ_basic_block(std::weak_ptr<MachineBasicBlock> succ);
    void add_pre_basic_block(std::weak_ptr<MachineBasicBlock> pred);
    std::vector<std::weak_ptr<MachineBasicBlock>> &get_succ_basic_blocks();
    std::vector<std::weak_ptr<MachineBasicBlock>> &get_pre_basic_blocks();
    std::weak_ptr<MachineFunction> get_parent() const;
    std::list<std::shared_ptr<MachineInstr>> &get_instrs();
    RegisterSet get_def();
    RegisterSet get_use();
    BasicBlock *get_IR_basic_block() const;
    std::string get_name() const;
    std::string print() const;

  private:
    std::string name;
    BasicBlock *IR_bb;
    std::weak_ptr<MachineFunction> parent;
    // std::vector<std::shared_ptr<MachineInstr>> instrs;
    std::list<std::shared_ptr<MachineInstr>> instrs;
    std::vector<std::weak_ptr<MachineBasicBlock>> succs;
    std::vector<std::weak_ptr<MachineBasicBlock>> preds;
};
