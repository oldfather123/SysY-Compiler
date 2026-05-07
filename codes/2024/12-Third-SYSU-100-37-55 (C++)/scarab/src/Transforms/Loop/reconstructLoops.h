#pragma once
#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include "Module.h"
#include "Block.h"

bool isLegalLoop(LoopPtr loop);
void reconstructLoop(LoopPtr loop, FunctionPtr func);
void reconstructLoops(FunctionPtr func);