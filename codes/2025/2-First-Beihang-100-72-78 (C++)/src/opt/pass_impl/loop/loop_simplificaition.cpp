// Note: this pass disables `LoopAnalyzation` pass, because it will change the loop structure,
// also, it's recommended to execute `DCE` before construct simplified loop.
// TODO(Xingkun): keep loop and dom relation when canonicalizing

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <vector>

#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

static void canonicalize_pre_header(std::shared_ptr<ir::opt_support::LoopInfo> loop);
static void canonicalize_latch(std::shared_ptr<ir::opt_support::LoopInfo> loop);
static void canonicalize_exit(std::shared_ptr<ir::opt_support::LoopInfo> loop);

bool LoopSimplification::run(ir::Module *module) {
    logger::INFO("Running LoopSimplification...");
    module->for_each_func([](auto &func) {
        // FIXME(Xingkun): canonicalize_pre_header and canonicalize_latch have no cases in function testcases,
        // so it may be wrong
        for (const auto &loop : func->opt_info.loop_forest) {
            canonicalize_pre_header(loop);
            canonicalize_exit(loop);
            canonicalize_latch(loop);
            // simplify phi X = phi(X, Y) => phi = Y

            // std::cout << "\n\n\nloop: " << std::endl;
            // std::cout << "depth: " << loop->depth << std::endl;
            // std::cout << "header: " << loop->header->get_name() << std::endl;
            // for (auto &block : loop->blocks) {
            //     std::cout << "block: " << block->get_name() << std::endl;
            // }
            // std::cout << "\n";
            // for (auto &latch : loop->latches) {
            //     std::cout << "latch: " << latch->get_name() << std::endl;
            // }
            // std::cout << "\n";
            // for (auto &exit : loop->exits) {
            //     std::cout << "exit: " << exit->get_name() << std::endl;
            // }
            // std::cout << "\n";
            // for (auto &exit : loop->exitings) {
            //     std::cout << "exiting: " << exit->get_name() << std::endl;
            // }
            // std::cout << "\n";
            // for (auto &entering : loop->enterings) {
            //     std::cout << "entering: " << entering->get_name() << std::endl;
            // }
            // std::cout << "\n";
            // if (loop->pre_header) {
            //     std::cout << "pre_header: " << loop->pre_header->get_name() << std::endl;
            // }
            // std::cout << "\n";
        }
    });
    return false;
}

static void canonicalize_pre_header(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (loop->enterings.empty()) {
        // TODO(Xingkun): dead loop, should be deleted
        auto function = loop->header->get_parent_func().lock();
        auto pre_header = std::make_shared<ir::BasicBlock>(function, gen_block_name());
        function->add_basic_blocks({pre_header});
        pre_header->add_instructions({ir::Br::create(pre_header, loop->header, gen_local_var_name())});
        loop->pre_header = pre_header;
        loop->header->opt_info.predecessors.push_back(pre_header);
        return;
    }
    if (loop->enterings.size() == 1) {
        // TODO(Xingkun): if to hold pure pre-header/
        loop->pre_header = *loop->enterings.begin();
        return;
    }
    __builtin_unreachable();

    auto pre_header = util::split_block_predecessors(loop->header, loop->enterings);
    if (auto parent_loop = loop->parent.lock()) parent_loop->blocks.push_back(pre_header);
}

static void canonicalize_latch(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    if (loop->latches.size() == 1) return;
    auto latch = util::split_block_predecessors(loop->header, loop->latches);
    loop->latches.clear();
    loop->latches.insert(latch);
    loop->blocks.push_back(latch);
}

static void canonicalize_exit(std::shared_ptr<ir::opt_support::LoopInfo> loop) {
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> new_exits;
    for (const auto &exit : loop->exits) {
        if (loop->header->opt_info.dominates(exit)) {
            new_exits.insert(exit);
            continue;
        }

        auto trampoline = std::make_shared<ir::BasicBlock>(loop->header->get_parent_func().lock(), gen_block_name());
        loop->header->get_parent_func().lock()->add_basic_blocks({trampoline});

        std::unordered_set<std::shared_ptr<ir::BasicBlock>> pred_exitings;
        std::copy_if(loop->exitings.begin(),
                     loop->exitings.end(),
                     std::inserter(pred_exitings, pred_exitings.end()),
                     [&exit](const auto &exiting) {
                         return std::find_if(exit->opt_info.predecessors.begin(),
                                             exit->opt_info.predecessors.end(),
                                             [&exiting](const auto &pred) { return pred.lock() == exiting; }) !=
                                exit->opt_info.predecessors.end();
                     });
        util::split_block_predecessors(exit, pred_exitings, trampoline);
        new_exits.insert(trampoline);
        if (auto exit_loop = exit->opt_info.loop.lock()) exit_loop->blocks.push_back(trampoline);
    }
    loop->exits.clear();
    loop->exits.insert(new_exits.begin(), new_exits.end());
}

}  // namespace opt::pass
