#include <memory>
#include <unordered_set>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;
static ir::opt_support::AliasInfo *alias_info;

static std::unordered_set<std::shared_ptr<ir::Load>> loads;
static std::unordered_set<std::shared_ptr<ir::Store>> stores;
static std::unordered_set<std::shared_ptr<ir::Instruction>> after;
static std::unordered_set<std::shared_ptr<ir::Instruction>> before;
static std::unordered_set<std::shared_ptr<ir::Value>> changed_pointers;
static std::unordered_set<std::shared_ptr<ir::Value>> used_pointers;

static bool move_on_loop(std::shared_ptr<ir::opt_support::LoopInfo> loop);
static bool may_conflict(std::shared_ptr<ir::Value> value, std::unordered_set<std::shared_ptr<ir::Value>> ptrs);

bool LoopInvariantCodeMotion::run(ir::Module *module) {
    logger::INFO("Running LoopInvariantCodeMotion...");
    modified = false;
    alias_info = &module->opt_info.alias_info;
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            while (true) {
                if (!move_on_loop(loop)) break;
                modified = true;
            }
        }
    }
    // util::static_check(module);
    return modified;
}

static bool move_on_loop(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    bool inner_modified = false;
    loads.clear();
    stores.clear();
    after.clear();
    before.clear();
    changed_pointers.clear();
    used_pointers.clear();

    // traverse all blocks in the loop, collect loads and stores
    auto all_blocks = loop->all_blocks();
    for (const auto &block : all_blocks) {
        for (const auto &inst : block->get_instructions_ref()) {
            if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
                // loop has side effect call, operate conservatively
                auto callee = call->get_function();
                if (callee->opt_info.has_side_effect) return false;
            } else if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
                stores.insert(store);
                changed_pointers.insert(store->addr());
            } else if (auto load = std::dynamic_pointer_cast<ir::Load>(inst)) {
                loads.insert(load);
                used_pointers.insert(load->addr());
            }
        }
    }

    for (const auto &store : stores) {
        // Skip if store address is computed inside the loop
        if (auto i = std::dynamic_pointer_cast<ir::Instruction>(store->addr());
            i && all_blocks.count(i->get_parent_block().lock()))
            continue;

        // Skip if stored value is computed inside the loop
        if (loop->defined(store->val())) continue;

        // For stores with loop-invariant addresses and values, check if they can be safely hoisted
        bool can_hoist = true;

        // Check if this store conflicts with other stores to different addresses
        for (const auto &other : stores) {
            if (store == other) continue;

            // If two stores write to provably distinct addresses, they don't conflict
            if (alias_info->is_distinct(store->addr(), other->addr())) {
                continue;
            }

            // If we can't prove they're distinct, be conservative and don't hoist
            can_hoist = false;
            break;
        }

        // Even if there are potential address conflicts, we can still hoist if:
        // 1. The store writes to a loop-invariant location
        // 2. The stored value is loop-invariant
        // 3. No loads in the loop read from potentially conflicting addresses before this store
        if (!can_hoist) {
            // Try a more aggressive approach for constant array stores
            bool all_stores_to_distinct_constants = true;
            for (const auto &other : stores) {
                if (store == other) continue;

                // Check if both addresses are GEP instructions with constant indices
                auto store_gep = std::dynamic_pointer_cast<ir::Getelementptr>(store->addr());
                auto other_gep = std::dynamic_pointer_cast<ir::Getelementptr>(other->addr());

                if (store_gep && other_gep) {
                    // Check if they access the same base array
                    if (store_gep->get_operands_ref()[0] == other_gep->get_operands_ref()[0]) {
                        // Check if indices are different constants
                        auto store_idx =
                            std::dynamic_pointer_cast<ir::ConstantInt>(store_gep->get_operands_ref().back());
                        auto other_idx =
                            std::dynamic_pointer_cast<ir::ConstantInt>(other_gep->get_operands_ref().back());

                        if (store_idx && other_idx && store_idx->get_val() != other_idx->get_val()) {
                            // Different constant indices, no conflict
                            continue;
                        }
                    }
                }

                all_stores_to_distinct_constants = false;
                break;
            }

            can_hoist = all_stores_to_distinct_constants;
        }

        if (can_hoist) {
            before.insert(store);
        }
    }

    for (const auto &load : loads) {
        if (auto i = std::dynamic_pointer_cast<ir::Instruction>(load->addr());
            i && all_blocks.count(i->get_parent_block().lock()))
            continue;
        if (may_conflict(load->addr(), changed_pointers)) continue;
        bool userd_in_loop = false;
        for (auto &weak_user : load->get_users_ref()) {
            userd_in_loop |= all_blocks.count(
                std::dynamic_pointer_cast<ir::Instruction>(weak_user.lock())->get_parent_block().lock());
        }
        if (!userd_in_loop)
            after.insert(load);
        else
            before.insert(load);
    }

    inner_modified |= !before.empty() || !after.empty();

    for (const auto &inst : before) {
        inst->get_parent_block().lock()->get_instructions_ref().erase(inst->node);
        auto pre_header = loop->pre_header;
        inst->set_parent_block(pre_header);
        auto &instructions = pre_header->get_instructions_ref();
        pre_header->add_instruction(std::prev(instructions.end()), inst);
    }

    for (const auto &inst : after) {
        inst->get_parent_block().lock()->get_instructions_ref().erase(inst->node);
        assert(loop->exits.size() == 1);
        auto exit_instructions = (*loop->exits.begin())->get_instructions_ref();
        auto first_none_phi = std::find_if(exit_instructions.begin(), exit_instructions.end(), [](const auto &inst) {
            return !std::dynamic_pointer_cast<ir::Phi>(inst);
        });
        (*loop->exits.begin())->add_instruction((*first_none_phi)->node, inst);
    }

    return inner_modified;
}

static bool may_conflict(std::shared_ptr<ir::Value> value, std::unordered_set<std::shared_ptr<ir::Value>> ptrs) {
    for (const auto &ptr : ptrs) {
        if (!alias_info->is_distinct(value, ptr)) return true;
    }
    return false;
}

}  // namespace opt::pass
