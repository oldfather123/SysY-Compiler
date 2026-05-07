// Modulo Loop Optimization Pass
// Optimizes loops with affine recurrence relations of the form: x_next = (x + C) mod M
// Uses induction variable analysis and SCEV to detect and optimize these patterns.
// Transforms O(n) loop into O(1) closed-form calculation.

#include <iostream>
#include <memory>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

struct ModuloRecurrence {
    std::shared_ptr<ir::Phi> phi_var;                 // The recurrence variable
    std::shared_ptr<ir::Value> init_val;              // Initial value (can be non-constant)
    std::shared_ptr<ir::ConstantInt> step;            // Step value C
    std::shared_ptr<ir::ConstantInt> modulus;         // Modulus M
    std::shared_ptr<ir::opt_support::LoopInfo> loop;  // The containing loop
    int trip_count;                                   // Number of iterations (static), if known
    std::shared_ptr<ir::Value> dyn_trip_count;        // Dynamic iteration count (value), if derivable
    bool has_external_uses;                           // Whether phi is used outside loop
};

static bool modified = false;
static std::vector<ModuloRecurrence> recurrences_to_optimize;

// Check if a phi variable represents a modulo recurrence
static bool analyze_phi_for_modulo_recurrence(std::shared_ptr<ir::Phi> phi,
                                              std::shared_ptr<ir::opt_support::LoopInfo> loop,
                                              ModuloRecurrence &result) {
    if (!phi->get_type()->is_integer_ty()) return false;

    // Find initial value (from outside the loop) and update value (from inside the loop)
    std::shared_ptr<ir::Value> init_val = nullptr;
    std::shared_ptr<ir::Value> update_val = nullptr;

    // std::cout << "[ModuloOpt] analyzing phi: " << phi->get_name() << " in loop header: " << loop->header->get_name()
    //           << std::endl;

    for (size_t i = 1; i < phi->get_operands_ref().size(); i += 2) {
        auto block = std::dynamic_pointer_cast<ir::BasicBlock>(phi->get_operands_ref()[i]);
        if (!block) continue;

        if (loop->contains(block)) {
            update_val = phi->get_operands_ref()[i - 1];
        } else {
            init_val = phi->get_operands_ref()[i - 1];
        }
    }

    if (!init_val || !update_val) {
        // std::cout << "[ModuloOpt]   skip: missing init/update for phi " << phi->get_name() << std::endl;
        return false;
    }

    // Initial value can be any value (constant or not); we'll compute (init + N*step) % M

    // Analyze the update expression to find modulo pattern
    // Look for: update_val = srem(add(phi, constant), modulus)
    // Or more generally: update_val = srem(add(constant, phi), modulus)

    auto srem_inst = std::dynamic_pointer_cast<ir::SRem>(update_val);
    if (!srem_inst) {
        // std::cout << "[ModuloOpt]   skip: update is not srem for phi " << phi->get_name() << std::endl;
        return false;
    }

    // Check modulus is constant
    auto modulus = std::dynamic_pointer_cast<ir::ConstantInt>(srem_inst->get_operands_ref()[1]);
    if (!modulus || modulus->get_val() <= 0) {
        // std::cout << "[ModuloOpt]   skip: modulus not positive constant for phi " << phi->get_name() << std::endl;
        return false;
    }

    // Check first operand is an add instruction
    auto add_inst = std::dynamic_pointer_cast<ir::Add>(srem_inst->get_operands_ref()[0]);
    if (!add_inst) {
        // std::cout << "[ModuloOpt]   skip: srem operand is not add for phi " << phi->get_name() << std::endl;
        return false;
    }

    // Check if add involves phi and a constant
    auto add_operands = add_inst->get_operands_ref();
    std::shared_ptr<ir::ConstantInt> step = nullptr;
    bool involves_phi = false;

    if (add_operands[0] == phi) {
        involves_phi = true;
        step = std::dynamic_pointer_cast<ir::ConstantInt>(add_operands[1]);
    } else if (add_operands[1] == phi) {
        involves_phi = true;
        step = std::dynamic_pointer_cast<ir::ConstantInt>(add_operands[0]);
    }

    if (!involves_phi || !step) {
        // std::cout << "[ModuloOpt]   skip: add does not involve phi+const for phi " << phi->get_name() << std::endl;
        return false;
    }

    // Check if phi has uses outside the loop (for final output)
    bool has_external_uses = false;
    for (const auto &weak_user : phi->get_users_ref()) {
        auto user = weak_user.lock();
        if (!user) continue;

        auto inst_user = std::dynamic_pointer_cast<ir::Instruction>(user);
        if (inst_user && !loop->contains(inst_user->get_parent_block().lock())) {
            has_external_uses = true;
            break;
        }
    }

    // Fill in the result
    result.phi_var = phi;
    result.init_val = init_val;
    result.step = step;
    result.modulus = modulus;
    result.loop = loop;
    result.has_external_uses = has_external_uses;
    result.dyn_trip_count = nullptr;

    // std::cout << "[ModuloOpt]   candidate: step=" << step->get_val() << ", mod=" << modulus->get_val()
    //           << ", external_uses=" << (has_external_uses ? 1 : 0) << std::endl;

    return true;
}

