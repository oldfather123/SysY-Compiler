#pragma once
#include "IR.h"
#include "recycle.h"
namespace IR
{
    struct ConstantFoldPass
    {
        ConstantFoldPass() = default;

        bool isConstant(Value *val);

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

        void foldInstruction(Instruction *instr);

        void runOnFunction(IR::Function *func);
    };
}