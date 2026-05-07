#ifndef __ASM_BUILDER_H__
#define __ASM_BUILDER_H__

#include "codegen/MachineUnit.hpp"

#include <memory>

class AsmBuilder {
public:
    MachineUnit *mUnit;
    MachineFunction *mFunction;
    MachineBlock *mBlock;
    
    void setUnit(MachineUnit *unit) { mUnit = unit; }
    void setFunction(MachineFunction *func) { mFunction = func; }
    void setBlock(MachineBlock *block) { mBlock = block; }

    MachineUnit *getUnit() { return mUnit; }
    MachineFunction *getFunction() { return mFunction; }
    MachineBlock *getBlock() { return mBlock; }
};

#endif