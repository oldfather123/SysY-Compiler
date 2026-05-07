#pragma once

#include "IR.h"
#include <map>

namespace IR
{
    class CloneBase
    {
    private:
        std::map<Value *, Value *> valueMap;

    public:
        void addBasicBlock(BasicBlock *basicBlock);
        void addSymbolFor(Value *value);
        Value *getReplacedValue(Value *value) const;
        void setValueMapKv(Value *key, Value *value);
        Instruction *cloneInstruction(Instruction *instruction);
    };
}