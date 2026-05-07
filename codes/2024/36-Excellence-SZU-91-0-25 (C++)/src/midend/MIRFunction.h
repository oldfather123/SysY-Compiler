#pragma once

#include "../utility/System.h"
#include "../midend/MIRBasicBlock.h"
#include "../utility/Type.h"
#include "../midend/Variable.h"

#define FUNC_SIZE 36

class MIRFunction
{
public:
    bool is_lib;
    DataType return_type;
    string name;
    int number;
    
    int parameter_number;
    vector<Variable> local_variables;
    int local_var_number;

    int entry_block_number; //1
    vector<MIRBasicBlock> basic_blocks;
    int exit_block_number; //0

    MIRFunction(){}
    MIRFunction(bool is_lib,DataType return_type,string name,int number):
        is_lib(is_lib),return_type(return_type),name(name),number(number),
        parameter_number(0),local_var_number(0){
            if(is_lib)local_variables.resize(3);
            else local_variables.resize(VAR_SIZE);

            exit_block_number=NewBasicBlock();
            entry_block_number=NewBasicBlock();
        }

    Variable& NewLocalVariable(bool is_const,DataType data_type,string name,vector<int> dimensions);
    shared_ptr<ValueVariable> NewTempLocalVar(DataType data_type);
    void FreeTempLocalVar(shared_ptr<Value>& value);
    Variable& NewParameter(DataType data_type,string name,vector<int> dimensions);
    int NewBasicBlock();

    MIRBasicBlock& GetBasicBlock(int block_number);
    MIRBasicBlock& GetEntryBasicBlock();
    MIRBasicBlock& GetExitBasicBlock();

    string GetStrLocalVariables();
    string GetStr();
};
