#pragma once
#include "partialUnrollLoop.h"
#include "Function.h"

FunctionPtr loopExtract(LoopPtr loop, Module &ir);