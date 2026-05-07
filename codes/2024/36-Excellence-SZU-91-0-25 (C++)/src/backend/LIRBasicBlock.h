#pragma once

#include "../utility/System.h"
#include "../backend/LIRInstruction.h"

class LIRBasicBlock
{
public:
    int number;
    vector<shared_ptr<LIRInstruction>> instructions;

    unordered_set<int> precursors;
    unordered_set<int> succeeds;

    LIRBasicBlock(int number):number(number){}
    
    void AddInsturction(shared_ptr<LIRInstruction> instruction_ptr);
    string GetStr();
};