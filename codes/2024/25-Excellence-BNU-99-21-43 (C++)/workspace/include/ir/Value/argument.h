#pragma once

#include "value.h"

namespace IR
{

    struct Function;
    // czr: 这个更是随便测测
    struct Argument : public Value
    {
        Function *parent;
        Argument(BasicType *type, std::string name) : Value(type, name, ValueTy::ArgumentVal) {}
        Argument(BasicType *type, int argNum) : Value(type, ValueTy::ArgumentVal)
        {
            name = "arg" + std::to_string(argNum);
        }
    };
}