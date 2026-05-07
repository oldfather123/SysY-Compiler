#pragma once
#include "IR.h"
#include <queue>
#include "recycle.h"

#include <map>

namespace IR
{
    using PAP = std::pair<AllocaInstruction *, PhiInstruction *>;
    struct Mem2RegPass
    {
        std::map<BasicBlock *, std::map<AllocaInstruction *, Value *>> lastDef;
        std::queue<PAP> worklist;
        void runOnFunction(Function *func);
        void removeUselessStoreLoad(Function *func);
        void removeUseLessAlloca(Function *func);
        Value *getAllocaLastDef(Function *func, BasicBlock *bb, AllocaInstruction *alloca);
        bool isNeedAlloca(Instruction *alloca);
        void run(Module *module)
        {
            for (auto &func : module->getFunctionList())
            {
                if (func->isBuiltinFunction())
                    continue;
                runOnFunction(func);
            }
            utils::Recycle::free();
        }
    };
}