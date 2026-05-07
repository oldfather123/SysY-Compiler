#include <memory>
#include <unordered_set>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

// NOTE:
// This is a conservative scaffold for LoopInterchange using the project's loop/dominance
// architecture. It currently detects simple perfectly-nested loops and validates
// preconditions, but does not yet perform the CFG transformation. It integrates into
// the pipeline and is safe to enable. Further iterations can extend `try_interchange`
// to construct the swapped loop nest.

static bool is_canonical_unit_step(const ir::opt_support::LoopInfo &loop) {
    // Check for a simple canonical indvar: init is ConstantInt, step is ConstantInt, ALU is Add
    if (loop.ind_vars.empty()) return false;
    for (const auto &info : loop.ind_vars) {
        auto step_c = std::dynamic_pointer_cast<ir::ConstantInt>(info.step);
        auto add_i = std::dynamic_pointer_cast<ir::Add>(info.alu);
        auto phi_i = std::dynamic_pointer_cast<ir::Phi>(info.ind_var);
        if (!step_c || !add_i || !phi_i) continue;
        // Prefer step = 1 as canonical
        if (step_c->get_val() != 1) continue;
        // Condition type should be a strict- or non-strict less-than
        if (info.cond_type != ir::opt_support::IndVarInfo::LT && info.cond_type != ir::opt_support::IndVarInfo::LE)
            continue;
        return true;
    }
    return false;
}

static bool is_perfectly_nested(const std::shared_ptr<ir::opt_support::LoopInfo> &outer,
                                std::shared_ptr<ir::opt_support::LoopInfo> &inner) {
    if (!outer) return false;
    if (outer->children.size() != 1) return false;
    inner = outer->children.front();
    // Require single latch for simplicity
    if (outer->latches.size() != 1) return false;
    if (inner->latches.size() != 1) return false;
    // Require header dominance already satisfied by analysis; ensure inner header is in outer blocks
    if (!outer->contains(inner->header)) return false;
    // Entry to inner should come from a block within outer (ideally from outer header/body)
    if (inner->enterings.empty()) return false;
    for (const auto &ent : inner->enterings) {
        if (!outer->contains(ent)) return false;
    }
    // Prefer simple loops
    if (!is_canonical_unit_step(*outer)) return false;
    if (!is_canonical_unit_step(*inner)) return false;
    return true;
}

static bool has_side_effects_inside(const std::shared_ptr<ir::opt_support::LoopInfo> &loop) {
    for (const auto &bb : loop->blocks) {
        for (const auto &inst : bb->get_instructions_ref()) {
            // disallow calls and stores to be conservative
            if (inst->is_type(ir::Instruction::InstructionType::CALL)) return true;
            if (inst->is_type(ir::Instruction::InstructionType::STORE)) return true;
            // memory set as well
            if (inst->is_type(ir::Instruction::InstructionType::MEMSET)) return true;
        }
    }
    return false;
}

bool LoopInterchange::run(ir::Module *module) {
    logger::INFO("Running LoopInterchange...");
    bool modified = false;
    // Mark helpers as used for some linters
    (void)&is_perfectly_nested;
    (void)&has_side_effects_inside;
    module->for_each_func([&](auto &func) {
        // Iterate over top-level loops in the loop forest
        for (const auto &outer : func->opt_info.loop_forest) {
            std::shared_ptr<ir::opt_support::LoopInfo> inner;
            if (!is_perfectly_nested(outer, inner)) continue;
            if (has_side_effects_inside(outer) || has_side_effects_inside(inner)) continue;
            // TODO: perform CFG rewrite to swap nesting order
            // Placeholder: detection only; no changes yet
        }
    });
    return modified;
}

}  // namespace opt::pass
