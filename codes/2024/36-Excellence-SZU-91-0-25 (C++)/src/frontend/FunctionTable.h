#pragma once

#include "../utility/System.h"
#include "../midend/MIRFunction.h"
#include "../utility/Cursor.h"

class FunctionTable
{
private:
    unordered_set<string> table;
    bool Exist(string function_name);

public:
    void Add(MIRFunction& function);
    void Add(MIRFunction& function,Cursor location);
    MIRFunction& Visit(string function_name,Cursor location);
};
