#include <memory>

#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"
namespace opt::pass {
static void analyze_ind_var(std::shared_ptr<ir::opt_support::LoopInfo> loop);

bool InductionVariableAnalyzation::run(ir::Module *module) {
    logger::INFO("Running InductionVariableAnalyzation...");
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            analyze_ind_var(loop);
        }
    }
    return false;
}

static ir::opt_support::IndVarInfo::CondType get_cond_type(std::shared_ptr<ir::ICmp> icmp) {
    switch (icmp->get_cmp_type()) {
        case ir::ICmp::ICmpType::SLT:
            return ir::opt_support::IndVarInfo::CondType::LT;
        case ir::ICmp::ICmpType::SLE:
            return ir::opt_support::IndVarInfo::CondType::LE;
        case ir::ICmp::ICmpType::SGT:
            return ir::opt_support::IndVarInfo::CondType::GT;
        case ir::ICmp::ICmpType::SGE:
            return ir::opt_support::IndVarInfo::CondType::GE;
        case ir::ICmp::ICmpType::EQ:
            return ir::opt_support::IndVarInfo::CondType::EQ;
        case ir::ICmp::ICmpType::NE:
            return ir::opt_support::IndVarInfo::CondType::NE;
        default:
            logger::ERROR("[InductionVariableAnalyzation::get_cond_type] icmp's cmp type is not a valid type");
            __builtin_unreachable();
    }
}

static void analyze_ind_var(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    loop->ind_vars.clear();
    if (loop->header->opt_info.predecessors.size() != 2 || loop->latches.size() != 1 || loop->exits.size() != 1) return;

    auto latch = loop->latch();
    for (const auto &ind_var : util::get_phis(loop->header)) {
        std::shared_ptr<ir::Value> init, bound, alu;
        std::shared_ptr<ir::ICmp> icmp;
        for (const auto &exiting : loop->blocks) {
            auto tail_br = std::dynamic_pointer_cast<ir::Br>(exiting->tail_instruction());
            if (!tail_br || !tail_br->is_cond_branch()) continue;
            auto icmp = std::dynamic_pointer_cast<ir::ICmp>(tail_br->get_operands_ref()[0]);
            if (!icmp) continue;
            if (!(contains(icmp->get_operands_ref(), ind_var))) continue;
            bound = icmp->get_operands_ref()[0] == ind_var ? icmp->get_operands_ref()[1] : icmp->get_operands_ref()[0];
            for (auto &prec : loop->header->opt_info.predecessors) {
                if (prec.lock() == latch) {
                    if (ind_var->get_operands_ref()[1] == prec.lock()) {
                        alu = ind_var->get_operands_ref()[0];
                        init = ind_var->get_operands_ref()[2];
                    } else {
                        alu = ind_var->get_operands_ref()[2];
                        init = ind_var->get_operands_ref()[0];
                    }
                }
            }
            if (!alu || !init) continue;

            auto alu_inst = std::dynamic_pointer_cast<ir::Instruction>(alu);
            if (!alu_inst || !alu_inst->is_binary()) continue;
            auto step = alu_inst->get_operands_ref()[0] == ind_var ? alu_inst->get_operands_ref()[1]
                                                                   : alu_inst->get_operands_ref()[0];
            if (auto step_int = std::dynamic_pointer_cast<ir::ConstantInt>(step); step_int && step_int->get_val() == 0)
                continue;
            if (alu_inst->get_ins_type() == ir::Instruction::InstructionType::ADD ||
                alu_inst->get_ins_type() == ir::Instruction::InstructionType::SUB ||
                alu_inst->get_ins_type() == ir::Instruction::InstructionType::MUL) {
                // std::cout << "\n\n";
                // std::cout << "ind_var: " << ind_var->get_name() << std::endl;
                // std::cout << "init: " << init->get_name() << std::endl;
                // std::cout << "bound: " << bound->get_name() << std::endl;
                // std::cout << "alu: " << alu->get_name() << std::endl;
                // std::cout << "type: " << get_cond_type(icmp) << std::endl;
                // std::cout << "step: " << step->get_name() << std::endl;
                loop->ind_vars.push_back({ind_var, init, bound, alu, get_cond_type(icmp), step});
            }
        }
    }
}

}  // namespace opt::pass
