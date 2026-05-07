#ifndef __MACHINE_FUNCTION_H_
#define __MACHINE_FUNCTION_H_

#include "codegen/MachineBlock.hpp"
#include "codegen/MachineInstruction.hpp"
#include "codegen/MachineOperand.hpp"
#include "Function.h"
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cassert>

class MachineUnit;

class MachineFunction {
public:
    MachineUnit *parent;
    std::vector<MachineBlock *> block_list;
    Function *IR_func;
    std::vector<MachineInstruction *> no2inst;
    std::unordered_map<std::string, std::set<std::pair<int, int>>> live_ranges;
    std::unordered_map<std::string, std::set<std::pair<int, int>>> f_live_ranges;
    std::unordered_map<std::string, std::pair<int, int>> live_intervals;
    std::unordered_map<std::string, std::pair<int, int>> f_live_intervals;
    int framesize{};

    MachineFunction(MachineUnit *parent_p, Function *func) : parent(parent_p), IR_func(func) {}
    ~MachineFunction() {
        for (auto &block : block_list) {
            delete block;
            block = nullptr;
        }
    }

    void insert_block(MachineBlock *block) {
        block_list.push_back(block);
    }

    void output(std::ostream &os) {
        for (auto block : block_list) {
            block->output(os);
        }
        // auto rpo(std::move(IR_func->getReversePostOrder()));
        // std::vector<BasicBlock *> blocks(rpo.rbegin(), rpo.rend());
        // for (auto block : blocks) {
        //     block->mBlock->output(os);
        // }
    }

};

#endif