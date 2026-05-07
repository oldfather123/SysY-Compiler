#pragma once
#include "partialUnrollLoop.h"
#include "loopExtract.h"

bool loopParallel(FunctionPtr func, Module &ir, std::vector<FunctionPtr> &funcToInsert);