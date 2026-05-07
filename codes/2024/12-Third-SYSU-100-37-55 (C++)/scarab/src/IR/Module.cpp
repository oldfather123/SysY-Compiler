#include "Module.h"

Module::Module()
{
    globalStringTable = std::make_shared<StringTableNode>();
    currentScopeStringTable = globalStringTable;

    // 初始化全局函数
    vector<ValuePtr> memsetArgs = {
        std::make_shared<Reg>(std::make_shared<PtrType>(Type::getInt8()), ""),
        std::make_shared<Reg>(Type::getInt8(), ""),
        std::make_shared<Reg>(Type::getInt64(), ""),
        std::make_shared<Reg>(Type::getBool(), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "llvm.memset.p0i8.i64", true, memsetArgs));

    vector<ValuePtr> memcpyArgs = {
        std::make_shared<Reg>(std::make_shared<PtrType>(Type::getInt8()), ""),
        std::make_shared<Reg>(std::make_shared<PtrType>(Type::getInt8()), ""),
        std::make_shared<Reg>(Type::getInt64(), ""),
        std::make_shared<Reg>(Type::getBool(), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "llvm.memcpy.p0i8.p0i8.i64", true, memcpyArgs));

    vector<ValuePtr> putintArgs = {
        std::make_shared<Reg>(Type::getInt(), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "putint", false, putintArgs));
    
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "putch", false, putintArgs));

    vector<ValuePtr> putfloatArgs = {
        std::make_shared<Reg>(Type::getFloat(), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "putfloat", false, putfloatArgs));

    vector<ValuePtr> putArrayArgs = {
        std::make_shared<Reg>(Type::getInt(), ""),
        std::make_shared<Reg>(std::make_shared<PtrType>(Type::getInt()), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "putarray", false, putArrayArgs));

    vector<ValuePtr> putfArrayArgs = {
        std::make_shared<Reg>(Type::getInt(), ""),
        std::make_shared<Reg>(std::make_shared<PtrType>(Type::getFloat()), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "putfarray", false, putfArrayArgs));

    globalFunctions.emplace_back(std::make_shared<Function>(Type::getInt(), "getint", false));
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getInt(), "getch", false));
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getFloat(), "getfloat", false));

    vector<ValuePtr> getArrayArgs = {
        std::make_shared<Reg>(std::make_shared<PtrType>(Type::getInt()), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getInt(), "getarray", false, getArrayArgs));

    vector<ValuePtr> getfArrayArgs = {
        std::make_shared<Reg>(std::make_shared<PtrType>(Type::getFloat()), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getInt(), "getfarray", false, getfArrayArgs));

    vector<ValuePtr> timeArgs = {
        std::make_shared<Reg>(Type::getInt(), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "_sysy_starttime", false, timeArgs));
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "_sysy_stoptime", false, timeArgs));

    // 初始化全局函数
    vector<ValuePtr> parallelArgs = {std::make_shared<Reg>(Type::getInt(), ""), std::make_shared<Reg>(Type::getInt(), "")};
    globalFunctions.emplace_back(std::make_shared<Function>(Type::getVoid(), "scarabsParallelFor", true, parallelArgs));
}

void Module::addFunction(FunctionPtr function)
{
    globalFunctions.emplace_back(function);
}

void Module::addGlobalVariable(VariablePtr globalVariable)
{
    globalVariables.emplace_back(globalVariable);
    globalStringTable->varMap[globalVariables.back()->name] = globalVariables.back();
}

void Module::pushFunctionToStack(FunctionPtr function)
{
    functionStack.emplace(function);
}

FunctionPtr Module::popFunctionFromStack()
{
    FunctionPtr topFunction = functionStack.top();
    functionStack.pop();
    return topFunction;
}

StringTableNodePtr Module::popStringTable()
{
    StringTableNodePtr topStringTable = currentScopeStringTable;
    currentScopeStringTable = currentScopeStringTable->father;
    return topStringTable;
}

FunctionPtr Module::getCurrentFunction()
{
    return functionStack.top();
}

BasicBlockPtr Module::getCurrentBasicBlock()
{
    return functionStack.top()->basicBlocks.back();
}

FunctionPtr Module::findFunctionByName(string name)
{
    if(name=="starttime") return globalFunctions[12]; 
    if(name=="stoptime") return globalFunctions[13]; 
    for (const auto& func : globalFunctions) {
        if (name == func->name)
            return func;
    }
    std::cerr << "Function " << name << " not found!" << std::endl;
    return nullptr;
}

bool Module::isFunctionStackEmpty()
{
    return functionStack.empty();
}

void Module::restorePreviousStringTable()
{
    currentScopeStringTable = std::make_shared<StringTableNode>(currentScopeStringTable);
}

void Module::recalculateCallerAndCallee()
{
    for (auto& func : globalFunctions) {
        func->clearCallerAndCalleeInfo();
    }

    for (auto& func : globalFunctions) {
        if (!func->isLibraryFunction) {
            func->calculateCallerAndCalleeInfo();
        }
    }
}

void Module::registerGlobalVariable(VariablePtr variable)
{
    VariablePtr existingVariable = currentScopeStringTable->lookUp(variable->name);
    if (variable->name.length() > 1024) {
        string newName = variable->name.substr(0, 1024);
        currentScopeStringTable->varMap[variable->name] = variable;
        variable->name = newName;
    }
    else if (existingVariable && !(existingVariable->isGlobal) || getCurrentFunction()->findVariable(variable->name)) {
        int suffix = 0;
        string oldName = variable->name;
        string newName = oldName;
        do {
            newName = oldName + to_string(suffix++);
        } while (currentScopeStringTable->lookUp(newName) || getCurrentFunction()->findVariable(newName));
        variable->name += to_string(suffix - 1);
        currentScopeStringTable->varMap[oldName] = variable;
    }
    else {
        currentScopeStringTable->varMap[variable->name] = variable;
    }
    getCurrentFunction()->addGlobalVariable(variable);
}

void Module::print()
{
    for (const auto& globalVar : globalVariables) {
        globalVar->print();
    }
    for (const auto& function : globalFunctions) {
        if (!function->isLibraryFunction) {
            function->print();
        }
    }
    for (const auto& function : globalFunctions) {
        if (function->isLibraryFunction) {
            function->print();
        }
    }
}