// Generate efficient modulo calculation for specific moduli
static std::shared_ptr<ir::Value> generate_efficient_mod(std::shared_ptr<ir::BasicBlock> block,
                                                         std::shared_ptr<ir::Value> value,
                                                         int modulus,
                                                         ir::InstructionNode insert_pos) {
    // Detect Mersenne modulus: modulus = 2^k - 1
    const uint32_t m = static_cast<uint32_t>(modulus);
    const uint32_t mp1 = m + 1U;
    const bool is_power_of_two = (mp1 != 0U) && ((mp1 & (mp1 - 1U)) == 0U);
    if (is_power_of_two) {
        // Compute k = log2(modulus + 1)
        uint32_t tmp = mp1;
        int k = 0;
        while (tmp > 1U) {
            tmp >>= 1U;
            ++k;
        }

        // Number of folds to cover 32-bit values
        int folds = (32 + k - 1) / k;  // ceil(32 / k)

        // Constants
        auto mask_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), static_cast<int>(m));
        auto shift_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), k);

        // Perform folding: t = (t & mask) + (t >> k), repeated
        std::shared_ptr<ir::Value> t = value;
        for (int i = 0; i < folds; ++i) {
            auto lo = ir::And::create(block, t, mask_const, gen_local_var_name());
            block->add_instruction(insert_pos, lo);
            auto hi = ir::LShr::create(block, t, shift_const, gen_local_var_name());
            block->add_instruction(insert_pos, hi);
            auto sum = ir::Add::create(block, lo, hi, gen_local_var_name());
            block->add_instruction(insert_pos, sum);
            t = sum;
        }

        // Final conditional subtract: if (t >= m) t -= m (branchless)
        auto cmp = ir::ICmp::create(block, ir::ICmp::ICmpType::UGE, t, mask_const, gen_local_var_name());
        block->add_instruction(insert_pos, cmp);
        auto z = ir::ZExt::create(block, cmp, ir::IntegerType::get(32), gen_local_var_name());
        block->add_instruction(insert_pos, z);
        auto submask = ir::Mul::create(block, z, mask_const, gen_local_var_name());
        block->add_instruction(insert_pos, submask);
        auto res = ir::Sub::create(block, t, submask, gen_local_var_name());
        block->add_instruction(insert_pos, res);
        return res;
    }

    // For other moduli, use regular srem
    auto mod_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), modulus);
    auto srem = ir::SRem::create(block, value, mod_const, gen_local_var_name());
    block->add_instruction(insert_pos, srem);
    return srem;
}

