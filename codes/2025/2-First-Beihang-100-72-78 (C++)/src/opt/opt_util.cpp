#include "opt_util.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <queue>
#include <stack>
#include <unordered_map>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "ir/module.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "util.hpp"

namespace opt::util {
void remove_all_operands(std::shared_ptr<ir::User> user) {
    for (auto &operand : user->get_operands_ref()) {
        operand->remove_user(user);
    }
    user->get_operands_ref().clear();
}

void remove_value(const std::shared_ptr<ir::Value> &value) {
    // remove from all users' operands
    for (const auto &weak_user : value->get_users_ref()) {
        if (auto user = weak_user.lock()) {
            auto &ops = user->get_operands_ref();
            ops.erase(std::remove_if(
                          ops.begin(), ops.end(), [&value](const std::shared_ptr<ir::Value> &v) { return v == value; }),
                      ops.end());
        }
    }
    value->get_users_ref().clear();

    // if value is a user, remove from all operands' users
    if (auto user = std::dynamic_pointer_cast<ir::User>(value)) {
        remove_all_operands(user);
    }
}

void remove_value_safe(const std::shared_ptr<ir::Value> &value) {
    // remove from all users' operands
    for (const auto &weak_user : value->get_users_ref()) {
        if (auto user = weak_user.lock()) {
            if (auto phi = std::dynamic_pointer_cast<ir::Phi>(user)) {
                // special handling for phi nodes
                remove_phi_value(phi, value);
                continue;
            }
            auto &ops = user->get_operands_ref();
            ops.erase(std::remove_if(
                          ops.begin(), ops.end(), [&value](const std::shared_ptr<ir::Value> &v) { return v == value; }),
                      ops.end());
        }
    }
    value->get_users_ref().clear();

    // if value is a user, remove from all operands' users
    if (auto user = std::dynamic_pointer_cast<ir::User>(value)) {
        remove_all_operands(user);
    }
}

ir::InstructionNode remove_instruction(const std::shared_ptr<ir::Instruction> &instruction) {
    remove_value(instruction);
    return instruction->get_parent_block().lock()->get_instructions_ref().erase(instruction->node);
}

ir::BasicBlockNode remove_basic_block(const std::shared_ptr<ir::BasicBlock> &basic_block) {
    // special treatment for phi instruction
    // collect phis first to avoid iterator invalidation
    std::vector<std::shared_ptr<ir::Phi>> phis_to_remove;
    for (const auto &weak_user : basic_block->get_users_ref()) {
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(weak_user.lock())) {
            phis_to_remove.push_back(phi);
        }
    }
    // remove phi blocks after collecting all phis
    for (const auto &phi : phis_to_remove) {
        remove_phi_block(phi, basic_block);
    }
    // make a copy to avoid potential iterator invalidation
    auto instructions_copy = basic_block->get_instructions_ref();
    for (const auto &instruction : instructions_copy) {
        remove_value_safe(instruction);
    }

    for (const auto &weak_successor : basic_block->opt_info.successors) {
        if (auto successor = weak_successor.lock()) {
            successor->opt_info.remove_predecessor(basic_block);
        }
    }
    for (const auto &weak_predecessor : basic_block->opt_info.predecessors) {
        if (auto predecessor = weak_predecessor.lock()) {
            predecessor->opt_info.remove_successor(basic_block);
        }
    }
    auto res = basic_block->get_parent_func().lock()->get_basic_blocks_ref().erase(basic_block->node);
    remove_value(basic_block);
    return res;
}

void remove_function(ir::Module *module, const std::shared_ptr<ir::Function> &function) {
    // remove function from module
    auto &functions = module->get_functions_ref();
    functions.erase(std::remove(functions.begin(), functions.end(), function), functions.end());

    // prevent it from effecting global variables and functions
    for (auto &basic_block : function->get_basic_blocks_ref()) {
        for (const auto &instruction : basic_block->get_instructions_ref()) {
            for (auto &global_var : module->get_global_variables_ref()) {
                for (auto &user : global_var->get_users_ref()) {
                    if (user.lock() == instruction) {
                        // remove global variable from instruction's operands
                        auto &operands = instruction->get_operands_ref();
                        operands.erase(std::remove(operands.begin(), operands.end(), global_var), operands.end());
                        // remove instruction from global variable's users
                        global_var->remove_user(instruction);
                    }
                }
            }
            for (auto &func : module->get_all_functions()) {
                for (auto &user : func->get_users_ref()) {
                    if (user.lock() == instruction) {
                        // remove function from instruction's operands
                        auto &operands = instruction->get_operands_ref();
                        operands.erase(std::remove(operands.begin(), operands.end(), func), operands.end());
                        // remove instruction from function's users
                        func->remove_user(instruction);
                    }
                }
            }
        }
    }

    remove_value(function);
}

