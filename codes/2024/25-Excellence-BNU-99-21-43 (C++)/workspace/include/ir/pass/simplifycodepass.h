#pragma once

#include "IR.h"
#include "recycle.h"

namespace IR
{

    /*
    代码简化，主要包括：
    1. 死代码删除
    2. 删除不可达块
    3. phi 简化
    */

    struct SimplifyCodePass
    {
        SimplifyCodePass() = default;

        void runOnFunction(IR::Function *func);

        bool simplifyPhi(IR::Function *func);

        bool simplifyCondBranch(IR::Function *func);

        bool simplifyDirectBranch(IR::Function *func);

        bool deadCodeDelete(IR::Function *func);

        bool unreachableBlockDelete(IR::Function *func);

        bool simplifyIdentity(IR::Function *func);

        bool simplifyMultiAddToMul(IR::Function* func);

        bool deleteEmptyBlock(IR::Function *func);

        bool mergeBlock(IR::Function *func);

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