#pragma once
#include "Module.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <set>
#include <climits>
#include <map>

using namespace std;

void blockReorder(Module& ir);

void deleteDeadBlock(FunctionPtr& func);