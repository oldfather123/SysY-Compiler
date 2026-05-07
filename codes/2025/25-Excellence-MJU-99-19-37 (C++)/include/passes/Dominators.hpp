#pragma once

#include "BasicBlock.hpp"
#include "PassManager.hpp"

#include <map>
#include <set>

class Dominators : public Pass {
  public:
    using BBSet = std::set<BasicBlock *>;

    explicit Dominators(Module *m) : Pass(m) {}
    ~Dominators() = default;
    void run() override;
    void run_on_func(Function *f);

    // functions for getting information
    BasicBlock *get_idom(BasicBlock *bb) { 
        auto it = idom_.find(bb);
        return (it != idom_.end()) ? it->second : nullptr;
    }
    
   
    const BBSet &get_dominance_frontier(BasicBlock *bb) {
        auto it = dom_frontier_.find(bb);
        if (it != dom_frontier_.end()) {
            return it->second;
        }
        // 如果没有找到，返回一个空的集合
        static BBSet empty_set;
        return empty_set;
    }
    
    const BBSet &get_dom_tree_succ_blocks(BasicBlock *bb) {
        auto it = dom_tree_succ_blocks_.find(bb);
        if (it != dom_tree_succ_blocks_.end()) {
            return it->second;
        }
        static BBSet empty_set;
        return empty_set;
    }

    void dump_cfg(Function *f);
    void dump_dominator_tree(Function *f);
    
    //调试输出
    void print_idom(Function *f);
    void print_dominance_frontier(Function *f);

    const bool is_dominate(BasicBlock *bb1, BasicBlock *bb2) {
        auto it1 = dom_tree_L_.find(bb1);
        auto it2 = dom_tree_L_.find(bb2);
        auto it3 = dom_tree_R_.find(bb1);
        
        if (it1 == dom_tree_L_.end() || it2 == dom_tree_L_.end() || it3 == dom_tree_R_.end()) {
            return false;
        }
        
        return it1->second <= it2->second && it3->second >= it2->second;
    }

    const std::vector<BasicBlock *> &get_dom_dfs_order() {
        return dom_dfs_order_;
    }

    const std::vector<BasicBlock *> &get_dom_post_order() {
        return dom_post_order_;
    }

  private:

    void dfs(BasicBlock *bb, std::set<BasicBlock *> &visited);
    void create_idom(Function *f);
    void create_dominance_frontier(Function *f);
    void create_dom_tree_succ(Function *f);
    void create_dom_dfs_order(Function *f);
    std::list<BasicBlock *> post_order;
    std::map<BasicBlock *, int> post_order_num;
    std::map<Function *, std::vector<BasicBlock *>> post_order_map;
    std::map<Function *, std::vector<BasicBlock *>> reverse_post_order_map;
    
    void post_order_traversal(BasicBlock *bb, BBSet &visit);

    BasicBlock * intersect(BasicBlock *b1, BasicBlock *b2);

    void create_reverse_post_order(Function *f);
    void set_idom(BasicBlock *bb, BasicBlock *idom) { idom_[bb] = idom; }
    void set_dominance_frontier(BasicBlock *bb, BBSet &df) {
        dom_frontier_[bb].clear();
        dom_frontier_[bb].insert(df.begin(), df.end());
    }
    void add_dom_tree_succ_block(BasicBlock *bb, BasicBlock *dom_tree_succ_bb) {
        dom_tree_succ_blocks_[bb].insert(dom_tree_succ_bb);
    }
    unsigned int get_post_order(BasicBlock *bb) {
        auto it = post_order_.find(bb);
        if (it != post_order_.end()) {
            return it->second;
        }
        return 0;  // 返回默认值
    }

    // TODO 补充需要的函数
    std::list<BasicBlock *> reverse_post_order_{};
    std::map<BasicBlock *, int> post_order_id_{}; // the root has highest ID

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