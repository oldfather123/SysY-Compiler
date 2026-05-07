#include <iostream>
#include <memory>
#include <optional>
#include <queue>

#include "ir/instruction.hpp"
#include "ir/value.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool judge_pure_inside(std::shared_ptr<ir::Function> func);
static std::optional<std::shared_ptr<ir::Store>> store2alloca(std::shared_ptr<ir::Store> store);
static std::optional<std::shared_ptr<ir::Alloca>> load2alloca(std::shared_ptr<ir::Load> load);
static void pure_spread();

static std::queue<std::shared_ptr<ir::Function>> worklist;
static std::unordered_set<std::shared_ptr<ir::Function>> visited;

bool PureFunctionAnalysis::run(ir::Module *module) {
    logger::INFO("Running PureFunctionAnalysis...");
    visited.clear();
    worklist = std::queue<std::shared_ptr<ir::Function>>();
    module->for_each_func([](auto &func) {
        func->opt_info.is_pure = false;
        func->opt_info.effected_global_vars.clear();
    });
    module->for_each_func([&](auto &func) {
        func->opt_info.is_pure = judge_pure_inside(func);
        if (!func->opt_info.is_pure || !func->opt_info.effected_global_vars.empty()) worklist.push(func);
        pure_spread();
    });
    return false;
}

static bool judge_pure_inside(std::shared_ptr<ir::Function> func) {
    for (const auto &block : func->get_basic_blocks_ref()) {
        for (const auto &inst : block->get_instructions_ref()) {
            if (auto store = std::dynamic_pointer_cast<ir::Store>(inst); store && store2alloca(store) == std::nullopt) {
                if (auto global_var = std::dynamic_pointer_cast<ir::GlobalVariable>(store->addr())) {
                    func->opt_info.effected_global_vars.insert(global_var);
                }
                return false;
            } else if (auto load = std::dynamic_pointer_cast<ir::Load>(inst);
                       load && load2alloca(load) == std::nullopt) {
                if (auto global_var = std::dynamic_pointer_cast<ir::GlobalVariable>(load->addr())) {
                    func->opt_info.effected_global_vars.insert(global_var);
                }
                return false;
            }
        }
    }
    return true;
}
/**
 * Attempt to restore store to lvalue (alloca instruction)
 * Used to determine if there are side effects
 */
static std::optional<std::shared_ptr<ir::Store>> store2alloca(std::shared_ptr<ir::Store> store) {
    // If lval cannot be converted to alloca instruction, it means it's non-local and has side effects
    auto lval = store->addr();
    while (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(lval)) lval = gep->base_ptr();
    if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(lval)) return store;
    return std::nullopt;
}

static std::optional<std::shared_ptr<ir::Alloca>> load2alloca(std::shared_ptr<ir::Load> load) {
    // If lval cannot be converted to alloca instruction, it means it's non-local and has side effects
    auto lval = load->addr();
    while (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(lval)) lval = gep->base_ptr();
    if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(lval)) return alloca;
    return std::nullopt;
}

static void pure_spread() {
    while (!worklist.empty()) {
        auto callee = worklist.front();
        worklist.pop();
        visited.insert(callee);  // use a flag in case of infinite loop
        for (const auto &weak_user : callee->get_users_ref()) {
            auto user = weak_user.lock();
            auto call = std::dynamic_pointer_cast<ir::Call>(user);
            if (!call) continue;
            auto caller = call->get_parent_block().lock()->get_parent_func().lock();
            bool effected = false;
            for (const auto &value : callee->opt_info.effected_global_vars) {
                if (caller->opt_info.effected_global_vars.count(value)) {
                    caller->opt_info.effected_global_vars.insert(value);
                    effected = true;
                }
            }
            if (caller->opt_info.is_pure) {
                caller->opt_info.is_pure = false;
                effected = true;
            }
            if (effected && !visited.count(caller)) worklist.push(caller);
        }
    }
}

}  // namespace opt::pass
