#pragma once
#include "Block.h"
#include "Instruction.h"

void addEdgeToCFG(shared_ptr<BasicBlock> pred,shared_ptr<BasicBlock> succ);
void generateCFG(FunctionPtr func);