void remove_global_variable(ir::Module *module, const std::shared_ptr<ir::GlobalVariable> &global_var) {
    // remove global variable from module
    auto &global_variables = module->get_global_variables_ref();
    global_variables.erase(std::remove(global_variables.begin(), global_variables.end(), global_var),
                           global_variables.end());

    remove_value(global_var);
}

void substitute_operand(std::shared_ptr<ir::User> user, size_t pos, std::shared_ptr<ir::Value> new_operand) {
    auto &old_operand = user->get_operands_ref()[pos];

    // remove user from old operand's users using the remove_user method
    old_operand->remove_user(user);

    // set new operand
    user->get_operands_ref()[pos] = new_operand;
    // add user to new operand's users
    new_operand->add_user(user);
}

void substitute_operand(std::shared_ptr<ir::User> user,
                        std::shared_ptr<ir::Value> old_operand,
                        std::shared_ptr<ir::Value> new_operand) {
    for (size_t i = 0; i < user->get_operands_ref().size(); i++) {
        if (user->get_operands_ref()[i] == old_operand) {
            substitute_operand(user, i, new_operand);
        }
    }
}

void substitute_operand(std::shared_ptr<ir::User> user,
                        std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> &value_clone_map) {
    for (size_t i = 0; i < user->get_operands_ref().size(); i++) {
        if (value_clone_map.count(user->get_operands_ref()[i])) {
            substitute_operand(user, i, value_clone_map[user->get_operands_ref()[i]]);
        }
    }
}

void replace_all_uses_with(std::shared_ptr<ir::Value> ori, std::shared_ptr<ir::Value> replacement) {
    // make a copy to avoid iterator invalidation
    auto users_copy = ori->get_users_ref();
    for (const auto &weak_user : users_copy) {
        if (auto user = weak_user.lock()) {
            substitute_operand(user, ori, replacement);
        }
    }
}

ir::InstructionNode substitute(std::shared_ptr<ir::Instruction> cur, std::shared_ptr<ir::Value> target) {
    if (cur == target) {
        logger::ERROR("[opt::util::substitute] cur == target");
        __builtin_unreachable();
    }

    replace_all_uses_with(cur, target);
    assert(cur->get_users_ref().empty());

    // remove from operand
    for (const auto &operand : cur->get_operands_ref()) {
        operand->remove_user(cur);
    }

    // remove from parent block
    return cur->get_parent_block().lock()->get_instructions_ref().erase(cur->node);
}

void add_incoming(const std::shared_ptr<ir::Phi> &phi,
                  std::shared_ptr<ir::BasicBlock> block,
                  std::shared_ptr<ir::Value> value) {
    phi->add_operand(value);
    value->add_user(phi);
    phi->add_operand(block);
    block->add_user(phi);
    if (value->get_type() != phi->get_type()) phi->set_type(value->get_type());
}

void remove_phi_block(std::shared_ptr<ir::Phi> phi, const std::shared_ptr<ir::BasicBlock> &block) {
    auto &operands = phi->get_operands_ref();

    for (size_t i = 0; i < operands.size(); i += 2) {
        if (i + 1 < operands.size() && operands[i + 1] == block) {
            auto value = operands[i];
            operands.erase(operands.begin() + static_cast<std::ptrdiff_t>(i),
                           operands.begin() + static_cast<std::ptrdiff_t>(i + 2));
            if (!contains(operands, value)) {
                value->remove_user(phi);
            }
            block->remove_user(phi);
            return;
        }
    }
    logger::ERROR("[opt::util::remove_phi_block] phi block not found");
    __builtin_unreachable();
}

void remove_phi_value(std::shared_ptr<ir::Phi> phi, std::shared_ptr<ir::Value> value) {
    auto &operands = phi->get_operands_ref();
    for (size_t i = 0; i < operands.size(); i += 2) {
        if (operands[i] == value) {
            auto block = std::dynamic_pointer_cast<ir::BasicBlock>(operands[i + 1]);
            block->remove_user(phi);
            value->remove_user(phi);
            operands.erase(operands.begin() + static_cast<std::ptrdiff_t>(i),
                           operands.begin() + static_cast<std::ptrdiff_t>(i + 2));
            return;
        }
    }
}

bool is_immediate_dominator_of(const std::shared_ptr<ir::BasicBlock> &self,
                               const std::shared_ptr<ir::BasicBlock> &other) {
    // strict dominance: dominator basic block must not be the dominated basic block itself
    if (self == other) {
        return false;
    }

    // immediate dominance: dominator basic block must be directly dominated by dominated basic block
    for (const auto &weak_dominated_basic_block : self->opt_info.dominated) {
        auto dominated_basic_block = weak_dominated_basic_block.lock();
        if (dominated_basic_block != self && dominated_basic_block != other &&
            dominated_basic_block->opt_info.dominates(other)) {
            return false;
        }
    }

    return true;
}

