#ifndef MACHINESTRENGTHREDUCE_H
#define MACHINESTRENGTHREDUCE_H
#include "MachinePass.h"
class MachineStrengthReducePass : MachinePass {
    // 标量强度削弱
    void ScalarStrengthReduction() ;
    void GepStrengthReduction() ;
public:
    MachineStrengthReducePass(MachineUnit *unit) : MachinePass(unit) {}
    void Execute();
    void LSOffsetCompute();
};
#endif