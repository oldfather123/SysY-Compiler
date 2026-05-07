#pragma once
#include "Module.h"
#include <unordered_set>

ValuePtr createConstantFromVariable(Variable* variable);
void replaceGlobalConstants(Module& module);