#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <stack>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ir/support.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
// temp record loops in case of weak ref in block expired
static std::vector<std::shared_ptr<ir::opt_support::LoopInfo>> loops;

static void collect_loops(std::shared_ptr<ir::Function> func);
static void clear(std::shared_ptr<ir::Function> func);
static void construct_loop_in_block(std::shared_ptr<ir::BasicBlock> block,
                                    std::unordered_set<std::shared_ptr<ir::BasicBlock>> latches);
static void build_loop_relation(std::shared_ptr<ir::BasicBlock> entry_block);

bool LoopAnalyzation::run(ir::Module *module) {
    logger::INFO("Running LoopAnalyzation...");
    module->for_each_func([](auto &func) {
        clear(func);
        collect_loops(func);
    });
    loops.clear();
    return false;
}

static void clear(std::shared_ptr<ir::Function> func) {
    // 1. remove from blocks
    for (auto loop : func->opt_info.loop_forest) {
        for (const auto &block : loop->blocks) {
            block->opt_info.loop.reset();
            block->opt_info.is_loop_header = false;
        }
    }
    // 2. remove from loop forest
    func->opt_info.loop_forest.clear();
}

static void collect_loops(std::shared_ptr<ir::Function> func) {
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> latches;
    for (const auto &block : opt::util::post_order_traversal_dom(func)) {
        for (const auto &prec : block->opt_info.predecessors) {
            if (block->opt_info.dominates(prec.lock())) {
                latches.insert(prec.lock());
            }
        }
        if (!latches.empty()) {
            construct_loop_in_block(block, std::move(latches));
            // latches.clear();
        }
    }
    build_loop_relation(func->entry_block());
}

static void construct_loop_in_block(std::shared_ptr<ir::BasicBlock> block,
                                    std::unordered_set<std::shared_ptr<ir::BasicBlock>> latches) {
    auto loop = std::make_shared<ir::opt_support::LoopInfo>();
    // temp record here, because block->opt_info.loop is a weak ref, otherwise it will be expired after function end
    loops.push_back(loop);
    block->opt_info.loop = loop;
    block->opt_info.is_loop_header = true;
    loop->header = block;
    loop->latches.insert(latches.begin(), latches.end());

    std::queue<std::shared_ptr<ir::BasicBlock>> loop_latches(
        std::deque<std::shared_ptr<ir::BasicBlock>>(latches.begin(), latches.end()));

    while (!loop_latches.empty()) {
        auto latch = loop_latches.front();
        loop_latches.pop();

        auto cur_loop = latch->opt_info.loop.lock();
        if (!cur_loop) {
            latch->opt_info.loop = loop;
            if (latch == block) continue;
            for (const auto &prec : latch->opt_info.predecessors) loop_latches.push(prec.lock());
        } else {
            auto parent = cur_loop->parent.lock();
            while (parent) {
                cur_loop = parent;
                parent = cur_loop->parent.lock();
            }
            if (cur_loop == loop) continue;
            cur_loop->parent = loop;
            for (const auto &weak_prec : cur_loop->header->opt_info.predecessors) {
                auto prec = weak_prec.lock();
                if (!prec->opt_info.loop.lock() || prec->opt_info.loop.lock() != cur_loop) loop_latches.push(prec);
            }
        }
    }
}

static void build_loop_relation(std::shared_ptr<ir::BasicBlock> entry_block) {
    ir::opt_support::LoopForest loop_forest;

    std::stack<std::shared_ptr<ir::BasicBlock>> stack;
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> visited;
    stack.push(entry_block);

    while (!stack.empty()) {
        auto cur_block = stack.top();
        stack.pop();
        visited.insert(cur_block);

        auto child = cur_block->opt_info.loop.lock();
        if (child && cur_block == child->header) {
            auto parent = child->parent.lock();
            if (parent) {
                parent->children.push_back(child);
            } else {
                loop_forest.add_top_loop(child);
            }
            int depth = 1;
            auto tmp = child->parent.lock();
            while (tmp) {
                tmp = tmp->parent.lock();
                depth++;
            }
            child->depth = depth;
        }
        if (child) {
            child->blocks.push_back(cur_block);
        }
        for (const auto &succ : cur_block->opt_info.successors) {
            if (!visited.count(succ.lock())) stack.push(succ.lock());
        }
    }

    auto func = entry_block->get_parent_func().lock();
    for (auto loop : loop_forest) {
        for (const auto &block : loop->blocks) {
            for (const auto &succ : block->opt_info.successors) {
                if (!contains(loop->blocks, succ.lock())) {
                    loop->exits.insert(succ.lock());
                    loop->exitings.insert(block);
                }
            }
        }

        for (const auto &pred : loop->header->opt_info.predecessors) {
            auto pred_block = pred.lock();
            if (!contains(loop->blocks, pred_block)) {
                loop->enterings.insert(pred_block);
            }
        }
        if (loop->enterings.size() == 1) {
            loop->pre_header = *loop->enterings.begin();
        }
    }
    func->opt_info.loop_forest = std::move(loop_forest);
}
}  // namespace opt::pass
