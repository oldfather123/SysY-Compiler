#ifndef __MACHINE_CODE_PASS_H_
#define __MACHINE_CODE_PASS_H_

class MachineUnit;

class MachineCodePass {
public:
    virtual void pass(MachineUnit *mUnit) = 0;
    virtual ~MachineCodePass() = default;
};

#endif