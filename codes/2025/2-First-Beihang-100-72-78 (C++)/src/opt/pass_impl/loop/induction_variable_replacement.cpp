#include <functional>
#include <memory>

#include "ir/support.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;
static ir::opt_support::SCEVInfo *scev_info;

static void execute(std::shared_ptr<ir::opt_support::LoopInfo> loop);
static int calculate_scev_val(std::shared_ptr<ir::opt_support::SCEVExpr> scev, int trip_count);

bool InductionVariableReplacement::run(ir::Module *module) {
    logger::INFO("Running InductionVariableReplacement...");
    modified = false;
    scev_info = &module->opt_info.scev_info;
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            execute(loop);
        }
    }
    return modified;
}

static void execute(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (!loop->trip_count.has_value()) return;
    if (loop->exits.size() != 1) return;
    int trip_count = *loop->trip_count;
    auto exit = *loop->exits.begin();
    for (auto &inst : util::get_phis(exit)) {
        auto val = inst->get_operands_ref()[0];
        if (inst->is_lcssa && inst->get_operands_ref().size() == 2 && scev_info->contains(val)) {
            modified = true;
            int expr_res = calculate_scev_val(scev_info->query(val).value(), trip_count);
            util::substitute(inst, std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), expr_res));
        }
    }
}

static int calculate_scev_val(std::shared_ptr<ir::opt_support::SCEVExpr> scev, int trip_count) {
    static int n, k, c;
    n = trip_count;
    k = 0;
    c = 1;

    std::function<int(std::shared_ptr<ir::opt_support::SCEVExpr>)> dfs =
        [&](std::shared_ptr<ir::opt_support::SCEVExpr> cur) {
            if (k > n) return 0;
            if (cur->type == ir::opt_support::SCEVExpr::SCEVType::CONSTANT) {
                int prim_c = c;
                c = (n - k) * c / (k + 1);
                k++;
                return cur->constant().value() * prim_c;
            }
            int sum = 0;
            auto operands = cur->operands().value();
            for (auto &operand : operands) {
                sum += dfs(operand);
            }
            return sum;
        };

    return dfs(scev);
}

}  // namespace opt::pass
