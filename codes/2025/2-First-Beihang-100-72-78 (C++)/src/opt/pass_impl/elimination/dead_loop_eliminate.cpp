#include <memory>
#include <unordered_set>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static bool modified = false;

static bool has_strong_effect(std::shared_ptr<ir::Instruction> inst);
static bool loop_can_remove(std::shared_ptr<ir::opt_support::LoopInfo> loop);
static bool run_on_loop(std::shared_ptr<ir::opt_support::LoopInfo> loop);

bool DeadLoopEliminate::run(ir::Module *module) {
    logger::INFO("Running DeadLoopEliminate...");
    bool any_modified = false;

    module->for_each_func([&](auto &func) {
        modified = false;

        for (auto it = func->opt_info.loop_forest.begin(); it != func->opt_info.loop_forest.end(); ++it) {
            auto loop = *it;
            if (!loop) continue;
            run_on_loop(loop);
        }

        any_modified |= modified;
    });

    return any_modified;
}

static bool run_on_loop(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    bool loop_modified = false;

    // Process child loops first (bottom-up)
    for (auto &child : loop->children) {
        run_on_loop(child);
    }

    // Check if current loop can be removed
    if (loop_can_remove(loop)) {
        modified = true;
        loop_modified = true;

        // Get exit block (we know there's exactly one)
        auto exit = *loop->exits.begin();
        auto header = loop->header;

        // Find preheader or entering blocks
        std::unordered_set<std::shared_ptr<ir::BasicBlock>> entering_blocks;
        for (auto &pred_weak : header->opt_info.predecessors) {
            auto pred = pred_weak.lock();
            if (pred && !loop->contains(pred)) {
                entering_blocks.insert(pred);
            }
        }

        // Redirect all entering blocks to exit instead of header
        for (const auto &entering : entering_blocks) {
            opt::util::redirect(entering, header, exit);
        }

        // Remove all loop blocks
        for (auto &block : loop->blocks) {
            opt::util::remove_basic_block(block);
        }
    }

    return loop_modified;
}

static bool loop_can_remove(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    // 1. Must have no child loops
    if (!loop->children.empty()) {
        return false;
    }

    // 2. Must have exactly one exit
    if (loop->exits.size() != 1) {
        return false;
    }

    auto exit = *loop->exits.begin();

    // 3. Exit block should not have PHI instructions that depend on loop
    for (auto &inst : exit->get_instructions_ref()) {
        auto phi = std::dynamic_pointer_cast<ir::Phi>(inst);
        if (phi) {
            // Check if this PHI has incoming edges from loop blocks
            for (auto &operand : phi->get_operands_ref()) {
                auto operand_block = std::dynamic_pointer_cast<ir::BasicBlock>(operand);
                if (operand_block && loop->contains(operand_block)) {
                    return false;
                }
            }
        } else {
            // Non-PHI instructions in exit block are fine
            break;
        }
    }

    // 4. All instructions in loop must have no strong effects
    // 5. All users of loop instructions must be within the loop
    for (auto &block : loop->blocks) {
        for (auto &inst : block->get_instructions_ref()) {
            // Check for strong effects
            if (has_strong_effect(inst)) {
                return false;
            }

            // Check if all users are within the loop
            for (auto &user_weak : inst->get_users_ref()) {
                auto user = user_weak.lock();
                if (!user) continue;

                auto user_inst = std::dynamic_pointer_cast<ir::Instruction>(user);
                if (user_inst) {
                    auto user_block = user_inst->get_parent_block().lock();
                    if (!loop->contains(user_block)) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

static bool has_strong_effect(std::shared_ptr<ir::Instruction> inst) {
    auto inst_type = inst->get_ins_type();

    // Check for instructions with strong side effects
    switch (inst_type) {
        case ir::Instruction::InstructionType::STORE:
        case ir::Instruction::InstructionType::LOAD:
        case ir::Instruction::InstructionType::RET:
        case ir::Instruction::InstructionType::MEMSET:
            return true;

        case ir::Instruction::InstructionType::CALL: {
            auto call = std::dynamic_pointer_cast<ir::Call>(inst);
            if (!call) return true;

            auto func = call->get_function();
            if (!func) return true;

            // Check if function has side effects
            return func->opt_info.has_side_effect;
        }

        default:
            return false;
    }
}

}  // namespace opt::pass
