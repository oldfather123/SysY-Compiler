#pragma once

#include "IR.h"
#include "recycle.h"

namespace IR
{
    struct InlineFunctionPass
    {
        std::map<Function *, BasicBlock *> entryBlockMap;

        void inlineFunction(Function *parent, BasicBlock *pos, Function *callee, const std::vector<Value *> &args, CallInstruction *callInstr);

        void runOnFunction(Function *func);

        void run(Module *module)
        {
            for (auto func : module->getFunctionList())
            {
                if (func->isBuiltinFunction())
                    continue;
                runOnFunction(func);
            }
            utils::Recycle::free();
        }
    };
}