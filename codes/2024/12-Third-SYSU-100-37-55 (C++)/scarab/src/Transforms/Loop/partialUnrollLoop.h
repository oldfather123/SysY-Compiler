#pragma once
#include "loopUnroll.h"

void partialUnrollLoop(FunctionPtr func);
bool checkLegalLoop_partial(LoopPtr loop);