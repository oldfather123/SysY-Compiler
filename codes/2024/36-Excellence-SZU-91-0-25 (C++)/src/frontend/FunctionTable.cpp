#include "../frontend/FunctionTable.h"
#include "../utility/Diagnostor.h"
#include "../utility/Product.h"

bool FunctionTable::Exist(string function_name)
{
    if(table.find(function_name)==table.end())
        return false;
    return true;
}



void FunctionTable::Add(MIRFunction& function)
{
    if(!Exist(function.name)) table.insert(function.name);
    else SymbolError("The function: "+function.name+" is redefined");
}

void FunctionTable::Add(MIRFunction& function,Cursor location)
{
    if(!Exist(function.name)) table.insert(function.name);
    else SymbolError("The function: "+function.name+" is redefined",location);
}

MIRFunction& FunctionTable::Visit(string function_name,Cursor location)
{
    if(!Exist(function_name))
        SymbolError("The function: "+function_name+" is undefined",location);
    
    return mir.GetFunction(function_name);
}
