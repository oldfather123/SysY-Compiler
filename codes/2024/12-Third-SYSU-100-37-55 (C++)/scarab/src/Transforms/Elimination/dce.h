#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <stack>
#include "Module.h"

void markDeadCode(Instruction* instr, unordered_map<Instruction*, bool>& deletelist);
void eliminateDeadCode(FunctionPtr func);
void eliminateDeadCodeInModule(Module& module);
bool isLoopDeadCodeEliminable(InstructionPtr instr);
bool visitInstructionsDFS(InstructionPtr instr, LoopPtr loop, unordered_map<InstructionPtr, bool>& visitedMap, function<bool(InstructionPtr)> cond, function<void(InstructionPtr)> action);
void eliminateLoopDeadCode(FunctionPtr func);