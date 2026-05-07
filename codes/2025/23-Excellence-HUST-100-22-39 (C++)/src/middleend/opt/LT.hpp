#ifndef __LENGAUER_TARJAN_HPP__
#define __LENGAUER_TARJAN_HPP__

#include "opt.hpp"

class LengauerTarjan: public Optimization {
private:
    map<BasicBlock*, int> dfs_num;          // 深度优先访问基本块的顺序
    vector<BasicBlock*> vertex;             // 深度优先访问的基本块列表
    map<BasicBlock*, BasicBlock*> parent;   // 深度优先访问下基本块的父结点
    map<BasicBlock*, BasicBlock*> semi;     // 半支配者
    map<BasicBlock*, BasicBlock*> idom;     // 立即支配者
    map<BasicBlock*, BasicBlock*> label;                    // 用于路径压缩的标签
    map<BasicBlock*, BasicBlock*> ancestor;                 // 祖先结点
    map<BasicBlock*, vector<BasicBlock*>> bucket;           // 用于存储半支配者的基本块列表
    map<BasicBlock*, vector<BasicBlock*>> reverseDomTree;   // 逆支配树

    /// @brief 深度优先遍历控制流图，并记录遍历层级
    /// @param v 当前基本块
    /// @param N 遍历层级
    void dfs(BasicBlock* v, int& N);

    /// @brief 路径压缩，通过连接当前结点到其祖先结点的祖先结点上来实现并查集的路径压缩
    /// @param v 需要进行路径压缩的基本块
    void compress(BasicBlock* v);

    /// @brief 求取最优半支配者
    /// @param v 
    /// @return 
    BasicBlock* eval(BasicBlock* v);

    /// @brief 连接基本块，建立父子关系
    /// @param v 父结点
    /// @param w 子结点
    void link(BasicBlock* v, BasicBlock* w);

    /// @brief 构建支配树
    /// @param func 
    void build_dominator_tree(Function* func);

    /// @brief 计算支配前沿
    /// @param func 
    void compute_dom_frontier(Function* func);

    /// @brief 构建逆支配树
    /// @param func 
    void build_reverse_dom_tree(Function* func);

    /// @brief 计算反向支配前沿
    /// @param func 
    void build_reverse_dom_frontier(Function* func);
public:
    LengauerTarjan(Module* m): Optimization(m) {}
    void execute() override;
};

#endif // __LENGAUER_TARJAN_HPP__