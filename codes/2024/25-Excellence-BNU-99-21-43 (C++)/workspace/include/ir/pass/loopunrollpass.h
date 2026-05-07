#pragma once

#include "IR.h"
#include "loopforest.h"
#include "loopclone.h"
#include <set>
#include <map>

namespace IR
{
    struct LoopUnrollPass
    {
    private:
        static constexpr int MAXIMUM_EXTRACTED_SIZE = 10;
        static constexpr int UNROLLING_FACTOR = 4;
        Function *function;     // 当前函数
        LoopForest *loopForest; // 当前函数的循环森林

    public:
        explicit LoopUnrollPass(Function *function);
        static void run(Module *module);

    private:
        static bool changed;

        bool runOnFunction(Function *func);

        // 计算给定循环的大小（基本块的数量）
        int collectLoopInfo(Loop *loop, std::set<BasicBlock *> &visitedBlocks);

        // 尝试展开给定的循环
        bool tryUnrollLoop(Loop *loop);

        // 合并两个循环的基本块和操作数
        void concatLoop(const std::map<BasicBlock *, std::map<PhiInstruction *, Value *>> &innerEntries, LoopClone *secondLoop, LoopClone *firstLoop);

        // 连接两个循环的终止指令
        void concatFalseExit(Loop *originalLoop, LoopClone *firstLoop, LoopClone *secondLoop);

        // 创建展开后的循环条件
        void makeExitConditionExtracted(Loop *loop, LoopClone *clone);
    };

}