std::vector<std::shared_ptr<ir::BasicBlock>> post_order_traversal_dom(std::shared_ptr<ir::Function> func) {
    std::vector<std::shared_ptr<ir::BasicBlock>> result;
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> can_visited;
    std::stack<std::shared_ptr<ir::BasicBlock>> stack;
    stack.push(func->entry_block());

    while (!stack.empty()) {
        auto parent = stack.top();
        if (can_visited.count(parent)) {
            result.push_back(parent);
            stack.pop();
            continue;
        }
        for (auto &idomee : parent->opt_info.immediate_dominated) {
            stack.push(idomee.lock());
        }
        can_visited.insert(parent);
    }
    return result;
}

std::vector<std::shared_ptr<ir::BasicBlock>> layer_traversal_dom(std::shared_ptr<ir::Function> func) {
    if (func->get_basic_blocks_ref().empty()) return {};
    std::vector<std::shared_ptr<ir::BasicBlock>> result;
    auto entry = func->entry_block();
    std::queue<std::shared_ptr<ir::BasicBlock>> queue;
    queue.push(entry);
    while (!queue.empty()) {
        auto cur = queue.front();
        queue.pop();
        result.push_back(cur);
        for (auto &idomee : cur->opt_info.immediate_dominated) {
            queue.push(idomee.lock());
        }
    }
    return result;
}

void redirect(std::shared_ptr<ir::BasicBlock> from,
              std::shared_ptr<ir::BasicBlock> ori_target,
              std::shared_ptr<ir::BasicBlock> new_target) {
    auto tail_br = std::dynamic_pointer_cast<ir::Br>(from->tail_instruction());
    if (!tail_br) {
        logger::ERROR("[opt::util::redirect] tail instruction is not a branch");
        __builtin_unreachable();
    }
    substitute_operand(tail_br, ori_target, new_target);
    from->opt_info.remove_successor(ori_target);
    from->opt_info.successors.push_back(new_target);
    ori_target->opt_info.remove_predecessor(from);
    new_target->opt_info.predecessors.push_back(from);
}

void jump(std::shared_ptr<ir::BasicBlock> from, std::shared_ptr<ir::BasicBlock> to) {
    auto br = ir::Br::create(from, to, gen_local_var_name());
    from->add_instructions({br});
    from->opt_info.successors.push_back(to);
    to->opt_info.predecessors.push_back(from);
}

std::vector<std::shared_ptr<ir::Phi>> get_phis(std::shared_ptr<ir::BasicBlock> block) {
    std::vector<std::shared_ptr<ir::Phi>> phis;
    for (const auto &instruction : block->get_instructions_ref()) {
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(instruction))
            phis.push_back(phi);
        else
            break;
    }
    return phis;
}

std::shared_ptr<ir::Value> get_phi_value(std::shared_ptr<ir::Phi> phi, std::shared_ptr<ir::BasicBlock> block) {
    for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
        if (phi->get_operands_ref()[i + 1] == block) {
            return phi->get_operands_ref()[i];
        }
    }
    logger::ERROR("[opt::util::get_phi_value] phi block not found");
    __builtin_unreachable();
}

std::shared_ptr<ir::Value> get_phi_value_safe(std::shared_ptr<ir::Phi> phi, std::shared_ptr<ir::BasicBlock> block) {
    for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
        if (phi->get_operands_ref()[i + 1] == block) {
            return phi->get_operands_ref()[i];
        }
    }
    return nullptr;
}

std::shared_ptr<ir::BasicBlock> get_phi_block(std::shared_ptr<ir::Phi> phi, std::shared_ptr<ir::Value> value) {
    for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
        if (phi->get_operands_ref()[i] == value) {
            return std::dynamic_pointer_cast<ir::BasicBlock>(phi->get_operands_ref()[i + 1]);
        }
    }
    logger::ERROR("[opt::util::get_phi_block] phi block not found");
    __builtin_unreachable();
}

std::vector<std::shared_ptr<ir::Value>> get_phi_values(std::shared_ptr<ir::Phi> phi) {
    std::vector<std::shared_ptr<ir::Value>> values;
    for (size_t i = 0; i < phi->get_operands_ref().size(); i += 2) {
        values.push_back(phi->get_operands_ref()[i]);
    }
    return values;
}

