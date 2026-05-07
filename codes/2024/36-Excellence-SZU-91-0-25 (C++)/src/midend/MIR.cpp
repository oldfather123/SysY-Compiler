#include "../midend/MIR.h"
#include "../utility/Diagnostor.h"

Variable& MIR::NewGlobalVariable(bool is_const,DataType data_type,string name,vector<int> dimensions)
{
    if(global_var_number>=VAR_SIZE) CompilerError("Too many global varibales!");

    global_variables[global_var_number]=
        Variable(is_const,GLOBAL,data_type,name,global_var_number,dimensions,false);

    return global_variables[global_var_number++];
}

MIRFunction& MIR::NewFunction(DataType return_type,string function_name)
{
    if(function_number>=FUNC_SIZE) CompilerError("Too many functions!");

    functions[function_number]=
        MIRFunction(false,return_type,function_name,function_number);
    name2number[function_name]=function_number;

    return functions[function_number++];
}

MIRFunction& MIR::NewLibFunction(DataType return_type,string function_name)
{
    if(function_number>=FUNC_SIZE) CompilerError("Too many functions!");

    functions[function_number]=
        MIRFunction(true,return_type,function_name,function_number);
    name2number[function_name]=function_number;

    return functions[function_number++];
}



MIRFunction& MIR::GetFunction(string function_name)
{
    return functions[name2number[function_name]];
}



string MIR::GetStrGlobalVariables()
{
    string global_variables_str;
    
    for(int i=0;i<global_var_number;i++)
    {
        global_variables_str+="$";
        global_variables_str+=global_variables[i].GetStr();
        global_variables_str+="\n";
    }
    global_variables_str+="\n";

    return global_variables_str;
}

string MIR::GetStr()
{
    string MIR_str;

    //Global variables
    if(global_var_number)
        MIR_str+=GetStrGlobalVariables();
    
    //MIRFunction
    for(int i=0;i<function_number;i++)
    {
        if(!functions[i].is_lib){
            MIR_str+=functions[i].GetStr();
            MIR_str+="\n";
        }
    }

    return MIR_str;
}
