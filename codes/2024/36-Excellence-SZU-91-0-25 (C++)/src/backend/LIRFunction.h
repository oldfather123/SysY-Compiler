#pragma once

#include "../utility/System.h"
#include "../backend/LIRBasicBlock.h"
#include "../backend/VirtualRegister.h"
#include "../midend/Variable.h"

class LIRFunction
{
public:
    string name;
    unordered_map<int,Variable> local_variables;
    int stack_parameter_use;
    int sp_offset;

    int entry_block_number; //1
    vector<LIRBasicBlock> basic_blocks;
    int exit_block_number; //0
    
    LIRFunction():stack_parameter_use(0),sp_offset(0){}

    int NewBasicBlock();

    string GetStr();
};