#include <stack>
#include <unordered_set>

#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool has_cycle_dfs(std::shared_ptr<ir::Function> func,
                          std::unordered_set<std::shared_ptr<ir::Function>> &visited,
                          std::unordered_set<std::shared_ptr<ir::Function>> &rec_stack);

bool CallGraphAnalyzation::run(ir::Module *module) {
    logger::INFO("Running CallGraphAnalyzation...");
    module->for_each_func([](auto &func) {
        func->opt_info.callees.clear();
        func->opt_info.callers.clear();
        func->opt_info.is_recursive = false;
    });

    // build call graph
    module->for_each_func([](auto &caller) {
        caller->for_each_block([&](auto &block) {
            block->for_each_instruction([&](auto &inst) {
                if (inst->is_type(ir::Instruction::InstructionType::CALL)) {
                    auto callee = std::dynamic_pointer_cast<ir::Call>(inst)->get_function();
                    callee->opt_info.callers.insert(caller);
                    caller->opt_info.callees.insert(callee);
                }
            });
        });
    });

    // detect indirect recursion (cycle detection)
    std::unordered_set<std::shared_ptr<ir::Function>> visited;
    module->for_each_func([&](auto &func) {
        if (visited.find(func) == visited.end()) {
            std::unordered_set<std::shared_ptr<ir::Function>> rec_stack;
            has_cycle_dfs(func, visited, rec_stack);
        }
    });

    return false;
}

static bool has_cycle_dfs(std::shared_ptr<ir::Function> func,
                          std::unordered_set<std::shared_ptr<ir::Function>> &visited,
                          std::unordered_set<std::shared_ptr<ir::Function>> &rec_stack) {
    visited.insert(func);
    rec_stack.insert(func);

    for (const auto &weak_callee : func->opt_info.callees) {
        auto callee = weak_callee.lock();
        if (visited.find(callee) == visited.end()) {
            if (has_cycle_dfs(callee, visited, rec_stack)) {
                return true;
            }
        } else if (rec_stack.find(callee) != rec_stack.end()) {
            // find cycle, mark all functions in the cycle as recursive
            std::stack<std::shared_ptr<ir::Function>> cycle_stack;
            cycle_stack.push(callee);
            while (!cycle_stack.empty()) {
                auto current = cycle_stack.top();
                cycle_stack.pop();
                current->opt_info.is_recursive = true;

                for (const auto &weak_caller : current->opt_info.callers) {
                    auto caller = weak_caller.lock();
                    if (rec_stack.find(caller) != rec_stack.end() && !caller->opt_info.is_recursive) {
                        cycle_stack.push(caller);
                    }
                }
            }
            return true;
        }
    }

    rec_stack.erase(func);
    return false;
}
}  // namespace opt::pass
