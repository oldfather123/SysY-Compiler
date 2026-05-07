#include "loopforest.h"

namespace IR
{
    void LoopForest::dfsSetLoopDepth(Loop *currentLoop)
    {
        for (auto subLoop : currentLoop->getSubLoops())
        {
            subLoop->setLoopDepth(currentLoop->getLoopDepth() + 1);
            dfsSetLoopDepth(subLoop);
        }
    }

    bool LoopForest::doesAlphaDominatesBeta(BasicBlock *alpha, BasicBlock *beta, const std::map<BasicBlock *, std::set<BasicBlock *>> &dom) const
    {
        auto it = dom.find(beta);
        if (it == dom.end())
        {
            throw std::invalid_argument("BasicBlock not found in DomTree");
        }

        const auto &dominators = it->second;
        return dominators.find(alpha) != dominators.end();
    }

    void LoopForest::dfsFindLoops(Builder &builder, BasicBlock *basicBlock)
    {
        // std::cerr << basicBlock->getIRName() << " DomChildren: " << builder.domTree->getDomChildren(basicBlock).size() << std::endl;
        for (BasicBlock *domSon : builder.domTree->getDomChildren(basicBlock))
        {
            dfsFindLoops(builder, domSon);
        }

        bool isLoopHeader = false;
        std::set<Loop *> subLoops;
        std::set<BasicBlock *> loopBasicBlocks;
        loopBasicBlocks.insert(basicBlock);

        for (BasicBlock *entryBlock : getEntryBlocks(basicBlock))
        {
            if (doesAlphaDominatesBeta(basicBlock, entryBlock, builder.domTree->dom))
            {
                isLoopHeader = true;
                std::queue<BasicBlock *> queue;
                queue.push(entryBlock);

                while (!queue.empty())
                {
                    BasicBlock *u = queue.front();
                    queue.pop();

                    if (basicBlockLoopMap.count(u))
                    {
                        Loop *cloop = basicBlockLoopMap[u];
                        while (cloop->getParentLoop())
                        {
                            cloop = cloop->getParentLoop();
                        }

                        if (!subLoops.count(cloop))
                        {
                            subLoops.insert(cloop);
                            for (auto *block : getEntryBlocks(cloop->getHeaderBasicBlock()))
                            {
                                queue.push(block);
                            }
                        }
                    }
                    else
                    {
                        if (!loopBasicBlocks.count(u))
                        {
                            loopBasicBlocks.insert(u);
                            for (auto *block : getEntryBlocks(u))
                            {
                                queue.push(block);
                            }
                        }
                    }
                }
            }
        }
        // std::cerr << "======================" << std::endl;
        // basicBlock->emitIR(std::cerr);
        if (isLoopHeader)
        {
            Loop *loop = new Loop(basicBlock, subLoops, loopBasicBlocks);
            for (auto subLoop : subLoops)
            {
                subLoop->setParentLoop(loop);
            }
            for (auto *loopBasicBlock : loopBasicBlocks)
            {
                basicBlockLoopMap[loopBasicBlock] = loop;
            }
            basicBlockLoopMap[basicBlock] = loop;
        }
    }

    void LoopForest::initInverseCFG(Function *function)
    {
        auto cfg = function->cfg();
        for (auto &pair : cfg)
        {
            BasicBlock *from = pair.first;
            for (auto &to : pair.second)
            {
                inverse_cfg[to].insert(from);
            }
        }
    }

    std::set<BasicBlock *> LoopForest::getEntryBlocks(BasicBlock *basicBlock)
    {
        if (inverse_cfg.count(basicBlock) == 0)
        {
            return {};
        }
        return inverse_cfg[basicBlock];
    }

    LoopForest::LoopForest(Function *function)
    {
        Builder builder(function);

        initInverseCFG(function);

        dfsFindLoops(builder, function->getEntryBlock());
        // std::cerr << basicBlockLoopMap.size() << std::endl;
        for (const auto &pair : basicBlockLoopMap)
        {
            // auto loop = pair.second;
            // for (auto bb : loop->getBasicBlocks())
            // {
            //     bb->emitIR(std::cerr);
            // }
            if (pair.second->getParentLoop() == nullptr)
            {
                rootLoops.insert(pair.second);
            }
        }
        for (auto rootLoop : rootLoops)
        {
            rootLoop->setLoopDepth(0);
            dfsSetLoopDepth(rootLoop);
        }
    }

    LoopForest::~LoopForest()
    {
        // 清理rootLoops中的Loop对象
        for (auto loop : rootLoops)
        {
            delete loop;
        }
    }
}