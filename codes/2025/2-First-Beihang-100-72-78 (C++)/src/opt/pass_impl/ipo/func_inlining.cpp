#include <cstddef>
#include <list>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
static bool modified = false;
static std::queue<std::shared_ptr<ir::Function>> inline_worklist;

static void collect_worklist(ir::Module *module);
static void inline_one_function(std::shared_ptr<ir::Function> callee);
static void substitute_call(std::shared_ptr<ir::Call> call);
std::shared_ptr<ir::Function> clone_func(const std::shared_ptr<ir::Function> &function);

bool FunctionInlining::run(ir::Module *module) {
    logger::INFO("Running FunctionInlining...");
    modified = false;
    collect_worklist(module);
    modified = !inline_worklist.empty();
    while (!inline_worklist.empty()) {
        inline_one_function(inline_worklist.front());
        inline_worklist.pop();
    }

    std::vector<std::shared_ptr<ir::Function>> to_remove;
    for (const auto &func : module->get_functions_ref()) {
        if (!func->is_main() && func->opt_info.callers.empty()) {
            to_remove.push_back(func);
        }
    }
    for (const auto &func : to_remove) {
        util::remove_function(module, func);
    }

    // util::static_check(module);
    return modified;
}

static void collect_worklist(ir::Module *module) {
    module->for_each_func([](auto &function) {
        if (function->is_main()) return;
        if (function->opt_info.is_recursive) return;
        inline_worklist.push(function);
    });
}

static void inline_one_function(std::shared_ptr<ir::Function> callee) {
    std::vector<std::shared_ptr<ir::Call>> call_sites;
    // collect call sites
    if (!callee->opt_info.callers.empty()) {
        for (const auto &weak_caller : callee->opt_info.callers) {
            auto caller = weak_caller.lock();
            caller->for_each_block([&](auto &block) {
                block->for_each_instruction([&](auto &inst) {
                    if (inst->is_type(ir::Instruction::InstructionType::CALL)) {
                        auto call = std::dynamic_pointer_cast<ir::Call>(inst);
                        if (call->get_function() == callee) {
                            call_sites.push_back(call);
                        }
                    }
                });
            });
        }
    }

    for (auto &call : call_sites) {
        substitute_call(call);
    }

    for (const auto &weak_caller : callee->opt_info.callers) {
        auto caller = weak_caller.lock();
        caller->opt_info.callees.erase(callee);
        caller->opt_info.callees.insert(callee->opt_info.callees.begin(), callee->opt_info.callees.end());
    }
    callee->opt_info.callers.clear();
}

