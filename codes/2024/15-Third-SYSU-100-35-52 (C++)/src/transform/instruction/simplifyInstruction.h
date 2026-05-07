#include "Module.h"
ValuePtr simplifyInstructionBinary(Instruction* instr);
bool simplifyInst(FunctionPtr func);
void runSimplifyInst(Module& mod);