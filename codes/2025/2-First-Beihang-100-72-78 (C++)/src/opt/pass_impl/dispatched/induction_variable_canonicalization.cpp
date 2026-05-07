#include <memory>

#include "ir/instruction.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
bool InductionVariableCanonicalization::run(ir::Module *module) {
    logger::INFO("Running InductionVariableCanonicalization...");
    for (auto &func : module->get_functions_ref()) {
        for (auto &block : func->get_basic_blocks_ref()) {
            auto tail_br = std::dynamic_pointer_cast<ir::Br>(block->tail_instruction());
            if (!tail_br || !tail_br->is_cond_branch()) continue;
            auto cond_cmp = std::dynamic_pointer_cast<ir::ICmp>(tail_br->get_operands_ref()[0]);
            if (!cond_cmp) continue;
            // make conversion right
            if (std::dynamic_pointer_cast<ir::Constant>(cond_cmp->get_operands_ref()[0]) &&
                !std::dynamic_pointer_cast<ir::Constant>(cond_cmp->get_operands_ref()[1])) {
                auto operands_ref = cond_cmp->get_operands_ref();
                std::swap(operands_ref[0], operands_ref[1]);
                // cond_cmp->set_cmp_type(~cond_cmp->get_cmp_type());
            }

            // i <= n -> i < n + 1
            // TODO(Xingkun): need rotate
            auto true_target = std::dynamic_pointer_cast<ir::BasicBlock>(tail_br->get_operands_ref()[1]);
            auto false_target = std::dynamic_pointer_cast<ir::BasicBlock>(tail_br->get_operands_ref()[2]);
            if (true_target != false_target && cond_cmp->get_users_ref().size() == 1 &&
                true_target->opt_info.dominates(block) &&
                (!false_target->opt_info.dominates(block) || false_target->opt_info.dominates(true_target))) {
                // FIXME: if is possible to overflow
                if (cond_cmp->get_cmp_type() == ir::ICmp::ICmpType::SLE) {
                    auto next_add = std::dynamic_pointer_cast<ir::Add>(cond_cmp->get_operands_ref()[0]);
                    if (!next_add) continue;
                    if (auto match_res = util::match_binary_operands<ir::Phi, ir::ConstantInt>(next_add);
                        match_res.has_value()) {
                        auto [ind_var, step] = match_res.value();
                        auto bound = cond_cmp->get_operands_ref()[1];
                        auto bound_inst = std::dynamic_pointer_cast<ir::Instruction>(bound);
                        if (bound_inst) {
                            auto add = ir::Add::create(bound_inst->get_parent_block().lock(),
                                                       bound,
                                                       std::make_shared<ir::ConstantInt>(bound->get_type(), 1),
                                                       gen_local_var_name());
                            auto instructions = bound_inst->get_parent_block().lock()->get_instructions_ref();
                            auto first_none_phi =
                                std::find_if(instructions.begin(), instructions.end(), [](const auto &inst) {
                                    return !std::dynamic_pointer_cast<ir::Phi>(inst);
                                });
                            bound_inst->get_parent_block().lock()->add_instruction((*first_none_phi)->node, add);
                            cond_cmp->get_operands_ref()[1] = add;
                            cond_cmp->set_cmp_type(ir::ICmp::ICmpType::SLT);
                        } else if (bound_inst->get_parent_block().lock() == ind_var->get_parent_block().lock() &&
                                   bound_inst->get_parent_block().lock()->opt_info.dominates(
                                       ind_var->get_parent_block().lock())) {
                            auto add = ir::Add::create(func->entry_block(),
                                                       bound,
                                                       std::make_shared<ir::ConstantInt>(bound->get_type(), 1),
                                                       gen_local_var_name());
                            func->entry_block()->add_instruction(func->entry_block()->get_instructions_ref().begin(),
                                                                 add);
                            cond_cmp->get_operands_ref()[1] = add;
                            cond_cmp->set_cmp_type(ir::ICmp::ICmpType::SLT);
                        }
                    }
                }
            }
        }
    }
    return false;
}
}  // namespace opt::pass
