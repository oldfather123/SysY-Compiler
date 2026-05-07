#include "Module.h"
#include "Instruction.h"

ValuePtr  simplifyInstruction(Instruction* instr);

ValuePtr  simplifyInstructionBinary(Instruction *instr);

ValuePtr  simplifyInstructionPhi(PhiInstruction* instr);

ValuePtr  simplifyInstructionFneg(FnegInstruction* instr);