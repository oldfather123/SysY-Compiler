#pragma once

#include "IR.h"
#include <set>

namespace IR
{
    class Loop;
    struct SimpleLoopInformation
    {

        struct Builder
        {
            BasicBlock *loopEntryBlock;

            std::set<BasicBlock *> allBlocks;

            std::set<Instruction *> allInstructions;

            IR::Value *initValue;

            IR::Value *exitValue;

            Builder();

            SimpleLoopInformation *buildLoopInformation(Loop *loop);

        private:
            IR::BinaryInstruction *getLoopIncrementInstruction();
            // 获取循环退出的比较指令，要求必须在循环的入口块中
            std::pair<IR::ICmpInstruction *, IR::BasicBlock*> getLoopExitCompareInstruction();
            // 分析循环入口块的 phi 指令获取循环变量的初始值和退出值
            void analyzePhiInstruction(IR::PhiInstruction *instruction);

            void getAllBlocksAndInstructions(Loop *loop);
        };

        IR::BinaryInstruction *loopIncrementInstruction;

        IR::ICmpInstruction *loopExitCompareInstruction;

        IR::Value *initValue;

        IR::BasicBlock *loopNextBlock;
        

        SimpleLoopInformation(IR::BinaryInstruction *loopIncrementInstruction, IR::ICmpInstruction *loopExitCompareInstruction,
                              IR::Value *initValue, IR::BasicBlock *exitBlock);

        int getLoopNumbs();

        static SimpleLoopInformation *analyze(Loop *loop);

        bool isFixedLoop() { return getLoopNumbs() != -1; }
    };
}