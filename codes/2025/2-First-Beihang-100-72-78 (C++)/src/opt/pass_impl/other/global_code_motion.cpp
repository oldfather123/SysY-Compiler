#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;
std::unordered_set<std::shared_ptr<ir::Instruction>> visited;
void schedule_early(std::shared_ptr<ir::Instruction> inst, std::shared_ptr<ir::Function> func);
void schedule_late(std::shared_ptr<ir::Instruction> inst);
bool movable(std::shared_ptr<ir::Instruction> inst);

bool GlobalCodeMotion::run(ir::Module *module) {
    logger::INFO("Running GlobalCodeMotion...");
    modified = false;
    module->for_each_func([](auto &&function) {
        auto post_order_blocks = util::post_order_traversal_dom(function);

        std::vector<std::shared_ptr<ir::Instruction>> instructions;
        for (auto it = post_order_blocks.rbegin(); it != post_order_blocks.rend(); ++it) {
            auto &block = *it;
            for (auto &inst : block->get_instructions_ref()) {
                instructions.push_back(inst);
            }
        }

        // schedule early
        for (auto &inst : instructions) {
            schedule_early(inst, function);
        }
        visited.clear();

        // schedule late
        for (auto it = instructions.rbegin(); it != instructions.rend(); ++it) {
            schedule_late(*it);
        }
        visited.clear();
    });
    return modified;
}

void schedule_early(std::shared_ptr<ir::Instruction> inst, std::shared_ptr<ir::Function> func) {
    if (visited.find(inst) != visited.end() || !movable(inst)) {
        return;
    }
    visited.insert(inst);

    // add before the end of the entry block
    auto entry = func->entry_block();
    util::remove_instruction_from_parent_basic_block(inst);
    util::add_instruction_before(inst, entry->get_instructions_ref().back());

    for (auto &operand : inst->get_operands_ref()) {
        if (auto inst_operand = std::dynamic_pointer_cast<ir::Instruction>(operand)) {
            schedule_early(inst_operand, func);
            if (inst->get_parent_block().lock()->opt_info.dominance_depth <
                inst_operand->get_parent_block().lock()->opt_info.dominance_depth) {
                auto operand_block = inst_operand->get_parent_block().lock();
                util::remove_instruction_from_parent_basic_block(inst);
                util::add_instruction_before(inst, operand_block->get_instructions_ref().back());
            }
        }
    }
}

void schedule_late(std::shared_ptr<ir::Instruction> inst) {
    if (visited.find(inst) != visited.end() || !movable(inst)) {
        return;
    }
    visited.insert(inst);

    // calculate youngest ancestor
    std::shared_ptr<ir::BasicBlock> ancestor = nullptr;
    for (auto &user : inst->get_users_ref()) {
        if (auto inst_user = std::dynamic_pointer_cast<ir::Instruction>(user.lock())) {
            schedule_late(inst_user);
            if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst_user)) {
                for (auto idx = 0; idx < phi->get_operands_ref().size(); idx += 2) {
                    auto value = phi->get_operands_ref()[idx];
                    auto block = std::dynamic_pointer_cast<ir::BasicBlock>(phi->get_operands_ref()[idx + 1]);
                    if (value == inst) {
                        ancestor = util::dom_lca(ancestor, block);
                    }
                }
            } else {
                auto user_block = inst_user->get_parent_block().lock();
                ancestor = util::dom_lca(ancestor, user_block);
            }
        }
    }

    // search for final position
    std::shared_ptr<ir::BasicBlock> best_position;
    if (!inst->get_users_ref().empty()) {
        best_position = ancestor;
        while (ancestor != inst->get_parent_block().lock()) {
            ancestor = ancestor->opt_info.immediate_dominator.lock();
            if (ancestor->opt_info.get_loop_depth() < best_position->opt_info.get_loop_depth() ||
                (ancestor->opt_info.successors.size() == 1 &&
                 ancestor->opt_info.successors.front().lock() == best_position)) {
                best_position = ancestor;
            }
        }
        util::remove_instruction_from_parent_basic_block(inst);
        util::add_instruction_before(inst, best_position->get_instructions_ref().back());
    }

    best_position = inst->get_parent_block().lock();
    for (auto &other_inst : best_position->get_instructions_ref()) {
        if (other_inst != inst && other_inst->get_ins_type() != ir::Instruction::InstructionType::PHI &&
            contains(other_inst->get_operands_ref(), inst)) {
            util::remove_instruction_from_parent_basic_block(inst);
            util::add_instruction_before(inst, other_inst);
            break;
        }
    }
}

bool movable(std::shared_ptr<ir::Instruction> inst) {
    switch (inst->get_ins_type()) {
        case ir::Instruction::InstructionType::PHI:
        case ir::Instruction::InstructionType::BR:
        case ir::Instruction::InstructionType::RET:
        case ir::Instruction::InstructionType::STORE:
        case ir::Instruction::InstructionType::LOAD:
        case ir::Instruction::InstructionType::MEMSET:
            return false;
        case ir::Instruction::InstructionType::CALL: {
            auto func = std::dynamic_pointer_cast<ir::Call>(inst)->get_function();
            auto func_info = func->opt_info;
            return !ir::Function::is_lib(func) && func_info.is_pure && !func_info.has_inout;
        }
        default:
            return true;
    }
}

}  // namespace opt::pass
