#pragma once

#include "IR.h"
#include "recycle.h"

namespace IR
{
    struct StrengthReductionPass
    {
        StrengthReductionPass() = default;

        void runOnFunction(IR::Function *func);

        void runOnBlock(IR::BasicBlock *bb);

        bool addConstantTrans(IR::BasicBlock *bb);

        bool add2Mul(IR::BasicBlock *bb);

        bool mulConstantTrans(IR::BasicBlock* bb);

        void run(IR::Module *module)
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