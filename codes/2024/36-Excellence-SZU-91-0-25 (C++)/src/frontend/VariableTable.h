#pragma once

#include "../utility/System.h"
#include "../midend/Variable.h"
#include "../utility/Cursor.h"

class VariableTable
{
private:
    vector<unordered_map<string,int>> table;
    bool Exist(string variable_name);

public:
    void EnterScope();
    void LeaveScope();
    
    void Add(Variable& variable,Cursor location);
    Variable& Visit(string variable_name,Cursor location);
};
