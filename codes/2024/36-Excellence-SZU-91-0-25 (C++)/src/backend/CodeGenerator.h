#pragma once

#include "../utility/System.h"
#include "../midend/MIR.h"
#include "../backend/LIR.h"

class CodeGenerator
{
private:
    int in_function_number;
    unordered_map<int,int> offset_table;

public:
    LIRFunction& InFunction();

    void CodeGen();
    void CodeGenGlobalDef();
    void CodeGenFunction(LIRFunction& lir_function);
    void CodeGenBasicBlock(LIRBasicBlock& lir_block);
    void CodeGenInstruction(shared_ptr<LIRInstruction>& lir_instruction_ptr);
};

extern CodeGenerator code_generator;
