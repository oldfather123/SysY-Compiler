
#include <iostream>
#include <vector>
#include <optional>
#include "Module.h"

void forwardBranch(BasicBlockPtr block, ValuePtr cond, bool direction, BasicBlockPtr& target);
void optimizeBranches(FunctionPtr func);