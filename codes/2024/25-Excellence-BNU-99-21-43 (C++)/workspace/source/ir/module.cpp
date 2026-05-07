#include "module.h"
#include "voidtype.h"

namespace IR
{
    Module::Module(std::string name) : name(name)
    {
        addBuiltinFunction("memset",
                           ExternalFunction::create(BasicType::getVoidType(), {PointerType::get(BasicType::getInt32Type()), BasicType::getInt32Type(), BasicType::getInt32Type()}, "memset"));
    }

    void Module::addBuiltinFunction(std::string name, ExternalFunction *func)
    {
        builtinFunctions[name] = func;
        functionList.push_back(func);
    }

    Function *Module::getBuiltinFunction(std::string name)
    {
        auto idx = builtinFunctions.find(name);
        if (idx == builtinFunctions.end())
            Error::Error(__PRETTY_FUNCTION__, "Builtin function not found");
        return (*idx).second;
    }

    void Module::gen(std::ostream &os)
    {
        os << "module " << name << std::endl;
        // for (auto &[name, builtinFunctions] : builtinFunctions)
        // {
        //     builtinFunctions->emitIR(os);
        // }
        for (auto &global : globalVariableList)
        {
            global->emitIR(os);
        }
        for (auto &function : functionList)
        {
            function->emitIR(os);
        }
    }

    void Module::emitUse(std::ostream &os)
    {
        // for (auto &builtinFunctions : builtinFunctions)
        // {
        //     builtinFunctions.second->emitUse(os);
        // }
        for (auto &global : globalVariableList)
        {
            global->emitUse(os);
        }
        for (auto &function : functionList)
        {
            function->emitUse(os);
        }
    }
}