#ifndef BACKEND_INSTRUCTIONSELECTION_H_
#define BACKEND_INSTRUCTIONSELECTION_H_

#include "Common.h"
#include "IR.h"
#include "machine_code.h"
#include "instruction.h"
#include "DFA.h"

namespace backend {
    // Get the next argument register or memory for a function call
    Ptr<Operand> getNextArgOperand(Ptr<RiscFunction> func, int generalArgCount, int floatArgCount, bool isFloatArg);

    // Load a value into a register or memory
    Ptr<Instruction> storeValueTo(Ptr<RiscBasicBlock> &bb, const ir::Value &irValue, const Ptr<Operand> &dest, bool dword);
    Ptr<Instruction> storeValueTo64(Ptr<RiscBasicBlock> &bb, const ir::Value &irValue, const Ptr<Operand> &dest);

    // Load an immediate or register or memory into a register or memory
    Ptr<Instruction> storeFromTo(Ptr<RiscBasicBlock> &bb, const Ptr<Operand> &src, bool srcIsFloatImm, const Ptr<Operand> &dest, bool dword = false);
    Ptr<Instruction> storeFromTo64(Ptr<RiscBasicBlock> &bb, const Ptr<Operand> &src, bool srcIsFloatImm, const Ptr<Operand> &dest);

    void mapArguments(Ptr<RiscFunction>);
    void instructionSelection(const ir::InstPtr &, Ptr<RiscBasicBlock> &);
    void generateInst(Ptr<RiscFunction>);
    void fillOffsets(Ptr<RiscFunction>);
    void saveAndRestoreCallSites(Ptr<RiscFunction>);
}

#endif
