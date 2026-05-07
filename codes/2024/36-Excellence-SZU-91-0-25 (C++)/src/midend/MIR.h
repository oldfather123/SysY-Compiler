#pragma once

#include "../utility/System.h"
#include "../midend/MIRFunction.h"
#include "../midend/Variable.h"

class MIR
{
public:
    vector<Variable> global_variables;
    int global_var_number;

    vector<MIRFunction> functions;
    int function_number;
    unordered_map<string,int> name2number;

    MIR():global_var_number(0),function_number(0)
    { 
        global_variables.resize(VAR_SIZE); 
        functions.resize(FUNC_SIZE);
    }

    Variable& NewGlobalVariable(bool is_const,DataType data_type,string name,vector<int> dimensions);
    MIRFunction& NewFunction(DataType return_type,string function_name);
    MIRFunction& NewLibFunction(DataType return_type,string function_name);

    MIRFunction& GetFunction(string function_name);
    
    string GetStrGlobalVariables();
    string GetStr();
};
