#ifndef MACHINE_PASS_H
#define MACHINE_PASS_H
#include "../basic/machine.h"
#include "../inst_process/inst_select/inst_select.h"
#include "../inst_process/machine_instruction.h"
class MachinePass {
protected:
    MachineUnit *unit;

public:
    virtual void Execute() = 0;
    MachinePass(MachineUnit *unit) : unit(unit) {}
};
#endif