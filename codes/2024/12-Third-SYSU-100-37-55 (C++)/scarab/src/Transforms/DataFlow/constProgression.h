#pragma once
#include <iostream>
#include <vector>
#include <set>
#include <queue>
#include "Module.h"
#include "generateCFG.h"
#include "SimplifyInstructions.h"
#include "LICM.h"


bool constProgressionImpl(FunctionPtr func);
void constProgression(FunctionPtr func);
