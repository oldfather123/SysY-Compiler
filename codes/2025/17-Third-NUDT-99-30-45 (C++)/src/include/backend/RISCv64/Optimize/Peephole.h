#ifndef RISCV64_PEEPHOLE_H
#define RISCV64_PEEPHOLE_H

#include "RISCv64LLIR.h"
#include "Pass.h"

namespace sysy {

/**
 * @class PeepholeOptimizer
 * @brief 窥孔优化器
 * * 在已分配物理寄存器的指令流上，通过一个小的滑动窗口来查找
 * 并替换掉一些冗余或低效的指令模式。
 */
class PeepholeOptimizer : public Pass {
public:
    static char ID;
    
    PeepholeOptimizer() : Pass("peephole-optimizer", Granularity::Function, PassKind::Optimization) {}
    
    void *getPassID() const override { return &ID; }
    
    bool runOnFunction(Function *F, AnalysisManager& AM) override;
    
    void runOnMachineFunction(MachineFunction* mfunc);
    
    /**
     * @brief 设置是否启用浮点乘加融合优化
     * @param enabled 是否启用
     */
    static void setFusedMulAddEnabled(bool enabled) { fusedMulAddEnabled = enabled; }
    
    /**
     * @brief 检查是否启用了浮点乘加融合优化
     * @return 是否启用
     */
    static bool isFusedMulAddEnabled() { return fusedMulAddEnabled; }

private:
    static bool fusedMulAddEnabled;  // 浮点乘加融合优化开关
};

} // namespace sysy

#endif // RISCV64_PEEPHOLE_H