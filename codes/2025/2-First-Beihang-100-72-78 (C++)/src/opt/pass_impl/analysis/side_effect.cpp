#include <algorithm>
#include <memory>
#include <unordered_set>

#include "ir/instruction.hpp"
#include "ir/value.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static std::unordered_set<std::shared_ptr<ir::Function>> visited;

static bool process(std::shared_ptr<ir::Function> func);

bool SideEffectAnalyzation::run(ir::Module *module) {
    logger::INFO("Running SideEffectAnalyzation...");
    visited.clear();
    module->for_each_func([](auto &func) { func->opt_info.has_side_effect = false; });
    auto &functions = module->get_functions_ref();
    auto main_func = std::find_if(functions.begin(), functions.end(), [](const auto &func) { return func->is_main(); });
    process(*main_func);
    (*main_func)->opt_info.has_side_effect = true;
    // assert(visited.size() == functions.size());
    return false;
}

static bool process(std::shared_ptr<ir::Function> func) {
    if (ir::Function::is_lib(func)) return func->opt_info.has_side_effect = true;
    visited.insert(func);

    for (const auto &weak_callee : func->opt_info.callees) {
        auto callee = weak_callee.lock();
        if (callee == func) continue;
        if (visited.count(callee)) {
            if (callee->opt_info.has_side_effect) func->opt_info.has_side_effect = true;
            continue;
        }
        if (process(callee)) func->opt_info.has_side_effect = true;
    }

    for (const auto &block : func->get_basic_blocks_ref()) {
        for (const auto &inst : block->get_instructions_ref()) {
            if (std::dynamic_pointer_cast<ir::Store>(inst)) {
                func->opt_info.has_side_effect = true;
                goto end;
            }
        }
    }
end:
    return func->opt_info.has_side_effect;
}
}  // namespace opt::pass
