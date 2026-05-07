#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <set>
#include <climits>
#include <map>
#include <algorithm>

#include "Module.h"

using namespace std;

void loadStoreMerge(Module &ir);
void setEntryBlockFlag(FunctionPtr func);