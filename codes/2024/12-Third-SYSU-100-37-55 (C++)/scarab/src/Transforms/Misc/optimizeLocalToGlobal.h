#include <map>
#include <unordered_map>
#include <vector>
#include "Instruction.h"
#include "Variable.h"
#include "Module.h"

bool hasStoreBeforeLoad(ValuePtr array, FunctionPtr func);
vector<int> extractShape(TypePtr type);
void traverseUses(InstructionPtr instr, function<bool(InstructionPtr)> shouldSkip, function<void(InstructionPtr)> action);
bool optimizeLocalToGlobal(Module &ir);