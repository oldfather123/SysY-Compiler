#pragma  once
#include "Function.h"
#include "phiEliminate.h"

using NodeIndex = uint32_t;
using Graph = std::vector<std::vector<NodeIndex>>;



static bool removeRedundantPhis(const std::unordered_set<Value*>& scc);

bool runPhiEliminate(FunctionPtr func);