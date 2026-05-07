#pragma once

#include "../utility/System.h"
#include "../midend/MIRInstruction.h"

class MIRBasicBlock
{
public:
    bool is_valid;
    int number;
    
    list<shared_ptr<MIRInstruction>> instructions;

    unordered_set<int> precursors;
    unordered_set<int> succeeds;

    MIRBasicBlock(int number):is_valid(true),number(number){}

    void AddInsturction(shared_ptr<MIRInstruction> mir_instruction_ptr);

    void AddPrecursor(int pre_number);
    void DeletePrecursor(int pre_number);
    void AddSucceed(int suc_number);
    void DeleteSucceed(int suc_number);
    
    string GetStr();
};