// Try derive dynamic trip count from canonical indvar: i = phi [0, pre], [i+1, latch], cond i < bound
static std::shared_ptr<ir::Value> derive_dyn_trip_count(const std::shared_ptr<ir::opt_support::LoopInfo> &loop) {
    // 1) Prefer information from InductionVariableAnalyzation
    for (const auto &info : loop->ind_vars) {
        // require: init is ConstantInt 0, step is ConstantInt 1, alu is Add, cond LT (i < bound)
        auto init_c = std::dynamic_pointer_cast<ir::ConstantInt>(info.init);
        auto step_c = std::dynamic_pointer_cast<ir::ConstantInt>(info.step);
        auto alu_inst = std::dynamic_pointer_cast<ir::Instruction>(info.alu);
        if (!init_c || !step_c || !alu_inst) continue;
        if (init_c->get_val() != 0) continue;
        if (step_c->get_val() != 1) continue;
        if (alu_inst->get_ins_type() != ir::Instruction::InstructionType::ADD) continue;
        if (info.cond_type != ir::opt_support::IndVarInfo::CondType::LT) continue;
        // bound can be any value; trip count = max(bound - 0, 0) = max(bound, 0)
        // We will not clamp here to avoid CFG edits; assume non-negative bound in common cases
        return info.bound;
    }

    // 2) Fallback: scan header for canonical indvar pattern without relying on analysis
    auto header = loop->header;
    // Find a phi like: phi [0, preheader], [next, latch]
    for (const auto &phi : util::get_phis(header)) {
        auto ty = phi->get_type();
        if (!ty || !ty->is_integer_ty()) continue;
        if (phi->get_operands_ref().size() != 4) continue;  // two incoming
        std::shared_ptr<ir::Value> init = nullptr;
        std::shared_ptr<ir::BasicBlock> init_from = nullptr;
        std::shared_ptr<ir::Value> back = nullptr;
        std::shared_ptr<ir::BasicBlock> back_from = nullptr;
        for (size_t i = 0; i < 4; i += 2) {
            auto val = phi->get_operands_ref()[i];
            auto from = std::dynamic_pointer_cast<ir::BasicBlock>(phi->get_operands_ref()[i + 1]);
            if (!from) continue;
            if (loop->contains(from)) {
                back = val;
                back_from = from;
            } else {
                init = val;
                init_from = from;
            }
        }
        auto init_c = std::dynamic_pointer_cast<ir::ConstantInt>(init);
        if (!init_c || init_c->get_val() != 0) continue;
        // back should be add phi, 1
        auto add_inst = std::dynamic_pointer_cast<ir::Add>(back);
        if (!add_inst) continue;
        auto &ops = add_inst->get_operands_ref();
        bool has_phi = (ops[0] == phi) || (ops[1] == phi);
        auto other = (ops[0] == phi) ? ops[1] : (ops[1] == phi ? ops[0] : nullptr);
        auto step_c = std::dynamic_pointer_cast<ir::ConstantInt>(other);
        if (!has_phi || !step_c || step_c->get_val() != 1) continue;
        // find icmp slt in header comparing this phi with some bound
        auto term = std::dynamic_pointer_cast<ir::Br>(header->tail_instruction());
        if (!term || !term->is_cond_branch()) continue;
        auto cond = std::dynamic_pointer_cast<ir::ICmp>(term->get_operands_ref()[0]);
        if (!cond) continue;
        if (cond->get_cmp_type() != ir::ICmp::ICmpType::SLT) continue;
        auto lhs = cond->get_operands_ref()[0];
        auto rhs = cond->get_operands_ref()[1];
        if (lhs == phi) return rhs;
        if (rhs == phi) return lhs;  // though unusual for slt
    }
    return nullptr;
}

