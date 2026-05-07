// pre-requirements:
// - simple loop (after `LoopSimplification`)
// - loop has only one exiting block
// - loop has only one induction variable (after `InductionVariableAnalyzation`)
// - ind var's init, bound, step are all constant int
// TODO(Xingkun): Is it possible to enhance this analysis?

#include <cmath>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>

#include "ir/value.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static void calculate_trip_count(const std::shared_ptr<ir::opt_support::LoopInfo> &loop);

bool TripCountAnalyzation::run(ir::Module *module) {
    logger::INFO("Running TripCountAnalyzation...");
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            calculate_trip_count(loop);
        }
    }
    return false;
}

static void calculate_trip_count(const std::shared_ptr<ir::opt_support::LoopInfo> &loop) {
    // clear first
    loop->trip_count = std::nullopt;

    if (loop->exitings.size() != 1) return;
    if (loop->ind_vars.size() != 1) return;

    auto ind_var_info = loop->ind_vars.front();
    const auto init_con = std::dynamic_pointer_cast<ir::ConstantInt>(ind_var_info.init);
    const auto bound_con = std::dynamic_pointer_cast<ir::ConstantInt>(ind_var_info.bound);
    const auto step_con = std::dynamic_pointer_cast<ir::ConstantInt>(ind_var_info.step);
    const auto alu = std::dynamic_pointer_cast<ir::Instruction>(ind_var_info.alu);
    const auto cond_type = ind_var_info.cond_type;

    // especially, alu is one of `Add`, `Sub`, `Mul`
    if (!init_con || !bound_con || !step_con || !alu || !alu->is_binary()) return;
    int init = init_con->get_val();
    int bound = bound_con->get_val();
    int step = step_con->get_val();

    // init + step * n ? bound
    // init - step * n ? bound
    // init * step ^ n ? bound
    std::optional<int> trip_count = std::nullopt;
    using InstructionType = ir::Instruction::InstructionType;
    using CondType = ir::opt_support::IndVarInfo::CondType;

    if (cond_type == CondType::EQ) {
        if (init == bound) trip_count = 1;
        return;
    }
    if (alu->get_ins_type() == InstructionType::ADD || alu->get_ins_type() == InstructionType::SUB) {
        // For SUB operation, the effective step is negative
        int effective_step = (alu->get_ins_type() == InstructionType::SUB) ? -step : step;

        if (cond_type == CondType::NE) {
            if ((bound - init) % effective_step == 0) trip_count = (bound - init) / effective_step;
        } else if (cond_type == CondType::GE || cond_type == CondType::LE) {
            trip_count = (bound - init) / effective_step + 1;
        } else if (cond_type == CondType::LT || cond_type == CondType::GT) {
            trip_count = (bound - init) % effective_step == 0 ? (bound - init) / effective_step
                                                              : (bound - init) / effective_step + 1;
        }
    } else if (alu->get_ins_type() == InstructionType::MUL) {
        if (step == 0 || init == 0) return;

        double val =
            std::log(static_cast<double>(bound) / static_cast<double>(init)) / std::log(static_cast<double>(step));
        bool tag = init * std::pow(step, static_cast<int>(val)) == bound;

        if (cond_type == CondType::NE) {
            trip_count = tag ? static_cast<int>(val) : -1;
        } else if (cond_type == CondType::GE || cond_type == CondType::LE) {
            trip_count = static_cast<int>(val) + 1;
        } else if (cond_type == CondType::GT || cond_type == CondType::LT) {
            trip_count = tag ? static_cast<int>(val) : static_cast<int>(val) + 1;
        }
    }
    loop->trip_count = trip_count;
}
}  // namespace opt::pass
