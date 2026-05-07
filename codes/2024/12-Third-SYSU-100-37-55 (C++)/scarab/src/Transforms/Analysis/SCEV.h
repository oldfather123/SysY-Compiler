#include <iostream>
#include <vector>
#include <stack>
#include <sstream>
#include "Module.h"
#include "Loop.h"

void convertBIVToSCEV(LoopPtr loop, PhiInstruction* phiInstr, set<ValuePtr>& addOperands, set<ValuePtr>& subOperands);
void registerBIV(LoopPtr loop, PhiInstruction* phiInstr);
void identifyAndRegisterBIVs(LoopPtr loop);
void linkBinaryInstrToSCEV(LoopPtr loop, BinaryInstruction* binaryInstr, bool &hasChanged);
void ScalarEvolution(LoopPtr loop);