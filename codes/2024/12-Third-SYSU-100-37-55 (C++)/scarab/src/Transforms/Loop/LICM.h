#pragma once

#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <queue>
#include "Module.h"
#include "Loop.h"

bool isArgumentIdempotent(ValuePtr arg);
bool isFunctionIdempotent(FunctionPtr func, unordered_map<FunctionPtr, bool>& idempotentFunctions);
bool isCallIdempotent(CallInstruction *instr, unordered_map<FunctionPtr, bool>& idempotentFunctions);
bool isSafeToSpeculativelyExecute(InstructionPtr instr, unordered_map<FunctionPtr, bool>& idempotentFunctions);
bool isSafeToHoist(InstructionPtr instr, LoopPtr currentLoop, FunctionPtr func, unordered_map<FunctionPtr, bool>& idempotentFunctions);
bool isArgumentInCall(ValuePtr value, CallInstruction *callInstr);
bool isModifiedInLoop(LoadInstruction *loadInstr, LoopPtr currentLoop);
bool isLoopInvariant(InstructionPtr instr, LoopPtr currentLoop, FunctionPtr func, unordered_set<Instruction *>& toDelete, unordered_map<FunctionPtr, bool>& idempotentFunctions);
bool canMoveToExit(InstructionPtr instr, LoopPtr currentLoop, vector<InstructionPtr> &toMoveExitInst);
void processLoop(LoopPtr currentLoop, FunctionPtr func, bool licmHasModifications, unordered_map<FunctionPtr, bool>& idempotentFunctions);
void LICM(FunctionPtr func, bool hasModifications);