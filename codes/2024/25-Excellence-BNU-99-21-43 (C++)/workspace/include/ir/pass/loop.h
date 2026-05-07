#pragma once

#include "basicblock.h"
#include "simpleloopinformation.h"
#include <set>

namespace IR
{
    class Loop
    {
    private:
        BasicBlock *headerBasicBlock = nullptr;
        Loop *parentLoop = nullptr;
        std::set<Loop *> subLoops;
        std::set<BasicBlock *> basicBlocks;
        int loopDepth = 0; // 0 for top-level loops
        SimpleLoopInformation *simpleLoopInfo = nullptr;

    public:
        Loop(BasicBlock *headerBasicBlock, std::set<Loop *> subLoops, std::set<BasicBlock *> basicBlocks)
            : headerBasicBlock(headerBasicBlock), subLoops(subLoops), basicBlocks(basicBlocks), loopDepth(-1)
        {
            simpleLoopInfo = SimpleLoopInformation::analyze(this);
        }

        void setParentLoop(Loop *parentLoop);

        void setLoopDepth(int loopDepth);

        BasicBlock *getHeaderBasicBlock();

        Loop *getParentLoop();

        std::set<Loop *> getSubLoops();

        int getLoopDepth();

        std::set<BasicBlock *> getBasicBlocks();

        bool isSimpleLoop() { return simpleLoopInfo != nullptr; }

        SimpleLoopInformation *getSimpleLoopInfo() { return simpleLoopInfo; }
    };
};