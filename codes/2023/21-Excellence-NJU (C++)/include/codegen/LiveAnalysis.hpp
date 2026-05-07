#ifndef __LIVE_ANALYSIS_H_
#define __LIVE_ANALYSIS_H_

#include "codegen/MachineCodePass.hpp"

class MachineFunction;

class LiveAnalysis : public MachineCodePass{
public:
    void pass(MachineUnit *mUnit) override;
    
    void calcLiveDefUse(MachineFunction *mFunc);
    void calcLiveInOut(MachineFunction *mFunc);
};

#endif