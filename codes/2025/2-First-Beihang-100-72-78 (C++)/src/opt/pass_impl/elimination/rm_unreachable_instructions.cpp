#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;
bool RemoveUnreachableInstructions::run(ir::Module *module) {
    logger::INFO("Running RemoveUnreachableInstructions...");
    module->for_each_func([&](const auto &func) {
        func->for_each_block([&](const auto &basic_block) {
            bool reachable = true;
            std::vector<std::shared_ptr<ir::Instruction>> instructions_to_remove;

            basic_block->for_each_instruction([&](const auto &ins) {
                if (!reachable) {
                    instructions_to_remove.push_back(ins);
                    return;
                }
                if (ins->get_ins_type() == ir::Instruction::InstructionType::BR ||
                    ins->get_ins_type() == ir::Instruction::InstructionType::RET) {
                    reachable = false;
                }
            });

            modified |= !instructions_to_remove.empty();

            for (const auto &ins : instructions_to_remove) {
                util::remove_instruction(ins);
            }
        });
    });

    return modified;
}
}  // namespace opt::pass
