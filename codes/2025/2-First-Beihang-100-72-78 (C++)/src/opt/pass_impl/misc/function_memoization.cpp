// Function Memoization Pass - Simplified but Correct Implementation
// Focuses on correctness first, performance second

#include <memory>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

struct MemoizationCandidate {
    std::shared_ptr<ir::Function> func;
    std::vector<std::shared_ptr<ir::Argument>> args;
    std::shared_ptr<ir::Type> return_type;
    int recursive_call_count;
};

static bool modified = false;
static std::vector<MemoizationCandidate> candidates_to_memoize;

// Configuration
static constexpr int MIN_RECURSIVE_CALLS = 2;
static constexpr int CACHE_SIZE = 4096;  // 2^12 for good balance of size vs performance

// Check if function is suitable for memoization
static bool analyze_function_for_memoization(std::shared_ptr<ir::Function> func, MemoizationCandidate &result) {
    if (ir::Function::is_lib(func)) return false;
    if (func->is_main()) return false;
    if (func->get_basic_blocks_ref().empty()) return false;

    auto ret_type = func->get_return_type();
    if (!ret_type->is_integer_ty() && !ret_type->is_float_ty()) return false;

    auto &args = func->get_arguments_ref();
    if (args.empty() || args.size() > 3) return false;

    for (auto &arg : args) {
        if (!arg->get_type()->is_integer_ty() && !arg->get_type()->is_float_ty()) return false;
    }

    // Count recursive calls and check purity
    int recursive_calls = 0;
    bool has_side_effects = false;

    for (auto &block : func->get_basic_blocks_ref()) {
        for (auto &inst : block->get_instructions_ref()) {
            if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
                auto callee = call->get_function();
                if (callee == func) {
                    recursive_calls++;
                } else if (!ir::Function::is_lib(callee)) {
                    if (callee->opt_info.has_side_effect) {
                        has_side_effects = true;
                        break;
                    }
                }
            }
            if (inst->get_ins_type() == ir::Instruction::InstructionType::STORE) {
                auto store = std::dynamic_pointer_cast<ir::Store>(inst);
                if (!std::dynamic_pointer_cast<ir::Alloca>(store->addr())) {
                    has_side_effects = true;
                    break;
                }
            }
        }
        if (has_side_effects) break;
    }

    if (recursive_calls < MIN_RECURSIVE_CALLS || has_side_effects) return false;

    result.func = func;
    result.args = args;
    result.return_type = ret_type;
    result.recursive_call_count = recursive_calls;

    logger::INFO("Memoization candidate: ", func->get_name(), " with ", args.size(), " args");
    return true;
}

