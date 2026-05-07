#ifndef __MACHINE_BLOCK_H_
#define __MACHINE_BLOCK_H_

#include "codegen/MachineInstruction.hpp"
#include <memory>
#include <ostream>
#include <set>
#include <vector>
#include <list>

class MachineFunction;
class BasicBlock;

class MachineBlock {
public:
    BasicBlock *IRBlock;
    MachineFunction *parent;
    int no;
    std::vector<MachineBlock *> pred, succ;

    std::list<MachineInstruction *> inst_list;

    std::set<std::string> live_in;
    std::set<std::string> live_out;
    std::set<std::string> f_live_in;
    std::set<std::string> f_live_out;

    std::set<std::string> live_use;
    std::set<std::string> live_def;
    std::set<std::string> f_live_use;
    std::set<std::string> f_live_def;

    int begin_no, end_no;

    BasicBlock *bb;

    MachineBlock(MachineFunction *parent_p, BasicBlock *IR_Block_p) : parent(parent_p), IRBlock(IR_Block_p) {}
    ~MachineBlock() {
        for (auto &inst : inst_list) {
            delete inst;
            inst = nullptr;
        }
    }
    
    void insertInst(MachineInstruction *inst) {
        inst->current_it = inst_list.insert(inst_list.end(), inst);
        inst->parent = this;
    }
    
    void output(std::ostream &os) const;
};

#endif