#include "log.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
bool ControlFlowGraphAnalyzation::run(ir::Module *module) {
    logger::INFO("Running ControlFlowGraphAnalyzation...");
    module->for_each_func([](auto &func) {
        func->for_each_block([](auto &block) {
            block->opt_info.successors.clear();
            block->opt_info.predecessors.clear();
        });
    });

    module->for_each_func([&](const auto &func) {
        std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::vector<std::shared_ptr<ir::BasicBlock>>>
            predecessor_map;
        std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::vector<std::shared_ptr<ir::BasicBlock>>> successor_map;

        func->for_each_block([&](const auto &basic_block) {
            auto &br = basic_block->get_instructions_ref().back();

            if (br->get_ins_type() != ir::Instruction::InstructionType::BR) {
                return;
            }
            const auto br_ins = std::dynamic_pointer_cast<ir::Br>(br);

            // conditional br
            if (br->get_operands_ref().size() > 1) {
                if (auto if_basic_block = std::dynamic_pointer_cast<ir::BasicBlock>(br_ins->get_operands_ref().at(1))) {
                    successor_map[basic_block].push_back(if_basic_block);
                    predecessor_map[if_basic_block].push_back(basic_block);
                }
                if (auto else_basic_block =
                        std::dynamic_pointer_cast<ir::BasicBlock>(br_ins->get_operands_ref().at(2))) {
                    successor_map[basic_block].push_back(else_basic_block);
                    predecessor_map[else_basic_block].push_back(basic_block);
                }
            }
            // unconditional br
            else if (br->get_operands_ref().size() == 1) {
                auto target = br_ins->get_operands_ref().front();
                if (auto target_basic_block = std::dynamic_pointer_cast<ir::BasicBlock>(target)) {
                    successor_map[basic_block].push_back(target_basic_block);
                    predecessor_map[target_basic_block].push_back(basic_block);
                }
            } else {
                __builtin_unreachable();
            }
        });

        // set predecessors and successors
        func->for_each_block([&](const auto &basic_block) {
            basic_block->opt_info.successors = to_weak_vector(successor_map[basic_block]);
            basic_block->opt_info.predecessors = to_weak_vector(predecessor_map[basic_block]);
        });
    });
    return false;
}
}  // namespace opt::pass
