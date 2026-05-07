#pragma once

#include "IR.h"

namespace IR
{
    struct FinalPass
    {
        FinalPass() = default;

        void runOnFunction(IR::Function *func);

        void run(IR::Module *module)
        {
            for (auto &func : module->getFunctionList())
            {
                if (func->isBuiltinFunction())
                    continue;
                runOnFunction(func);
            }
        }
    };
}