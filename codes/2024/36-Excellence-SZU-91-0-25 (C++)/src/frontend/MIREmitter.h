#pragma once

#include "../utility/System.h"
#include "../frontend/Token.h"
#include "../frontend/VariableTable.h"
#include "../frontend/FunctionTable.h"
#include "../midend/MIR.h"

class LoopContext
{
public:
    int condition_block_begin_number;
    int next_block_begin_number;

    LoopContext(int condition_block_begin_number,int next_block_begin_number):
        condition_block_begin_number(condition_block_begin_number),
        next_block_begin_number(next_block_begin_number){}
};

class MIREmitter
{
private:
    string in_function_name;
    int in_block_number;

public:
    VariableTable vartable;
    FunctionTable functable;

    //Record loop information
    stack<LoopContext> loop_context;

    Token def_vartoken;
    bool def_const;
    bool def_global;
    DataType def_data_type;

    //Store expression evaluation pointer
    shared_ptr<Value> result_expression;
    
    MIREmitter():in_block_number(-1),def_const(false),def_global(false){}
    
    void InitializeLibFunction();

    void SetFunction(string function_name);
    MIRFunction& InFunction();
    void SetBasicBlock(int block_number);
    MIRBasicBlock& InBasicBlock();

    void EmitMIR();
};

extern MIREmitter mir_emitter;