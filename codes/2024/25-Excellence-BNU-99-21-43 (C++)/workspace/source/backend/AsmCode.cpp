#include "AsmCode.h"

using namespace Backend;

AsmCode::AsmCode(IR::Module *module)
    : module(module)
{
    for (auto globalVar : this->module->getGlobalVariableList())
    {
        globalVariableMap[globalVar] = new AsmGlobalVariable(globalVar);
    }

    for (auto irFunction : this->module->getFunctionList())
    {
        functionMap[irFunction] = new AsmFunction(this, irFunction);
    }

    for (auto &[key, value] : functionMap)
    {
        value->genFunction();
    }
}

AsmGlobalVariable *AsmCode::getGlobalVariable(IR::GlobalVariable *globalVar)
{
    if (globalVariableMap.find(globalVar) == globalVariableMap.end())
    {
        Error::Error(__PRETTY_FUNCTION__, "Global variable not found");
    }
    return globalVariableMap[globalVar];
}

AsmFunction *AsmCode::getFunction(IR::Function *irFunction)
{
    if (functionMap.find(irFunction) == functionMap.end())
    {
        Error::Error(__PRETTY_FUNCTION__, "Function not found");
    }
    return functionMap[irFunction];
}

AsmConstantFloat *AsmCode::getConstantFloat(float value)
{
    if (constFloatMap.find(value) == constFloatMap.end())
    {
        constFloatMap[value] = new AsmConstantFloat(value);
    }
    return constFloatMap[value];
}

AsmConstantLong *AsmCode::getConstantLong(long long value)
{
    if (constLongMap.find(value) == constLongMap.end())
    {
        constLongMap[value] = new AsmConstantLong(value);
    }
    return constLongMap[value];
}

std::string AsmCode::emit()
{
    std::string s;

    // Emit global variables
    for (auto [irGlobalVar, asmGlobalVar] : globalVariableMap)
        s += asmGlobalVar->emit();

    // Emit functions
    for (auto [irFunction, asmFunction] : functionMap)
        if (!irFunction->isBuiltinFunction())
            s += asmFunction->emit();

    // Emit constant values
    s += ".section .rodata\n";

    for (auto [key, value] : constFloatMap)
        s += value->emit();

    for (auto [key, value] : constLongMap)
        s += value->emit();

    return s;
}