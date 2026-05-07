#pragma once
#include <iostream>
#include <vector>
#include <stack>
#include "Module.h"

void eliminateUnreachableBlocks(FunctionPtr func);
void mergeBasicBlocks(BasicBlockPtr pred, BasicBlockPtr curr);
void mergeSinglePredBlocks(FunctionPtr func);
void optimizeCFG(FunctionPtr func);