// Simplified correct implementation - all logic in one block to avoid SSA issues
static void add_memoization_to_function(const MemoizationCandidate &candidate, ir::Module *module) {
    auto func = candidate.func;
    const auto &args = candidate.args;

    // Create cache: [arg0, arg1, arg2, result, has_value] per entry
    int entry_size = static_cast<int>(args.size()) + 2;
    int total_size = CACHE_SIZE * entry_size;

    auto cache_array_type = ir::ArrayType::get(ir::IntegerType::get(32), total_size);
    auto cache_var = std::make_shared<ir::GlobalVariable>(ir::PointerType::get(cache_array_type),
                                                          std::make_shared<ir::ZeroInitializer>(cache_array_type),
                                                          false,
                                                          "@memo_cache_" + func->get_name().substr(1));

    module->get_global_variables_ref().push_back(cache_var);

    // Split entry block
    auto entry_block = func->entry_block();
    auto split_point = entry_block->get_instructions_ref().begin();
    while (split_point != entry_block->get_instructions_ref().end() &&
           std::dynamic_pointer_cast<ir::Alloca>(*split_point)) {
        ++split_point;
    }

    // Allocate a slot to hold the cache entry pointer (i32*) for reuse in return blocks
    auto memo_entry_slot =
        ir::Alloca::create(entry_block, ir::PointerType::get(ir::IntegerType::get(32)), gen_local_var_name());
    entry_block->add_instruction(split_point, memo_entry_slot);

    // Create blocks
    auto cache_lookup_block = std::make_shared<ir::BasicBlock>(func, "cache_lookup");
    auto compute_block = std::make_shared<ir::BasicBlock>(func, "compute");

    auto insert_pos = std::next(entry_block->node);
    func->add_basic_block(insert_pos, cache_lookup_block);
    func->add_basic_block(insert_pos, compute_block);

    // Move original logic to compute block
    std::list<std::shared_ptr<ir::Instruction>> to_move;
    for (auto it = split_point; it != entry_block->get_instructions_ref().end(); ++it) {
        to_move.push_back(*it);
    }

    for (auto &inst : to_move) {
        entry_block->erase_instruction(inst->node);
        inst->set_parent_block(compute_block);
        compute_block->add_instruction(compute_block->get_instructions_ref().end(), inst);
    }

    // Jump from entry to cache lookup FIRST
    auto entry_jump = ir::Br::create(entry_block, cache_lookup_block, gen_local_var_name());
    entry_block->add_instruction(entry_block->get_instructions_ref().end(), entry_jump);

    // CRITICAL FIX: Update control flow AND CFG info properly
    // Any blocks that previously jumped to entry_block should now jump to cache_lookup_block
    for (auto &func_block : func->get_basic_blocks_ref()) {
        if (func_block == entry_block) continue;  // Skip the entry block itself

        auto tail = func_block->tail_instruction();
        if (auto br = std::dynamic_pointer_cast<ir::Br>(tail)) {
            auto &operands = br->get_operands_ref();
            for (auto &operand : operands) {
                if (operand == entry_block) {
                    // Update IR operand
                    operand->remove_user(br);
                    operand = cache_lookup_block;
                    cache_lookup_block->add_user(br);

                    // Update CFG info: remove entry_block from successors, add cache_lookup_block
                    auto &successors = func_block->opt_info.successors;
                    for (auto it = successors.begin(); it != successors.end(); ++it) {
                        if (auto succ = it->lock()) {
                            if (succ == entry_block) {
                                successors.erase(it);
                                break;
                            }
                        }
                    }
                    successors.push_back(cache_lookup_block);

                    // Update predecessor info
                    cache_lookup_block->opt_info.predecessors.push_back(func_block);

                    // Remove from entry_block predecessors
                    auto &entry_preds = entry_block->opt_info.predecessors;
                    for (auto it = entry_preds.begin(); it != entry_preds.end(); ++it) {
                        if (auto pred = it->lock()) {
                            if (pred == func_block) {
                                entry_preds.erase(it);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // Set up basic CFG info for entry → cache_lookup
    entry_block->opt_info.successors.clear();
    entry_block->opt_info.successors.push_back(cache_lookup_block);
    cache_lookup_block->opt_info.predecessors.push_back(entry_block);

    // === Cache Lookup Block (ALL logic in one block to avoid SSA issues) ===
    auto zero_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
    auto one_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
    auto cache_mask = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), CACHE_SIZE - 1);
    auto entry_size_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), entry_size);

    // Simple hash: hash = arg0 + arg1*33 + arg2*65 (using small multipliers)
    std::shared_ptr<ir::Value> hash_val = args[0];  // Start with first arg

    if (args.size() >= 2) {
        auto mult33 = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 33);
        auto arg1_contrib = ir::Mul::create(cache_lookup_block, args[1], mult33, gen_local_var_name());
        cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), arg1_contrib);

        auto new_hash = ir::Add::create(cache_lookup_block, hash_val, arg1_contrib, gen_local_var_name());
        cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), new_hash);
        hash_val = new_hash;
    }

    if (args.size() >= 3) {
        auto mult65 = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 65);
        auto arg2_contrib = ir::Mul::create(cache_lookup_block, args[2], mult65, gen_local_var_name());
        cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), arg2_contrib);

        auto final_hash = ir::Add::create(cache_lookup_block, hash_val, arg2_contrib, gen_local_var_name());
        cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), final_hash);
        hash_val = final_hash;
    }

    // Fast indexing: slot = hash & (CACHE_SIZE - 1)
    auto cache_slot = ir::And::create(cache_lookup_block, hash_val, cache_mask, gen_local_var_name());
    cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), cache_slot);

    auto cache_index = ir::Mul::create(cache_lookup_block, cache_slot, entry_size_const, gen_local_var_name());
    cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), cache_index);

    // Get cache entry
    auto cache_entry =
        ir::Getelementptr::create(cache_lookup_block, cache_var, {zero_const, cache_index}, gen_local_var_name());
    cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), cache_entry);
    // Store the entry pointer for later reuse on the miss/compute path
    auto store_entry_ptr = ir::Store::create(cache_lookup_block, cache_entry, memo_entry_slot, gen_local_var_name());
    cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), store_entry_ptr);

    // Check if entry is valid and arguments match (all in one block)
    auto has_value_offset = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), entry_size - 1);
    auto has_value_ptr =
        ir::Getelementptr::create(cache_lookup_block, cache_entry, {has_value_offset}, gen_local_var_name());
    cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), has_value_ptr);

    auto has_value = ir::Load::create(cache_lookup_block, has_value_ptr, gen_local_var_name());
    cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), has_value);

    auto is_valid =
        ir::ICmp::create(cache_lookup_block, ir::ICmp::ICmpType::NE, has_value, zero_const, gen_local_var_name());
    cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), is_valid);

    // Check arguments match (if valid)
    std::shared_ptr<ir::Value> args_match = is_valid;

    for (size_t i = 0; i < args.size(); ++i) {
        auto arg_offset = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), static_cast<int>(i));
        auto stored_arg_ptr =
            ir::Getelementptr::create(cache_lookup_block, cache_entry, {arg_offset}, gen_local_var_name());
        cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), stored_arg_ptr);

        auto stored_arg = ir::Load::create(cache_lookup_block, stored_arg_ptr, gen_local_var_name());
        cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), stored_arg);

        auto arg_eq =
            ir::ICmp::create(cache_lookup_block, ir::ICmp::ICmpType::EQ, stored_arg, args[i], gen_local_var_name());
        cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), arg_eq);

        auto new_match = ir::And::create(cache_lookup_block, args_match, arg_eq, gen_local_var_name());
        cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), new_match);
        args_match = new_match;
    }

    // Create cache hit path in same block to avoid SSA issues
    auto cache_hit_merge_block = std::make_shared<ir::BasicBlock>(func, "cache_hit_merge");
    auto cache_miss_block = std::make_shared<ir::BasicBlock>(func, "cache_miss");

    func->add_basic_block(insert_pos, cache_hit_merge_block);
    func->add_basic_block(insert_pos, cache_miss_block);

    // Branch based on complete match
    auto lookup_branch =
        ir::Br::create(cache_lookup_block, args_match, cache_hit_merge_block, cache_miss_block, gen_local_var_name());
    cache_lookup_block->add_instruction(cache_lookup_block->get_instructions_ref().end(), lookup_branch);

    // === Cache Hit Merge Block ===
    // Load result directly (cache_entry is accessible here via phi)
    auto hit_cache_entry = ir::Phi::create(cache_hit_merge_block,
                                           {cache_entry},
                                           {cache_lookup_block},
                                           ir::PointerType::get(ir::IntegerType::get(32)),
                                           gen_local_var_name());
    cache_hit_merge_block->add_instruction(cache_hit_merge_block->get_instructions_ref().end(), hit_cache_entry);

    auto result_offset = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), static_cast<int>(args.size()));
    auto result_ptr =
        ir::Getelementptr::create(cache_hit_merge_block, hit_cache_entry, {result_offset}, gen_local_var_name());
    cache_hit_merge_block->add_instruction(cache_hit_merge_block->get_instructions_ref().end(), result_ptr);

    auto cached_result = ir::Load::create(cache_hit_merge_block, result_ptr, gen_local_var_name());
    cache_hit_merge_block->add_instruction(cache_hit_merge_block->get_instructions_ref().end(), cached_result);

    auto hit_ret = ir::Ret::create(cache_hit_merge_block, cached_result, gen_local_var_name());
    cache_hit_merge_block->add_instruction(cache_hit_merge_block->get_instructions_ref().end(), hit_ret);

    // === Cache Miss Block ===
    // Jump to compute
    auto miss_jump = ir::Br::create(cache_miss_block, compute_block, gen_local_var_name());
    cache_miss_block->add_instruction(cache_miss_block->get_instructions_ref().end(), miss_jump);

    // === Update CFG info for the newly created blocks ===
    // Clear and rebuild successors for cache_lookup_block
    cache_lookup_block->opt_info.successors.clear();
    cache_lookup_block->opt_info.successors.push_back(cache_hit_merge_block);
    cache_lookup_block->opt_info.successors.push_back(cache_miss_block);

    // Set predecessors for new blocks
    cache_hit_merge_block->opt_info.predecessors.push_back(cache_lookup_block);
    cache_miss_block->opt_info.predecessors.push_back(cache_lookup_block);

    // cache_miss_block → compute_block
    cache_miss_block->opt_info.successors.push_back(compute_block);
    compute_block->opt_info.predecessors.push_back(cache_miss_block);

    // === Add Basic Cache Storage ===
    // Add storage logic to return instructions in compute path
    std::vector<std::shared_ptr<ir::BasicBlock>> return_blocks;
    for (auto &block : func->get_basic_blocks_ref()) {
        if (block == entry_block || block == cache_lookup_block || block == cache_hit_merge_block ||
            block == cache_miss_block)
            continue;

        auto tail = block->tail_instruction();
        if (auto ret = std::dynamic_pointer_cast<ir::Ret>(tail)) {
            if (!ret->get_operands_ref().empty()) {
                return_blocks.push_back(block);
            }
        }
    }

    // For each return block, add simplified cache storage (reusing cached entry pointer)
    for (auto &block : return_blocks) {
        auto tail = block->tail_instruction();
        if (auto ret = std::dynamic_pointer_cast<ir::Ret>(tail)) {
            auto ret_val = ret->get_operands_ref()[0];
            // Load cached entry pointer saved in cache_lookup
            auto local_one = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 1);
            auto local_cache_entry = ir::Load::create(block, memo_entry_slot, gen_local_var_name());
            block->add_instruction(ret->node, local_cache_entry);

            // Store arguments
            for (size_t i = 0; i < args.size(); ++i) {
                auto local_arg_offset =
                    std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), static_cast<int>(i));
                auto local_arg_ptr =
                    ir::Getelementptr::create(block, local_cache_entry, {local_arg_offset}, gen_local_var_name());
                block->add_instruction(ret->node, local_arg_ptr);

                auto store_arg = ir::Store::create(block, args[i], local_arg_ptr, gen_local_var_name());
                block->add_instruction(ret->node, store_arg);
            }

            // Store result
            auto local_result_offset =
                std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), static_cast<int>(args.size()));
            auto local_result_ptr =
                ir::Getelementptr::create(block, local_cache_entry, {local_result_offset}, gen_local_var_name());
            block->add_instruction(ret->node, local_result_ptr);

            auto store_result = ir::Store::create(block, ret_val, local_result_ptr, gen_local_var_name());
            block->add_instruction(ret->node, store_result);

            // Mark as valid
            auto local_valid_offset = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), entry_size - 1);
            auto local_valid_ptr =
                ir::Getelementptr::create(block, local_cache_entry, {local_valid_offset}, gen_local_var_name());
            block->add_instruction(ret->node, local_valid_ptr);

            auto store_valid = ir::Store::create(block, local_one, local_valid_ptr, gen_local_var_name());
            block->add_instruction(ret->node, store_valid);
        }
    }

    logger::INFO(
        "Added correct memoization for: ", func->get_name(), " (", CACHE_SIZE, " slots, ", total_size * 4, " bytes)");
    logger::WARN("CFG and dominance info invalidated - will be rebuilt by optimization engine");

    modified = true;
}

bool FunctionMemoization::run(ir::Module *module) {
    logger::INFO("Running FunctionMemoization (corrected)...");
    modified = false;
    candidates_to_memoize.clear();

    for (auto &func : module->get_functions_ref()) {
        MemoizationCandidate candidate;
        if (analyze_function_for_memoization(func, candidate)) {
            candidates_to_memoize.push_back(candidate);
        }
    }

    for (const auto &candidate : candidates_to_memoize) {
        add_memoization_to_function(candidate, module);
    }

    logger::INFO("Applied memoization to ", candidates_to_memoize.size(), " functions");
    return modified;
}

}  // namespace opt::pass