// Optimize a modulo recurrence by computing the closed form
static void optimize_recurrence(const ModuloRecurrence &rec) {
    if (!rec.has_external_uses) {
        logger::INFO("Skipping optimization: no external uses found for phi variable");
        return;
    }

    // Find insertion point before the loop
    auto pre_header = rec.loop->pre_header;
    if (!pre_header) {
        // Find a predecessor block outside the loop
        for (const auto &weak_pred : rec.loop->header->opt_info.predecessors) {
            auto pred = weak_pred.lock();
            if (!rec.loop->contains(pred)) {
                pre_header = pred;
                break;
            }
        }
    }

    if (!pre_header) {
        logger::WARN("Cannot find pre-header for loop optimization");
        return;
    }

    auto insert_pos = pre_header->get_instructions_ref().end();
    --insert_pos;  // Insert before terminator

    // Compute closed form: result = (init_val + N * step) mod modulus, where N can be static or dynamic
    std::shared_ptr<ir::Value> final_result;
    if (rec.dyn_trip_count) {
        // std::cout << "[ModuloOpt]   using dynamic trip_count value: " << rec.dyn_trip_count->get_name() << std::endl;
        // Clamp N = max(N, 0)
        auto zero = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), 0);
        auto is_pos =
            ir::ICmp::create(pre_header, ir::ICmp::ICmpType::SGT, rec.dyn_trip_count, zero, gen_local_var_name());
        pre_header->add_instruction(insert_pos, is_pos);
        auto z = ir::ZExt::create(pre_header, is_pos, ir::IntegerType::get(32), gen_local_var_name());
        pre_header->add_instruction(insert_pos, z);
        auto n_clamped = ir::Mul::create(pre_header, z, rec.dyn_trip_count, gen_local_var_name());
        pre_header->add_instruction(insert_pos, n_clamped);
        // Reduce N modulo M first to avoid overflow
        auto n_mod = generate_efficient_mod(pre_header, n_clamped, rec.modulus->get_val(), insert_pos);
        // Multiply (n_mod * step)
        auto total_step = ir::Mul::create(pre_header, n_mod, rec.step, gen_local_var_name());
        pre_header->add_instruction(insert_pos, total_step);
        // Sum with init
        auto total_sum = ir::Add::create(pre_header, rec.init_val, total_step, gen_local_var_name());
        pre_header->add_instruction(insert_pos, total_sum);
        // Final modulo
        final_result = generate_efficient_mod(pre_header, total_sum, rec.modulus->get_val(), insert_pos);
    } else {
        auto trip_count_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), rec.trip_count);
        // trip_count * step
        auto total_step = ir::Mul::create(pre_header, trip_count_const, rec.step, gen_local_var_name());
        pre_header->add_instruction(insert_pos, total_step);
        // init_val + (trip_count * step)
        auto total_sum = ir::Add::create(pre_header, rec.init_val, total_step, gen_local_var_name());
        pre_header->add_instruction(insert_pos, total_sum);
        // Apply modulo
        final_result = generate_efficient_mod(pre_header, total_sum, rec.modulus->get_val(), insert_pos);
    }

    // Replace external uses of phi with the computed result
    std::vector<std::pair<std::shared_ptr<ir::User>, std::shared_ptr<ir::Value>>> replacements;
    for (const auto &weak_user : rec.phi_var->get_users_ref()) {
        auto user = weak_user.lock();
        if (!user) continue;

        auto inst_user = std::dynamic_pointer_cast<ir::Instruction>(user);
        if (inst_user && !rec.loop->contains(inst_user->get_parent_block().lock())) {
            replacements.push_back({user, final_result});
        }
    }

    for (const auto &[user, replacement] : replacements) {
        util::substitute_operand(user, rec.phi_var, replacement);
    }

    logger::INFO("Optimized modulo recurrence: x_next = (x + ",
                 rec.step->get_val(),
                 ") mod ",
                 rec.modulus->get_val(),
                 " with trip count ",
                 rec.trip_count);
}

// Detect modulo recurrences in a loop
static void detect_modulo_recurrences(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (!loop->trip_count.has_value() || loop->trip_count.value() <= 0) {
        //    std::cout << "[ModuloOpt] skip loop header " << loop->header->get_name() << ": missing/invalid trip_count"
        //           << std::endl;
    }

    // Analyze each phi variable in the loop header
    for (const auto &phi : util::get_phis(loop->header)) {
        ModuloRecurrence rec;
        if (analyze_phi_for_modulo_recurrence(phi, loop, rec)) {
            if (loop->trip_count.has_value() && loop->trip_count.value() > 0) {
                rec.trip_count = loop->trip_count.value();
                recurrences_to_optimize.push_back(rec);
            } else {
                // Try derive dynamic trip count from canonical induction var
                rec.dyn_trip_count = derive_dyn_trip_count(loop);
                if (rec.dyn_trip_count) {
                    // std::cout << "[ModuloOpt]   found candidate with dynamic trip_count from indvar: "
                    //           << rec.dyn_trip_count->get_name() << std::endl;
                    recurrences_to_optimize.push_back(rec);
                } else {
                    // std::cout << "[ModuloOpt]   found candidate but cannot optimize: no trip_count" << std::endl;
                }
            }
        }
    }
}

bool ModuloLoopOptimization::run(ir::Module *module) {
    logger::INFO("Running ModuloLoopOptimization...");
    modified = false;
    recurrences_to_optimize.clear();

    // Detect modulo recurrences in all loops
    for (auto &func : module->get_functions_ref()) {
        for (const auto &loop : func->opt_info.loop_forest) {
            detect_modulo_recurrences(loop);
        }
    }

    // Optimize detected recurrences
    for (const auto &rec : recurrences_to_optimize) {
        optimize_recurrence(rec);
        modified = true;
    }

    // std::cout << "[ModuloOpt] optimized recurrences: " << recurrences_to_optimize.size() << std::endl;

    return modified;
}

}  // namespace opt::pass
