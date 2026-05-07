#pragma once
#include <map>
#include <set>
#include "IR.h"

namespace IR
{
    // 使用朴素方法构建Dom树，时间复杂度为O(n^3)
    // 支配边界是插入Phi节点的地方
    struct DominatorTree
    {
        BasicBlock *root = nullptr;
        std::map<BasicBlock *, std::set<BasicBlock *>> dom;         // 存储支配关系，dom[b]表示支配b的所有基本块
        std::map<BasicBlock *, BasicBlock *> idom;                  // 存储支配树，idom[b]表示b的直接支配点
        std::map<BasicBlock *, std::set<BasicBlock *>> domFrontier; // 存储支配边界，domFrontier[b]表示b的支配边界
        std::map<BasicBlock *, std::set<BasicBlock *>> domChildren; // 存储支配树，domChildren[b]表示b的子节点

        DominatorTree(BasicBlock *root) : root(root) {}

        std::set<BasicBlock *> getDom(BasicBlock *b) { return dom[b]; }
        BasicBlock *getIDom(BasicBlock *b) { return idom[b]; }
        std::set<BasicBlock *> getDomFrontier(BasicBlock *b) { return domFrontier[b]; }
        std::set<BasicBlock *> getDomChildren(BasicBlock *b) { return domChildren[b]; }
        static DominatorTree *create() { return new DominatorTree(nullptr); }
        void build(Function *func);
    };
}