#ifndef POST_RA_SCHEDULER_H
#define POST_RA_SCHEDULER_H

#include "RISCv64LLIR.h"
#include "Pass.h"

namespace sysy {

/**
 * @class PostRA_Scheduler
 * @brief 寄存器分配后的局部指令调度器
 * * 主要目标是优化寄存器分配器插入的spill/fill代码(lw/sw)，
 * 尝试将加载指令提前，以隐藏其访存延迟。
 */
struct MemoryAccess {
    PhysicalReg base_reg;
    int64_t offset;
    bool valid;
    
    MemoryAccess() : valid(false) {}
    MemoryAccess(PhysicalReg base, int64_t off) : base_reg(base), offset(off), valid(true) {}
};

struct InstrRegInfo {
    std::unordered_set<PhysicalReg> defined_regs;
    std::unordered_set<PhysicalReg> used_regs;
    bool is_load;
    bool is_store;
    bool is_control_flow;
    MemoryAccess mem_access;
    
    InstrRegInfo() : is_load(false), is_store(false), is_control_flow(false) {}
};

class PostRA_Scheduler : public Pass {
public:
    static char ID;
    
    PostRA_Scheduler() : Pass("post-ra-scheduler", Granularity::Function, PassKind::Optimization) {}
    
    void *getPassID() const override { return &ID; }
    
    bool runOnFunction(Function *F, AnalysisManager& AM) override;
    
    void runOnMachineFunction(MachineFunction* mfunc);
};

} // namespace sysy

#endif // POST_RA_SCHEDULER_H