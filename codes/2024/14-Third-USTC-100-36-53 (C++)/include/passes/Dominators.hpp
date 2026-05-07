#pragma once

#include "AnalysisPass.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "PassManager.hpp"

#include <map>
#include <set>

class Dominators : public AnalysisPassBase<Function> {
  public:
    using IRUnit = Function;
    using BBSet = std::set<BasicBlock *>;

    ~Dominators() = default;
    void run(Function *f, AnalysisPassManager &APM) override;

    const BBSet &get_dominance_frontier(BasicBlock *bb) const {
        return dom_frontier_.at(bb);
    }
    const BBSet &get_dom_tree_succ_blocks(BasicBlock *bb) const {
        return dom_tree_succ_blocks_.at(bb);
    }

    const bool is_dominate(BasicBlock *bb1, BasicBlock *bb2) const {
        return dom_tree_L_.at(bb1) <= dom_tree_L_.at(bb2) &&
               dom_tree_R_.at(bb1) >= dom_tree_L_.at(bb2);
    }

    const std::vector<BasicBlock *> &get_dom_dfs_order() const {
        return dom_dfs_order_;
    }

    const std::vector<BasicBlock *> &get_dom_post_order() const {
        return dom_post_order_;
    }

  private:
    void create_reverse_post_order(Function *f);
    void create_idom(Function *f);
    void create_dominance_frontier(Function *f);
    void create_dom_tree_succ(Function *f);
    void create_dom_dfs_order(Function *f);
    void dfs(BasicBlock *bb, std::set<BasicBlock *> &visited);
    BasicBlock *intersect(BasicBlock *bb1, BasicBlock *bb2);

    BasicBlock *get_idom(BasicBlock *bb) { return idom_.at(bb); }
    unsigned int get_post_order(BasicBlock *bb) {
        return post_order_.at(bb);
    }

    std::vector<BasicBlock *> post_order_vec_{}; // 逆后序
    std::map<BasicBlock *, unsigned int> post_order_{}; // 逆后序
    std::map<BasicBlock *, BasicBlock *> idom_{};  // 直接支配
    std::map<BasicBlock *, BBSet> dom_frontier_{}; // 支配边界集合
    std::map<BasicBlock *, BBSet> dom_tree_succ_blocks_{}; // 支配树中的后继节点

    // 支配树上的dfs序L,R
    std::map<BasicBlock *, unsigned int> dom_tree_L_;
    std::map<BasicBlock *, unsigned int> dom_tree_R_;

    std::vector<BasicBlock *> dom_dfs_order_;
    std::vector<BasicBlock *> dom_post_order_;
};
