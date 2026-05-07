#include <memory>
#include <unordered_set>
#include <vector>

#include "ir/mod.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

bool RemoveUnusedFunctions::run(ir::Module *module) {
    // 基于可达性删除：从 @main 出发，经由 callees DFS 标记；未被标记的非库、非 main 函数删除
    bool modified = false;
    std::unordered_set<std::shared_ptr<ir::Function>> reachable;
    std::vector<std::shared_ptr<ir::Function>> worklist;
    auto main_func = module->get_main();
    if (main_func) {
        reachable.insert(main_func);
        worklist.push_back(main_func);
    }
    while (!worklist.empty()) {
        auto cur = worklist.back();
        worklist.pop_back();
        for (const auto &wcallee : cur->opt_info.callees) {
            auto callee = wcallee.lock();
            if (!callee) continue;
            if (reachable.insert(callee).second) {
                worklist.push_back(callee);
            }
        }
    }
    std::vector<std::shared_ptr<ir::Function>> victims;
    for (auto &func : module->get_functions_ref()) {
        if (func->is_main()) continue;
        if (ir::Function::is_lib(func)) continue;
        if (!reachable.count(func)) victims.push_back(func);
    }
    for (auto &fv : victims) {
        opt::util::remove_function(module, fv);
        modified = true;
    }
    return modified;
}

}  // namespace opt::pass
