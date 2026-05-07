#pragma once
#include "Block.h"
#include "Instruction.h"

void addEdgeInCFG(shared_ptr<BasicBlock> pred,shared_ptr<BasicBlock> next);
void computeCFG(BlockPtr block);
