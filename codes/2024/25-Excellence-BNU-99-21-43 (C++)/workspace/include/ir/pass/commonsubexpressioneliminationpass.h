#pragma once

#include "IR.h"
#include "recycle.h"
#include <variant>

namespace IR
{
    using variableKey = std::variant<int, float, Value *, BasicType *>;
    using POO = std::pair<unsigned int, std::vector<variableKey>>;

    struct CommonSubExpressionEliminationPass
    {

        std::map<POO, Instruction *> commonSubExpressions;
        std::map<Instruction *, int> instrIndex;
        std::map<Value *, Instruction *> lastDef;

        CommonSubExpressionEliminationPass() = default;

        POO createUniqueKey(Instruction *instr);

        void updateLastDef(Instruction *instr);

        bool checkAfterLastDef(Instruction *instr);

        void runOnFunction(Function *func);

        bool runOnBlock(BasicBlock *bb);

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