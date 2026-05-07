#include "Instruction.h"
#include "Variable.h"

bool canMergeFcmp(const FcmpInstruction* currentFcmp, const FcmpInstruction* nextFcmp, float currentRhsValue, float nextRhsValue, bool isTrueBranch);
bool mergeCondBranches(FunctionPtr func);