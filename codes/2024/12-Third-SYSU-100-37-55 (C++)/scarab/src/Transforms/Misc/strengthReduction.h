#pragma once
#include <iostream>
#include <vector>
#include <unordered_set>
#include <cmath>
#include <memory>
#include "Module.h"

bool isPowerOfTwo(int value);
void replaceMulWithShift(shared_ptr<Instruction>& mulInstr, ValuePtr operand, int shiftAmount, int& regCounter);
void strengthReduction(FunctionPtr func);