static void substitute_call(std::shared_ptr<ir::Call> call) {
    auto cur_block = call->get_parent_block().lock();
    auto cur_function = cur_block->get_parent_func().lock();
    auto callee = call->get_function();
    auto ret_type = callee->get_return_type();

    auto next_block = std::make_shared<ir::BasicBlock>(cur_function, gen_block_name());
    cur_function->add_basic_block(std::next(cur_block->node), next_block);

    // split block
    auto &instructions = cur_block->get_instructions_ref();
    bool split_point = false;
    for (auto it = instructions.begin(); it != instructions.end();) {
        if (!split_point && *it == call) {
            split_point = true;
            it++;
            continue;
        }
        if (split_point) {
            auto instruction = *it;
            it = instructions.erase(it);
            next_block->add_instruction(next_block->get_instructions_ref().end(), instruction);
            instruction->set_parent_block(next_block);
        } else {
            ++it;
        }
    }

    // Update PHI instructions in successor blocks to reflect the new control flow
    // When we split the current block, we need to update any PHI instructions in successor blocks
    // that reference the current block to now reference the new block (next_block)
    for (auto &weak_successor : cur_block->opt_info.successors) {
        auto successor = weak_successor.lock();
        for (auto &inst : successor->get_instructions_ref()) {
            if (inst->is_type(ir::Instruction::InstructionType::PHI) && contains(inst->get_operands_ref(), cur_block)) {
                util::substitute_operand(inst, cur_block, next_block);
            }
        }
    }
    next_block->opt_info.successors.insert(next_block->opt_info.successors.end(),
                                           cur_block->opt_info.successors.begin(),
                                           cur_block->opt_info.successors.end());

    for (auto &weak_successor : cur_block->opt_info.successors) {
        auto successor = weak_successor.lock();
        auto pre_blocks = successor->opt_info.predecessors;
        for (auto &pre_block : pre_blocks) {
            if (pre_block.lock() == cur_block) {
                successor->opt_info.remove_predecessor(cur_block);
                successor->opt_info.predecessors.push_back(next_block);
            }
        }
    }

    // Create a copy of successors to avoid iterator invalidation
    auto successors_copy = cur_block->opt_info.successors;
    for (auto &weak_successor : successors_copy) {
        auto successor = weak_successor.lock();
        if (successor) {
            // Remove cur_block from successor's predecessors
            successor->opt_info.remove_predecessor(cur_block);
            // Add next_block to successor's predecessors
            successor->opt_info.predecessors.push_back(next_block);
        }
    }
    cur_block->opt_info.successors.clear();

    // clone a function
    auto function_copy = clone_func(callee);
    // replacec arguments
    for (size_t i = 0; i < function_copy->get_arguments_ref().size(); i++) {
        auto f_arg = function_copy->get_arguments_ref()[i];
        auto r_arg = call->get_operands_ref()[i + 1];
        if (r_arg->get_type()->is_integer_ty() || r_arg->get_type()->is_float_ty()) {
            util::replace_all_uses_with(f_arg, r_arg);
            continue;
        }
        auto users = f_arg->get_users_ref();
        for (auto &user : users) {
            if (auto store = std::dynamic_pointer_cast<ir::Store>(user.lock())) {
                if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(store->get_operands_ref()[1])) {
                    alloca->get_parent_block().lock()->erase_instruction(alloca->node);
                    for (auto &usr : alloca->get_users_ref()) {
                        if (std::dynamic_pointer_cast<ir::Load>(usr.lock())) {
                            util::replace_all_uses_with(usr.lock(), r_arg);
                        }
                        auto inst = std::dynamic_pointer_cast<ir::Instruction>(usr.lock());
                        inst->get_parent_block().lock()->erase_instruction(inst->node);
                    }
                    util::remove_all_operands(alloca);
                }
            } else {
                util::replace_all_uses_with(f_arg, r_arg);
            }
        }
    }

    // replace Ret with Br and Phi
    std::vector<std::shared_ptr<ir::Ret>> ret_list;
    for (auto &block : function_copy->get_basic_blocks_ref()) {
        for (auto &inst : block->get_instructions_ref()) {
            if (inst->is_type(ir::Instruction::InstructionType::RET))
                ret_list.push_back(std::dynamic_pointer_cast<ir::Ret>(inst));
        }
    }

    // Collect return values and their corresponding blocks for Phi creation
    std::vector<std::shared_ptr<ir::Value>> ret_values;
    std::vector<std::shared_ptr<ir::BasicBlock>> ret_blocks;

    for (auto &ret : ret_list) {
        if (ret_type->is_float_ty() || ret_type->is_integer_ty()) {
            ret_values.push_back(ret->get_operands_ref()[0]);
            ret_blocks.push_back(ret->get_parent_block().lock());
        }
        auto parent_block = ret->get_parent_block().lock();
        parent_block->opt_info.remove_successor(next_block);
        next_block->opt_info.remove_predecessor(parent_block);
        auto br = ir::Br::create(parent_block, next_block, gen_block_name());
        parent_block->add_instruction(ret->node, br);
        util::substitute(ret, br);
    }

    if (ret_type->is_float_ty() || ret_type->is_integer_ty()) {
        // Create Phi with values and their corresponding blocks using the overloaded create method
        auto phi = ir::Phi::create(next_block, ret_values, ret_blocks, ret_type, gen_local_var_name());
        // Add the phi instruction to next_block
        next_block->add_instruction(next_block->get_instructions_ref().begin(), phi);
        util::replace_all_uses_with(call, phi);
    }

    auto copy_function_blocks = function_copy->get_basic_blocks_ref();
    auto blocks_copy = copy_function_blocks;
    for (auto &block : blocks_copy) {
        block->get_parent_func().lock()->erase_basic_block(block->node);
        cur_function->add_basic_block(next_block->node, block);
        block->set_parent_func(cur_function);
    }
    cur_block->add_instructions({ir::Br::create(cur_block, blocks_copy.front(), gen_local_var_name())});
    util::remove_instruction(call);
}

std::shared_ptr<ir::Function> clone_func(const std::shared_ptr<ir::Function> &function) {
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> value_clone_map;
    auto new_function = std::make_shared<ir::Function>(function->get_type(), function->get_name());

    // Clone arguments
    std::vector<std::shared_ptr<ir::Argument>> new_args;
    for (auto &&arg : function->get_arguments_ref()) {
        auto new_arg = std::make_shared<ir::Argument>(arg->get_type(), new_function, gen_local_var_name());
        value_clone_map[arg] = new_arg;
        new_args.push_back(new_arg);
    }
    new_function->set_arguments(std::move(new_args));

    // Clone basic blocks
    std::list<std::shared_ptr<ir::BasicBlock>> new_blocks;
    for (auto &block : function->get_basic_blocks_ref()) {
        auto new_block = std::make_shared<ir::BasicBlock>(new_function, gen_block_name());
        value_clone_map[block] = new_block;

        // Clone instructions
        for (auto &inst : block->get_instructions_ref()) {
            auto new_inst = inst->clone();
            new_block->add_instruction(new_block->get_instructions_ref().end(), new_inst);
            new_inst->set_parent_block(new_block);
            value_clone_map[inst] = new_inst;
        }

        new_blocks.push_back(new_block);
    }
    for (auto &block : new_blocks) {
        for (auto &inst : block->get_instructions_ref()) {
            util::substitute_operand(inst, value_clone_map);
        }
    }
    new_function->add_basic_blocks(std::move(new_blocks));

    return new_function;
}

}  // namespace opt::pass
