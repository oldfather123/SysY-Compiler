#include <memory>
#include <queue>
#include <vector>

#include "ir/instruction.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static std::queue<std::shared_ptr<ir::Alloca>> alloca_worklist;

static void try_reduce_alloca();

bool PtrAllocaReduction::run(ir::Module *module) {
    logger::INFO("Running PtrAllocaReduction...");
    module->for_each_func([](auto &func) {
        func->for_each_block([](auto &block) {
            block->for_each_instruction([](auto &inst) {
                if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(inst)) alloca_worklist.push(alloca);
            });
        });
    });

    try_reduce_alloca();

    return false;
}

static void try_reduce_alloca() {
    while (!alloca_worklist.empty()) {
        auto alloca = alloca_worklist.front();
        alloca_worklist.pop();

        std::vector<std::shared_ptr<ir::Store>> stores;
        std::vector<std::shared_ptr<ir::Load>> loads;

        for (auto &weak_user : alloca->get_users_ref()) {
            auto user = weak_user.lock();
            if (auto store = std::dynamic_pointer_cast<ir::Store>(user)) stores.push_back(store);
            if (auto load = std::dynamic_pointer_cast<ir::Load>(user)) loads.push_back(load);
        }
        if (stores.size() != 1) continue;

        auto store = stores.front();
        auto val = store->val();

        for (auto &load : loads) util::substitute(load, val);

        util::remove_instruction(alloca);
        util::remove_instruction(store);
    }
}
}  // namespace opt::pass
