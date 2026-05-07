#include <memory>
#include <unordered_map>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

static std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::shared_ptr<ir::Phi>> exit_phi_map;

static void construct_lcssa(std::shared_ptr<ir::opt_support::LoopInfo> loop);
static bool is_used_outside_loop(std::shared_ptr<ir::Instruction> inst,
                                 std::shared_ptr<ir::opt_support::LoopInfo> loop);
static void insert_phi_at_exit(std::shared_ptr<ir::Instruction> inst, std::shared_ptr<ir::opt_support::LoopInfo> loop);
static std::shared_ptr<ir::Phi> search_phi(std::shared_ptr<ir::BasicBlock> used_block,
                                           std::shared_ptr<ir::opt_support::LoopInfo> loop);
bool LoopClosedSSA::run(ir::Module *module) {
    logger::INFO("Running LoopClosedSSA...");
    module->for_each_func([](auto &func) {
        for (const auto &loop : func->opt_info.loop_forest) {
            construct_lcssa(loop);
        }
    });

    // static check
    // for (auto &func : module->get_functions_ref()) {
    //     for (const auto &loop : func->opt_info.loop_forest) {
    //         for (const auto &block : loop->blocks) {
    //             for (const auto &inst : block->get_instructions()) {
    //                 if (is_used_outside_loop(inst, loop)) {
    //                     throw std::runtime_error("inst " + inst->get_name() + " is used outside loop");
    //                 }
    //             }
    //         }
    //     }
    // }
    // util::static_check(module);
    return false;
}

static void construct_lcssa(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    for (auto &block : loop->blocks) {
        for (auto &inst : block->get_instructions()) {
            if (is_used_outside_loop(inst, loop)) insert_phi_at_exit(inst, loop);
        }
    }
}

static bool is_used_outside_loop(std::shared_ptr<ir::Instruction> inst,
                                 std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    for (auto &weak_user : inst->get_users_ref()) {
        auto used_block = std::dynamic_pointer_cast<ir::Instruction>(weak_user.lock())->get_parent_block().lock();
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(weak_user.lock()))
            used_block = util::get_phi_block(phi, inst);
        if (!loop->contains(used_block)) return true;
    }
    return false;
}

static void insert_phi_at_exit(std::shared_ptr<ir::Instruction> inst, std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    exit_phi_map.clear();
    auto block = inst->get_parent_block().lock();
    for (const auto &exit : loop->exits) {
        if (exit_phi_map.find(exit) == exit_phi_map.end() && block->opt_info.dominates(exit)) {
            std::vector<std::shared_ptr<ir::Value>> phi_values(exit->opt_info.predecessors.size(), inst);
            std::vector<std::shared_ptr<ir::BasicBlock>> phi_blocks = to_shared_vector(exit->opt_info.predecessors);
            auto phi = ir::Phi::create(exit, phi_values, phi_blocks, inst->get_type(), gen_local_var_name());
            phi->is_lcssa = true;
            exit->add_instruction(exit->get_instructions_ref().begin(), phi);
            exit_phi_map[exit] = phi;
        }
    }

    std::vector<std::pair<std::shared_ptr<ir::Instruction>, std::shared_ptr<ir::Phi>>> replacements;
    for (auto &weak_user : inst->get_users_ref()) {
        auto user_inst = std::dynamic_pointer_cast<ir::Instruction>(weak_user.lock());
        auto used_block = user_inst->get_parent_block().lock();
        if (auto phi = std::dynamic_pointer_cast<ir::Phi>(user_inst)) used_block = util::get_phi_block(phi, inst);
        if (used_block == block || loop->contains(used_block)) continue;
        auto phi = search_phi(used_block, loop);
        replacements.emplace_back(user_inst, phi);
    }
    for (const auto &[user_inst, phi] : replacements) {
        util::substitute_operand(user_inst, inst, phi);
    }
}

static std::shared_ptr<ir::Phi> search_phi(std::shared_ptr<ir::BasicBlock> used_block,
                                           std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (exit_phi_map.find(used_block) != exit_phi_map.end()) return exit_phi_map[used_block];
    auto i_domer = used_block->opt_info.immediate_dominator.lock();
    if (!loop->contains(i_domer)) {
        auto value = search_phi(i_domer, loop);
        exit_phi_map[used_block] = value;
        return value;
    }
    auto phi = ir::Phi::create(used_block, {}, {}, nullptr, gen_local_var_name());
    for (const auto &prec : used_block->opt_info.predecessors)
        util::add_incoming(phi, prec.lock(), search_phi(prec.lock(), loop));
    exit_phi_map[used_block] = phi;
    return phi;
}

}  // namespace opt::pass
