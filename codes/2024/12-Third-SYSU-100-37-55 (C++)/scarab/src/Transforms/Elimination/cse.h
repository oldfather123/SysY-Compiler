//公共子表达式消除
#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include "Module.h"

bool isNotCandidateForCSE(InsID type);
bool canMergeConstants(Const* const1, Const* const2);
bool possiblySameGEP(Instruction *instr1, Instruction *instr2);
void replaceInstruction(ValuePtr oldvalue, ValuePtr newvalue, InstructionPtr instr, BasicBlockPtr succBlock, unordered_map<InstructionPtr, BasicBlockPtr>& deletelist, unordered_set<BasicBlockPtr>& modifiedBlocks);
void cse(FunctionPtr func);