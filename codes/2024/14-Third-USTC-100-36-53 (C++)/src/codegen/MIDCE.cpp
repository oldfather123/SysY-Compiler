#include "MIDCE.hpp"
#include "MachineBasicBlock.hpp"
#include "MachineFunction.hpp"
#include "Operand.hpp"

void MIDeadCodeElimination::run() {
    LA = std::make_unique<LivenessAnalysis>(module);
    LA->run();

    for (auto &func : module->get_functions()) {
        run_on_func(func);
    }
}

void MIDeadCodeElimination::run_on_func(std::shared_ptr<MachineFunction> func) {
    bool changed = false;
    do {
        changed = false;
        for (auto &block : func->get_basic_blocks()) {
            RegisterSet use_set = LA->get_live_out(block);
            for (auto inst_ = block->get_instrs().rbegin();
                 inst_ != block->get_instrs().rend();) {
                auto inst = *inst_;
                bool is_dead = true;
                for (auto def : inst->get_def()) {
                    if (use_set.find(def) != use_set.end()) {
                        is_dead = false;
                        break;
                    }
                    if (def == PhysicalRegister::fp() ||
                        def == PhysicalRegister::sp()) {
                        is_dead = false;
                        break;
                    }
                }
                if (is_dead && inst->has_dst() && !inst->is_reserved()) {
                    inst_ = decltype(inst_)(
                        block->get_instrs().erase(std::next(inst_).base()));
                    changed = true;
                    continue;
                } else {
                    inst_++;
                }
                for (auto def : inst->get_def()) {
                    use_set.erase(def);
                }
                for (auto use : inst->get_use()) {
                    use_set.insert(use);
                }
            }
        }
    } while (changed);
}
