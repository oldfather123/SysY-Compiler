// TODO(Xingkun): use SCEV to do this, instead of ind_var_info
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
static bool modified = false;
// mul -> [mul_int, ind_var_info, loop_info]
struct ReductionInfo {
    std::shared_ptr<ir::ConstantInt> mul_int;
    ir::opt_support::IndVarInfo ind_var_info;
    std::shared_ptr<ir::opt_support::LoopInfo> loop_info;
};
static std::unordered_map<std::shared_ptr<ir::Mul>, ReductionInfo> mul_replace_map;

static void collect(std::shared_ptr<ir::opt_support::LoopInfo> loop);
static void reduct(std::shared_ptr<ir::Mul> mul,
                   std::shared_ptr<ir::ConstantInt> mul_int,
                   ir::opt_support::IndVarInfo ind_var_info,
                   std::shared_ptr<ir::opt_support::LoopInfo> loop);

bool LoopStrengthReduction::run(ir::Module *module) {
    logger::INFO("Running LoopStrengthReduction...");
    modified = false;
    mul_replace_map.clear();
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            collect(loop);
        }
    }
    modified = !mul_replace_map.empty();
    for (auto &[mul, info] : mul_replace_map) {
        reduct(mul, info.mul_int, std::move(info.ind_var_info), info.loop_info);
    }
    return modified;
}

static void collect(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (loop->ind_vars.empty()) return;
    for (auto &block : loop->blocks) {
        for (auto &inst : block->get_instructions_ref()) {
            auto mul = std::dynamic_pointer_cast<ir::Mul>(inst);
            if (!mul) continue;
            auto match_res = util::match_binary_operands<ir::Value, ir::ConstantInt>(mul);
            if (!match_res) continue;
            auto match_pair = match_res.value();
            auto var = match_pair.first;
            auto const_int = match_pair.second;
            auto it = std::find_if(
                loop->ind_vars.begin(), loop->ind_vars.end(), [&var](auto &info) { return info.ind_var == var; });
            if (it == loop->ind_vars.end()) continue;
            auto ind_var_info = *it;
            if (!std::dynamic_pointer_cast<ir::ConstantInt>(ind_var_info.step)) continue;
            if (!std::dynamic_pointer_cast<ir::Add>(ind_var_info.alu)) continue;
            if (!std::dynamic_pointer_cast<ir::Phi>(ind_var_info.ind_var)) continue;
            mul_replace_map[mul] = {const_int, ind_var_info, loop};
        }
    }
}

static void reduct(std::shared_ptr<ir::Mul> mul,
                   std::shared_ptr<ir::ConstantInt> mul_int,
                   ir::opt_support::IndVarInfo ori_ind_var_info,
                   std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    // 1. construct new ind_var - insert after the last phi in header
    auto ori_ind_var = std::dynamic_pointer_cast<ir::Phi>(ori_ind_var_info.ind_var);
    auto header = loop->header;

    // Calculate new init value: init * mul_int
    auto ori_init = ori_ind_var_info.init;
    std::shared_ptr<ir::Value> new_init;
    if (auto const_init = std::dynamic_pointer_cast<ir::ConstantInt>(ori_init)) {
        new_init =
            std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), const_init->get_val() * mul_int->get_val());
    } else {
        // If init is not constant, we need to multiply it
        auto mul_init = ir::Mul::create(header, ori_init, mul_int, gen_local_var_name());
        // Insert before the first phi
        auto first_phi_it = header->get_instructions_ref().begin();
        header->add_instruction(first_phi_it, mul_init);
        new_init = mul_init;
    }

    auto new_ind_var = ir::Phi::create(header,
                                       {new_init},
                                       {util::get_phi_block(ori_ind_var, ori_ind_var_info.init)},
                                       ori_ind_var->get_type(),
                                       gen_local_var_name());

    // Find the position after the last phi instruction
    auto insert_pos = header->get_instructions_ref().begin();
    for (auto it = header->get_instructions_ref().begin(); it != header->get_instructions_ref().end(); ++it) {
        if (std::dynamic_pointer_cast<ir::Phi>(*it)) {
            insert_pos = std::next(it);
        } else {
            break;
        }
    }
    header->add_instruction(insert_pos, new_ind_var);

    // 2. calculate the new step
    auto new_step = std::make_shared<ir::ConstantInt>(
        ir::IntegerType::get(32),
        std::dynamic_pointer_cast<ir::ConstantInt>(ori_ind_var_info.step)->get_val() * mul_int->get_val());

    // 3. construct new add inst - insert before terminator in latch
    auto latch = loop->latch();
    auto new_add = ir::Add::create(latch, new_ind_var, new_step, gen_local_var_name());

    // Insert before terminator instruction
    auto terminator = latch->tail_instruction();
    util::add_instruction_before(new_add, terminator);

    // 4. add incoming
    util::add_incoming(new_ind_var, latch, new_add);

    // 5. substitute mul with new_ind_var
    util::substitute(mul, new_ind_var);

    // TODO(Xingkun): 6. maintain ind_var_info
}

}  // namespace opt::pass
