#include "Dominators.hpp"
#include "BasicBlock.hpp"

void Dominators::run(Function *f, AnalysisPassManager &APM) {
    dom_post_order_.clear();
    dom_dfs_order_.clear();
    if (f->get_basic_blocks().size() == 0)
        return;
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        idom_.insert({bb, {}});
        dom_frontier_.insert({bb, {}});
        dom_tree_succ_blocks_.insert({bb, {}});
    }

    create_reverse_post_order(f);
    create_idom(f);
    create_dominance_frontier(f);
    create_dom_tree_succ(f);
    create_dom_dfs_order(f);
}

void Dominators::dfs(BasicBlock *bb, std::set<BasicBlock *> &visited) {
    visited.insert(bb);
    for (auto &succ : bb->get_succ_basic_blocks()) {
        if (visited.find(succ) == visited.end()) {
            dfs(succ, visited);
        }
    }
    post_order_vec_.push_back(bb);
    post_order_.insert({bb, post_order_.size()});
}

void Dominators::create_reverse_post_order(Function *f) {
    // 分析得到 f 中各个基本块的逆后序遍历
    std::set<BasicBlock *> visited;
    dfs(f->get_entry_block(), visited);
}

BasicBlock *Dominators::intersect(BasicBlock *bb1, BasicBlock *bb2) {
    while (bb1 != bb2) {
        while (get_post_order(bb1) < get_post_order(bb2)) {
            bb1 = get_idom(bb1);
        }
        while (get_post_order(bb2) < get_post_order(bb1)) {
            bb2 = get_idom(bb2);
        }
    }
    return bb1;
}

void Dominators::create_idom(Function *f) {
    // 分析得到 f 中各个基本块的 idom
    idom_[f->get_entry_block()] = f->get_entry_block();
    bool changed;
    do {
        changed = false;
        for (auto it = post_order_vec_.rbegin() + 1; it != post_order_vec_.rend(); ++it) {
            auto bb = *it;
            if (bb->get_pre_basic_blocks().empty())
                continue;
            BasicBlock *first_pred = bb->get_pre_basic_blocks().front();
            for (auto &pred : bb->get_pre_basic_blocks())
                if(get_post_order(pred) > get_post_order(first_pred))
                    first_pred = pred;
            BasicBlock *new_idom = first_pred;
            for (auto &pred : bb->get_pre_basic_blocks()) {
                if (pred == first_pred)
                    continue;
                if (get_idom(pred) != nullptr) {
                    // if (get_idom(new_idom) == nullptr)
                    //     new_idom = pred;
                    // else
                    new_idom = intersect(pred, new_idom);
                }
            }
            if (new_idom != get_idom(bb)) {
                changed = true;
                idom_[bb] = new_idom;
            }
        }
    } while (changed);
}

void Dominators::create_dominance_frontier(Function *f) {
    // 分析得到 f 中各个基本块的支配边界集合
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        if (bb->get_pre_basic_blocks().size() >= 2) {
            for (auto &pred : bb->get_pre_basic_blocks()) {
                auto runner = pred;
                while (runner != get_idom(bb)) {
                    dom_frontier_[runner].insert(bb);
                    runner = get_idom(runner);
                }
            }
        }
    }
}

void Dominators::create_dom_tree_succ(Function *f) {
    // 分析得到 f 中各个基本块的支配树后继
    for (auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        if (get_idom(bb) != nullptr && get_idom(bb) != bb) {
            dom_tree_succ_blocks_[get_idom(bb)].insert(bb);
        }
    }
}

void Dominators::create_dom_dfs_order(Function *f) {
    // 分析得到 f 中各个基本块的支配树上的dfs序L,R
    unsigned int order = 0;
    std::function<void(BasicBlock *)> dfs = [&](BasicBlock *bb) {
        dom_tree_L_[bb] = ++order;
        dom_dfs_order_.push_back(bb);
        for (auto &succ : dom_tree_succ_blocks_[bb]) {
            dfs(succ);
        }
        dom_tree_R_[bb] = order;
    };
    dfs(f->get_entry_block());
    dom_post_order_ =
        std::vector(dom_dfs_order_.rbegin(), dom_dfs_order_.rend());
}