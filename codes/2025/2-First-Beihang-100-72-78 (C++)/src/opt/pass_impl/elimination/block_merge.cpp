#include <memory>
#include <unordered_set>

#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;

static std::unordered_set<std::shared_ptr<ir::BasicBlock>> deleted_blocks;

static void try_merge(std::shared_ptr<ir::Function> func);
static void merge_to(std::shared_ptr<ir::BasicBlock> from, std::shared_ptr<ir::BasicBlock> to);
static void merge_from(std::shared_ptr<ir::BasicBlock> from, std::shared_ptr<ir::BasicBlock> to);

bool BlockMerge::run(ir::Module *module) {
    logger::INFO("Running BlockMerge...");
    modified = false;
    module->for_each_func([](auto &func) { try_merge(func); });
    return modified;
}

static void try_merge(std::shared_ptr<ir::Function> func) {
    auto blocks = func->get_basic_blocks();
    auto it = blocks.rbegin();
    while (it != blocks.rend()) {
        auto block = *it;
        if (deleted_blocks.count(block)) {
            it++;
            continue;
        }
        if (block->opt_info.successors.size() != 1) {
            it++;
            continue;
        }
        auto tail_br = std::dynamic_pointer_cast<ir::Br>(block->tail_instruction());
        if (!tail_br || tail_br->is_cond_branch()) {
            it++;
            continue;
        }
        auto to = std::dynamic_pointer_cast<ir::BasicBlock>(tail_br->get_operands_ref()[0]);
        if (!to || to->opt_info.predecessors.size() != 1) {
            it++;
            continue;
        }
        assert(to->opt_info.predecessors.front().lock() == block);
        modified = true;

        auto entry = func->entry_block();

        // NOTE: we could not move entry block's localtion, depends on this, determine the merge direction
        if (entry != block) {
            merge_to(block, to);
            deleted_blocks.insert(block);
            it++;
        } else {
            merge_from(block, to);
            deleted_blocks.insert(to);
        }
    }
    for (const auto &block : deleted_blocks) {
        util::remove_basic_block(block);
    }
    deleted_blocks.clear();
}

// merge `from` to `to`
static void merge_to(std::shared_ptr<ir::BasicBlock> from, std::shared_ptr<ir::BasicBlock> to) {
    // clear phi
    for (const auto &phi : util::get_phis(to)) {
        assert(phi->get_operands_ref().size() == 2);
        assert(phi->get_operands_ref()[1] == from);
        auto value = phi->get_operands_ref()[0];
        util::substitute(phi, value);
    }

    // remove tail br
    auto tail_br = std::dynamic_pointer_cast<ir::Br>(from->tail_instruction());
    assert(tail_br && !tail_br->is_cond_branch());
    util::remove_instruction(tail_br);
    from->opt_info.remove_successor(to);
    to->opt_info.remove_predecessor(from);

    // move instructions to `to`
    auto instructions = from->get_instructions();
    for (auto it = instructions.rbegin(); it != instructions.rend(); it++) {
        auto inst = *it;
        inst->set_parent_block(to);
        from->get_instructions_ref().erase(inst->node);
        to->add_instruction(to->get_instructions_ref().begin(), inst);
    }

    // redirect `from`'s successors
    auto preds = from->opt_info.predecessors;
    for (auto &weak_pred : preds) {
        util::redirect(weak_pred.lock(), from, to);
    }
}

// merge `to` to `from`
static void merge_from(std::shared_ptr<ir::BasicBlock> from, std::shared_ptr<ir::BasicBlock> to) {
    // clear phi
    for (const auto &phi : util::get_phis(to)) {
        assert(phi->get_operands_ref().size() == 2);
        assert(phi->get_operands_ref()[1] == from);
        auto value = phi->get_operands_ref()[0];
        util::substitute(phi, value);
    }

    // remove tail br
    auto tail_br = std::dynamic_pointer_cast<ir::Br>(from->tail_instruction());
    assert(tail_br && !tail_br->is_cond_branch());
    util::remove_instruction(tail_br);
    from->opt_info.remove_successor(to);
    to->opt_info.remove_predecessor(from);

    // move instructions to block
    auto instructions = to->get_instructions();
    for (auto it = instructions.begin(); it != instructions.end(); it++) {
        auto inst = *it;
        inst->set_parent_block(from);
        to->get_instructions_ref().erase(inst->node);
        from->add_instruction(from->get_instructions_ref().end(), inst);
    }

    for (auto &weak_succ : to->opt_info.successors) {
        auto succ = weak_succ.lock();
        succ->opt_info.remove_predecessor(to);
        succ->opt_info.predecessors.push_back(from);
        from->opt_info.successors.push_back(succ);
    }
    util::replace_all_uses_with(to, from);
}
}  // namespace opt::pass
