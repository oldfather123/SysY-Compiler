#include "LivenessAnalysis.hpp"
#include "MachineBasicBlock.hpp"
#include "MachineFunction.hpp"
#include "Operand.hpp"
#include "logging.hpp"
#include <memory>
#include <queue>

void LivenessAnalysis::run_on_func(std::shared_ptr<MachineFunction> func) {
    MBBSetMap use;
    MBBSetMap def;

    for (auto &block : func->get_basic_blocks()) {
        use[block] = block->get_use();
        def[block] = block->get_def();
    }
    for (auto &block : func->get_basic_blocks()) {
        live_in[block] = RegisterSet();
        live_out[block] = RegisterSet();
    }

    // use the Worklist-Algorithm
    std::queue<std::shared_ptr<MachineBasicBlock>> worklist;
    std::set<std::shared_ptr<MachineBasicBlock>,
             std::owner_less<std::shared_ptr<MachineBasicBlock>>>
        in_worklist;

    worklist.push(func->get_epilogue_block());
    in_worklist.insert(func->get_epilogue_block());
    while (!worklist.empty()) {
        auto block = worklist.front();
        worklist.pop();
        in_worklist.erase(block);

        RegisterSet new_in;
        live_out[block] = RegisterSet();
        for (auto &succ : block->get_succ_basic_blocks()) {
            live_out[block].insert(live_in[succ.lock()].begin(),
                                   live_in[succ.lock()].end());
        }

        for (auto &reg : live_out[block]) {
            if (def[block].find(reg) == def[block].end()) {
                new_in.insert(reg);
            }
        }
        new_in.insert(use[block].begin(), use[block].end());

        if (new_in != live_in[block]) {
            live_in[block] = new_in;
            for (auto &pred : block->get_pre_basic_blocks()) {
                if (in_worklist.find(pred.lock()) == in_worklist.end()) {
                    worklist.push(pred.lock());
                    in_worklist.insert(pred.lock());
                }
            }
        }
    }
}
void LivenessAnalysis::run() {
    for (auto &func : module->get_functions()) {
        run_on_func(func);
    }
}

RegisterSet 
LivenessAnalysis::get_live_in(std::shared_ptr<MachineBasicBlock> block) {
    return live_in[block];
}

RegisterSet 
LivenessAnalysis::get_live_out(std::shared_ptr<MachineBasicBlock> block) {
    return live_out[block];
}

std::string LivenessAnalysis::print() {
    std::string res;
    for (auto &func : module->get_functions()) {
        res += "Function: " + func->get_name() + "\n";
        for (auto &block : func->get_basic_blocks()) {
            res += "Block: " + block->get_name() + "\n";
            res += "Live in: ";
            for (auto &reg : live_in[block]) {
                res += reg->get_name() + " ";
            }
            res += "\n";
            res += "Live out: ";
            for (auto &reg : live_out[block]) {
                res += reg->get_name() + " ";
            }
            res += "\n";
        }
    }
    return res;
}