#include <functional>
#include <memory>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static bool modified = false;
static constexpr int MAX_INLINE_BLOCK_SIZE = 25;
static std::unordered_set<std::shared_ptr<ir::BasicBlock>> header_blocks;

static void gather_header(std::shared_ptr<ir::Function> function);
static void br_inline(std::shared_ptr<ir::Function> function);
static bool can_inline(std::shared_ptr<ir::BasicBlock> target_block);
static void clone_for_block(std::shared_ptr<ir::BasicBlock> cloner, std::shared_ptr<ir::BasicBlock> cloned);
static void update_phi(std::shared_ptr<ir::Function> function);

bool Unconditional_Br_Inlining::run(ir::Module *module) {
    logger::INFO("Unconditional_Br_Inlining...");
    modified = false;

    module->for_each_func([](auto &function) {
        gather_header(function);
        br_inline(function);
        update_phi(function);
    });

    return modified;
}

static void gather_header(std::shared_ptr<ir::Function> function) {
    header_blocks.clear();

    for (auto loop : function->opt_info.loop_forest) {
        if (loop->header) {
            header_blocks.insert(loop->header);
        }
    }
}

static void br_inline(std::shared_ptr<ir::Function> function) {
    auto basic_blocks = util::get_topo_sort_blocks(function);
    for (const auto &basic_block : basic_blocks) {
        auto terminator = basic_block->tail_instruction();

        // not ret
        if (terminator->get_ins_type() != ir::Instruction::InstructionType::BR) {
            continue;
        }

        // not conditional br
        auto br = std::dynamic_pointer_cast<ir::Br>(terminator);
        if (br->is_cond_branch()) {
            continue;
        }

        // not hard to inline
        auto target_block = std::dynamic_pointer_cast<ir::BasicBlock>(br->get_operands_ref()[0]);
        if (!can_inline(target_block)) {
            continue;
        }

        // manage cfg
        util::remove_instruction(terminator);
        basic_block->opt_info.successors = target_block->opt_info.successors;
        target_block->opt_info.remove_predecessor(basic_block);
        for (auto &weak_suc : basic_block->opt_info.successors) {
            if (target_block->opt_info.predecessors.empty()) {
                weak_suc.lock()->opt_info.remove_predecessor(target_block);
            }
            weak_suc.lock()->opt_info.predecessors.push_back(basic_block);
        }

        // clone
        clone_for_block(basic_block, target_block);
    }
}

static bool can_inline(std::shared_ptr<ir::BasicBlock> target_block) {
    // not large block
    if (target_block->get_instructions().size() > MAX_INLINE_BLOCK_SIZE) {
        return false;
    }

    // no phi in target block
    if (target_block->get_instructions().front()->get_ins_type() == ir::Instruction::InstructionType::PHI) {
        return false;
    }

    // not entrance of loop
    if (header_blocks.count(target_block) > 0) {
        return false;
    }

    // no further use for instructions (except in phis)
    for (auto &inst : target_block->get_instructions_ref()) {
        for (auto &user_weak : inst->get_users_ref()) {
            auto user = std::dynamic_pointer_cast<ir::Instruction>(user_weak.lock());
            if (auto phi = std::dynamic_pointer_cast<ir::Phi>(user)) {
                if (util::get_phi_block(phi, inst) != target_block) {
                    return false;
                }
            } else if (user->get_parent_block().lock() != target_block) {
                return false;
            }
        }
    }

    return true;
}

static void clone_for_block(std::shared_ptr<ir::BasicBlock> cloner, std::shared_ptr<ir::BasicBlock> cloned) {
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> value_clone_map;
    std::vector<std::shared_ptr<ir::Instruction>> new_instructions;

    for (auto &inst : cloned->get_instructions_ref()) {
        auto new_inst = inst->clone();
        cloner->add_instruction(cloner->get_instructions_ref().end(), new_inst);
        new_inst->set_parent_block(cloner);
        value_clone_map[inst] = new_inst;
        new_instructions.push_back(new_inst);
    }
    value_clone_map[cloned] = cloner;

    for (auto &weak_suc : cloned->opt_info.successors) {
        for (auto &inst : weak_suc.lock()->get_instructions_ref()) {
            if (inst->get_ins_type() == ir::Instruction::InstructionType::PHI) {
                auto phi = std::dynamic_pointer_cast<ir::Phi>(inst);

                std::shared_ptr<ir::Value> new_value = ir::get_zero(phi->get_type());
                for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
                    auto value = phi->get_operands_ref()[i];
                    auto block = std::dynamic_pointer_cast<ir::BasicBlock>(phi->get_operands_ref()[i + 1]);
                    if (block == cloned) {
                        new_value =
                            value_clone_map.find(value) != value_clone_map.end() ? value_clone_map[value] : value;
                        break;
                    }
                }
                util::add_incoming(phi, cloner, new_value);
            } else {
                break;
            }
        }
    }

    for (auto &inst : new_instructions) {
        util::substitute_operand(inst, value_clone_map);
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
