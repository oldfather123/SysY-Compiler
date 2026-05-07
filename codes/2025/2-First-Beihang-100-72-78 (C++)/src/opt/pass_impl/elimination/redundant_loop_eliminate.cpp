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
static void search_func(std::shared_ptr<ir::Function> function);
static bool search_loop(std::shared_ptr<ir::opt_support::LoopInfo> loop_info);
static bool can_be_removed(std::shared_ptr<ir::opt_support::LoopInfo> loop_info);
static bool has_side_effect(std::shared_ptr<ir::Instruction> &instruction);
static void update_phi(std::shared_ptr<ir::Function> function);

bool RedundantLoopElimination::run(ir::Module *module) {
    logger::INFO("Running RedundantLoopElimination...");

    auto s1 = module->to_string();

    modified = false;
    module->for_each_func([](auto &function) {
        search_func(function);
        update_phi(function);
    });

    auto s2 = module->to_string();

    return modified;
}

static void search_func(std::shared_ptr<ir::Function> function) {
    if (function->opt_info.loop_forest.empty()) {
        return;
    }
    std::vector<std::shared_ptr<ir::opt_support::LoopInfo>> to_remove;
    for (auto loop_info : function->opt_info.loop_forest) {
        if (search_loop(loop_info)) {
            to_remove.push_back(loop_info);
        }
    }
}

static bool search_loop(std::shared_ptr<ir::opt_support::LoopInfo> loop_info) {
    std::vector<std::shared_ptr<ir::opt_support::LoopInfo>> to_remove;

    for (auto &child_loop_info : loop_info->children) {
        if (search_loop(child_loop_info)) {
            to_remove.push_back(child_loop_info);
        }
    }
    loop_info->children.erase(std::remove_if(loop_info->children.begin(),
                                             loop_info->children.end(),
                                             [&to_remove](const std::shared_ptr<ir::opt_support::LoopInfo> &child) {
                                                 return std::find(to_remove.begin(), to_remove.end(), child) !=
                                                        to_remove.end();
                                             }),
                              loop_info->children.end());

    if (can_be_removed(loop_info)) {
        modified = true;

        if (loop_info->pre_header == nullptr) {
            logger::ERROR("Loop without pre-header found, this is a bug in the compiler.");
            __builtin_unreachable();
        }
        auto header = loop_info->header;
        auto exit = *loop_info->exits.begin();
        auto preheader = loop_info->pre_header;
        auto terminator = preheader->tail_instruction();

        util::substitute_operand(terminator, header, exit);
        preheader->opt_info.remove_successor(header);
        preheader->opt_info.successors.push_back(exit);
        header->opt_info.remove_predecessor(preheader);
        exit->opt_info.predecessors.push_back(preheader);

        return true;
    }
    return false;
}

static bool can_be_removed(std::shared_ptr<ir::opt_support::LoopInfo> loop_info) {
    // can only cover a few cases
    if (!loop_info->children.empty() && loop_info->exits.size() != 1) {
        return false;
    }
    auto exit = *loop_info->exits.begin();
    if (exit->get_instructions_ref().front()->get_ins_type() == ir::Instruction::InstructionType::PHI) {
        return false;
    }
    if (exit->opt_info.predecessors.size() != 1) {
        return false;
    }
    for (auto &basic_block : loop_info->blocks) {
        for (auto &instruction : basic_block->get_instructions_ref()) {
            // no side effects
            if (has_side_effect(instruction)) {
                return false;
            }
            // all users of the instruction must be in the same loop
            for (auto &user : instruction->get_users_ref()) {
                auto user_instruction = std::dynamic_pointer_cast<ir::Instruction>(user.lock());
                if (user_instruction->get_parent_block().lock()->opt_info.loop.lock() != loop_info) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool has_side_effect(std::shared_ptr<ir::Instruction> &instruction) {
    switch (instruction->get_ins_type()) {
        // TODO: LOAD may be removed?
        case ir::Instruction::InstructionType::STORE:
        case ir::Instruction::InstructionType::LOAD:
        case ir::Instruction::InstructionType::RET:
            return true;
        case ir::Instruction::InstructionType::CALL: {
            auto callee = std::dynamic_pointer_cast<ir::Call>(instruction)->get_function();
            return callee->opt_info.has_local_memory_write || callee->opt_info.has_global_memory_write ||
                   callee->opt_info.has_inout;
        }
        default:
            return false;
    }
}

static void update_phi(std::shared_ptr<ir::Function> function) {
    for (auto &block : function->get_basic_blocks_ref()) {
        for (auto &instruction : block->get_instructions_ref()) {
            if (instruction->get_ins_type() == ir::Instruction::InstructionType::PHI) {
                auto phi = std::dynamic_pointer_cast<ir::Phi>(instruction);
                std::vector<std::shared_ptr<ir::BasicBlock>> redundant_blocks;
                for (auto &basic_block : phi->blocks()) {
                    if (!contains(to_shared_vector(phi->get_parent_block().lock()->opt_info.predecessors),
                                  basic_block)) {
                        redundant_blocks.push_back(basic_block);
                    }
                }
                for (auto &redundant_block : redundant_blocks) {
                    util::remove_phi_block(phi, redundant_block);
                }
            } else {
                break;
            }
        }
    }
}

}  // namespace opt::pass
