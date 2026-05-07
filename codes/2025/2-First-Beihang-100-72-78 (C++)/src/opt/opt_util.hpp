#ifndef GEECEECEE_OPT_OPT_UTIL_HPP
#define GEECEECEE_OPT_OPT_UTIL_HPP

#include <memory>

#include "ir/mod.hpp"
#include "ir/value.hpp"
namespace opt::util {
void remove_all_operands(std::shared_ptr<ir::User> user);
ir::BasicBlockNode remove_basic_block(const std::shared_ptr<ir::BasicBlock> &);
ir::InstructionNode remove_instruction(const std::shared_ptr<ir::Instruction> &);
void remove_function(ir::Module *, const std::shared_ptr<ir::Function> &);
void remove_global_variable(ir::Module *, const std::shared_ptr<ir::GlobalVariable> &);
void substitute_operand(std::shared_ptr<ir::User>, size_t, std::shared_ptr<ir::Value>);
void substitute_operand(std::shared_ptr<ir::User>,
                        std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> &);
ir::InstructionNode substitute(std::shared_ptr<ir::Instruction>, std::shared_ptr<ir::Value>);
void replace_all_uses_with(std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>);
void substitute_operand(std::shared_ptr<ir::User>, std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>);
void add_incoming(const std::shared_ptr<ir::Phi> &phi,
                  std::shared_ptr<ir::BasicBlock> block,
                  std::shared_ptr<ir::Value> value);
void remove_phi_block(std::shared_ptr<ir::Phi> phi, const std::shared_ptr<ir::BasicBlock> &block);
void remove_phi_value(std::shared_ptr<ir::Phi> phi, std::shared_ptr<ir::Value> value);
bool is_immediate_dominator_of(const std::shared_ptr<ir::BasicBlock> &self,
                               const std::shared_ptr<ir::BasicBlock> &other);
std::vector<std::shared_ptr<ir::BasicBlock>> post_order_traversal_dom(std::shared_ptr<ir::Function> func);
std::vector<std::shared_ptr<ir::BasicBlock>> layer_traversal_dom(std::shared_ptr<ir::Function> func);
void redirect(std::shared_ptr<ir::BasicBlock> from,
              std::shared_ptr<ir::BasicBlock> ori_target,
              std::shared_ptr<ir::BasicBlock> new_target);
std::vector<std::shared_ptr<ir::Phi>> get_phis(std::shared_ptr<ir::BasicBlock> block);
std::shared_ptr<ir::Value> get_phi_value(std::shared_ptr<ir::Phi> phi, std::shared_ptr<ir::BasicBlock> block);
std::shared_ptr<ir::Value> get_phi_value_safe(std::shared_ptr<ir::Phi> phi, std::shared_ptr<ir::BasicBlock> block);
std::shared_ptr<ir::BasicBlock> get_phi_block(std::shared_ptr<ir::Phi> phi, std::shared_ptr<ir::Value> value);
std::vector<std::shared_ptr<ir::Value>> get_phi_values(std::shared_ptr<ir::Phi> phi);
void jump(std::shared_ptr<ir::BasicBlock> from, std::shared_ptr<ir::BasicBlock> to);
std::shared_ptr<ir::BasicBlock> split_block_predecessors(std::shared_ptr<ir::BasicBlock> block,
                                                         std::unordered_set<std::shared_ptr<ir::BasicBlock>> &preds,
                                                         std::shared_ptr<ir::BasicBlock> trampoline = nullptr);
void remove_instruction_from_parent_basic_block(std::shared_ptr<ir::Instruction> instruction);
void add_instruction_before(std::shared_ptr<ir::Instruction> instruction, std::shared_ptr<ir::Instruction> destination);
bool is_global(std::shared_ptr<ir::Value> value);
bool is_local(std::shared_ptr<ir::Value> value, std::shared_ptr<ir::Function> func);
template <typename T, typename U>
std::optional<std::pair<std::shared_ptr<T>, std::shared_ptr<U>>> match_binary_operands(
    std::shared_ptr<ir::Instruction> inst) {
    auto operands = inst->get_operands_ref();
    if (std::dynamic_pointer_cast<T>(operands[0]) && std::dynamic_pointer_cast<U>(operands[1]))
        return std::make_optional(
            std::make_pair(std::dynamic_pointer_cast<T>(operands[0]), std::dynamic_pointer_cast<U>(operands[1])));

    if (std::dynamic_pointer_cast<U>(operands[0]) && std::dynamic_pointer_cast<T>(operands[1]))
        return std::make_optional(
            std::make_pair(std::dynamic_pointer_cast<T>(operands[1]), std::dynamic_pointer_cast<U>(operands[0])));

    return std::nullopt;
}
std::vector<std::shared_ptr<ir::BasicBlock>> get_topo_sort_blocks(std::shared_ptr<ir::Function> func);
std::shared_ptr<ir::BasicBlock> dom_lca(std::shared_ptr<ir::BasicBlock> block1, std::shared_ptr<ir::BasicBlock> block2);
void static_check(ir::Module *module);
}  // namespace opt::util

#endif  // GEECEECEE_OPT_OPT_UTIL_HPP
