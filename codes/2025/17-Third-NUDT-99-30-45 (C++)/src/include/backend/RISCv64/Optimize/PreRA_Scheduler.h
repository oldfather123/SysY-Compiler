#ifndef PRE_RA_SCHEDULER_H
#define PRE_RA_SCHEDULER_H

#include "RISCv64LLIR.h"
#include "Pass.h"

namespace sysy {

/**
 * @class PreRA_Scheduler
 * @brief 寄存器分配前的指令调度器
 * * 在虚拟寄存器上进行操作，此时调度自由度最大，
 * 主要目标是隐藏指令延迟，提高流水线效率。
 */
class PreRA_Scheduler : public Pass {
public:
    static char ID;
    
    PreRA_Scheduler() : Pass("pre-ra-scheduler", Granularity::Function, PassKind::Optimization) {}
    
    void *getPassID() const override { return &ID; }
    
    bool runOnFunction(Function *F, AnalysisManager& AM) override;
    
    void runOnMachineFunction(MachineFunction* mfunc);
};

} // namespace sysy

#endif // PRE_RA_SCHEDULER_H