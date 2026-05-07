#include <optional>

#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
void build_dominance(std::shared_ptr<ir::Function> func);
void build_immediate_dominance(std::shared_ptr<ir::Function> func);
static void build_dominance_frontier(std::shared_ptr<ir::Function> func);
void build_reachable(std::vector<std::shared_ptr<ir::BasicBlock>> &reachable,
                     const std::shared_ptr<ir::BasicBlock> &root,
                     const std::shared_ptr<ir::BasicBlock> &dominator);
static void build_dominance_depth(std::shared_ptr<ir::BasicBlock> root, int current_depth);

bool DominanceAnalyzation::run(ir::Module *module) {
    logger::INFO("Running Simple Dominance Analyzation...");
    module->for_each_func([&](const auto &func) {
        func->for_each_block([](const auto &block) {
            block->opt_info.dominated.clear();
            block->opt_info.immediate_dominator.reset();
            block->opt_info.immediate_dominated.clear();
            block->opt_info.dominance_depth = std::nullopt;
            block->opt_info.dominance_frontier.clear();
        });
        build_dominance(func);
        build_immediate_dominance(func);
        build_dominance_frontier(func);
    });

    return false;
}

void build_dominance(std::shared_ptr<ir::Function> func) {
    func->for_each_block([&](const auto &dominator) {
        std::vector<std::shared_ptr<ir::BasicBlock>> reachable;
        build_reachable(reachable, func->get_basic_blocks_ref().front(), dominator);

        // dominated = all - reachable
        std::vector<std::shared_ptr<ir::BasicBlock>> dominated;
        func->for_each_block([&](const auto &bb) {
            if (std::find(reachable.begin(), reachable.end(), bb) == reachable.end()) {
                dominated.push_back(bb);
            }
        });

        dominator->opt_info.dominated = to_weak_vector(dominated);
    });
}

void build_immediate_dominance(std::shared_ptr<ir::Function> func) {
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::shared_ptr<ir::BasicBlock>> immediate_dominator_map;
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::vector<std::shared_ptr<ir::BasicBlock>>>
        immediate_dominated_map;

    for (const auto &dominator : func->get_basic_blocks_ref()) {
        for (const auto &weak_dominated : dominator->opt_info.dominated) {
            std::shared_ptr<ir::BasicBlock> dominated = weak_dominated.lock();
            if (util::is_immediate_dominator_of(dominator, dominated)) {
                immediate_dominator_map[dominated] = dominator;
                immediate_dominated_map[dominator].push_back(dominated);
            }
        }
    }

    for (const auto &basic_block : func->get_basic_blocks_ref()) {
        basic_block->opt_info.immediate_dominator = immediate_dominator_map[basic_block];
        basic_block->opt_info.immediate_dominated = to_weak_vector(immediate_dominated_map[basic_block]);
    }

    build_dominance_depth(func->get_basic_blocks_ref().front(), 0);
}

static void build_dominance_frontier(std::shared_ptr<ir::Function> func) {
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::vector<std::shared_ptr<ir::BasicBlock>>>
        dominance_frontier_map;

    for (const auto &basic_block : func->get_basic_blocks_ref()) {
        for (const auto &weak_successor : basic_block->opt_info.successors) {
            auto successor = weak_successor.lock();
            auto predecessor = basic_block;

            // predecessor dominates the predecessor of successor and does not strictly dominate successor
            while (!predecessor->opt_info.dominates(successor) || predecessor == successor) {
                dominance_frontier_map[predecessor].push_back(successor);
                predecessor = predecessor->opt_info.immediate_dominator.lock();
            }
        }
    }

    for (const auto &basic_block : func->get_basic_blocks_ref()) {
        basic_block->opt_info.dominance_frontier = to_weak_vector(dominance_frontier_map[basic_block]);
    }
}

void build_reachable(std::vector<std::shared_ptr<ir::BasicBlock>> &reachable,
                     const std::shared_ptr<ir::BasicBlock> &root,
                     const std::shared_ptr<ir::BasicBlock> &dominator) {
    if (root == dominator) {
        return;
    }
    reachable.push_back(root);

    // dfs
    for (const std::weak_ptr<ir::BasicBlock> &weak_successor : root->opt_info.successors) {
        if (std::shared_ptr<ir::BasicBlock> successor = weak_successor.lock()) {
            if (std::find(reachable.begin(), reachable.end(), successor) == reachable.end()) {
                build_reachable(reachable, successor, dominator);
            }
        }
    }
}

static void build_dominance_depth(std::shared_ptr<ir::BasicBlock> root, int current_depth) {
    root->opt_info.dominance_depth = current_depth;

    for (const std::weak_ptr<ir::BasicBlock> &immediate_dominated : root->opt_info.immediate_dominated) {
        build_dominance_depth(immediate_dominated.lock(), current_depth + 1);
    }
}
}  // namespace opt::pass
