#pragma once

#include "../utility/System.h"
#include "../backend/LIRFunction.h"
#include "../midend/Variable.h"

class LIR
{
public:
    unordered_map<int,Variable> global_variables;
    vector<LIRFunction> functions;

    int NewFunction();
    
    string GetStr();
};