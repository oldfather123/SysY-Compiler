#pragma once
#include "globalvalue.h"
#include "module.h"
#include "AsmFunction.h"
#include <map>

namespace Backend
{
    class AsmCode
    {
    private:
        IR::Module *module;

        std::map<IR::Function *, AsmFunction *> functionMap;
        std::map<IR::GlobalVariable *, AsmGlobalVariable *> globalVariableMap;
        std::map<float, AsmConstantFloat *> constFloatMap;
        std::map<long long, AsmConstantLong *> constLongMap;

    public:
        AsmCode(IR::Module *module);
        AsmGlobalVariable *getGlobalVariable(IR::GlobalVariable *globalVar);
        AsmFunction *getFunction(IR::Function *irFunction);
        AsmConstantFloat *getConstantFloat(float value);
        AsmConstantLong *getConstantLong(long long value);
        std::string emit();
    };
}