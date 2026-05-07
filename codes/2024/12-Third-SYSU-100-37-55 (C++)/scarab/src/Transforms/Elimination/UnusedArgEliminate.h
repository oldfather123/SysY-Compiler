#pragma once
#include <iostream>
#include <vector>
#include "Module.h"
#include "Function.h"
#include "Variable.h"

void removeUnusedArgs(FunctionPtr func, const vector<uint32_t>& UnusedIdx);
void UnusedArgEliminate(Module &module);