#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;
void check_reachable(const std::shared_ptr<ir::BasicBlock> &basic_block,
                     std::unordered_map<std::shared_ptr<ir::BasicBlock>, bool> &reachable_map);

bool RemoveUnreachableBasicBlocks::run(ir::Module *module) {
    logger::INFO("Running RemoveUnreachableBasicBlocks...");
    modified = false;
    module->for_each_func([&](const auto &func) {
        // create a map to check if basic blocks are reachable
        std::unordered_map<std::shared_ptr<ir::BasicBlock>, bool> reachable_map;
        func->for_each_block([&](const auto &basic_block) { reachable_map.emplace(basic_block, false); });

        // check reachability
        check_reachable(func->get_basic_blocks_ref().front(), reachable_map);

        // remove unreachable basic blocks
        for (const auto &p : reachable_map) {
            if (!p.second) {
                util::remove_basic_block(p.first);
                modified = true;
            }
        }
    });
    return modified;
}

void check_reachable(const std::shared_ptr<ir::BasicBlock> &basic_block,
                     std::unordered_map<std::shared_ptr<ir::BasicBlock>, bool> &reachable_map) {
    // if is already reachable, return
    if (reachable_map[basic_block]) {
        return;
    }

    // set reachable
    reachable_map[basic_block] = true;

    // check successors
    for (const std::weak_ptr<ir::BasicBlock> &weak_successor : basic_block->opt_info.successors) {
        if (std::shared_ptr<ir::BasicBlock> suc = weak_successor.lock()) {
            check_reachable(suc, reachable_map);
        }
    }
}
}  // namespace opt::pass
