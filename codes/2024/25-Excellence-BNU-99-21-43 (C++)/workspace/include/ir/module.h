#pragma once

#include "globalvalue.h"

namespace IR
{
    // czr: 这个也是随便测测
    struct Module
    {
        std::vector<GlobalVariable *> globalVariableList;
        std::vector<Function *> functionList;
        std::map<std::string, Function *> builtinFunctions;
        std::string name;

        Module(std::string name);

        void addGlobal(GlobalValue *global)
        {
            if (global->isGlobalVariable())
                globalVariableList.push_back(static_cast<GlobalVariable *>(global));
            else if (global->isFunction())
                functionList.push_back(static_cast<Function *>(global));
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid global value");
        }

        std::vector<GlobalVariable *> getGlobalVariableList()
        {
            return globalVariableList;
        }

        std::vector<Function *> getFunctionList()
        {
            return functionList;
        }

        void gen(std::ostream &os);

        void emitUse(std::ostream &os);

        void addBuiltinFunction(std::string name, ExternalFunction *func);

        Function *getBuiltinFunction(std::string name);
    };
}