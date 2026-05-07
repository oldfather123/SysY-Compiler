#include "../backend/LIR.h"

int LIR::NewFunction()
{
    int function_number=functions.size();
    functions.push_back(LIRFunction());
    
    return function_number;
}



string LIR::GetStr()
{
    string LIR_str;

    //Global variables @Todo

    //LIRFunction
    for(LIRFunction& lir_function:functions)
    {
        LIR_str+=lir_function.GetStr();
        LIR_str+="\n";
    }

    return LIR_str;
}