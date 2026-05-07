#include <memory>
#include <unordered_set>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;
static std::unordered_set<std::shared_ptr<ir::Instruction>> useful_instructions;

bool has_side_effect(const std::shared_ptr<ir::Instruction> &instruction);
void find_useful_instructions(const std::shared_ptr<ir::Instruction> &instruction);

bool DeadCodeElimination::run(ir::Module *module) {
    logger::INFO("Running DeadCodeElimination...");
    modified = false;
    module->for_each_func([](const auto &function) {
        // Find all useful instructions
        function->for_each_block([&](const auto &basic_block) {
            basic_block->for_each_instruction([&](const auto &instruction) {
                if (has_side_effect(instruction)) {
                    find_useful_instructions(instruction);
                }
            });
        });

        // Find dead instructions
        std::unordered_set<std::shared_ptr<ir::Instruction>> dead_instructions;
        function->for_each_block([&](const auto &basic_block) {
            basic_block->for_each_instruction([&](const auto &instruction) {
                if (useful_instructions.find(instruction) == useful_instructions.end()) {
                    dead_instructions.emplace(instruction);
                }
            });
        });
        modified |= !dead_instructions.empty();

        // Remove dead instructions
        for (const auto &dead_instruction : dead_instructions) {
            util::remove_instruction(dead_instruction);
        }

        // Clear the set for the next function
        useful_instructions.clear();
    });
    // util::static_check(module);
    return modified;
}

void find_useful_instructions(const std::shared_ptr<ir::Instruction> &instruction) {
    if (useful_instructions.find(instruction) != useful_instructions.end()) {
        return;
    }
    useful_instructions.insert(instruction);
    for (const auto &operand : instruction->get_operands_ref()) {
        if (auto operand_instruction = std::dynamic_pointer_cast<ir::Instruction>(operand)) {
            find_useful_instructions(operand_instruction);
        }
    }
}

bool has_side_effect(const std::shared_ptr<ir::Instruction> &instruction) {
    auto instruction_type = instruction->get_ins_type();
    if (instruction_type == ir::Instruction::InstructionType::CALL) {
        auto call_inst = std::dynamic_pointer_cast<ir::Call>(instruction);
        auto function = call_inst->get_function();
        return function->opt_info.has_side_effect;
    }
    return instruction_type == ir::Instruction::InstructionType::RET ||
           instruction_type == ir::Instruction::InstructionType::BR ||
           instruction_type == ir::Instruction::InstructionType::STORE ||
           instruction_type == ir::Instruction::InstructionType::MEMSET;
}

}  // namespace opt::pass