std::shared_ptr<ir::BasicBlock> split_block_predecessors(std::shared_ptr<ir::BasicBlock> block,
                                                         std::unordered_set<std::shared_ptr<ir::BasicBlock>> &preds,
                                                         std::shared_ptr<ir::BasicBlock> trampoline) {
    auto function = block->get_parent_func().lock();
    if (!trampoline) {
        trampoline = std::make_shared<ir::BasicBlock>(function, gen_block_name());
        function->add_basic_blocks({trampoline});
    }

    // redirect all predecessors to the new block
    for (const auto &pred : preds) {
        redirect(pred, block, trampoline);
    }
    // forward phis
    for (const auto &phi : get_phis(block)) {
        std::vector<std::shared_ptr<ir::Value>> phi_values;
        std::vector<std::shared_ptr<ir::BasicBlock>> phi_blocks;
        for (const auto &pred : preds) {
            phi_blocks.push_back(pred);
            phi_values.push_back(util::get_phi_value(phi, pred));
            util::remove_phi_block(phi, pred);
        }
        auto new_phi = ir::Phi::create(trampoline, phi_values, phi_blocks, phi->get_type(), gen_local_var_name());
        trampoline->add_instructions({new_phi});
        add_incoming(phi, trampoline, new_phi);
    }
    jump(trampoline, block);
    return trampoline;
}

void remove_instruction_from_parent_basic_block(std::shared_ptr<ir::Instruction> instruction) {
    instruction->get_parent_block().lock()->get_instructions_ref().remove(instruction);
    instruction->set_parent_block(nullptr);
}

void add_instruction_before(std::shared_ptr<ir::Instruction> instruction,
                            std::shared_ptr<ir::Instruction> destination) {
    auto destination_block = destination->get_parent_block().lock();
    instruction->set_parent_block(destination_block);
    destination_block->add_instruction(destination->node, instruction);
}

bool is_global(std::shared_ptr<ir::Value> value) {
    if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(value)) {
        return is_global(gep->base_ptr());
    }
    if (auto global_var = std::dynamic_pointer_cast<ir::GlobalVariable>(value)) {
        return true;
    }
    return false;
}

bool is_local(std::shared_ptr<ir::Value> value, std::shared_ptr<ir::Function> func) {
    if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(value)) {
        return is_local(gep->base_ptr(), func);
    }
    if (auto arg = std::dynamic_pointer_cast<ir::Argument>(value)) {
        return true;
    }
    if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(value)) {
        return alloca->get_parent_block().lock()->get_parent_func().lock() == func;
    }
    return false;
}

void static_check(ir::Module *module) {
    for (auto &func : module->get_functions_ref()) {
        for (auto &block : func->get_basic_blocks_ref()) {
            for (auto &inst : block->get_instructions_ref()) {
                if (inst->get_ins_type() == ir::Instruction::InstructionType::ICMP ||
                    inst->get_ins_type() == ir::Instruction::InstructionType::FCMP) {
                    if (inst->get_operands_ref().size() != 2) {
                        assert(inst->get_operands_ref().size() == 2);
                    }
                }

                for (auto &weak_user : inst->get_users_ref()) {
                    auto user = weak_user.lock();
                    assert(user);
                    assert(contains(user->get_operands_ref(), inst));
                }

                for (auto &operand : inst->get_operands_ref()) {
                    assert(operand);
                    assert(contains(to_shared_vector(operand->get_users_ref()), inst));
                }

                // check phi operands
                if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
                    assert(phi->get_operands_ref().size() % 2 == 0);
                }
            }

            for (auto &weak_user : block->get_users_ref()) {
                auto user = weak_user.lock();
                assert(user);
                assert(contains(user->get_operands_ref(), block));
            }
        }
        for (auto &weak_user : func->get_users_ref()) {
            auto user = weak_user.lock();
            assert(user);
            assert(contains(user->get_operands_ref(), func));
        }
    }
}

std::vector<std::shared_ptr<ir::BasicBlock>> get_topo_sort_blocks(std::shared_ptr<ir::Function> func) {
    std::vector<std::shared_ptr<ir::BasicBlock>> result;
    auto &blocks = func->get_basic_blocks_ref();
    for (auto &block : blocks) {
        if (block->opt_info.successors.empty()) result.push_back(block);
    }
    for (size_t i = 0; i < result.size(); i++) {
        for (auto &pre : result[i]->opt_info.predecessors) {
            if (!contains(result, pre.lock())) {
                result.push_back(pre.lock());
            }
        }
    }
    return result;
}

std::shared_ptr<ir::BasicBlock> dom_lca(std::shared_ptr<ir::BasicBlock> block1,
                                        std::shared_ptr<ir::BasicBlock> block2) {
    if (!block1) return block2;
    if (!block2) return block1;
    while (block1->opt_info.dominance_depth < block2->opt_info.dominance_depth) {
        block2 = block2->opt_info.immediate_dominator.lock();
    }
    while (block2->opt_info.dominance_depth < block1->opt_info.dominance_depth) {
        block1 = block1->opt_info.immediate_dominator.lock();
    }
    while (block1 != block2) {
        block1 = block1->opt_info.immediate_dominator.lock();
        block2 = block2->opt_info.immediate_dominator.lock();
    }
    return block1;
}

}  // namespace opt::util
