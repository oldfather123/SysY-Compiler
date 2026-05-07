#ifndef RISCV64_DIV_STRENGTH_REDUCTION_H
#define RISCV64_DIV_STRENGTH_REDUCTION_H

#include "RISCv64LLIR.h"
#include "Pass.h"

namespace sysy {

/**
 * @class DivStrengthReduction
 * @brief 除法强度削弱优化器
 * * 将除法运算转换为乘法运算，使用magic number算法
 * 适用于除数为常数的情况，可以显著提高性能
 */
class DivStrengthReduction : public Pass {
public:
    static char ID;
    
    DivStrengthReduction() : Pass("div-strength-reduction", Granularity::Function, PassKind::Optimization) {}
    
    void *getPassID() const override { return &ID; }
    
    bool runOnFunction(Function *F, AnalysisManager& AM) override;
    
    void runOnMachineFunction(MachineFunction* mfunc);
};

} // namespace sysy

#endif // RISCV64_DIV_STRENGTH_REDUCTION_H