#pragma once

#include "loop.h"
#include "domtree.h"

#include <iterator>
#include <vector>
#include <queue>
#include <stdexcept>
#include <set>
#include <map>

namespace IR
{
    class LoopForest
    {
    private:
        std::map<BasicBlock *, std::set<BasicBlock *>> inverse_cfg;

        struct Builder
        {
            DominatorTree *domTree;

            Builder(Function *function) : domTree(DominatorTree::create())
            {
                domTree->build(function);
            }
            ~Builder()
            {
                delete domTree;
            }
        };

        std::set<Loop *> rootLoops;
        
        std::map<BasicBlock *, Loop *> basicBlockLoopMap;

        void dfsSetLoopDepth(Loop *currentLoop);

        bool doesAlphaDominatesBeta(BasicBlock *alpha, BasicBlock *beta, const std::map<BasicBlock *, std::set<BasicBlock *>> &dom) const;

        void dfsFindLoops(Builder &builder, BasicBlock *basicBlock);

    private:
        void initInverseCFG(Function *function);

        std::set<BasicBlock *> getEntryBlocks(BasicBlock *basicBlock);

    public:
        LoopForest(Function *function);

        ~LoopForest();

        const std::set<Loop *> &getRootLoops() const { return rootLoops; }
        const std::map<BasicBlock *, Loop *> &getBasicBlockLoopMap() const { return basicBlockLoopMap; }

        static LoopForest *buildOver(Function *function)
        {
            return new LoopForest(function);
        }
    };
}
