#include "Dominators.hpp"
#include "BasicBlock.hpp"
#include "Function.hpp"
#include <algorithm>
#include <fstream>
#include <vector>

/**
 * @brief 支配器分析的入口函数
 * 
 * 遍历模块中的所有函数，对每个非声明的函数执行支配关系分析。
 */
void Dominators::run() {
    for(auto &f1 : m_->get_functions()) {
        auto f = &f1;
        if(f->is_declaration())
            continue;
        run_on_func(f);
    }
}

/**
 * @brief 对单个函数执行支配关系分析
 * @param f 要分析的函数
 * 
 * 该函数执行完整的支配关系分析流程：
 * 1. 初始化数据结构
 * 2. 创建反向后序遍历序列
 * 3. 计算直接支配者(idom)
 * 4. 计算支配边界
 * 5. 构建支配树的后继关系
 * 6. 创建支配树的DFS序
 */
void Dominators::run_on_func(Function *f) {
    dom_post_order_.clear();
    dom_dfs_order_.clear();
    for(auto &bb1 : f->get_basic_blocks()) {
        auto bb = &bb1;
        idom_.insert({bb, nullptr});
        dom_frontier_.insert({bb, {}});
        dom_tree_succ_blocks_.insert({bb, {}});
    }
    create_reverse_post_order(f);
    create_idom(f);
    create_dominance_frontier(f);
    create_dom_tree_succ(f);
    create_dom_dfs_order(f);
}

/**
 * @brief 计算两个基本块的支配关系交集
 * @param b1 第一个基本块
 * @param b2 第二个基本块
 * @return 返回在支配树上最深的同时支配b1和b2的节点
 * 
 * 该函数使用后序号来查找两个节点的最近公共支配者。
 * 通过在支配树上向上遍历直到找到交点。
 */
BasicBlock *Dominators::intersect(BasicBlock *b1, BasicBlock *b2) {
    while (b1 != b2) {
        while (get_post_order(b1) < get_post_order(b2)) {
            b1 = get_idom(b1);
        }
        while (get_post_order(b2) < get_post_order(b1)) {
            b2 = get_idom(b2);
        }
    }
    return b1;
}

/**
 * @brief 创建函数的反向后序遍历序列
 * @param f 要处理的函数
 * 
 * 通过DFS遍历CFG来构建基本块的后序遍历序列。
 * 这个序列用于后续的支配关系分析。
 */
void Dominators::create_reverse_post_order(Function *f) {
    BBSet visited;
    post_order_vec_.clear();
    post_order_.clear();
    dfs(f->get_entry_block(), visited);
    std::reverse(post_order_vec_.begin(), post_order_vec_.end());//reverse
}

/**
 * @brief 深度优先搜索辅助函数
 * @param bb 当前遍历的基本块
 * @param visited 已访问的基本块集合
 * 
 * 执行DFS遍历，维护后序遍历序列和每个基本块的后序号。
 */
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

/**
 * @brief 计算所有基本块的直接支配者(immediate dominator)
 * @param f 要分析的函数
 * 
 * 使用迭代算法计算每个基本块的直接支配者：
 * 1. 将入口块的直接支配者设置为自身
 * 2. 重复遍历所有基本块，更新它们的直接支配者
 * 3. 当没有变化时算法终止
 */
void Dominators::create_idom(Function *f) {
    // TODO 分析得到 f 中各个基本块的 idom
    //初始化
    for(auto &bb:f->get_basic_blocks())
        set_idom(&bb, nullptr);
    set_idom(f->get_entry_block(), f->get_entry_block());

    //迭代
    int ifcontinue=1;
    while(ifcontinue)
    {
        ifcontinue=0;
        for(auto bb:post_order_vec_)
        {
            if(bb!=f->get_entry_block())
            {
                BasicBlock * new_idom=nullptr;
                for(auto i:bb->get_pre_basic_blocks())
                {
                    if(get_idom(i))
                    {
                        new_idom=i;
                        break;
                    }
                }
                for(auto i:bb->get_pre_basic_blocks())
                {
                    if(i!=new_idom)
                    {
                        if(get_idom(i))
                            new_idom=intersect(i, new_idom);
                    }
                }
                if(get_idom(bb)!=new_idom)
                {
                    set_idom(bb, new_idom);
                    ifcontinue=1;
                }
            }
        }
    }
    //throw "Unimplemented create_idom";
}

/**
 * @brief 计算所有基本块的支配边界(dominance frontier)
 * @param f 要分析的函数
 * 
 * 对于每个有多个前驱的基本块B：
 * 从每个前驱P开始，沿着支配树向上遍历直到遇到B的直接支配者，
 * 将B加入路径上所有节点的支配边界中。
 */
void Dominators::create_dominance_frontier(Function *f) {
    // TODO 分析得到 f 中各个基本块的支配边界集合
    for(auto &bb:f->get_basic_blocks())
    {
        if(bb.get_pre_basic_blocks().size()>1)//有多个前驱
        {
            for(auto prebb:bb.get_pre_basic_blocks())
            {
                auto i=prebb;
                while(i!=get_idom(&bb))
                {
                    dom_frontier_[i].insert(&bb);
                    i=get_idom(i);
                }
            }
        }
    }
    //throw "Unimplemented create_dominance_frontier";

}

/**
 * @brief 构建支配树的后继关系
 * @param f 要处理的函数
 * 
 * 基于已计算的直接支配者关系，构建支配树的子节点关系。
 * 如果A是B的直接支配者，则B是A在支配树上的后继。
 */
void Dominators::create_dom_tree_succ(Function *f) {
    // TODO 分析得到 f 中各个基本块的支配树后继
    for(auto &bb:f->get_basic_blocks())
    {
        if((&bb)!=get_idom(&bb))
            add_dom_tree_succ_block(get_idom(&bb), &bb);
    }
    //throw "Unimplemented create_dom_tree_succ";
}

/**
 * @brief 为支配树创建深度优先搜索序
 * @param f 要处理的函数
 * 
 * 该函数通过深度优先搜索遍历支配树，为每个基本块分配两个序号：
 * 1. dom_tree_L_：记录DFS首次访问该节点的时间戳
 * 2. dom_tree_R_：记录DFS完成访问该节点子树的时间戳
 * 
 * 同时维护：
 * - dom_dfs_order_：按DFS访问顺序记录基本块
 * - dom_post_order_：dom_dfs_order_的逆序
 * 
 * 这些序号和顺序可用于快速判断支配关系：
 * 如果节点A支配节点B，则A的L值小于B的L值，且A的R值大于B的R值
 */
void Dominators::create_dom_dfs_order(Function *f) {
    // 分析得到 f 中各个基本块的支配树上的dfs序L,R
    unsigned int order = 0;
    std::function<void(BasicBlock *)> dfs = [&](BasicBlock *bb) {
        dom_tree_L_[bb] = ++ order;
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
