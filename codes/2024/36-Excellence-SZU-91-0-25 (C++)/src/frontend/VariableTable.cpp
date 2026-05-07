#include "../frontend/VariableTable.h"
#include "../utility/Diagnostor.h"
#include "../frontend/MIREmitter.h"
#include "../utility/Product.h"

bool VariableTable::Exist(string variable_name)
{
    if(table.back().find(variable_name)==table.back().end())
        return false;
    return true;
}



void VariableTable::EnterScope()
{
    table.push_back(unordered_map<string,int>());
}

void VariableTable::LeaveScope()
{
    table.pop_back();
}



void VariableTable::Add(Variable& variable,Cursor location)
{    
    if(!Exist(variable.name))
        table.back()[variable.name]=variable.number;
    else SymbolError("The variable: "+variable.name+" is redefined",location);
}

Variable& VariableTable::Visit(string variable_name,Cursor location)
{
    //Search From inside to outside
    bool is_exist=false;
    int exist_i;
    for(int i=table.size()-1;i>=0;i--)
    {
        if(table[i].find(variable_name)!=table[i].end())
        {
            is_exist=true;
            exist_i=i;
            break;
        }
    }

    if(!is_exist) SymbolError("The variable: "+variable_name+" is undefined",location);
    
    int variable_number=table[exist_i][variable_name];
    //Global variable
    if(exist_i==0) return mir.global_variables[variable_number];
    //Local variable
    return mir_emitter.InFunction().local_variables[variable_number];
}
