#pragma once

#include "Variable.h"
#include "Function.h"
#include "Instruction.h"
#include "Module.h"

bool analyseAble(InsID id);
void analyseUse(Module &ir);
void clearUse(Module &ir, bool isGlobal);
